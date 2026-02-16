#include "subscriptionController.h"

#include <QDebug>
#include <QDateTime>
#include <QEventLoop>
#include <QJsonDocument>
#include <QSet>
#include <QSysInfo>
#include <QUuid>
#include <QVariantMap>

#include "core/configurators/openVpnConfigurator.h"
#include "core/configurators/wireguardConfigurator.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/api/apiEnums.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/api/apiUtils.h"
#include "core/controllers/gatewayController.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "version.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"
#include "core/models/api/apiConfig.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
    #include <AmneziaVPN-Swift.h>
#endif

using namespace amnezia;

namespace
{
    namespace configKey
    {
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
        constexpr char publicKeyInfo[] = "public_key";
        constexpr char expiresAt[] = "expires_at";
    }
}

SubscriptionController::SubscriptionController(SecureServersRepository* serversRepository,
                                               SecureAppSettingsRepository* appSettingsRepository)
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
    if (protocol == configKey::awg) {
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
    if (protocol == configKey::awg) {
        apiPayload[configKey::publicKey] = protocolData.wireGuardClientPubKey;
    } else if (protocol == configKey::vless) {
        apiPayload[configKey::publicKey] = protocolData.xrayUuid;
    }
}

ErrorCode SubscriptionController::extractServerConfigJsonFromResponse(const QByteArray &apiResponseBody, const QString &protocol, 
                                                                        const ProtocolData &protocolData, QJsonObject &serverConfigJson)
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
    if (protocol == configKey::awg) {
        configStr.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", protocolData.wireGuardClientPrivKey);
        auto newServerConfig = QJsonDocument::fromJson(configStr.toUtf8()).object();
        auto containers = newServerConfig.value(config_key::containers).toArray();
        if (containers.isEmpty()) {
            qDebug() << "missing containers field";
            return ErrorCode::ApiConfigEmptyError;
        }
        auto container = containers.at(0).toObject();
        QString containerName = ContainerUtils::containerTypeToString(DockerContainer::Awg);
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

    serverConfigJson = QJsonDocument::fromJson(configStr.toUtf8()).object();
    return ErrorCode::NoError;
}

void SubscriptionController::updateApiConfigInJson(QJsonObject &serverConfigJson, const QString &serviceType, 
                                                    const QString &serviceProtocol, const QString &userCountryCode,
                                                    const QByteArray &apiResponseBody)
{
    QJsonObject apiConfig = serverConfigJson.value(configKey::apiConfig).toObject();
    
    apiConfig[apiDefs::key::serviceType] = serviceType;
    apiConfig[configKey::serviceProtocol] = serviceProtocol;
    apiConfig[configKey::userCountryCode] = userCountryCode;
    
    if (serverConfigJson.value(config_key::configVersion).toInt() == apiDefs::ConfigSource::AmneziaGateway) {
        QJsonObject responseObj = QJsonDocument::fromJson(apiResponseBody).object();
        if (responseObj.contains(apiDefs::key::supportedProtocols)) {
            apiConfig.insert(apiDefs::key::supportedProtocols, responseObj.value(apiDefs::key::supportedProtocols).toArray());
        }
        if (responseObj.contains(apiDefs::key::serviceInfo)) {
            apiConfig.insert(apiDefs::key::serviceInfo, responseObj.value(apiDefs::key::serviceInfo).toObject());
        }
    }
    
    serverConfigJson[configKey::apiConfig] = apiConfig;
}

ErrorCode SubscriptionController::executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody, bool isTestPurchase)
{
    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase), m_appSettingsRepository->isDevGatewayEnv(isTestPurchase), apiDefs::requestTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled());
    return gatewayController.post(endpoint, apiPayload, responseBody);
}

