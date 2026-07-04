#include "servicesCatalogController.h"

#include <QJsonDocument>
#include <QSysInfo>
#include <QJsonArray>
#include <QEventLoop>
#include <QDebug>
#include <QCoreApplication>
#include <QHash>
#include <QSet>
#include <limits>

#include "core/controllers/gatewayController.h"
#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "version.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
#include "platforms/ios/ios_controller.h"
#endif

namespace
{
    namespace configKey
    {
        constexpr char serviceDescription[] = "service_description";
        constexpr char subscriptionPlans[] = "subscription_plans";
        constexpr char storeProductId[] = "store_product_id";
        constexpr char priceLabel[] = "price_label";
        constexpr char subtitle[] = "subtitle";
        constexpr char isTrial[] = "is_trial";
        constexpr char minPriceLabel[] = "min_price_label";
    }

    namespace serviceType
    {
        constexpr char amneziaPremium[] = "amnezia-premium";
    }

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    struct StoreKitPlanQuote {
        QString displayPrice;
        double priceAmount = 0.0;
        double subscriptionBillingMonths = 0.0;
        QString displayPricePerMonth;
    };

    constexpr double oneMonthThreshold = 1.0 + 1e-6;
    constexpr double monthsFallbackThreshold = 1e-6;
    constexpr double monthlyPriceEpsilon = 1e-9;

    QStringList collectPremiumStoreProductIds(const QJsonArray &services)
    {
        QStringList productIds;
        QSet<QString> seenProductIds;
        for (const QJsonValue &serviceValue : services) {
            const QJsonObject serviceObject = serviceValue.toObject();
            if (serviceObject.value(apiDefs::key::serviceType).toString() != serviceType::amneziaPremium) {
                continue;
            }
            const QJsonArray subscriptionPlans =
                    serviceObject.value(configKey::serviceDescription).toObject().value(configKey::subscriptionPlans).toArray();
            for (const QJsonValue &planValue : subscriptionPlans) {
                if (!planValue.isObject()) {
                    continue;
                }
                const QString storeProductId = planValue.toObject().value(configKey::storeProductId).toString();
                if (storeProductId.isEmpty() || seenProductIds.contains(storeProductId)) {
                    continue;
                }
                seenProductIds.insert(storeProductId);
                productIds.append(storeProductId);
            }
        }
        return productIds;
    }

    QHash<QString, StoreKitPlanQuote> buildStoreKitQuoteMap(const QList<QVariantMap> &fetchedProducts)
    {
        QHash<QString, StoreKitPlanQuote> quotesByProductId;
        quotesByProductId.reserve(fetchedProducts.size());

        for (const QVariantMap &productInfo : fetchedProducts) {
            const QString productId = productInfo.value(QStringLiteral("productId")).toString();
            if (productId.isEmpty()) {
                continue;
            }

            QString displayPrice = productInfo.value(QStringLiteral("displayPrice")).toString();
            if (displayPrice.isEmpty()) {
                const QString price = productInfo.value(QStringLiteral("price")).toString();
                const QString currencyCode = productInfo.value(QStringLiteral("currencyCode")).toString();
                displayPrice = currencyCode.isEmpty() ? price : (price + QLatin1Char(' ') + currencyCode);
            }

            StoreKitPlanQuote quote;
            quote.displayPrice = displayPrice;
            quote.priceAmount = productInfo.value(QStringLiteral("priceAmount")).toDouble();
            quote.subscriptionBillingMonths = productInfo.value(QStringLiteral("subscriptionBillingMonths")).toDouble();
            quote.displayPricePerMonth = productInfo.value(QStringLiteral("displayPricePerMonth")).toString();
            quotesByProductId.insert(productId, quote);
        }

        return quotesByProductId;
    }

