#include "subscriptionController.h"

#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QSet>
#include <QSysInfo>
#include <QUuid>
#include <QVariantMap>

#include "core/configurators/openVpnConfigurator.h"
#include "core/configurators/wireguardConfigurator.h"
#include "containers/containers_defs.h"
#include "core/utils/api/apiDefs.h"
#include "core/utils/api/apiUtils.h"
#include "core/controllers/gatewayController.h"
#include "core/controllers/serversController.h"
#include "protocols/protocols_defs.h"
#include "version.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
#endif

using namespace amnezia;

namespace
{
    namespace configKey
    {
        constexpr char cloak[] = "cloak";
        constexpr char awg[] = "awg";
        constexpr char vless[] = "vless";

        constexpr char osVersion[] = "os_version";
        constexpr char appVersion[] = "app_version";
        constexpr char uuid[] = "installation_uuid";
        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serverCountryCode[] = "server_country_code";
        constexpr char serviceType[] = "service_type";
        constexpr char serviceProtocol[] = "service_protocol";
        constexpr char authData[] = "auth_data";
        constexpr char subscription[] = "subscription";
        constexpr char endDate[] = "end_date";
        constexpr char isConnectEvent[] = "is_connect_event";
        constexpr char config[] = "config";
        constexpr char certificate[] = "certificate";
        constexpr char publicKey[] = "public_key";
        constexpr char apiConfig[] = "api_config";
        constexpr char accessToken[] = "api_key";
        constexpr char apiEndpoint[] = "api_endpoint";
        constexpr char protocol[] = "protocol";
    }
}

SubscriptionController::SubscriptionController(ServersRepository* serversRepository,
                                               AppSettingsRepository* appSettingsRepository)
    : m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository)
{
}

QJsonObject SubscriptionController::GatewayRequestData::toJsonObject() const
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

SubscriptionController::ProtocolData SubscriptionController::generateProtocolData(const QString &protocol)
{
    ProtocolData protocolData;
    if (protocol == configKey::cloak) {
        auto certRequest = OpenVpnConfigurator::createCertRequest();
        protocolData.certRequest = certRequest.request;
        protocolData.certPrivKey = certRequest.privKey;
    } else if (protocol == configKey::awg) {
        auto connData = WireguardConfigurator::genClientKeys();
        protocolData.wireGuardClientPubKey = connData.clientPubKey;
        protocolData.wireGuardClientPrivKey = connData.clientPrivKey;
    } else if (protocol == configKey::vless) {
        protocolData.xrayUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    return protocolData;
}

void SubscriptionController::appendProtocolDataToApiPayload(const QString &protocol, const ProtocolData &protocolData, QJsonObject &apiPayload)
{
    if (protocol == configKey::cloak) {
        apiPayload[configKey::certificate] = protocolData.certRequest;
    } else if (protocol == configKey::awg) {
        apiPayload[configKey::publicKey] = protocolData.wireGuardClientPubKey;
    } else if (protocol == configKey::vless) {
        apiPayload[configKey::publicKey] = protocolData.xrayUuid;
    }
}

ErrorCode SubscriptionController::fillServerConfig(const QString &protocol, const ProtocolData &protocolData, const QByteArray &apiResponseBody,
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
        configStr.replace("$OPENVPN_PRIV_KEY", protocolData.certPrivKey);
    } else if (protocol == configKey::awg) {
        configStr.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", protocolData.wireGuardClientPrivKey);
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

        apiConfig.insert(apiDefs::key::serviceInfo,
                         QJsonDocument::fromJson(apiResponseBody).object().value(apiDefs::key::serviceInfo).toObject());
    }

    serverConfig[configKey::apiConfig] = apiConfig;

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody, bool isTestPurchase)
{
    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase), m_appSettingsRepository->isDevGatewayEnv(isTestPurchase), apiDefs::requestTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled());
    return gatewayController.post(endpoint, apiPayload, responseBody);
}