ErrorCode SubscriptionController::importServiceFromGateway(const QString &userCountryCode, const QString &serviceType,
                                                            const QString &serviceProtocol, const ProtocolData &protocolData,
                                                            ServerConfig &serverConfig)
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

    QJsonObject serverConfigJson;
    errorCode = extractServerConfigJsonFromResponse(responseBody, serviceProtocol, protocolData, serverConfigJson);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }
    
    updateApiConfigInJson(serverConfigJson, serviceType, serviceProtocol, userCountryCode, responseBody);
    
    ServerConfig serverConfigModel = ServerConfig::fromJson(serverConfigJson);
    
    if (!serverConfigModel.isApiV2()) {
        return ErrorCode::InternalError;
    }

    m_serversRepository->addServer(serverConfigModel);
    serverConfig = serverConfigModel;
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::importServiceFromAppStore(const QString &userCountryCode, const QString &serviceType,
                                                            const QString &serviceProtocol, const ProtocolData &protocolData,
                                                            const QString &transactionId, bool isTestPurchase,
                                                            ServerConfig &serverConfig)
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
        ServerConfig existingServerConfig = m_serversRepository->server(i);
        QString existingVpnKey;
        if (existingServerConfig.isApiV1()) {
            const ApiV1ServerConfig* apiV1 = existingServerConfig.as<ApiV1ServerConfig>();
            existingVpnKey = apiV1 ? apiV1->vpnKey() : QString();
        } else if (existingServerConfig.isApiV2()) {
            const ApiV2ServerConfig* apiV2 = existingServerConfig.as<ApiV2ServerConfig>();
            existingVpnKey = apiV2 ? apiV2->vpnKey() : QString();
        }
        if (existingVpnKey == key) {
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
    
    ServerConfig serverConfigModel = ServerConfig::fromJson(configObject);
    
    if (!serverConfigModel.isApiV2()) {
        return ErrorCode::InternalError;
    }

    ApiV2ServerConfig* apiV2 = serverConfigModel.as<ApiV2ServerConfig>();
    if (!apiV2) {
        return ErrorCode::InternalError;
    }
    apiV2->apiConfig.vpnKey = normalizedKey;
    apiV2->apiConfig.isTestPurchase = isTestPurchase;
    apiV2->crc = crc;

    m_serversRepository->addServer(serverConfigModel);
    serverConfig = serverConfigModel;

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::updateServiceFromGateway(int serverIndex, const QString &newCountryCode, bool isConnectEvent)
{
    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);
    
    if (!serverConfigModel.isApiV2()) {
        return ErrorCode::InternalError;
    }

    const ApiV2ServerConfig* apiV2 = serverConfigModel.as<ApiV2ServerConfig>();
    if (!apiV2) {
        return ErrorCode::InternalError;
    }
    QString serviceProtocol = apiV2->serviceProtocol();
    ProtocolData protocolData = generateProtocolData(serviceProtocol);
    
    QJsonObject authDataJson = apiV2->authData.toJson();
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_appSettingsRepository->getAppLanguage().name().split("_").first(),
                                            m_appSettingsRepository->getInstallationUuid(true),
                                            apiV2->apiConfig.userCountryCode,
                                            newCountryCode,
                                            apiV2->serviceType(),
                                            serviceProtocol,
                                            authDataJson };

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

    QJsonObject serverConfigJson;
    errorCode = extractServerConfigJsonFromResponse(responseBody, serviceProtocol, protocolData, serverConfigJson);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }
    
    updateApiConfigInJson(serverConfigJson, apiV2->apiConfig.serviceType, serviceProtocol, apiV2->apiConfig.userCountryCode, responseBody);
    
    ServerConfig newServerConfigModel = ServerConfig::fromJson(serverConfigJson);
    
    if (!newServerConfigModel.isApiV2()) {
        return ErrorCode::InternalError;
    }

    ApiV2ServerConfig* newApiV2 = newServerConfigModel.as<ApiV2ServerConfig>();
    if (!newApiV2) {
        return ErrorCode::InternalError;
    }
    
    newApiV2->apiConfig.vpnKey = apiV2->apiConfig.vpnKey;
    
    newApiV2->authData = apiV2->authData;
    newApiV2->crc = apiV2->crc;
    
    if (apiV2->nameOverriddenByUser) {
        newApiV2->name = apiV2->name;
        newApiV2->nameOverriddenByUser = true;
    }

    m_serversRepository->editServer(serverIndex, newServerConfigModel);
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::revokeServiceFromGateway(int serverIndex, bool isRemoveEvent)
{
    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);
    
    if (!serverConfigModel.isApiV2()) {
        return ErrorCode::NoError;
    }

    const ApiV2ServerConfig* apiV2 = serverConfigModel.as<ApiV2ServerConfig>();
    if (!apiV2) {
        return ErrorCode::NoError;
    }
    
    if (!apiV2->isPremium()) {
        return ErrorCode::NoError;
    }

    QJsonObject authDataJson = apiV2->authData.toJson();
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_appSettingsRepository->getAppLanguage().name().split("_").first(),
                                            m_appSettingsRepository->getInstallationUuid(true),
                                            apiV2->apiConfig.userCountryCode,
                                            apiV2->apiConfig.serverCountryCode,
                                            apiV2->serviceType(),
                                            "",
                                            authDataJson };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        return errorCode;
    }

    serverConfigModel.visit([](auto& arg) {
        arg.containers.clear();
    });
    m_serversRepository->editServer(serverIndex, serverConfigModel);
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::revokeExternalDevice(int serverIndex, const QString &uuid, const QString &serverCountryCode)
{
    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);
    
    if (!serverConfigModel.isApiV2()) {
        return ErrorCode::NoError;
    }

    const ApiV2ServerConfig* apiV2 = serverConfigModel.as<ApiV2ServerConfig>();
    if (!apiV2) {
        return ErrorCode::NoError;
    }
    
    if (!apiV2->isPremium()) {
        return ErrorCode::NoError;
    }

    QJsonObject authDataJson = apiV2->authData.toJson();
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_appSettingsRepository->getAppLanguage().name().split("_").first(),
                                            uuid,
                                            apiV2->apiConfig.userCountryCode,
                                            serverCountryCode,
                                            apiV2->serviceType(),
                                            "",
                                            authDataJson };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        return errorCode;
    }

    if (uuid == m_appSettingsRepository->getInstallationUuid(true)) {
        serverConfigModel.visit([](auto& arg) {
            arg.containers.clear();
        });
        m_serversRepository->editServer(serverIndex, serverConfigModel);
    }

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::exportNativeConfig(int serverIndex, const QString &serverCountryCode, QString &nativeConfig)
{
    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);
    
    if (!serverConfigModel.isApiV2()) {
        return ErrorCode::InternalError;
    }

    const ApiV2ServerConfig* apiV2 = serverConfigModel.as<ApiV2ServerConfig>();
    if (!apiV2) {
        return ErrorCode::InternalError;
    }
    QString protocol = configKey::awg;
    ProtocolData protocolData = generateProtocolData(protocol);

    QJsonObject authDataJson = apiV2->authData.toJson();
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_appSettingsRepository->getAppLanguage().name().split("_").first(),
                                            m_appSettingsRepository->getInstallationUuid(true),
                                            apiV2->apiConfig.userCountryCode,
                                            serverCountryCode,
                                            apiV2->serviceType(),
                                            protocol,
                                            authDataJson };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    appendProtocolDataToApiPayload(protocol, protocolData, apiPayload);

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/native_config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonObject jsonConfig = QJsonDocument::fromJson(responseBody).object();
    nativeConfig = jsonConfig.value(configKey::config).toString();
    nativeConfig.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", protocolData.wireGuardClientPrivKey);
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::revokeNativeConfig(int serverIndex, const QString &serverCountryCode)
{
    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);
    
    if (!serverConfigModel.isApiV2()) {
        return ErrorCode::InternalError;
    }

    const ApiV2ServerConfig* apiV2 = serverConfigModel.as<ApiV2ServerConfig>();
    if (!apiV2) {
        return ErrorCode::InternalError;
    }
    QString protocol = configKey::awg;

    QJsonObject authDataJson = apiV2->authData.toJson();
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_appSettingsRepository->getAppLanguage().name().split("_").first(),
                                            m_appSettingsRepository->getInstallationUuid(true),
                                            apiV2->apiConfig.userCountryCode,
                                            serverCountryCode,
                                            apiV2->serviceType(),
                                            protocol,
                                            authDataJson };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_native_config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        return errorCode;
    }

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::updateServiceFromTelegram(int serverIndex)
{
    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);
    
    if (!serverConfigModel.isApiV1()) {
        return ErrorCode::InternalError;
    }

    const ApiV1ServerConfig* apiV1 = serverConfigModel.as<ApiV1ServerConfig>();
    if (!apiV1) {
        return ErrorCode::InternalError;
    }
    QString serviceProtocol = apiV1->protocol;
    ProtocolData protocolData = generateProtocolData(serviceProtocol);
    QString installationUuid = m_appSettingsRepository->getInstallationUuid(true);

    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled());

    QJsonObject apiPayload;
    appendProtocolDataToApiPayload(serviceProtocol, protocolData, apiPayload);
    apiPayload[configKey::uuid] = installationUuid;
    apiPayload[configKey::osVersion] = QSysInfo::productType();
    apiPayload[configKey::appVersion] = QString(APP_VERSION);
    apiPayload[configKey::accessToken] = apiV1->apiKey;
    apiPayload[configKey::apiEndpoint] = apiV1->apiEndpoint;

    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/proxy_config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonObject serverConfigJson;
    errorCode = extractServerConfigJsonFromResponse(responseBody, serviceProtocol, protocolData, serverConfigJson);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }
    
    ServerConfig newServerConfigModel = ServerConfig::fromJson(serverConfigJson);
    
    if (!newServerConfigModel.isApiV1()) {
        return ErrorCode::InternalError;
    }

    ApiV1ServerConfig* newApiV1 = newServerConfigModel.as<ApiV1ServerConfig>();
    if (!newApiV1) {
        return ErrorCode::InternalError;
    }
    newApiV1->apiKey = apiV1->apiKey;
    newApiV1->apiEndpoint = apiV1->apiEndpoint;
    newApiV1->crc = apiV1->crc;

    m_serversRepository->editServer(serverIndex, newServerConfigModel);
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::prepareVpnKeyExport(int serverIndex, QString &vpnKey)
{
    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);
    
    if (serverConfigModel.isApiV1()) {
        const ApiV1ServerConfig* apiV1 = serverConfigModel.as<ApiV1ServerConfig>();
        vpnKey = apiV1 ? apiV1->vpnKey() : QString();
    } else if (serverConfigModel.isApiV2()) {
        ApiV2ServerConfig* apiV2 = serverConfigModel.as<ApiV2ServerConfig>();
        vpnKey = apiV2 ? apiV2->vpnKey() : QString();
        if (vpnKey.isEmpty()) {
            QJsonObject serverJson = serverConfigModel.toJson();
            vpnKey = apiUtils::getPremiumV2VpnKey(serverJson);
            if (vpnKey.isEmpty()) {
                return ErrorCode::ApiConfigEmptyError;
            }
            apiV2->apiConfig.vpnKey = vpnKey;
            m_serversRepository->editServer(serverIndex, serverConfigModel);
        }
    } else {
        return ErrorCode::ApiConfigEmptyError;
    }

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::validateAndUpdateConfig(int serverIndex, bool hasInstalledContainers)
{
    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);
    
    apiDefs::ConfigSource configSource;
    if (serverConfigModel.isApiV1()) {
        configSource = apiDefs::ConfigSource::Telegram;
    } else if (serverConfigModel.isApiV2()) {
        configSource = apiDefs::ConfigSource::AmneziaGateway;
    } else {
        return ErrorCode::NoError;
    }

    if (configSource == apiDefs::ConfigSource::Telegram && !hasInstalledContainers) {
        removeApiConfig(serverIndex);
        return updateServiceFromTelegram(serverIndex);
    } else if (configSource == apiDefs::ConfigSource::AmneziaGateway && !hasInstalledContainers) {
        return updateServiceFromGateway(serverIndex, "", false);
    } else if (configSource && isApiKeyExpired(serverIndex)) {
        qDebug() << "attempt to update api config by expires_at event";
        if (configSource == apiDefs::ConfigSource::AmneziaGateway) {
            return updateServiceFromGateway(serverIndex, "", false);
        } else {
            removeApiConfig(serverIndex);
            return updateServiceFromTelegram(serverIndex);
        }
    }
    return ErrorCode::NoError;
}

