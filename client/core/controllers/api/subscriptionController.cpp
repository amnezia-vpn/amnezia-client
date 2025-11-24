#include "subscriptionController.h"

#include <QDebug>
#include <QJsonDocument>
#include <QSysInfo>
#include <QUuid>

#include "configurators/openvpn_configurator.h"
#include "configurators/wireguard_configurator.h"
#include "containers/containers_defs.h"
#include "core/api/apiDefs.h"
#include "core/api/apiUtils.h"
#include "core/controllers/gatewayController.h"
#include "core/server_defs.h"
#include "protocols/protocols_defs.h"
#include "version.h"

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

SubscriptionController::SubscriptionController(std::shared_ptr<Settings> settings)
    : m_settings(settings)
{
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

        apiConfig.insert(apiDefs::key::serviceInfo,
                         QJsonDocument::fromJson(apiResponseBody).object().value(apiDefs::key::serviceInfo).toObject());
    }

    serverConfig[configKey::apiConfig] = apiConfig;

    return ErrorCode::NoError;
}

bool SubscriptionController::isSubscriptionExpired(const QJsonObject &apiConfig)
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

ErrorCode SubscriptionController::executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody)
{
    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_settings->isStrictKillSwitchEnabled());
    return gatewayController.post(endpoint, apiPayload, responseBody);
}

ErrorCode SubscriptionController::importServiceFromGateway(const QString &userCountryCode, const QString &serviceType,
                                                            const QString &serviceProtocol, const ProtocolData &protocolData,
                                                            QJsonObject &serverConfig)
{
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getAppLanguage().name().split("_").first(),
                                            m_settings->getInstallationUuid(true),
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

    m_settings->addServer(serverConfig);
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::updateServiceFromGateway(int serverIndex, const QString &newCountryCode, bool isConnectEvent,
                                                           const ProtocolData &protocolData)
{
    QJsonObject serverConfig = m_settings->server(serverIndex);
    QJsonObject apiConfig = serverConfig.value(configKey::apiConfig).toObject();

    if (isSubscriptionExpired(apiConfig)) {
        return ErrorCode::ApiSubscriptionExpiredError;
    }

    QString serviceProtocol = apiConfig.value(configKey::serviceProtocol).toString();
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getAppLanguage().name().split("_").first(),
                                            m_settings->getInstallationUuid(true),
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

    m_settings->editServer(serverIndex, newServerConfig);
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::revokeServiceFromGateway(int serverIndex, bool isRemoveEvent)
{
    QJsonObject serverConfig = m_settings->server(serverIndex);
    QJsonObject apiConfig = serverConfig.value(configKey::apiConfig).toObject();

    if (!apiUtils::isPremiumServer(serverConfig)) {
        return ErrorCode::NoError;
    }

    if (isSubscriptionExpired(apiConfig)) {
        if (isRemoveEvent) {
            return ErrorCode::NoError;
        } else {
            return ErrorCode::ApiSubscriptionExpiredError;
        }
    }

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getAppLanguage().name().split("_").first(),
                                            m_settings->getInstallationUuid(true),
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
    m_settings->editServer(serverIndex, serverConfig);
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::revokeExternalDevice(int serverIndex, const QString &uuid, const QString &serverCountryCode)
{
    QJsonObject serverConfig = m_settings->server(serverIndex);
    QJsonObject apiConfig = serverConfig.value(configKey::apiConfig).toObject();

    if (!apiUtils::isPremiumServer(serverConfig)) {
        return ErrorCode::NoError;
    }

    if (isSubscriptionExpired(apiConfig)) {
        return ErrorCode::ApiSubscriptionExpiredError;
    }

    QJsonObject apiConfigForRevoke = apiConfig;
    apiConfigForRevoke[configKey::serverCountryCode] = serverCountryCode;

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getAppLanguage().name().split("_").first(),
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

    if (uuid == m_settings->getInstallationUuid(true)) {
        serverConfig.remove(config_key::containers);
        m_settings->editServer(serverIndex, serverConfig);
    }

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::exportNativeConfig(const QJsonObject &apiConfig, const QJsonObject &authData,
                                                     const QString &serverCountryCode, const QString &serviceProtocol,
                                                     const ProtocolData &protocolData, QString &nativeConfig)
{
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getAppLanguage().name().split("_").first(),
                                            m_settings->getInstallationUuid(true),
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
                                            m_settings->getAppLanguage().name().split("_").first(),
                                            m_settings->getInstallationUuid(true),
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
    QJsonObject serverConfig = m_settings->server(serverIndex);
    QString serviceProtocol = serverConfig.value(configKey::protocol).toString();
    QString installationUuid = m_settings->getInstallationUuid(true);

    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_settings->isStrictKillSwitchEnabled());

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

    m_settings->editServer(serverIndex, newServerConfig);
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::prepareVpnKeyExport(int serverIndex, QString &vpnKey)
{
    QJsonObject serverConfig = m_settings->server(serverIndex);
    QJsonObject apiConfig = serverConfig.value(configKey::apiConfig).toObject();

    vpnKey = apiConfig.value(apiDefs::key::vpnKey).toString();
    if (vpnKey.isEmpty()) {
        vpnKey = apiUtils::getPremiumV2VpnKey(serverConfig);
        if (vpnKey.isEmpty()) {
            return ErrorCode::ApiConfigEmptyError;
        }
        apiConfig.insert(apiDefs::key::vpnKey, vpnKey);
        serverConfig.insert(configKey::apiConfig, apiConfig);
        m_settings->editServer(serverIndex, serverConfig);
    }

    return ErrorCode::NoError;
}

