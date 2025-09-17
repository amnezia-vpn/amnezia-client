#include "apiConfigsController.h"

#include <QClipboard>
#include <QEventLoop>
#include <algorithm>

#include "amnezia_application.h"
#include "configurators/wireguard_configurator.h"
#include "core/api/apiDefs.h"
#include "core/api/apiUtils.h"
#include "core/controllers/gatewayController.h"
#include "core/qrCodeUtils.h"
#include "ui/controllers/systemController.h"
#include "version.h"

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
        constexpr char appLanguage[] = "app_language";

        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serverCountryCode[] = "server_country_code";
        constexpr char serviceType[] = "service_type";
        constexpr char serviceInfo[] = "service_info";
        constexpr char serviceProtocol[] = "service_protocol";

        constexpr char apiPayload[] = "api_payload";
        constexpr char keyPayload[] = "key_payload";

        constexpr char apiConfig[] = "api_config";
        constexpr char authData[] = "auth_data";

        constexpr char config[] = "config";

        constexpr char subscription[] = "subscription";
        constexpr char endDate[] = "end_date";
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
            auto container = containers.at(0).toObject();
            QString containerName = ContainerProps::containerTypeToString(DockerContainer::Awg);
            auto serverProtocolConfig = container.value(containerName).toObject();
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
            serverProtocolConfig[config_key::controlledJunk1] = clientProtocolConfig.value(config_key::controlledJunk1);
            serverProtocolConfig[config_key::controlledJunk2] = clientProtocolConfig.value(config_key::controlledJunk2);
            serverProtocolConfig[config_key::controlledJunk3] = clientProtocolConfig.value(config_key::controlledJunk3);
            serverProtocolConfig[config_key::specialHandshakeTimeout] = clientProtocolConfig.value(config_key::specialHandshakeTimeout);

            //

            container[containerName] = serverProtocolConfig;
            containers.replace(0, container);
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
        }

        serverConfig[configKey::apiConfig] = apiConfig;

        return ErrorCode::NoError;
    }

    bool isSubscriptionExpired(const QJsonObject &apiConfig)
    {
        auto subscription = apiConfig.value(configKey::subscription).toObject();
        if (subscription.isEmpty()) {
            return false;
        }
        auto subscriptionEndDate = subscription.value(configKey::endDate).toString();
        if (apiUtils::isSubscriptionExpired(subscriptionEndDate)) {
            return true;
        }
        return false;
    }
}

ApiConfigsController::ApiConfigsController(const QSharedPointer<ServersModel> &serversModel,
                                           const QSharedPointer<ApiServicesModel> &apiServicesModel,
                                           const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_apiServicesModel(apiServicesModel), m_settings(settings)
{
}

bool ApiConfigsController::exportNativeConfig(const QString &serverCountryCode, const QString &fileName)
{
    if (fileName.isEmpty()) {
        emit errorOccurred(ErrorCode::PermissionsError);
        return false;
    }

    auto serverConfigObject = m_serversModel->getServerConfig(m_serversModel->getProcessedServerIndex());
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    if (isSubscriptionExpired(apiConfigObject)) {
        emit errorOccurred(ErrorCode::ApiSubscriptionExpiredError);
        return false;
    }

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
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

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/native_config"), apiPayload, responseBody);
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

    if (isSubscriptionExpired(apiConfigObject)) {
        emit errorOccurred(ErrorCode::ApiSubscriptionExpiredError);
        return false;
    }

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getInstallationUuid(true),
                                            apiConfigObject.value(configKey::userCountryCode).toString(),
                                            serverCountryCode,
                                            apiConfigObject.value(configKey::serviceType).toString(),
                                            configKey::awg, // apiConfigObject.value(configKey::serviceProtocol).toString(),
                                            serverConfigObject.value(configKey::authData).toObject() };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_native_config"), apiPayload, responseBody);
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
    // iOS: only fetch and log available IAP products (no purchase here)
#ifdef Q_OS_IOS
    {
        const QStringList productIds = { QStringLiteral("com.amnezia.amneziavpn.1_month"), QStringLiteral("com.amnezia.AmneziaVPN.6_month") };
        IosController::Instance()->fetchProducts(
                productIds, [](const QList<QVariantMap> &products, const QStringList &invalidIds, const QString &errorString) {
                    if (!errorString.isEmpty()) {
                        qDebug() << "IAP fetch error:" << errorString;
                        return;
                    }
                    for (const auto &p : products) {
                        qDebug().nospace() << "IAP product id=" << p.value("productId").toString()
                                           << ", title=" << p.value("title").toString() << ", price=" << p.value("price").toString() << " "
                                           << p.value("currencyCode").toString();
                    }
                    if (!invalidIds.isEmpty()) {
                        qDebug() << "Invalid IAP IDs:" << invalidIds;
                    }
                });
    }
#endif

    QJsonObject apiPayload;
    apiPayload[configKey::osVersion] = QSysInfo::productType();
    apiPayload[configKey::appLanguage] = m_settings->getAppLanguage().name().split("_").first();

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
    m_apiServicesModel->updateModel(data);
    return true;
}