void SubscriptionController::removeApiConfig(int serverIndex)
{
    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    QString description = serverConfigModel.description();
    QString hostName = serverConfigModel.hostName();
    QString vpncName = QString("%1 (%2) %3")
                               .arg(description)
                               .arg(hostName)
                               .arg("");

    AmneziaVPN::removeVPNC(vpncName.toStdString());
#endif

    serverConfigModel.visit([](auto& arg) {
        arg.dns1.clear();
        arg.dns2.clear();
        arg.containers.clear();
        arg.hostName.clear();
        arg.defaultContainer = DockerContainer::None;
    });

    if (serverConfigModel.isApiV2()) {
        ApiV2ServerConfig* apiV2 = serverConfigModel.as<ApiV2ServerConfig>();
        if (apiV2) {
            apiV2->apiConfig.publicKey = ApiConfig::PublicKeyInfo{};
        }
    }

    m_serversRepository->editServer(serverIndex, serverConfigModel);
}

bool SubscriptionController::isApiKeyExpired(int serverIndex) const
{
    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);
    
    if (!serverConfigModel.isApiV2()) {
        return false;
    }

    const ApiV2ServerConfig* apiV2 = serverConfigModel.as<ApiV2ServerConfig>();
    if (!apiV2) {
        return false;
    }
    const QString expiresAt = apiV2->apiConfig.publicKey.expiresAt;
    
    if (expiresAt.isEmpty()) {
        return false;
    }

    auto expiresAtDateTime = QDateTime::fromString(expiresAt, Qt::ISODate).toUTC();
    if (expiresAtDateTime < QDateTime::currentDateTimeUtc()) {
        return true;
    }
    
    return false;
}

