#include "apiConfigsController.h"

#include "amnezia_application.h"
#include "configurators/wireguard_configurator.h"
#include "core/api/apiDefs.h"
#include "core/api/apiUtils.h"
#include "core/controllers/gatewayController.h"
#include "core/qrCodeUtils.h"
#include "ui/controllers/systemController.h"
#include "version.h"
#include <QClipboard>
#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QHash>
#include <QJsonArray>
#include <QSet>
#include <QVariantMap>
#include <limits>

#include "platforms/ios/ios_controller.h"

namespace
{
    namespace configKey
    {
        constexpr char cloak[] = "cloak";
        constexpr char awg[] = "awg";
        constexpr char vless[] = "vless";

        constexpr char apiEndpoint[] = "api_endpoint";
        constexpr char accessToken[] = "api_key";
        constexpr char certificate[] = "certificate";
        constexpr char publicKey[] = "public_key";
        constexpr char protocol[] = "protocol";

        constexpr char uuid[] = "installation_uuid";
        constexpr char osVersion[] = "os_version";
        constexpr char appVersion[] = "app_version";

        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serverCountryCode[] = "server_country_code";
        constexpr char serviceType[] = "service_type";
        constexpr char serviceInfo[] = "service_info";
        constexpr char serviceProtocol[] = "service_protocol";

        constexpr char services[] = "services";
        constexpr char serviceDescription[] = "service_description";
        constexpr char subscriptionPlans[] = "subscription_plans";
        constexpr char storeProductId[] = "store_product_id";
        constexpr char priceLabel[] = "price_label";
        constexpr char subtitle[] = "subtitle";
        constexpr char isTrial[] = "is_trial";
        constexpr char minPriceLabel[] = "min_price_label";

        constexpr char apiPayload[] = "api_payload";
        constexpr char keyPayload[] = "key_payload";

        constexpr char apiConfig[] = "api_config";
        constexpr char authData[] = "auth_data";

        constexpr char config[] = "config";

        constexpr char isConnectEvent[] = "is_connect_event";
    }

    namespace serviceType
    {
        constexpr char amneziaFree[] = "amnezia-free";
        constexpr char amneziaPremium[] = "amnezia-premium";
    }

    struct ProtocolData
    {
        OpenVpnConfigurator::ConnectionData certRequest;

        QString wireGuardClientPrivKey;
        QString wireGuardClientPubKey;

        QString xrayUuid;
    };

    struct GatewayRequestData
    {
        QString osVersion;
        QString appVersion;
        QString appLanguage;

        QString installationUuid;

        QString userCountryCode;
        QString serverCountryCode;
        QString serviceType;
        QString serviceProtocol;

        QJsonObject authData;

        QJsonObject toJsonObject() const
        {
            QJsonObject obj;
            if (!osVersion.isEmpty()) {
                obj[configKey::osVersion] = osVersion;
            }
            if (!appVersion.isEmpty()) {
                obj[configKey::appVersion] = appVersion;
            }
            if (!appLanguage.isEmpty()) {
                obj[apiDefs::key::appLanguage] = appLanguage;
            }
            if (!installationUuid.isEmpty()) {
                obj[configKey::uuid] = installationUuid;
            }
            if (!userCountryCode.isEmpty()) {
                obj[configKey::userCountryCode] = userCountryCode;
            }
            if (!serverCountryCode.isEmpty()) {
                obj[configKey::serverCountryCode] = serverCountryCode;
            }
            if (!serviceType.isEmpty()) {
                obj[configKey::serviceType] = serviceType;
            }
            if (!serviceProtocol.isEmpty()) {
                obj[configKey::serviceProtocol] = serviceProtocol;
            }
            if (!authData.isEmpty()) {
                obj[configKey::authData] = authData;
            }
            return obj;
        }
    };

    ProtocolData generateProtocolData(const QString &protocol)
    {
        ProtocolData protocolData;
        if (protocol == configKey::cloak) {
            protocolData.certRequest = OpenVpnConfigurator::createCertRequest();
        } else if (protocol == configKey::awg) {
            auto connData = WireguardConfigurator::genClientKeys();
            protocolData.wireGuardClientPubKey = connData.clientPubKey;
            protocolData.wireGuardClientPrivKey = connData.clientPrivKey;
        } else if (protocol == configKey::vless) {
            protocolData.xrayUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
        }

        return protocolData;
    }

    void appendProtocolDataToApiPayload(const QString &protocol, const ProtocolData &protocolData, QJsonObject &apiPayload)
    {
        if (protocol == configKey::cloak) {
            apiPayload[configKey::certificate] = protocolData.certRequest.request;
        } else if (protocol == configKey::awg) {
            apiPayload[configKey::publicKey] = protocolData.wireGuardClientPubKey;
        } else if (protocol == configKey::vless) {
            apiPayload[configKey::publicKey] = protocolData.xrayUuid;
        }
    }