bool ApiConfigsController::importSerivceFromAppStore()
{
#ifdef Q_OS_IOS
    QString chosenProductId;
    {
        const QStringList productIds = { QStringLiteral("com.amnezia.amneziavpn.1_month"), QStringLiteral("com.amnezia.AmneziaVPN.6_month") };

        QList<QVariantMap> products;
        QStringList invalidIds;
        QString fetchError;
        QEventLoop waitFetch;
        IosController::Instance()->fetchProducts(productIds,
                                                 [&](const QList<QVariantMap> &prods, const QStringList &invalid, const QString &err) {
                                                     products = prods;
                                                     invalidIds = invalid;
                                                     fetchError = err;
                                                     waitFetch.quit();
                                                 });
        waitFetch.exec();

        if (fetchError.isEmpty()) {
            for (const auto &candidate : productIds) {
                if (invalidIds.contains(candidate))
                    continue;
                const bool found = std::any_of(products.begin(), products.end(),
                                               [&](const QVariantMap &p) { return p.value("productId").toString() == candidate; });
                if (found) {
                    chosenProductId = candidate;
                    break;
                }
            }
        }
        if (chosenProductId.isEmpty() && !productIds.isEmpty()) {
            chosenProductId = productIds.first();
        }
    }

    bool purchaseOk = false;
    QString originalTransactionId;
    QString purchaseError;
    QEventLoop waitPurchase;
    IosController::Instance()->purchaseProduct(chosenProductId,
                                               [&](bool success, const QString &transactionId, const QString &purchasedProductId,
                                                   const QString &originalTxId, const QString &errorString) {
                                                   Q_UNUSED(transactionId)
                                                   Q_UNUSED(purchasedProductId)
                                                   purchaseOk = success;
                                                   originalTransactionId = originalTxId;
                                                   purchaseError = errorString;
                                                   waitPurchase.quit();
                                               });
    waitPurchase.exec();

    if (!purchaseOk || originalTransactionId.isEmpty()) {
        qDebug() << "IAP purchase failed:" << purchaseError;
        emit errorOccurred(ErrorCode::ApiPurchaseError);
        return false;
    }

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getInstallationUuid(true),
                                            m_apiServicesModel->getCountryCode(),
                                            "",
                                            m_apiServicesModel->getSelectedServiceType(),
                                            m_apiServicesModel->getSelectedServiceProtocol(),
                                            QJsonObject() };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    apiPayload[apiDefs::key::transactionId] = originalTransactionId;

    ErrorCode errorCode;
    QByteArray responseBody;
    errorCode = executeRequest(QString("%1v1/subscriptions"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    QString key = QJsonDocument::fromJson(responseBody).object().value("key").toString();
    key.replace("vpn://", "");
    QByteArray config = QByteArray::fromBase64(key.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QByteArray configUncompressed = qUncompress(config);
    if (!configUncompressed.isEmpty()) {
        config = configUncompressed;
    }

    QJsonObject configJson = QJsonDocument::fromJson(config).object();

    quint16 crc = qChecksum(QJsonDocument(configJson).toJson());
    auto apiConfig = configJson.value(apiDefs::key::apiConfig).toObject();
    apiConfig[apiDefs::key::vpnKey] = key;
    configJson.insert(apiDefs::key::apiConfig, apiConfig);
    configJson.insert(config_key::crc, crc);
    m_serversModel->addServer(configJson);

    qDebug() << configJson;
    emit installServerFromApiFinished(tr("%1 installed successfully.").arg(m_apiServicesModel->getSelectedServiceName()));
#endif
    return true;
}

bool ApiConfigsController::importServiceFromGateway()
{
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
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

bool ApiConfigsController::updateServiceFromGateway(const int serverIndex, const QString &newCountryCode, const QString &newCountryName,
                                                    bool reloadServiceConfig)
{
    auto serverConfig = m_serversModel->getServerConfig(serverIndex);
    auto apiConfig = serverConfig.value(configKey::apiConfig).toObject();

    if (isSubscriptionExpired(apiConfig)) {
        emit errorOccurred(ErrorCode::ApiSubscriptionExpiredError);
        return false;
    }

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getInstallationUuid(true),
                                            apiConfig.value(configKey::userCountryCode).toString(),
                                            newCountryCode,
                                            apiConfig.value(configKey::serviceType).toString(),
                                            apiConfig.value(configKey::serviceProtocol).toString(),
                                            serverConfig.value(configKey::authData).toObject() };

    ProtocolData protocolData = generateProtocolData(gatewayRequestData.serviceProtocol);

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    appendProtocolDataToApiPayload(gatewayRequestData.serviceProtocol, protocolData, apiPayload);

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/config"), apiPayload, responseBody);

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

        newServerConfig.insert(configKey::apiConfig, newApiConfig);
        newServerConfig.insert(configKey::authData, gatewayRequestData.authData);
        newServerConfig.insert(config_key::crc, serverConfig.value(config_key::crc));

        if (serverConfig.value(config_key::nameOverriddenByUser).toBool()) {
            newServerConfig.insert(config_key::name, serverConfig.value(config_key::name));
            newServerConfig.insert(config_key::nameOverriddenByUser, true);
        }
        m_serversModel->editServer(newServerConfig, serverIndex);
        if (reloadServiceConfig) {
            emit reloadServerFromApiFinished(tr("API config reloaded"));
        } else if (newCountryName.isEmpty()) {
            emit updateServerFromApiFinished();
        } else {
            emit changeApiCountryFinished(tr("Successfully changed the country of connection to %1").arg(newCountryName));
        }
        return true;
    } else {
        emit errorOccurred(errorCode);
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

bool ApiConfigsController::deactivateDevice()
{
    auto serverIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfigObject = m_serversModel->getServerConfig(serverIndex);
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    if (!apiUtils::isPremiumServer(serverConfigObject)) {
        return true;
    }

    if (isSubscriptionExpired(apiConfigObject)) {
        emit errorOccurred(ErrorCode::ApiSubscriptionExpiredError);
        return false;
    }

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getInstallationUuid(true),
                                            apiConfigObject.value(configKey::userCountryCode).toString(),
                                            apiConfigObject.value(configKey::serverCountryCode).toString(),
                                            apiConfigObject.value(configKey::serviceType).toString(),
                                            "",
                                            serverConfigObject.value(configKey::authData).toObject() };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_config"), apiPayload, responseBody);
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

    if (isSubscriptionExpired(apiConfigObject)) {
        emit errorOccurred(ErrorCode::ApiSubscriptionExpiredError);
        return false;
    }

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            uuid,
                                            apiConfigObject.value(configKey::userCountryCode).toString(),
                                            serverCountryCode,
                                            apiConfigObject.value(configKey::serviceType).toString(),
                                            "",
                                            serverConfigObject.value(configKey::authData).toObject() };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_config"), apiPayload, responseBody);
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
    return m_qrCodes.size();
}

QString ApiConfigsController::getVpnKey()
{
    return m_vpnKey;
}

ErrorCode ApiConfigsController::executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody)
{
    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_settings->isStrictKillSwitchEnabled());
    return gatewayController.post(endpoint, apiPayload, responseBody);
}