void SubscriptionController::setApiServiceProtocol(int serverIndex, const QString &protocolName)
{
    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);
    if (serverConfigModel.isApiV2()) {
        ApiV2ServerConfig* apiV2 = serverConfigModel.as<ApiV2ServerConfig>();
        if (apiV2) {
            apiV2->apiConfig.serviceProtocol = protocolName;
        }
        m_serversRepository->editServer(serverIndex, serverConfigModel);
    }
}

bool SubscriptionController::isApiServiceProtocolVless(int serverIndex) const
{
    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);
    if (serverConfigModel.isApiV2()) {
        const ApiV2ServerConfig* apiV2 = serverConfigModel.as<ApiV2ServerConfig>();
        return apiV2 && apiV2->serviceProtocol() == "vless";
    }
    return false;
}

ErrorCode SubscriptionController::processAppStorePurchase(const QString &userCountryCode, const QString &serviceType,
                                                          const QString &serviceProtocol, const QString &productId,
                                                          ServerConfig &serverConfig)
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
    result.errorCode = ErrorCode::ApiPurchaseError;
    return result;
#endif
}

ErrorCode SubscriptionController::getAccountInfo(int serverIndex, QJsonObject &accountInfo)
{
    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);
    
    if (!serverConfigModel.isApiV2()) {
        return ErrorCode::InternalError;
    }

    const ApiV2ServerConfig* apiV2 = serverConfigModel.as<ApiV2ServerConfig>();
    if (!apiV2) {
        return ErrorCode::InternalError;
    }
    bool isTestPurchase = apiV2->apiConfig.isTestPurchase;
    
    QJsonObject authDataJson = apiV2->authData.toJson();
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_appSettingsRepository->getAppLanguage().name().split("_").first(),
                                            m_appSettingsRepository->getInstallationUuid(true),
                                            apiV2->apiConfig.userCountryCode,
                                            "",
                                            apiV2->serviceType(),
                                            "",
                                            authDataJson };

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    apiPayload[apiDefs::key::cliVersion] = QString(APP_VERSION);

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/account_info"), apiPayload, responseBody, isTestPurchase);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    accountInfo = QJsonDocument::fromJson(responseBody).object();
    return ErrorCode::NoError;
}