    ErrorCode fillServerConfig(const QString &protocol, const ProtocolData &apiPayloadData, const QByteArray &apiResponseBody,
                               QJsonObject &serverConfig)
    {
        QString data = QJsonDocument::fromJson(apiResponseBody).object().value(config_key::config).toString();

        data.replace("vpn://", "");
        QByteArray ba = QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

        if (ba.isEmpty()) {
            qDebug() << "empty vpn key";
            return ErrorCode::ApiConfigEmptyError;
        }

        QByteArray ba_uncompressed = qUncompress(ba);
        if (!ba_uncompressed.isEmpty()) {
            ba = ba_uncompressed;
        }

        QString configStr = ba;
        if (protocol == configKey::cloak) {
            configStr.replace("<key>", "<key>\n");
            configStr.replace("$OPENVPN_PRIV_KEY", apiPayloadData.certRequest.privKey);
        } else if (protocol == configKey::awg) {
            configStr.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", apiPayloadData.wireGuardClientPrivKey);
            auto newServerConfig = QJsonDocument::fromJson(configStr.toUtf8()).object();
            auto containers = newServerConfig.value(config_key::containers).toArray();
            if (containers.isEmpty()) {
                qDebug() << "missing containers field";
                return ErrorCode::ApiConfigEmptyError;
            }
            auto containerObject = containers.at(0).toObject();
            auto containerType = ContainerProps::containerFromString(containerObject.value(config_key::container).toString());
            QString containerName = ContainerProps::containerTypeToString(containerType);
            auto serverProtocolConfig = containerObject.value(containerName).toObject();
            auto clientProtocolConfig =
                    QJsonDocument::fromJson(serverProtocolConfig.value(config_key::last_config).toString().toUtf8()).object();

            // TODO looks like this block can be removed after v1 configs EOL

            serverProtocolConfig[config_key::junkPacketCount] = clientProtocolConfig.value(config_key::junkPacketCount);
            serverProtocolConfig[config_key::junkPacketMinSize] = clientProtocolConfig.value(config_key::junkPacketMinSize);
            serverProtocolConfig[config_key::junkPacketMaxSize] = clientProtocolConfig.value(config_key::junkPacketMaxSize);
            serverProtocolConfig[config_key::initPacketJunkSize] = clientProtocolConfig.value(config_key::initPacketJunkSize);
            serverProtocolConfig[config_key::responsePacketJunkSize] = clientProtocolConfig.value(config_key::responsePacketJunkSize);
            serverProtocolConfig[config_key::initPacketMagicHeader] = clientProtocolConfig.value(config_key::initPacketMagicHeader);
            serverProtocolConfig[config_key::responsePacketMagicHeader] = clientProtocolConfig.value(config_key::responsePacketMagicHeader);
            serverProtocolConfig[config_key::underloadPacketMagicHeader] = clientProtocolConfig.value(config_key::underloadPacketMagicHeader);
            serverProtocolConfig[config_key::transportPacketMagicHeader] = clientProtocolConfig.value(config_key::transportPacketMagicHeader);

            serverProtocolConfig[config_key::cookieReplyPacketJunkSize] = clientProtocolConfig.value(config_key::cookieReplyPacketJunkSize);
            serverProtocolConfig[config_key::transportPacketJunkSize] = clientProtocolConfig.value(config_key::transportPacketJunkSize);
            serverProtocolConfig[config_key::specialJunk1] = clientProtocolConfig.value(config_key::specialJunk1);
            serverProtocolConfig[config_key::specialJunk2] = clientProtocolConfig.value(config_key::specialJunk2);
            serverProtocolConfig[config_key::specialJunk3] = clientProtocolConfig.value(config_key::specialJunk3);
            serverProtocolConfig[config_key::specialJunk4] = clientProtocolConfig.value(config_key::specialJunk4);
            serverProtocolConfig[config_key::specialJunk5] = clientProtocolConfig.value(config_key::specialJunk5);

            //

            containerObject[containerName] = serverProtocolConfig;
            containers.replace(0, containerObject);
            newServerConfig[config_key::containers] = containers;
            configStr = QString(QJsonDocument(newServerConfig).toJson());
        }

        QJsonObject newServerConfig = QJsonDocument::fromJson(configStr.toUtf8()).object();
        serverConfig[config_key::dns1] = newServerConfig.value(config_key::dns1);
        serverConfig[config_key::dns2] = newServerConfig.value(config_key::dns2);
        serverConfig[config_key::containers] = newServerConfig.value(config_key::containers);
        serverConfig[config_key::hostName] = newServerConfig.value(config_key::hostName);

        if (newServerConfig.value(config_key::configVersion).toInt() == apiDefs::ConfigSource::AmneziaGateway) {
            serverConfig[config_key::configVersion] = newServerConfig.value(config_key::configVersion);
            serverConfig[config_key::description] = newServerConfig.value(config_key::description);
            serverConfig[config_key::name] = newServerConfig.value(config_key::name);
        }

        auto defaultContainer = newServerConfig.value(config_key::defaultContainer).toString();
        serverConfig[config_key::defaultContainer] = defaultContainer;

        QVariantMap map = serverConfig.value(configKey::apiConfig).toObject().toVariantMap();
        map.insert(newServerConfig.value(configKey::apiConfig).toObject().toVariantMap());
        auto apiConfig = QJsonObject::fromVariantMap(map);

        if (newServerConfig.value(config_key::configVersion).toInt() == apiDefs::ConfigSource::AmneziaGateway) {
            apiConfig.insert(apiDefs::key::supportedProtocols,
                             QJsonDocument::fromJson(apiResponseBody).object().value(apiDefs::key::supportedProtocols).toArray());

            apiConfig.insert(apiDefs::key::serviceInfo,
                             QJsonDocument::fromJson(apiResponseBody).object().value(apiDefs::key::serviceInfo).toObject());
        }

        serverConfig[configKey::apiConfig] = apiConfig;

        return ErrorCode::NoError;
    }

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    struct StoreKitPlanQuote {
        QString displayPrice;
        double priceAmount = 0.0;
        double subscriptionBillingMonths = 0.0;
        QString displayPricePerMonth;
    };

    constexpr double kOneMonthThreshold = 1.0 + 1e-6;
    constexpr double kMonthsFallbackThreshold = 1e-6;
    constexpr double kMonthlyPriceEpsilon = 1e-9;

