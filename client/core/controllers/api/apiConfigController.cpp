#include "apiConfigController.h"

#include <QClipboard>
#include <QEventLoop>

#include "amnezia_application.h"
#include "configurators/wireguard_configurator.h"
#include "core/api/apiDefs.h"
#include "core/api/apiUtils.h"
#include "core/controllers/api/gatewayController.h"
#include "core/qrCodeUtils.h"
#include "core/utils/fileUtils.h"
#include "core/models/servers/serverConfig.h"
#include "core/models/servers/apiV2ServerConfig.h"
#include "version.h"

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

        constexpr char apiPayload[] = "api_payload";
        constexpr char keyPayload[] = "key_payload";

        constexpr char apiConfig[] = "api_config";
        constexpr char authData[] = "auth_data";

        constexpr char config[] = "config";
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
                               QSharedPointer<ServerConfig> &serverConfigPtr)
    {
        QJsonObject serverConfig;
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

            //TODO looks like this block can be removed after v1 configs EOL

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

        if (newServerConfig.value(config_key::configVersion).toInt() == static_cast<int>(amnezia::ServerConfigType::ApiV2)) {
            serverConfig[config_key::configVersion] = newServerConfig.value(config_key::configVersion);
            serverConfig[config_key::description] = newServerConfig.value(config_key::description);
            serverConfig[config_key::name] = newServerConfig.value(config_key::name);
        }

        auto defaultContainer = newServerConfig.value(config_key::defaultContainer).toString();
        serverConfig[config_key::defaultContainer] = defaultContainer;

        QVariantMap map = serverConfig.value(configKey::apiConfig).toObject().toVariantMap();
        map.insert(newServerConfig.value(configKey::apiConfig).toObject().toVariantMap());
        auto apiConfig = QJsonObject::fromVariantMap(map);

        if (newServerConfig.value(config_key::configVersion).toInt() == static_cast<int>(amnezia::ServerConfigType::ApiV2)) {
            apiConfig.insert(apiDefs::key::supportedProtocols,
                             QJsonDocument::fromJson(apiResponseBody).object().value(apiDefs::key::supportedProtocols).toArray());
        }

        serverConfig[configKey::apiConfig] = apiConfig;

        serverConfigPtr = ServerConfig::createServerConfig(serverConfig);
        return ErrorCode::NoError;
    }
}

ApiConfigsController::ApiConfigsController(const QSharedPointer<ServersModel> &serversModel,
                                           const QSharedPointer<ApiServicesModel> &apiServicesModel,
                                           const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_apiServicesModel(apiServicesModel), m_settings(settings)
{
}

ErrorCode ApiConfigsController::exportNativeConfig(const QString &serverCountryCode, const QString &fileName)
{
    if (fileName.isEmpty()) {
        return ErrorCode::PermissionsError;
    }

    auto serverConfigPtr = m_serversModel->getServerConfig(m_serversModel->getProcessedServerIndex());
    auto serverConfigObject = serverConfigPtr->toJson();
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

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
        return errorCode;
    }

    QJsonObject jsonConfig = QJsonDocument::fromJson(responseBody).object();
    QString nativeConfig = jsonConfig.value(configKey::config).toString();
    nativeConfig.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", protocolData.wireGuardClientPrivKey);

    FileUtils::saveFile(fileName, nativeConfig);
    return ErrorCode::NoError;
}

ErrorCode ApiConfigsController::revokeNativeConfig(const QString &serverCountryCode)
{
    auto serverConfigPtr = m_serversModel->getServerConfig(m_serversModel->getProcessedServerIndex());
    auto serverConfigObject = serverConfigPtr->toJson();
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

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
        return errorCode;
    }
    return ErrorCode::NoError;
}

void ApiConfigsController::prepareVpnKeyExport()
{
    auto serverConfigPtr = m_serversModel->getServerConfig(m_serversModel->getProcessedServerIndex());
    auto serverConfigObject = serverConfigPtr->toJson();
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    auto vpnKey = apiConfigObject.value(apiDefs::key::vpnKey).toString();
    m_vpnKey = vpnKey;

    vpnKey.replace("vpn://", "");

    m_qrCodes = qrCodeUtils::generateQrCodeImageSeries(vpnKey.toUtf8());

    
}