ErrorCode SubscriptionController::importServiceFromGateway(const QString &userCountryCode, const QString &serviceType,
                                                            const QString &serviceProtocol, const ProtocolData &protocolData,
                                                            QJsonObject &serverConfig)
{
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_appSettingsRepository->getAppLanguage().name().split("_").first(),
                                            m_appSettingsRepository->getInstallationUuid(true),
                                            userCountryCode,
                                            "",
                                            serviceType,
                                            serviceProtocol,
                                            QJsonObject() };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    appendProtocolDataToApiPayload(serviceProtocol, protocolData, apiPayload);

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    errorCode = fillServerConfig(serviceProtocol, protocolData, responseBody, serverConfig);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonObject apiConfig = serverConfig.value(configKey::apiConfig).toObject();
    apiConfig.insert(configKey::userCountryCode, userCountryCode);
    apiConfig.insert(configKey::serviceType, serviceType);
    apiConfig.insert(configKey::serviceProtocol, serviceProtocol);
    serverConfig.insert(configKey::apiConfig, apiConfig);

    m_serversRepository->addServer(serverConfig);
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::importServiceFromAppStore(const QString &userCountryCode, const QString &serviceType,
                                                            const QString &serviceProtocol, const ProtocolData &protocolData,
                                                            const QString &transactionId, bool isTestPurchase,
                                                            QJsonObject &serverConfig)
{
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_appSettingsRepository->getAppLanguage().name().split("_").first(),
                                            m_appSettingsRepository->getInstallationUuid(true),
                                            userCountryCode,
                                            "",
                                            serviceType,
                                            serviceProtocol,
                                            QJsonObject() };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    appendProtocolDataToApiPayload(serviceProtocol, protocolData, apiPayload);
    apiPayload[apiDefs::key::transactionId] = transactionId;

    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(),
                                        m_appSettingsRepository->isDevGatewayEnv(),
                                        apiDefs::requestTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled());

    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/subscriptions"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    // Parse the subscription response
    QJsonObject responseObject = QJsonDocument::fromJson(responseBody).object();
    QString key = responseObject.value(QStringLiteral("key")).toString();
    if (key.isEmpty()) {
        qWarning().noquote() << "[IAP] Subscription response does not contain a key field";
        return ErrorCode::ApiPurchaseError;
    }

    // Check if server with this VPN key already exists
    for (int i = 0; i < m_serversRepository->serversCount(); ++i) {
        QJsonObject existingServer = m_serversRepository->server(i);
        QJsonObject existingApiConfig = existingServer.value(configKey::apiConfig).toObject();
        if (existingApiConfig.value(apiDefs::key::vpnKey).toString() == key) {
            qInfo().noquote() << "[IAP] Subscription config with the same vpn_key already exists";
            return ErrorCode::ApiConfigAlreadyAdded;
        }
    }

    QString normalizedKey = key;
    normalizedKey.replace(QStringLiteral("vpn://"), QString());

    QByteArray configString = QByteArray::fromBase64(normalizedKey.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QByteArray configUncompressed = qUncompress(configString);
    if (!configUncompressed.isEmpty()) {
        configString = configUncompressed;
    }

    if (configString.isEmpty()) {
        qWarning().noquote() << "[IAP] Subscription response config payload is empty";
        return ErrorCode::ApiPurchaseError;
    }

    QJsonObject configObject = QJsonDocument::fromJson(configString).object();

    quint16 crc = qChecksum(QJsonDocument(configObject).toJson());
    auto apiConfig = configObject.value(apiDefs::key::apiConfig).toObject();
    apiConfig[apiDefs::key::vpnKey] = normalizedKey;
    apiConfig[apiDefs::key::isTestPurchase] = isTestPurchase;

    configObject.insert(apiDefs::key::apiConfig, apiConfig);
    configObject.insert(config_key::crc, crc);

    serverConfig = configObject;
    m_serversRepository->addServer(configObject);

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::updateServiceFromGateway(int serverIndex, const QString &newCountryCode, bool isConnectEvent,
                                                           const ProtocolData &protocolData)
{
    QJsonObject serverConfig = m_serversRepository->server(serverIndex);
    QJsonObject apiConfig = serverConfig.value(configKey::apiConfig).toObject();

    QString serviceProtocol = apiConfig.value(configKey::serviceProtocol).toString();
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_appSettingsRepository->getAppLanguage().name().split("_").first(),
                                            m_appSettingsRepository->getInstallationUuid(true),
                                            apiConfig.value(configKey::userCountryCode).toString(),
                                            newCountryCode,
                                            apiConfig.value(configKey::serviceType).toString(),
                                            serviceProtocol,
                                            serverConfig.value(configKey::authData).toObject() };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    appendProtocolDataToApiPayload(serviceProtocol, protocolData, apiPayload);

    if (isConnectEvent) {
        apiPayload[configKey::isConnectEvent] = true;
    }

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonObject newServerConfig;
    errorCode = fillServerConfig(serviceProtocol, protocolData, responseBody, newServerConfig);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonObject newApiConfig = newServerConfig.value(configKey::apiConfig).toObject();
    newApiConfig.insert(configKey::userCountryCode, apiConfig.value(configKey::userCountryCode));
    newApiConfig.insert(configKey::serviceType, apiConfig.value(configKey::serviceType));
    newApiConfig.insert(configKey::serviceProtocol, apiConfig.value(configKey::serviceProtocol));
    newApiConfig.insert(apiDefs::key::vpnKey, apiConfig.value(apiDefs::key::vpnKey));

    newServerConfig.insert(configKey::apiConfig, newApiConfig);
    newServerConfig.insert(configKey::authData, serverConfig.value(configKey::authData));
    newServerConfig.insert(config_key::crc, serverConfig.value(config_key::crc));

    if (serverConfig.value(config_key::nameOverriddenByUser).toBool()) {
        newServerConfig.insert(config_key::name, serverConfig.value(config_key::name));
        newServerConfig.insert(config_key::nameOverriddenByUser, true);
    }

    m_serversRepository->editServer(serverIndex, newServerConfig);
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::revokeServiceFromGateway(int serverIndex, bool isRemoveEvent)
{
    QJsonObject serverConfig = m_serversRepository->server(serverIndex);
    QJsonObject apiConfig = serverConfig.value(configKey::apiConfig).toObject();

    if (!apiUtils::isPremiumServer(serverConfig)) {
        return ErrorCode::NoError;
    }

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_appSettingsRepository->getAppLanguage().name().split("_").first(),
                                            m_appSettingsRepository->getInstallationUuid(true),
                                            apiConfig.value(configKey::userCountryCode).toString(),
                                            apiConfig.value(configKey::serverCountryCode).toString(),
                                            apiConfig.value(configKey::serviceType).toString(),
                                            "",
                                            serverConfig.value(configKey::authData).toObject() };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        return errorCode;
    }

    serverConfig.remove(config_key::containers);
    m_serversRepository->editServer(serverIndex, serverConfig);
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::revokeExternalDevice(int serverIndex, const QString &uuid, const QString &serverCountryCode)
{
    QJsonObject serverConfig = m_serversRepository->server(serverIndex);
    QJsonObject apiConfig = serverConfig.value(configKey::apiConfig).toObject();

    if (!apiUtils::isPremiumServer(serverConfig)) {
        return ErrorCode::NoError;
    }

    QJsonObject apiConfigForRevoke = apiConfig;
    apiConfigForRevoke[configKey::serverCountryCode] = serverCountryCode;

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_appSettingsRepository->getAppLanguage().name().split("_").first(),
                                            uuid,
                                            apiConfigForRevoke.value(configKey::userCountryCode).toString(),
                                            serverCountryCode,
                                            apiConfigForRevoke.value(configKey::serviceType).toString(),
                                            "",
                                            serverConfig.value(configKey::authData).toObject() };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        return errorCode;
    }

    if (uuid == m_appSettingsRepository->getInstallationUuid(true)) {
        serverConfig.remove(config_key::containers);
        m_serversRepository->editServer(serverIndex, serverConfig);
    }

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::exportNativeConfig(const QJsonObject &apiConfig, const QJsonObject &authData,
                                                      const QString &serverCountryCode, const QString &serviceProtocol,
                                                      const ProtocolData &protocolData, QString &nativeConfig)
{
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_appSettingsRepository->getAppLanguage().name().split("_").first(),
                                            m_appSettingsRepository->getInstallationUuid(true),
                                            apiConfig.value(configKey::userCountryCode).toString(),
                                            serverCountryCode,
                                            apiConfig.value(configKey::serviceType).toString(),
                                            serviceProtocol,
                                            authData };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    appendProtocolDataToApiPayload(serviceProtocol, protocolData, apiPayload);

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/native_config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonObject jsonConfig = QJsonDocument::fromJson(responseBody).object();
    nativeConfig = jsonConfig.value(configKey::config).toString();
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::revokeNativeConfig(const QJsonObject &apiConfig, const QJsonObject &authData,
                                                      const QString &serverCountryCode, const QString &serviceProtocol)
{
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_appSettingsRepository->getAppLanguage().name().split("_").first(),
                                            m_appSettingsRepository->getInstallationUuid(true),
                                            apiConfig.value(configKey::userCountryCode).toString(),
                                            serverCountryCode,
                                            apiConfig.value(configKey::serviceType).toString(),
                                            serviceProtocol,
                                            authData };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_native_config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        return errorCode;
    }

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::updateServiceFromTelegram(int serverIndex, const ProtocolData &protocolData)
{
    QJsonObject serverConfig = m_serversRepository->server(serverIndex);
    QString serviceProtocol = serverConfig.value(configKey::protocol).toString();
    QString installationUuid = m_appSettingsRepository->getInstallationUuid(true);

    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled());

    QJsonObject apiPayload;
    appendProtocolDataToApiPayload(serviceProtocol, protocolData, apiPayload);
    apiPayload[configKey::uuid] = installationUuid;
    apiPayload[configKey::osVersion] = QSysInfo::productType();
    apiPayload[configKey::appVersion] = QString(APP_VERSION);
    apiPayload[configKey::accessToken] = serverConfig.value(configKey::accessToken).toString();
    apiPayload[configKey::apiEndpoint] = serverConfig.value(configKey::apiEndpoint).toString();

    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/proxy_config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonObject newServerConfig;
    errorCode = fillServerConfig(serviceProtocol, protocolData, responseBody, newServerConfig);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    newServerConfig.insert(configKey::authData, serverConfig.value(configKey::authData));
    newServerConfig.insert(config_key::crc, serverConfig.value(config_key::crc));

    m_serversRepository->editServer(serverIndex, newServerConfig);
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::prepareVpnKeyExport(int serverIndex, QString &vpnKey)
{
    QJsonObject serverConfig = m_serversRepository->server(serverIndex);
    QJsonObject apiConfig = serverConfig.value(configKey::apiConfig).toObject();

    vpnKey = apiConfig.value(apiDefs::key::vpnKey).toString();
    if (vpnKey.isEmpty()) {
        vpnKey = apiUtils::getPremiumV2VpnKey(serverConfig);
        if (vpnKey.isEmpty()) {
            return ErrorCode::ApiConfigEmptyError;
        }
        apiConfig.insert(apiDefs::key::vpnKey, vpnKey);
        serverConfig.insert(configKey::apiConfig, apiConfig);
        m_serversRepository->editServer(serverIndex, serverConfig);
    }

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::validateAndUpdateConfig(int serverIndex, bool hasInstalledContainers, ServersController* serversController)
{
    QJsonObject serverConfigObject = serversController->getServerConfig(serverIndex);
    auto configSource = apiUtils::getConfigSource(serverConfigObject);

    if (configSource == apiDefs::ConfigSource::Telegram && !hasInstalledContainers) {
        serversController->removeApiConfig(serverIndex);
        ProtocolData protocolData = generateProtocolData(serverConfigObject.value(configKey::protocol).toString());
        return updateServiceFromTelegram(serverIndex, protocolData);
    } else if (configSource == apiDefs::ConfigSource::AmneziaGateway && !hasInstalledContainers) {
        ProtocolData protocolData = generateProtocolData(serverConfigObject.value(configKey::apiConfig).toObject().value(configKey::serviceProtocol).toString());
        return updateServiceFromGateway(serverIndex, "", false, protocolData);
    } else if (configSource && serversController->isApiKeyExpired(serverIndex)) {
        qDebug() << "attempt to update api config by expires_at event";
        if (configSource == apiDefs::ConfigSource::AmneziaGateway) {
            ProtocolData protocolData = generateProtocolData(serverConfigObject.value(configKey::apiConfig).toObject().value(configKey::serviceProtocol).toString());
            return updateServiceFromGateway(serverIndex, "", false, protocolData);
        } else {
            serversController->removeApiConfig(serverIndex);
            ProtocolData protocolData = generateProtocolData(serverConfigObject.value(configKey::protocol).toString());
            return updateServiceFromTelegram(serverIndex, protocolData);
        }
    }
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::processAppStorePurchase(const QString &userCountryCode, const QString &serviceType,
                                                          const QString &serviceProtocol, const QString &productId,
                                                          QJsonObject &serverConfig)
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    bool purchaseOk = false;
    QString originalTransactionId;
    QString storeTransactionId;
    QString storeProductId;
    QString purchaseError;
    QEventLoop waitPurchase;

    IosController::Instance()->purchaseProduct(productId,
                                               [&](bool success, const QString &txId, const QString &purchasedProductId,
                                                   const QString &originalTxId, const QString &errorString) {
                                                   purchaseOk = success;
                                                   originalTransactionId = originalTxId;
                                                   storeTransactionId = txId;
                                                   storeProductId = purchasedProductId;
                                                   purchaseError = errorString;
                                                   waitPurchase.quit();
                                               });
    waitPurchase.exec();

    if (!purchaseOk || originalTransactionId.isEmpty()) {
        qDebug() << "IAP purchase failed:" << purchaseError;
        return ErrorCode::ApiPurchaseError;
    }
    qInfo().noquote() << "[IAP] Purchase success. transactionId =" << storeTransactionId
                      << "originalTransactionId =" << originalTransactionId << "productId =" << storeProductId;

    bool isTestPurchase = IosController::Instance()->isTestFlight();

    ProtocolData protocolData = generateProtocolData(serviceProtocol);
    return importServiceFromAppStore(userCountryCode, serviceType, serviceProtocol, protocolData, originalTransactionId, isTestPurchase, serverConfig);
#else
    Q_UNUSED(userCountryCode);
    Q_UNUSED(serviceType);
    Q_UNUSED(serviceProtocol);
    Q_UNUSED(productId);
    Q_UNUSED(serverConfig);
    return ErrorCode::ApiPurchaseError;
#endif
}

SubscriptionController::AppStoreRestoreResult SubscriptionController::processAppStoreRestore(const QString &userCountryCode, const QString &serviceType,
                                                                                             const QString &serviceProtocol)
{
    AppStoreRestoreResult result;

#if defined(Q_OS_IOS) || defined(MACOS_NE)
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
        result.errorCode = ErrorCode::ApiPurchaseError;
        return result;
    }

    if (restoredTransactions.isEmpty()) {
        qInfo().noquote() << "[IAP] Restore completed, but no transactions were returned";
        result.errorCode = ErrorCode::ApiPurchaseError;
        return result;
    }

    bool isTestPurchase = IosController::Instance()->isTestFlight();
    QSet<QString> processedTransactions;

    for (const QVariantMap &transaction : restoredTransactions) {
        const QString originalTransactionId = transaction.value(QStringLiteral("originalTransactionId")).toString();
        const QString transactionId = transaction.value(QStringLiteral("transactionId")).toString();
        const QString transactionProductId = transaction.value(QStringLiteral("productId")).toString();

        if (originalTransactionId.isEmpty()) {
            qWarning().noquote() << "[IAP] Skipping restored transaction without originalTransactionId" << transactionId;
            continue;
        }

        if (processedTransactions.contains(originalTransactionId)) {
            result.duplicateCount++;
            continue;
        }
        processedTransactions.insert(originalTransactionId);

        qInfo().noquote() << "[IAP] Restoring subscription. transactionId =" << transactionId
                          << "originalTransactionId =" << originalTransactionId << "productId =" << transactionProductId;

        ProtocolData protocolData = generateProtocolData(serviceProtocol);
        QJsonObject serverConfig;
        ErrorCode errorCode = importServiceFromAppStore(userCountryCode, serviceType, serviceProtocol, protocolData,
                                                        originalTransactionId, isTestPurchase, serverConfig);

        if (errorCode == ErrorCode::ApiConfigAlreadyAdded) {
            result.duplicateConfigAlreadyPresent = true;
            qInfo().noquote() << "[IAP] Skipping restored transaction" << originalTransactionId
                              << "because subscription config with the same vpn_key already exists";
        } else if (errorCode != ErrorCode::NoError) {
            qWarning().noquote() << "[IAP] Failed to process restored subscription response for transaction" << originalTransactionId;
            result.errorCode = errorCode;
        } else {
            result.hasInstalledConfig = true;
        }
    }

    if (!result.hasInstalledConfig) {
        result.errorCode = result.duplicateConfigAlreadyPresent ? ErrorCode::ApiConfigAlreadyAdded : ErrorCode::ApiPurchaseError;
    }

    return result;
#else
    Q_UNUSED(userCountryCode);
    Q_UNUSED(serviceType);
    Q_UNUSED(serviceProtocol);
    Q_UNUSED(transactions);
    result.errorCode = ErrorCode::ApiPurchaseError;
    return result;
#endif
}