    QStringList collectPremiumStoreProductIds(const QJsonArray &services)
    {
        QStringList productIds;
        QSet<QString> seenProductIds;
        for (const QJsonValue &serviceValue : services) {
            const QJsonObject serviceObject = serviceValue.toObject();
            if (serviceObject.value(configKey::serviceType).toString() != serviceType::amneziaPremium) {
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
        QJsonArray services = data.value(configKey::services).toArray();
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
            if (serviceObject.value(configKey::serviceType).toString() != serviceType::amneziaPremium) {
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
                if (!isTrialPlan && months > kOneMonthThreshold && !quote.displayPricePerMonth.isEmpty()) {
                    planObject.insert(
                            configKey::subtitle,
                            QCoreApplication::translate("ApiConfigsController", "%1/mo", "IAP: price per month in plan subtitle")
                                    .arg(quote.displayPricePerMonth));
                }

                if (!isTrialPlan && quote.priceAmount > 0.0) {
                    const double monthsForMin = months > kMonthsFallbackThreshold ? months : 1.0;
                    const double monthly = quote.priceAmount / monthsForMin;
                    if (monthly < minMonthlyAmount - kMonthlyPriceEpsilon) {
                        minMonthlyAmount = monthly;
                        minMonthlyDisplay = !quote.displayPricePerMonth.isEmpty() ? quote.displayPricePerMonth : quote.displayPrice;
                    }
                }

                mergedPlans.append(planObject);
            }

            descriptionObject.insert(configKey::subscriptionPlans, mergedPlans);
            if (minMonthlyAmount < std::numeric_limits<double>::infinity() && !minMonthlyDisplay.isEmpty()) {
                descriptionObject.insert(configKey::minPriceLabel,
                                         QCoreApplication::translate("ApiConfigsController", "from %1 per month",
                                                                     "IAP: card footer minimum monthly price from StoreKit")
                                                 .arg(minMonthlyDisplay));
            }
            serviceObject.insert(configKey::serviceDescription, descriptionObject);
            services.replace(serviceIndex, serviceObject);
        }
        data.insert(configKey::services, services);
    }
#endif
}

ApiConfigsController::ApiConfigsController(const QSharedPointer<ServersModel> &serversModel,
                                           const QSharedPointer<ApiServicesModel> &apiServicesModel,
                                           const QSharedPointer<ApiSubscriptionPlansModel> &subscriptionPlansModel,
                                           const QSharedPointer<ApiBenefitsModel> &benefitsModel,
                                           const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent)
    , m_serversModel(serversModel)
    , m_apiServicesModel(apiServicesModel)
    , m_subscriptionPlansModel(subscriptionPlansModel)
    , m_benefitsModel(benefitsModel)
    , m_settings(settings)
{
    connect(m_apiServicesModel.data(), &ApiServicesModel::serviceSelectionChanged, this, [this]() {
        const ApiServicesModel::ApiServicesData serviceData = m_apiServicesModel->selectedServiceData();
        m_subscriptionPlansModel->updateModel(serviceData.subscriptionPlansJson);
        m_benefitsModel->updateModel(serviceData.benefits);
    });
}

bool ApiConfigsController::exportVpnKey(const QString &fileName)
{
    if (fileName.isEmpty()) {
        emit errorOccurred(ErrorCode::PermissionsError);
        return false;
    }

    prepareVpnKeyExport();
    if (m_vpnKey.isEmpty()) {
        emit errorOccurred(ErrorCode::ApiConfigEmptyError);
        return false;
    }

    SystemController::saveFile(fileName, m_vpnKey);
    return true;
}

bool ApiConfigsController::exportNativeConfig(const QString &serverCountryCode, const QString &fileName)
{
    if (fileName.isEmpty()) {
        emit errorOccurred(ErrorCode::PermissionsError);
        return false;
    }

    auto serverConfigObject = m_serversModel->getServerConfig(m_serversModel->getProcessedServerIndex());
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getAppLanguage().name().split("_").first(),
                                            m_settings->getInstallationUuid(true),
                                            apiConfigObject.value(configKey::userCountryCode).toString(),
                                            serverCountryCode,
                                            apiConfigObject.value(configKey::serviceType).toString(),
                                            configKey::awg, // apiConfigObject.value(configKey::serviceProtocol).toString(),
                                            serverConfigObject.value(configKey::authData).toObject() };

    QString protocol = gatewayRequestData.serviceProtocol;
    ProtocolData protocolData = generateProtocolData(protocol);

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    appendProtocolDataToApiPayload(gatewayRequestData.serviceProtocol, protocolData, apiPayload);
    bool isTestPurchase = apiConfigObject.value(apiDefs::key::isTestPurchase).toBool(false);
    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/native_config"), apiPayload, responseBody, isTestPurchase);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    QJsonObject jsonConfig = QJsonDocument::fromJson(responseBody).object();
    QString nativeConfig = jsonConfig.value(configKey::config).toString();
    nativeConfig.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", protocolData.wireGuardClientPrivKey);

    SystemController::saveFile(fileName, nativeConfig);
    return true;
}

bool ApiConfigsController::revokeNativeConfig(const QString &serverCountryCode)
{
    auto serverConfigObject = m_serversModel->getServerConfig(m_serversModel->getProcessedServerIndex());
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getAppLanguage().name().split("_").first(),
                                            m_settings->getInstallationUuid(true),
                                            apiConfigObject.value(configKey::userCountryCode).toString(),
                                            serverCountryCode,
                                            apiConfigObject.value(configKey::serviceType).toString(),
                                            configKey::awg, // apiConfigObject.value(configKey::serviceProtocol).toString(),
                                            serverConfigObject.value(configKey::authData).toObject() };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    bool isTestPurchase = apiConfigObject.value(apiDefs::key::isTestPurchase).toBool(false);
    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_native_config"), apiPayload, responseBody, isTestPurchase);
    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        emit errorOccurred(errorCode);
        return false;
    }
    return true;
}

