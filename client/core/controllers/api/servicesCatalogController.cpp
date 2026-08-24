#include "servicesCatalogController.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QEventLoop>
#include <QDebug>
#include <QCoreApplication>
#include <QHash>
#include <QSet>
#include <limits>

#include "core/controllers/gatewayController.h"
#include "core/utils/api/gatewayPayloadBuilder.h"
#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "version.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
#include "platforms/ios/ios_controller.h"
#elif defined(Q_OS_ANDROID)
#include "platforms/android/android_controller.h"
#include <QFutureWatcher>
#include <QtConcurrent>
#endif

namespace
{
    namespace configKey
    {
        constexpr char serviceDescription[] = "service_description";
        constexpr char subscriptionPlans[] = "subscription_plans";
        constexpr char storeProductId[] = "store_product_id";
        constexpr char priceLabel[] = "price_label";
        constexpr char priceAmount[] = "price_amount";
        constexpr char subtitle[] = "subtitle";
        constexpr char isTrial[] = "is_trial";
        constexpr char minPriceLabel[] = "min_price_label";
        constexpr char hasFreeTrial[] = "has_free_trial";
        constexpr char trialDays[] = "trial_days";
    }

    namespace serviceType
    {
        constexpr char amneziaPremium[] = "amnezia-premium";
    }

#if defined(Q_OS_IOS) || defined(MACOS_NE) || defined(Q_OS_ANDROID)
    struct SubscriptionPlanQuote {
        QString displayPrice;
        double priceAmount = 0.0;
        double subscriptionBillingMonths = 0.0;
        QString displayPricePerMonth;
        bool hasFreeTrial = false;
        int trialDays = 0;
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

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    QHash<QString, SubscriptionPlanQuote> buildStoreKitQuoteMap(const QStringList &productIds)
    {
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

        QHash<QString, SubscriptionPlanQuote> quotesByProductId;
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

            SubscriptionPlanQuote quote;
            quote.priceAmount = productInfo.value(QStringLiteral("priceAmount")).toDouble();
            quote.subscriptionBillingMonths = productInfo.value(QStringLiteral("subscriptionBillingMonths")).toDouble();
            quote.displayPricePerMonth = productInfo.value(QStringLiteral("displayPricePerMonth")).toString();
            quote.hasFreeTrial = productInfo.value(QStringLiteral("hasFreeTrial")).toBool();
            quote.trialDays = productInfo.value(QStringLiteral("trialDays")).toInt();

            // If the account is eligible for a paid introductory discount (not a free trial, which
            // stays invisible here and just applies silently at purchase time), show that price instead.
            const QString introOfferDisplayPrice = productInfo.value(QStringLiteral("introOfferDisplayPrice")).toString();
            if (introOfferDisplayPrice.isEmpty()) {
                quote.displayPrice = displayPrice;
            } else {
                quote.displayPrice = introOfferDisplayPrice;
                qInfo().noquote() << "[IAP] Applying" << productInfo.value(QStringLiteral("introOfferPaymentMode")).toString()
                                  << "intro offer price for" << productId << ":" << introOfferDisplayPrice;
            }
            quotesByProductId.insert(productId, quote);
        }