void ApiConfigsController::copyVpnKeyToClipboard()
{
    auto clipboard = amnApp->getClipboard();
    clipboard->setText(m_vpnKey);
}

ErrorCode ApiConfigsController::fillAvailableServices()
{
    QJsonObject apiPayload;
    apiPayload[configKey::osVersion] = QSysInfo::productType();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/services"), apiPayload, responseBody);
    if (errorCode == ErrorCode::NoError) {
        if (!responseBody.contains("services")) {
            errorCode = ErrorCode::ApiServicesMissingError;
        }
    }

    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonObject data = QJsonDocument::fromJson(responseBody).object();
    m_apiServicesModel->updateModel(data);
    return ErrorCode::NoError;
}

bool ApiConfigsController::isServerFromApiAlreadyExists(const QString &userCountryCode,
                                                       const QString &serviceType,
                                                       const QString &serviceProtocol) const
{
    auto servers = m_settings->serversArray();
    for (const auto &server : servers) {
        auto serverConfig = ServerConfig::createServerConfig(server.toObject());
        if (serverConfig->type != amnezia::ServerConfigType::ApiV2) continue;
        auto apiV2Config = qSharedPointerCast<ApiV2ServerConfig>(serverConfig);
        if (!apiV2Config) continue;

        if (apiV2Config->apiConfig.userCountryCode == userCountryCode &&
            apiV2Config->apiConfig.serviceType == serviceType &&
            apiV2Config->apiConfig.serviceProtocol == serviceProtocol) {
            return true;
        }
    }
    return false;
}

bool ApiConfigsController::isApiKeyExpired(int serverIndex) const
{
    auto servers = m_settings->serversArray();
    if (serverIndex >= servers.size()) return false;

    auto serverConfig = ServerConfig::createServerConfig(servers.at(serverIndex).toObject());
    if (serverConfig->type != amnezia::ServerConfigType::ApiV2) return false;

    auto apiV2Config = qSharedPointerCast<ApiV2ServerConfig>(serverConfig);
    if (!apiV2Config) return false;

    QString expiresIso = apiV2Config->apiConfig.publicKey.expiresAt;
    if (expiresIso.isEmpty()) {
        expiresIso = apiV2Config->apiConfig.subscription.end_date;
    }
    if (expiresIso.isEmpty()) return false;

    auto expiresAt = QDateTime::fromString(expiresIso, Qt::ISODate);
    return QDateTime::currentDateTime() > expiresAt;
}

void ApiConfigsController::removeApiConfig(int serverIndex)
{
    auto servers = m_settings->serversArray();
    if (serverIndex >= servers.size()) return;

    auto serverConfig = ServerConfig::createServerConfig(servers.at(serverIndex).toObject());
    if (serverConfig->type != amnezia::ServerConfigType::ApiV2) return;

    auto apiV2 = qSharedPointerCast<ApiV2ServerConfig>(serverConfig);
    if (!apiV2) return;

    apiV2->containerConfigs.clear();
    apiV2->apiConfig.publicKey.expiresAt.clear();
    apiV2->apiConfig.vpnKey.clear();
    apiV2->defaultContainer = ContainerProps::containerToString(DockerContainer::None);

    m_serversModel->editServer(apiV2, serverIndex);
}