void ApiConfigsController::prepareVpnKeyExport()
{
    auto serverConfigObject = m_serversModel->getServerConfig(m_serversModel->getProcessedServerIndex());
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    auto vpnKey = apiConfigObject.value(apiDefs::key::vpnKey).toString();
    if (vpnKey.isEmpty()) {
        vpnKey = apiUtils::getPremiumV2VpnKey(serverConfigObject);
        apiConfigObject.insert(apiDefs::key::vpnKey, vpnKey);
        serverConfigObject.insert(configKey::apiConfig, apiConfigObject);
        m_serversModel->editServer(serverConfigObject, m_serversModel->getProcessedServerIndex());
    }

    m_vpnKey = vpnKey;

    vpnKey.replace("vpn://", "");

    m_qrCodes = qrCodeUtils::generateQrCodeImageSeries(vpnKey.toUtf8());

    emit vpnKeyExportReady();
}

void ApiConfigsController::copyVpnKeyToClipboard()
{
    auto clipboard = amnApp->getClipboard();
    clipboard->setText(m_vpnKey);
}

bool ApiConfigsController::fillAvailableServices()
{
    QJsonObject apiPayload;
    apiPayload[configKey::osVersion] = QSysInfo::productType();
    apiPayload[configKey::appVersion] = QString(APP_VERSION);
    apiPayload[apiDefs::key::cliName] = QString(APPLICATION_NAME);
    apiPayload[apiDefs::key::appLanguage] = m_settings->getAppLanguage().name().split("_").first();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/services"), apiPayload, responseBody);
    if (errorCode == ErrorCode::NoError) {
        if (!responseBody.contains("services")) {
            errorCode = ErrorCode::ApiServicesMissingError;
        }
    }

    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    QJsonObject data = QJsonDocument::fromJson(responseBody).object();

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    mergeStoreKitPricesIntoPremiumPlans(data);
#endif

    m_apiServicesModel->updateModel(data);
    if (m_apiServicesModel->rowCount() > 0) {
        m_apiServicesModel->setServiceIndex(0);
    }
    return true;
}

bool ApiConfigsController::importService()
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    const bool isIosOrMacOsNe = true;
#else
    const bool isIosOrMacOsNe = false;
#endif

    if (m_apiServicesModel->getSelectedServiceType() == serviceType::amneziaPremium) {
        if (isIosOrMacOsNe) {
            return importPremiumFromAppStore(QString());
        }
    } else if (m_apiServicesModel->getSelectedServiceType() == serviceType::amneziaFree) {
        return importFreeFromGateway();
    }
    return false;
}

bool ApiConfigsController::importPremiumFromAppStore(const QString &storeProductId)
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    QString productId = storeProductId.trimmed();
    if (productId.isEmpty()) {
        productId = QStringLiteral("amnezia_premium_6_month");
    }

    bool purchaseOk = false;
    QString originalTransactionId;
    QString storeTransactionId;
    QString purchasedStoreProductId;
    QString purchaseError;
    QEventLoop waitPurchase;
    IosController::Instance()->purchaseProduct(productId,
                                               [&](bool success, const QString &transactionId, const QString &purchasedProductId,
                                                   const QString &originalTransactionIdResponse, const QString &errorString) {
                                                   purchaseOk = success;
                                                   originalTransactionId = originalTransactionIdResponse;
                                                   storeTransactionId = transactionId;
                                                   purchasedStoreProductId = purchasedProductId;
                                                   purchaseError = errorString;
                                                   waitPurchase.quit();
                                               });
    waitPurchase.exec();

    if (!purchaseOk || originalTransactionId.isEmpty()) {
        qDebug() << "IAP purchase failed:" << purchaseError;
        emit errorOccurred(ErrorCode::ApiPurchaseError);
        return false;
    }
    qInfo().noquote() << "[IAP] Purchase success. transactionId =" << storeTransactionId
                      << "originalTransactionId =" << originalTransactionId << "productId =" << purchasedStoreProductId;

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getAppLanguage().name().split("_").first(),
                                            m_settings->getInstallationUuid(true),
                                            m_apiServicesModel->getCountryCode(),
                                            "",
                                            m_apiServicesModel->getSelectedServiceType(),
                                            m_apiServicesModel->getSelectedServiceProtocol(),
                                            QJsonObject() };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    apiPayload[apiDefs::key::transactionId] = originalTransactionId;
    auto isTestPurchase = IosController::Instance()->isTestFlight();

    ErrorCode errorCode;
    QByteArray responseBody;
    errorCode = executeRequest(QString("%1v1/subscriptions"), apiPayload, responseBody, isTestPurchase);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    int duplicateServerIndex = -1;
    errorCode = importServiceFromBilling(responseBody, isTestPurchase, duplicateServerIndex);
    if (errorCode == ErrorCode::ApiConfigAlreadyAdded) {
        emit installServerFromApiFinished(tr("This subscription has already been added"), duplicateServerIndex);
        return true;
    }
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }
    emit installServerFromApiFinished(
            tr("%1 has been added to the app").arg(m_apiServicesModel->getSelectedServiceName()));
    return true;
#else
    Q_UNUSED(storeProductId);
    return false;
#endif
}