        return quotesByProductId;
    }
#elif defined(Q_OS_ANDROID)
    QHash<QString, SubscriptionPlanQuote> buildGooglePlayQuoteMap(const QStringList &productIds)
    {
        Q_UNUSED(productIds); // Google Play returns every offer for the app's single product in one call

        auto androidController = AndroidController::instance();
        QFutureWatcher<QJsonObject> watcher;
        QEventLoop waitLoop;
        QObject::connect(&watcher, &QFutureWatcher<QJsonObject>::finished, &waitLoop, &QEventLoop::quit);

        QFuture<QJsonObject> future = QtConcurrent::run([androidController]() {
            return androidController->getSubscriptionPlans();
        });
        watcher.setFuture(future);
        waitLoop.exec();

        QJsonObject plansResult = watcher.result();
        QHash<QString, SubscriptionPlanQuote> quotesByProductId;
        if (plansResult.value("responseCode").toInt(-1) != 0) {
            qWarning() << "[Billing] Failed to get subscription plans for price display, responseCode:"
                       << plansResult.value("responseCode").toInt(-1);
            return quotesByProductId;
        }

        const QJsonArray products = plansResult.value("products").toArray();

        QHash<QString, int> trialDaysByBasePlanId;
        for (const QJsonValue &productValue : products) {
            const QJsonArray offers = productValue.toObject().value("offers").toArray();
            for (const QJsonValue &offerValue : offers) {
                const QJsonObject offer = offerValue.toObject();
                const QString basePlanId = offer.value("basePlanId").toString();
                if (basePlanId.isEmpty() || trialDaysByBasePlanId.contains(basePlanId)) {
                    continue;
                }
                if (offer.value("hasFreeTrial").toBool()) {
                    trialDaysByBasePlanId.insert(basePlanId, offer.value("trialDays").toInt());
                }
            }
        }

        for (const QJsonValue &productValue : products) {
            const QJsonArray offers = productValue.toObject().value("offers").toArray();
            for (const QJsonValue &offerValue : offers) {
                const QJsonObject offer = offerValue.toObject();
                const QString basePlanId = offer.value("basePlanId").toString();
                if (basePlanId.isEmpty() || quotesByProductId.contains(basePlanId)) {
                    continue;
                }

                const QJsonArray pricingPhases = offer.value("pricingPhases").toArray();
                if (pricingPhases.isEmpty()) {
                    continue;
                }
                // Last phase is always the ongoing recurring price, after any trial/intro phases
                const QJsonObject regularPhase = pricingPhases.last().toObject();

                SubscriptionPlanQuote quote;
                quote.displayPrice = regularPhase.value("formatedPrice").toString();
                if (quote.displayPrice.isEmpty()) {
                    continue;
                }
                quote.priceAmount = regularPhase.value("priceAmountMicros").toDouble() / 1000000.0;
                quote.subscriptionBillingMonths = regularPhase.value("subscriptionBillingMonths").toDouble();
                quote.displayPricePerMonth = regularPhase.value("displayPricePerMonth").toString();
                quote.hasFreeTrial = trialDaysByBasePlanId.contains(basePlanId);
                quote.trialDays = trialDaysByBasePlanId.value(basePlanId, 0);
                quotesByProductId.insert(basePlanId, quote);
            }
        }
        return quotesByProductId;
    }
#endif

    void mergeQuotesIntoPremiumPlans(QJsonObject &data, const QHash<QString, SubscriptionPlanQuote> &quotesByProductId)
    {
        QJsonArray services = data.value(apiDefs::key::services).toArray();
        if (services.isEmpty() || quotesByProductId.isEmpty()) {
            return;
        }

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
                const SubscriptionPlanQuote &quote = *quoteIterator;
                planObject.insert(configKey::priceLabel, quote.displayPrice);
                planObject.insert(configKey::priceAmount, quote.priceAmount);
                planObject.insert(configKey::hasFreeTrial, quote.hasFreeTrial);
                planObject.insert(configKey::trialDays, quote.trialDays);

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

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    void mergeStoreKitPricesIntoPremiumPlans(QJsonObject &data)
    {
        const QStringList productIds = collectPremiumStoreProductIds(data.value(apiDefs::key::services).toArray());
        if (productIds.isEmpty()) {
            qInfo().noquote() << "[IAP] No store_product_id in premium plans; skip StoreKit merge into services payload";
            return;
        }
        mergeQuotesIntoPremiumPlans(data, buildStoreKitQuoteMap(productIds));
    }
#elif defined(Q_OS_ANDROID)
    void mergeGooglePlayPricesIntoPremiumPlans(QJsonObject &data)
    {
        const QStringList productIds = collectPremiumStoreProductIds(data.value(apiDefs::key::services).toArray());
        if (productIds.isEmpty()) {
            qInfo().noquote() << "[Billing] No store_product_id in premium plans; skip Google Play merge into services payload";
            return;
        }
        mergeQuotesIntoPremiumPlans(data, buildGooglePlayQuoteMap(productIds));
    }
#endif
#endif // Q_OS_IOS || MACOS_NE || Q_OS_ANDROID
}

ServicesCatalogController::ServicesCatalogController(SecureAppSettingsRepository* appSettingsRepository)
    : m_appSettingsRepository(appSettingsRepository)
{
}

ErrorCode ServicesCatalogController::fillAvailableServices(QJsonObject &servicesData)
{
    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository).build();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/services"), apiPayload, responseBody);
    qWarning() << "[ServicesCatalog] errorCode:" << static_cast<int>(errorCode)
               << "response:" << QString::fromLocal8Bit(responseBody);
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
#elif defined(Q_OS_ANDROID)
    mergeGooglePlayPricesIntoPremiumPlans(servicesData);
#endif

    return ErrorCode::NoError;
}

ErrorCode ServicesCatalogController::executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody)
{
    QString gatewayEndpoint = m_appSettingsRepository->getGatewayEndpoint();
    GatewayController gatewayController(gatewayEndpoint, m_appSettingsRepository->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled(), m_appSettingsRepository);
    return gatewayController.post(endpoint, apiPayload, responseBody);
}