ErrorCode ApiConfigsController::importServiceFromGateway()
{
    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getInstallationUuid(true),
                                            m_apiServicesModel->getCountryCode(),
                                            "",
                                            m_apiServicesModel->getSelectedServiceType(),
                                            m_apiServicesModel->getSelectedServiceProtocol(),
                                            QJsonObject() };

    if (isServerFromApiAlreadyExists(gatewayRequestData.userCountryCode, gatewayRequestData.serviceType,
                                     gatewayRequestData.serviceProtocol)) {
        return ErrorCode::ApiConfigAlreadyAdded;
    }

    ProtocolData protocolData = generateProtocolData(gatewayRequestData.serviceProtocol);

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    appendProtocolDataToApiPayload(gatewayRequestData.serviceProtocol, protocolData, apiPayload);

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/config"), apiPayload, responseBody);

    QSharedPointer<ServerConfig> serverConfigPtr;
    if (errorCode == ErrorCode::NoError) {
        errorCode = fillServerConfig(gatewayRequestData.serviceProtocol, protocolData, responseBody, serverConfigPtr);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }
        if (serverConfigPtr->type == amnezia::ServerConfigType::ApiV2) {
            auto apiV2ServerConfig = qSharedPointerCast<ApiV2ServerConfig>(serverConfigPtr);
            apiV2ServerConfig->apiConfig.userCountryCode = m_apiServicesModel->getCountryCode();
            apiV2ServerConfig->apiConfig.serviceType = m_apiServicesModel->getSelectedServiceType();
            apiV2ServerConfig->apiConfig.serviceProtocol = m_apiServicesModel->getSelectedServiceProtocol();
        }
        m_serversModel->addServer(serverConfigPtr);
        return ErrorCode::NoError;
    } else {
        return errorCode;
    }
}

ErrorCode ApiConfigsController::updateServiceFromGateway(const int serverIndex, const QString &newCountryCode, const QString &newCountryName,
                                                    bool reloadServiceConfig)
{
    auto serverConfigPtr = m_serversModel->getServerConfig(serverIndex);
    auto serverConfigJson = serverConfigPtr->toJson();
    auto apiConfig = serverConfigJson.value(configKey::apiConfig).toObject();

    GatewayRequestData gatewayRequestData { QSysInfo::productType(),
                                            QString(APP_VERSION),
                                            m_settings->getInstallationUuid(true),
                                            apiConfig.value(configKey::userCountryCode).toString(),
        newCountryCode,
                                            apiConfig.value(configKey::serviceType).toString(),
                                            apiConfig.value(configKey::serviceProtocol).toString(),
                                            serverConfigJson.value(configKey::authData).toObject() };

    ProtocolData protocolData = generateProtocolData(gatewayRequestData.serviceProtocol);

    QJsonObject apiPayload = gatewayRequestData.toJsonObject();
    appendProtocolDataToApiPayload(gatewayRequestData.serviceProtocol, protocolData, apiPayload);

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/config"), apiPayload, responseBody);

    QSharedPointer<ServerConfig> newServerConfigPtr;
    if (errorCode == ErrorCode::NoError) {
        errorCode = fillServerConfig(gatewayRequestData.serviceProtocol, protocolData, responseBody, newServerConfigPtr);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }
        if (newServerConfigPtr->type == amnezia::ServerConfigType::ApiV2 && serverConfigPtr->type == amnezia::ServerConfigType::ApiV2) {
            auto newApiV2 = qSharedPointerCast<ApiV2ServerConfig>(newServerConfigPtr);
            auto oldApiV2 = qSharedPointerCast<ApiV2ServerConfig>(serverConfigPtr);
            newApiV2->apiConfig.userCountryCode = oldApiV2->apiConfig.userCountryCode;
            newApiV2->apiConfig.serviceType = oldApiV2->apiConfig.serviceType;
            newApiV2->apiConfig.serviceProtocol = oldApiV2->apiConfig.serviceProtocol;
            newApiV2->apiConfig.vpnKey = oldApiV2->apiConfig.vpnKey;
            newApiV2->apiConfig.authData.apiKey = gatewayRequestData.authData.value("api_key").toString();
            if (serverConfigPtr->nameOverriddenByUser) {
                newApiV2->name = oldApiV2->name;
                newApiV2->nameOverriddenByUser = true;
            }
        }
        m_serversModel->editServer(newServerConfigPtr, serverIndex);
        return ErrorCode::NoError;
    } else {
        return errorCode;
    }
}