bool ApiConfigsController::restoreServiceFromAppStore()
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    const QString premiumServiceType = QStringLiteral("amnezia-premium");

    if (!fillAvailableServices()) {
        qWarning().noquote() << "[IAP] Unable to fetch services list before restore";
        emit errorOccurred(ErrorCode::ApiServicesMissingError);
        return false;
    }

    if (m_apiServicesModel->rowCount() <= 0) {
        emit errorOccurred(ErrorCode::ApiServicesMissingError);
        return false;
    }

    const int premiumServiceIndex = m_apiServicesModel->serviceIndexForType(premiumServiceType);
    if (premiumServiceIndex < 0) {
        emit errorOccurred(ErrorCode::ApiServicesMissingError);
        return false;
    }
    m_apiServicesModel->setServiceIndex(premiumServiceIndex);

    bool restoreSuccess = false;
    QList<QVariantMap> restoredTransactions;
    QString restoreError;
    QEventLoop waitRestore;

    IosController::Instance()->restorePurchases([&](bool success, const QList<QVariantMap> &transactions, const QString &errorString) {
        restoreSuccess = success;
        restoredTransactions = transactions;
        restoreError = errorString;
        waitRestore.quit();
    });
    waitRestore.exec();

    if (!restoreSuccess) {
        qWarning().noquote() << "[IAP] Restore failed:" << restoreError;
        emit errorOccurred(ErrorCode::ApiPurchaseError);
        return false;
    }

    if (restoredTransactions.isEmpty()) {
        qInfo().noquote() << "[IAP] Restore completed, but no active entitlements found";
        emit errorOccurred(ErrorCode::ApiNoPurchasedSubscriptionsError);
        return false;
    }

    const bool isTestPurchase = IosController::Instance()->isTestFlight();
    const QString serviceType = m_apiServicesModel->getSelectedServiceType();
    const QString serviceProtocol = m_apiServicesModel->getSelectedServiceProtocol();
    const QString countryCode = m_apiServicesModel->getCountryCode();
    const QString appLanguage = m_settings->getAppLanguage().name().split("_").first();
    const QString installationUuid = m_settings->getInstallationUuid(true);

    bool hasInstalledConfig = false;
    bool duplicateConfigAlreadyPresent = false;
    int duplicateServerIndex = -1;
    QSet<QString> processedOriginalTransactionIds;

    for (const QVariantMap &transaction : restoredTransactions) {
        const QString originalTransactionId = transaction.value(QStringLiteral("originalTransactionId")).toString();
        const QString transactionId = transaction.value(QStringLiteral("transactionId")).toString();
        const QString productId = transaction.value(QStringLiteral("productId")).toString();

        if (originalTransactionId.isEmpty()) {
            qWarning().noquote() << "[IAP] Skipping restored transaction without originalTransactionId" << transactionId;
            continue;
        }

        if (processedOriginalTransactionIds.contains(originalTransactionId)) {
            qInfo().noquote() << "[IAP] Skipping duplicate restored transaction" << originalTransactionId;
            continue;
        }
        processedOriginalTransactionIds.insert(originalTransactionId);

        qInfo().noquote() << "[IAP] Restoring subscription. transactionId =" << transactionId
                          << "originalTransactionId =" << originalTransactionId << "productId =" << productId;

        GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                                QString(APP_VERSION),
                                                appLanguage,
                                                installationUuid,
                                                countryCode,
                                                "",
                                                serviceType,
                                                serviceProtocol,
                                                QJsonObject() };

        QJsonObject apiPayload = gatewayRequestData.toJsonObject();
        apiPayload[apiDefs::key::transactionId] = originalTransactionId;

        QByteArray responseBody;
        ErrorCode errorCode = executeRequest(QString("%1v1/subscriptions"), apiPayload, responseBody, isTestPurchase);
        if (errorCode != ErrorCode::NoError) {
            qWarning().noquote() << "[IAP] Failed to restore transaction" << originalTransactionId
                                 << "errorCode =" << static_cast<int>(errorCode);
            continue;
        }

        int currentDuplicateServerIndex = -1;
        errorCode = importServiceFromBilling(responseBody, isTestPurchase, currentDuplicateServerIndex);
        if (errorCode == ErrorCode::ApiConfigAlreadyAdded) {
            duplicateConfigAlreadyPresent = true;
            if (duplicateServerIndex < 0) {
                duplicateServerIndex = currentDuplicateServerIndex;
            }
            qInfo().noquote() << "[IAP] Subscription config with the same vpn_key already exists" << originalTransactionId;
            continue;
        }

        if (errorCode != ErrorCode::NoError) {
            qWarning().noquote() << "[IAP] Failed to process restored subscription response for transaction" << originalTransactionId
                                 << "errorCode =" << static_cast<int>(errorCode);
            continue;
        }

        hasInstalledConfig = true;
    }

    if (!hasInstalledConfig) {
        if (duplicateConfigAlreadyPresent) {
            emit installServerFromApiFinished(tr("This subscription has already been added"), duplicateServerIndex);
            return true;
        }

        emit errorOccurred(ErrorCode::ApiPurchaseError);
        return false;
    }

    emit installServerFromApiFinished(tr("Subscription restored successfully."));
#endif
    return true;
}