    void mergeStoreKitPricesIntoPremiumPlans(QJsonObject &data)
    {
        QJsonArray services = data.value(apiDefs::key::services).toArray();
        if (services.isEmpty()) {
            return;
        }

        const QStringList productIds = collectPremiumStoreProductIds(services);
        if (productIds.isEmpty()) {
            qInfo().noquote() << "[IAP] No store_product_id in premium plans; skip StoreKit merge into services payload";
            return;
        }

        QList<QVariantMap> fetchedProducts;
        QEventLoop loop;
        IosController::Instance()->fetchProducts(productIds,
                                                 [&](const QList<QVariantMap> &products, const QStringList &invalidIds,
                                                     const QString &errorString) {
                                                     if (!errorString.isEmpty()) {
                                                         qWarning().noquote() << "[IAP] StoreKit merge fetch:" << errorString;
                                                     }
                                                     if (!invalidIds.isEmpty()) {
                                                         qWarning().noquote() << "[IAP] Unknown App Store product ids:" << invalidIds;
                                                     }
                                                     fetchedProducts = products;
                                                     loop.quit();
                                                 });
        loop.exec();

        const QHash<QString, StoreKitPlanQuote> quotesByProductId = buildStoreKitQuoteMap(fetchedProducts);

        for (int serviceIndex = 0; serviceIndex < services.size(); ++serviceIndex) {
            QJsonObject serviceObject = services.at(serviceIndex).toObject();
            if (serviceObject.value(apiDefs::key::serviceType).toString() != serviceType::amneziaPremium) {
                continue;
            }

            QJsonObject descriptionObject = serviceObject.value(configKey::serviceDescription).toObject();
            const QJsonArray sourcePlans = descriptionObject.value(configKey::subscriptionPlans).toArray();

            QJsonArray mergedPlans;
            double minMonthlyAmount = std::numeric_limits<double>::infinity();
            QString minMonthlyDisplay;

            for (const QJsonValue &planValue : sourcePlans) {
                if (!planValue.isObject()) {
                    continue;
                }

                QJsonObject planObject = planValue.toObject();
                const QString storeProductId = planObject.value(configKey::storeProductId).toString();
                if (storeProductId.isEmpty()) {
                    continue;
                }

                const auto quoteIterator = quotesByProductId.constFind(storeProductId);
                if (quoteIterator == quotesByProductId.cend()) {
                    continue;
                }

                const bool isTrialPlan = planObject.value(configKey::isTrial).toBool();
                const StoreKitPlanQuote &quote = *quoteIterator;
                planObject.insert(configKey::priceLabel, quote.displayPrice);

                const double months = quote.subscriptionBillingMonths;
                if (!isTrialPlan && months > oneMonthThreshold && !quote.displayPricePerMonth.isEmpty()) {
                    planObject.insert(
                            configKey::subtitle,
                            QCoreApplication::translate("ServicesCatalogController", "%1/mo",
                                                        "IAP: price per month in plan subtitle")
                                    .arg(quote.displayPricePerMonth));
                }

                if (!isTrialPlan && quote.priceAmount > 0.0) {
                    const double monthsForMin = months > monthsFallbackThreshold ? months : 1.0;
                    const double monthly = quote.priceAmount / monthsForMin;
                    if (monthly < minMonthlyAmount - monthlyPriceEpsilon) {
                        minMonthlyAmount = monthly;
                        minMonthlyDisplay = !quote.displayPricePerMonth.isEmpty() ? quote.displayPricePerMonth : quote.displayPrice;
                    }
                }

                mergedPlans.append(planObject);
            }

            descriptionObject.insert(configKey::subscriptionPlans, mergedPlans);
            if (minMonthlyAmount < std::numeric_limits<double>::infinity() && !minMonthlyDisplay.isEmpty()) {
                descriptionObject.insert(configKey::minPriceLabel,
                                         QCoreApplication::translate("ServicesCatalogController", "from %1 per month",
                                                                     "IAP: card footer minimum monthly price from StoreKit")
                                                 .arg(minMonthlyDisplay));
            }
            serviceObject.insert(configKey::serviceDescription, descriptionObject);
            services.replace(serviceIndex, serviceObject);
        }
        data.insert(apiDefs::key::services, services);
    }
#endif
}

ServicesCatalogController::ServicesCatalogController(SecureAppSettingsRepository* appSettingsRepository)
    : m_appSettingsRepository(appSettingsRepository)
{
}

ErrorCode ServicesCatalogController::fillAvailableServices(QJsonObject &servicesData)
{
    QJsonObject apiPayload;
    apiPayload[apiDefs::key::osVersion] = QSysInfo::productType();
    apiPayload[apiDefs::key::appVersion] = QString(APP_VERSION);
    apiPayload[apiDefs::key::cliName] = QString(APPLICATION_NAME);
    apiPayload[apiDefs::key::appLanguage] = m_appSettingsRepository->getAppLanguage().name().split("_").first();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/services"), apiPayload, responseBody);
    if (errorCode == ErrorCode::NoError) {
        if (!responseBody.contains(apiDefs::key::services.data())) {
            errorCode = ErrorCode::ApiServicesMissingError;
        }
    }

    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    servicesData = QJsonDocument::fromJson(responseBody).object();

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    mergeStoreKitPricesIntoPremiumPlans(servicesData);
#endif

    return ErrorCode::NoError;
}

ErrorCode ServicesCatalogController::executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody)
{
    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled());
    return gatewayController.post(endpoint, apiPayload, responseBody);
}