ErrorCode ApiConfigsController::updateServiceFromTelegram(const int serverIndex)
{
#ifdef Q_OS_IOS
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_settings->isStrictKillSwitchEnabled());

    auto serverConfigPtr = m_serversModel->getServerConfig(serverIndex);
    auto installationUuid = m_settings->getInstallationUuid(true);

    auto serverConfigJson2 = serverConfigPtr->toJson();
    QString serviceProtocol = serverConfigJson2.value(configKey::protocol).toString();
    ProtocolData protocolData = generateProtocolData(serviceProtocol);

    QJsonObject apiPayload;
    appendProtocolDataToApiPayload(serviceProtocol, protocolData, apiPayload);
    apiPayload[configKey::uuid] = installationUuid;
    apiPayload[configKey::osVersion] = QSysInfo::productType();
    apiPayload[configKey::appVersion] = QString(APP_VERSION);
    apiPayload[configKey::accessToken] = serverConfigJson2.value(configKey::accessToken).toString();
    apiPayload[configKey::apiEndpoint] = serverConfigJson2.value(configKey::apiEndpoint).toString();

    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/proxy_config"), apiPayload, responseBody);

    if (errorCode == ErrorCode::NoError) {
        QSharedPointer<ServerConfig> updatedConfigPtr;
        errorCode = fillServerConfig(serviceProtocol, protocolData, responseBody, updatedConfigPtr);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }
        m_serversModel->editServer(updatedConfigPtr, serverIndex);
        return ErrorCode::NoError;
    } else {
        return errorCode;
    }
}

ErrorCode ApiConfigsController::deactivateDevice()
{
    auto serverIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfigPtr = m_serversModel->getServerConfig(serverIndex);
    auto serverConfigObject = serverConfigPtr->toJson();
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    if (!apiUtils::isPremiumServer(serverConfigObject)) {
        return ErrorCode::NoError;
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
        return errorCode;
    }

    serverConfigPtr->containerConfigs.clear();
    m_serversModel->editServer(serverConfigPtr, serverIndex);

    return ErrorCode::NoError;
}

ErrorCode ApiConfigsController::deactivateExternalDevice(const QString &uuid, const QString &serverCountryCode)
{
    auto serverIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfigPtr = m_serversModel->getServerConfig(serverIndex);
    auto serverConfigObject = serverConfigPtr->toJson();
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    if (!apiUtils::isPremiumServer(serverConfigObject)) {
        return ErrorCode::NoError;
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
        return errorCode;
    }

    if (uuid == m_settings->getInstallationUuid(true)) {
        serverConfigPtr->containerConfigs.clear();
        m_serversModel->editServer(serverConfigPtr, serverIndex);
    }

    return ErrorCode::NoError;
}

bool ApiConfigsController::isConfigValid()
{
    int serverIndex = m_serversModel->getDefaultServerIndex();
    auto serverConfigPtr = m_serversModel->getServerConfig(serverIndex);
    QJsonObject serverConfigObject = serverConfigPtr->toJson();
    auto configSource = apiUtils::getConfigSource(serverConfigObject);

    if (configSource == amnezia::ServerConfigType::ApiV1
        && !m_serversModel->data(serverIndex, ServersModel::Roles::HasInstalledContainers).toBool()) {
        removeApiConfig(serverIndex);
        return updateServiceFromTelegram(serverIndex);
    } else if (configSource == amnezia::ServerConfigType::ApiV2
               && !m_serversModel->data(serverIndex, ServersModel::Roles::HasInstalledContainers).toBool()) {
        return updateServiceFromGateway(serverIndex, "", "");
    } else if (configSource && isApiKeyExpired(serverIndex)) {
        qDebug() << "attempt to update api config by expires_at event";
        if (configSource == amnezia::ServerConfigType::ApiV2) {
            return updateServiceFromGateway(serverIndex, "", "");
        } else {
            removeApiConfig(serverIndex);
            return updateServiceFromTelegram(serverIndex);
        }
    }
    return true;
}

void ApiConfigsController::setCurrentProtocol(const QString &protocolName)
{
    auto serverIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfigPtr = m_serversModel->getServerConfig(serverIndex);
    auto serverConfigObject = serverConfigPtr->toJson();
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    apiConfigObject[configKey::serviceProtocol] = protocolName;

    serverConfigObject.insert(configKey::apiConfig, apiConfigObject);

    auto updatedPtr = ServerConfig::createServerConfig(serverConfigObject);
    m_serversModel->editServer(updatedPtr, serverIndex);
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