bool ApiConfigsController::importFreeFromGateway()
{
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getAppLanguage().name().split("_").first(),
                                            m_settings->getInstallationUuid(true),
                                            m_apiServicesModel->getCountryCode(),
                                            "",
                                            m_apiServicesModel->getSelectedServiceType(),
                                            m_apiServicesModel->getSelectedServiceProtocol(),
                                            QJsonObject() };

    if (m_serversModel->isServerFromApiAlreadyExists(gatewayRequestData.userCountryCode, gatewayRequestData.serviceType,
                                                     gatewayRequestData.serviceProtocol)) {
        emit errorOccurred(ErrorCode::ApiConfigAlreadyAdded);
        return false;
    }

    ProtocolData protocolData = generateProtocolData(gatewayRequestData.serviceProtocol);

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    appendProtocolDataToApiPayload(gatewayRequestData.serviceProtocol, protocolData, apiPayload);

    ErrorCode errorCode;
    QByteArray responseBody;

    errorCode = executeRequest(QString("%1v1/config"), apiPayload, responseBody);

    QJsonObject serverConfig;
    if (errorCode == ErrorCode::NoError) {
        errorCode = fillServerConfig(gatewayRequestData.serviceProtocol, protocolData, responseBody, serverConfig);
        if (errorCode != ErrorCode::NoError) {
            emit errorOccurred(errorCode);
            return false;
        }

        QJsonObject apiConfig = serverConfig.value(configKey::apiConfig).toObject();
        apiConfig.insert(configKey::userCountryCode, m_apiServicesModel->getCountryCode());
        apiConfig.insert(configKey::serviceType, m_apiServicesModel->getSelectedServiceType());
        apiConfig.insert(configKey::serviceProtocol, m_apiServicesModel->getSelectedServiceProtocol());

        serverConfig.insert(configKey::apiConfig, apiConfig);

        m_serversModel->addServer(serverConfig);
        emit installServerFromApiFinished(tr("%1 installed successfully.").arg(m_apiServicesModel->getSelectedServiceName()));
        return true;
    } else {
        emit errorOccurred(errorCode);
        return false;
    }
}

bool ApiConfigsController::importTrialFromGateway(const QString &email)
{
    emit trialEmailError(QString());

    const QString trimmedEmail = email.trimmed();
    if (trimmedEmail.isEmpty()) {
        emit errorOccurred(ErrorCode::ApiConfigEmptyError);
        return false;
    }

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getAppLanguage().name().split("_").first(),
                                            m_settings->getInstallationUuid(true),
                                            m_apiServicesModel->getCountryCode(),
                                            "",
                                            m_apiServicesModel->getSelectedServiceType(),
                                            m_apiServicesModel->getSelectedServiceProtocol(),
                                            QJsonObject() };

    ProtocolData protocolData = generateProtocolData(gatewayRequestData.serviceProtocol);

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    appendProtocolDataToApiPayload(gatewayRequestData.serviceProtocol, protocolData, apiPayload);
    apiPayload.insert(apiDefs::key::email, trimmedEmail);

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/trial"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        if (errorCode == ErrorCode::ApiTrialAlreadyUsedError) {
            emit trialEmailError(tr("This email address has already been used to activate a trial. If you like the service, you can upgrade to Premium"));
            return false;
        }
        emit errorOccurred(errorCode);
        return false;
    }

    QJsonObject responseObject = QJsonDocument::fromJson(responseBody).object();
    QString key = responseObject.value(apiDefs::key::config).toString();
    if (key.isEmpty()) {
        qWarning().noquote() << "[Trial] trial response does not contain config field";
        emit errorOccurred(ErrorCode::ApiConfigEmptyError);
        return false;
    }

    key.replace(QStringLiteral("vpn://"), QString());
    QByteArray configBytes = QByteArray::fromBase64(key.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QByteArray uncompressed = qUncompress(configBytes);
    if (!uncompressed.isEmpty()) {
        configBytes = uncompressed;
    }
    if (configBytes.isEmpty()) {
        qWarning().noquote() << "[Trial] trial response config payload is empty";
        emit errorOccurred(ErrorCode::ApiConfigEmptyError);
        return false;
    }

    QJsonObject configObject = QJsonDocument::fromJson(configBytes).object();
    quint16 crc = qChecksum(QJsonDocument(configObject).toJson());
    configObject.insert(config_key::crc, crc);
    m_serversModel->addServer(configObject);

    emit installServerFromApiFinished(tr("%1 installed successfully.").arg(m_apiServicesModel->getSelectedServiceName()));
    return true;
}

bool ApiConfigsController::updateServiceFromGateway(const int serverIndex, const QString &newCountryCode, const QString &newCountryName,
                                                    bool reloadServiceConfig)
{
    auto serverConfig = m_serversModel->getServerConfig(serverIndex);
    auto apiConfig = serverConfig.value(configKey::apiConfig).toObject();

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getAppLanguage().name().split("_").first(),
                                            m_settings->getInstallationUuid(true),
                                            apiConfig.value(configKey::userCountryCode).toString(),
                                            newCountryCode,
                                            apiConfig.value(configKey::serviceType).toString(),
                                            apiConfig.value(configKey::serviceProtocol).toString(),
                                            serverConfig.value(configKey::authData).toObject() };

    ProtocolData protocolData = generateProtocolData(gatewayRequestData.serviceProtocol);

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    appendProtocolDataToApiPayload(gatewayRequestData.serviceProtocol, protocolData, apiPayload);

    if (newCountryCode.isEmpty() && newCountryName.isEmpty() && !reloadServiceConfig) {
        apiPayload.insert(configKey::isConnectEvent, true);
    }

    bool isTestPurchase = apiConfig.value(apiDefs::key::isTestPurchase).toBool(false);
    bool wasSubscriptionExpired = m_serversModel->data(serverIndex, ServersModel::IsSubscriptionExpiredRole).toBool();
    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/config"), apiPayload, responseBody, isTestPurchase);

    QJsonObject newServerConfig;
    if (errorCode == ErrorCode::NoError) {
        errorCode = fillServerConfig(gatewayRequestData.serviceProtocol, protocolData, responseBody, newServerConfig);
        if (errorCode != ErrorCode::NoError) {
            emit errorOccurred(errorCode);
            return false;
        }

        QJsonObject newApiConfig = newServerConfig.value(configKey::apiConfig).toObject();
        newApiConfig.insert(configKey::userCountryCode, apiConfig.value(configKey::userCountryCode));
        newApiConfig.insert(configKey::serviceType, apiConfig.value(configKey::serviceType));
        newApiConfig.insert(configKey::serviceProtocol, apiConfig.value(configKey::serviceProtocol));
        newApiConfig.insert(apiDefs::key::vpnKey, apiConfig.value(apiDefs::key::vpnKey));
        if (apiConfig.contains(apiDefs::key::isInAppPurchase)) {
            newApiConfig.insert(apiDefs::key::isInAppPurchase, apiConfig.value(apiDefs::key::isInAppPurchase));
        }
        if (apiConfig.contains(apiDefs::key::isTestPurchase)) {
            newApiConfig.insert(apiDefs::key::isTestPurchase, apiConfig.value(apiDefs::key::isTestPurchase));
        }

        newServerConfig.insert(configKey::apiConfig, newApiConfig);
        newServerConfig.insert(configKey::authData, gatewayRequestData.authData);
        newServerConfig.insert(config_key::crc, serverConfig.value(config_key::crc));

        if (serverConfig.value(config_key::nameOverriddenByUser).toBool()) {
            newServerConfig.insert(config_key::name, serverConfig.value(config_key::name));
            newServerConfig.insert(config_key::nameOverriddenByUser, true);
        }
        m_serversModel->editServer(newServerConfig, serverIndex);

        if (wasSubscriptionExpired) {
            emit subscriptionRefreshNeeded();
        }

        if (reloadServiceConfig) {
            emit reloadServerFromApiFinished(tr("API config reloaded"));
        } else if (newCountryName.isEmpty()) {
            emit updateServerFromApiFinished();
        } else {
            emit changeApiCountryFinished(tr("Successfully changed the country of connection to %1").arg(newCountryName));
        }
        return true;
    } else {
        if (errorCode == ErrorCode::ApiSubscriptionExpiredError) {
            if (!apiConfig.value(apiDefs::key::isInAppPurchase).toBool(false)) {
                apiConfig.insert(apiDefs::key::subscriptionExpiredByServer, true);
                serverConfig.insert(configKey::apiConfig, apiConfig);
                m_serversModel->editServer(serverConfig, serverIndex);
                emit subscriptionExpiredOnServer();
            } else {
                emit errorOccurred(errorCode);
            }
        } else {
            emit errorOccurred(errorCode);
        }
        return false;
    }
}

bool ApiConfigsController::updateServiceFromTelegram(const int serverIndex)
{
#ifdef Q_OS_IOS
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_settings->isStrictKillSwitchEnabled());

    auto serverConfig = m_serversModel->getServerConfig(serverIndex);
    auto installationUuid = m_settings->getInstallationUuid(true);

    QString serviceProtocol = serverConfig.value(configKey::protocol).toString();
    ProtocolData protocolData = generateProtocolData(serviceProtocol);

    QJsonObject apiPayload;
    appendProtocolDataToApiPayload(serviceProtocol, protocolData, apiPayload);
    apiPayload[configKey::uuid] = installationUuid;
    apiPayload[configKey::osVersion] = QSysInfo::productType();
    apiPayload[configKey::appVersion] = QString(APP_VERSION);
    apiPayload[configKey::accessToken] = serverConfig.value(configKey::accessToken).toString();
    apiPayload[configKey::apiEndpoint] = serverConfig.value(configKey::apiEndpoint).toString();

    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/proxy_config"), apiPayload, responseBody);

    if (errorCode == ErrorCode::NoError) {
        errorCode = fillServerConfig(serviceProtocol, protocolData, responseBody, serverConfig);
        if (errorCode != ErrorCode::NoError) {
            emit errorOccurred(errorCode);
            return false;
        }

        m_serversModel->editServer(serverConfig, serverIndex);
        emit updateServerFromApiFinished();
        return true;
    } else {
        emit errorOccurred(errorCode);
        return false;
    }
}

bool ApiConfigsController::deactivateDevice(const bool isRemoveEvent)
{
    auto serverIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfigObject = m_serversModel->getServerConfig(serverIndex);
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    if (!apiUtils::isPremiumServer(serverConfigObject)) {
        return true;
    }

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getAppLanguage().name().split("_").first(),
                                            m_settings->getInstallationUuid(true),
                                            apiConfigObject.value(configKey::userCountryCode).toString(),
                                            apiConfigObject.value(configKey::serverCountryCode).toString(),
                                            apiConfigObject.value(configKey::serviceType).toString(),
                                            "",
                                            serverConfigObject.value(configKey::authData).toObject() };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    bool isTestPurchase = apiConfigObject.value(apiDefs::key::isTestPurchase).toBool(false);
    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_config"), apiPayload, responseBody, isTestPurchase);

    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        emit errorOccurred(errorCode);
        return false;
    }

    serverConfigObject.remove(config_key::containers);
    m_serversModel->editServer(serverConfigObject, serverIndex);

    return true;
}

bool ApiConfigsController::deactivateExternalDevice(const QString &uuid, const QString &serverCountryCode)
{
    auto serverIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfigObject = m_serversModel->getServerConfig(serverIndex);
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    if (!apiUtils::isPremiumServer(serverConfigObject)) {
        return true;
    }

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getAppLanguage().name().split("_").first(),
                                            uuid,
                                            apiConfigObject.value(configKey::userCountryCode).toString(),
                                            serverCountryCode,
                                            apiConfigObject.value(configKey::serviceType).toString(),
                                            "",
                                            serverConfigObject.value(configKey::authData).toObject() };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    bool isTestPurchase = apiConfigObject.value(apiDefs::key::isTestPurchase).toBool(false);
    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_config"), apiPayload, responseBody, isTestPurchase);
    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        emit errorOccurred(errorCode);
        return false;
    }

    if (uuid == m_settings->getInstallationUuid(true)) {
        serverConfigObject.remove(config_key::containers);
        m_serversModel->editServer(serverConfigObject, serverIndex);
    }

    return true;
}

bool ApiConfigsController::isConfigValid()
{
    int serverIndex = m_serversModel->getDefaultServerIndex();
    QJsonObject serverConfigObject = m_serversModel->getServerConfig(serverIndex);
    auto configSource = apiUtils::getConfigSource(serverConfigObject);

    if (configSource == apiDefs::ConfigSource::Telegram
        && !m_serversModel->data(serverIndex, ServersModel::Roles::HasInstalledContainers).toBool()) {
        m_serversModel->removeApiConfig(serverIndex);
        return updateServiceFromTelegram(serverIndex);
    } else if (configSource == apiDefs::ConfigSource::AmneziaGateway
               && !m_serversModel->data(serverIndex, ServersModel::Roles::HasInstalledContainers).toBool()) {
        return updateServiceFromGateway(serverIndex, "", "");
    } else if (configSource && m_serversModel->isApiKeyExpired(serverIndex)) {
        qDebug() << "attempt to update api config by expires_at event";
        if (configSource == apiDefs::ConfigSource::AmneziaGateway) {
            return updateServiceFromGateway(serverIndex, "", "");
        } else {
            m_serversModel->removeApiConfig(serverIndex);
            return updateServiceFromTelegram(serverIndex);
        }
    }
    return true;
}

void ApiConfigsController::setCurrentProtocol(const QString &protocolName)
{
    auto serverIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfigObject = m_serversModel->getServerConfig(serverIndex);
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    apiConfigObject[configKey::serviceProtocol] = protocolName;

    serverConfigObject.insert(configKey::apiConfig, apiConfigObject);

    m_serversModel->editServer(serverConfigObject, serverIndex);
}

bool ApiConfigsController::isVlessProtocol()
{
    auto serverIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfigObject = m_serversModel->getServerConfig(serverIndex);
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    if (apiConfigObject[configKey::serviceProtocol].toString() == "vless") {
        return true;
    }
    return false;
}

QList<QString> ApiConfigsController::getQrCodes()
{
    return m_qrCodes;
}

int ApiConfigsController::getQrCodesCount()
{
    return static_cast<int>(m_qrCodes.size());
}

QString ApiConfigsController::getVpnKey()
{
    return m_vpnKey;
}

ErrorCode ApiConfigsController::importServiceFromBilling(const QByteArray &responseBody, const bool isTestPurchase,
                                                         int &duplicateServerIndex)
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    duplicateServerIndex = -1;
    QJsonObject responseObject = QJsonDocument::fromJson(responseBody).object();
    const QString rawVpnKey = responseObject.value(QStringLiteral("key")).toString();
    if (rawVpnKey.isEmpty()) {
        qWarning().noquote() << "[IAP] Subscription response does not contain a key field";
        return ErrorCode::ApiPurchaseError;
    }

    QString normalizedVpnKey = rawVpnKey;
    normalizedVpnKey.replace(QStringLiteral("vpn://"), QString());

    duplicateServerIndex = m_serversModel->indexOfServerWithVpnKey(normalizedVpnKey);
    if (duplicateServerIndex >= 0) {
        qInfo().noquote() << "[IAP] Subscription config with the same vpn_key already exists";
        return ErrorCode::ApiConfigAlreadyAdded;
    }

    QByteArray configPayload =
            QByteArray::fromBase64(normalizedVpnKey.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QByteArray configUncompressed = qUncompress(configPayload);
    const bool payloadWasCompressed = !configUncompressed.isEmpty();
    if (payloadWasCompressed) {
        configPayload = configUncompressed;
    }

    if (configPayload.isEmpty()) {
        qWarning().noquote() << "[IAP] Subscription response config payload is empty";
        return ErrorCode::ApiPurchaseError;
    }

    QJsonObject configObject = QJsonDocument::fromJson(configPayload).object();

    auto apiConfig = configObject.value(apiDefs::key::apiConfig).toObject();
    apiConfig.insert(apiDefs::key::isTestPurchase, isTestPurchase);
    apiConfig.insert(apiDefs::key::isInAppPurchase, true);
    configObject.insert(apiDefs::key::apiConfig, apiConfig);

    configPayload = QJsonDocument(configObject).toJson();
    if (payloadWasCompressed) {
        configPayload = qCompress(configPayload, 8);
    }
    normalizedVpnKey = QString(configPayload.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));

    duplicateServerIndex = m_serversModel->indexOfServerWithVpnKey(normalizedVpnKey);
    if (duplicateServerIndex >= 0) {
        qInfo().noquote() << "[IAP] Subscription config with the same vpn_key already exists";
        return ErrorCode::ApiConfigAlreadyAdded;
    }

    apiConfig.insert(apiDefs::key::vpnKey, normalizedVpnKey);
    configObject.insert(apiDefs::key::apiConfig, apiConfig);

    quint16 crc = qChecksum(QJsonDocument(configObject).toJson());
    configObject.insert(config_key::crc, crc);
    m_serversModel->addServer(configObject);

    return ErrorCode::NoError;
#else
    Q_UNUSED(responseBody)
    Q_UNUSED(isTestPurchase)
    duplicateServerIndex = -1;
    return ErrorCode::NoError;
#endif
}

ErrorCode ApiConfigsController::executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody,
                                               bool isTestPurchase)
{
    GatewayController gatewayController(m_settings->getGatewayEndpoint(isTestPurchase), m_settings->isDevGatewayEnv(isTestPurchase),
                                        apiDefs::requestTimeoutMsecs, m_settings->isStrictKillSwitchEnabled());
    return gatewayController.post(endpoint, apiPayload, responseBody);
}
