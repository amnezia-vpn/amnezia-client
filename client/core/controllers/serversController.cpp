#include "serversController.h"
#include "core/utils/networkUtilities.h"
#include "core/utils/api/apiDefs.h"
#include "protocols/protocols_defs.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <AmneziaVPN-Swift.h>
#endif

namespace
{
    namespace configKey
    {
        constexpr char apiConfig[] = "api_config";
        constexpr char publicKeyInfo[] = "public_key";
        constexpr char expiresAt[] = "expires_at";
        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serviceType[] = "service_type";
    }
}

ServersController::ServersController(ServersRepository* serversRepository, 
                                      AppSettingsRepository* appSettingsRepository,
                                      QObject *parent)
    : QObject(parent), m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository)
{
    recomputeGatewayStacks();
}

void ServersController::addServer(const ServerConfig &server)
{
    m_serversRepository->addServer(server);
}

void ServersController::editServer(int index, const ServerConfig &server)
{
    m_serversRepository->editServer(index, server);
}

void ServersController::removeServer(int index)
{
    m_serversRepository->removeServer(index);
}

void ServersController::setDefaultServerIndex(int index)
{
    m_serversRepository->setDefaultServer(index);
}

void ServersController::setDefaultContainer(int serverIndex, DockerContainer container)
{
    m_serversRepository->setDefaultContainer(serverIndex, container);
}

void ServersController::updateContainerConfig(int serverIndex, DockerContainer container, const ContainerConfig &config)
{
    m_serversRepository->setContainerConfig(serverIndex, container, config);
}

void ServersController::addContainerConfig(int serverIndex, DockerContainer container, const ContainerConfig &config)
{
    ServerConfig serverConfig = m_serversRepository->server(serverIndex);
    ServerConfigUtils::visit(serverConfig, [container, &config](auto& arg) {
        arg.containers[container] = config;
        
        if (arg.defaultContainer == DockerContainer::None
            && ContainerProps::containerService(container) != ServiceType::Other 
            && ContainerProps::isSupportedByCurrentPlatform(container)) {
            arg.defaultContainer = container;
        }
    });
    
    m_serversRepository->editServer(serverIndex, serverConfig);
}

void ServersController::clearCachedProfile(int serverIndex, DockerContainer container)
{
    m_serversRepository->clearLastConnectionConfig(serverIndex, container);
}

void ServersController::reloadContainerConfig(int serverIndex, DockerContainer container)
{
    ContainerConfig config = m_serversRepository->containerConfig(serverIndex, container);
    m_serversRepository->setContainerConfig(serverIndex, container, config);
}

QJsonArray ServersController::getServersArray() const
{
    QJsonArray result;
    QVector<ServerConfig> servers = m_serversRepository->servers();
    for (const ServerConfig& server : servers) {
        result.append(ServerConfigUtils::toJson(server));
    }
    return result;
}

QJsonObject ServersController::getContainerConfig(int serverIndex, DockerContainer container) const
{
    ContainerConfig config = m_serversRepository->containerConfig(serverIndex, container);
    return config.toJson();
}

int ServersController::getDefaultServerIndex() const
{
    return m_serversRepository->defaultServerIndex();
}

int ServersController::getServersCount() const
{
    return m_serversRepository->serversCount();
}

ServerConfig ServersController::getServerConfig(int serverIndex) const
{
    return m_serversRepository->server(serverIndex);
}

ServerCredentials ServersController::getServerCredentials(int serverIndex) const
{
    return m_serversRepository->serverCredentials(serverIndex);
}

QPair<QString, QString> ServersController::getDnsPair(int serverIndex, bool isAmneziaDnsEnabled) const
{
    ServerConfig serverConfig = m_serversRepository->server(serverIndex);
    return ServerConfigUtils::getDnsPair(serverConfig, 
                                          isAmneziaDnsEnabled,
                                          m_appSettingsRepository->primaryDns(),
                                          m_appSettingsRepository->secondaryDns());
}

bool ServersController::isServerFromApiAlreadyExists(const quint16 crc) const
{
    QVector<ServerConfig> servers = m_serversRepository->servers();
    for (const ServerConfig& serverConfig : servers) {
        if (static_cast<quint16>(ServerConfigUtils::crc(serverConfig)) == crc) {
            return true;
        }
    }
    return false;
}

ServersController::GatewayStacks ServersController::gatewayStacks() const
{
    return m_gatewayStacks;
}

void ServersController::recomputeGatewayStacks()
{
    GatewayStacks computed;
    bool hasNewTags = false;
    QVector<ServerConfig> servers = m_serversRepository->servers();

    for (const ServerConfig& serverConfig : servers) {
        if (ServerConfigUtils::isApiV2Config(serverConfig)) {
            const ApiV2ServerConfig& apiV2 = ServerConfigUtils::asApiV2(serverConfig);
            const QString userCountryCode = apiV2.apiConfig.userCountryCode;
            const QString serviceType = apiV2.serviceType();

            if (!userCountryCode.isEmpty()) {
                if (!m_gatewayStacks.userCountryCodes.contains(userCountryCode)) {
                    hasNewTags = true;
                }
                computed.userCountryCodes.insert(userCountryCode);
            }

            if (!serviceType.isEmpty()) {
                if (!m_gatewayStacks.serviceTypes.contains(serviceType)) {
                    hasNewTags = true;
                }
                computed.serviceTypes.insert(serviceType);
            }
        }
    }

    m_gatewayStacks = std::move(computed);
    if (hasNewTags) {
        emit gatewayStacksExpanded();
    }
}

bool ServersController::GatewayStacks::operator==(const GatewayStacks &other) const
{
    return userCountryCodes == other.userCountryCodes && serviceTypes == other.serviceTypes;
}

QJsonObject ServersController::GatewayStacks::toJson() const
{
    QJsonObject json;
    
    QJsonArray userCountryCodesArray;
    for (const QString &code : userCountryCodes) {
        userCountryCodesArray.append(code);
    }
    json["user_country_code"] = userCountryCodesArray;
    
    QJsonArray serviceTypesArray;
    for (const QString &type : serviceTypes) {
        serviceTypesArray.append(type);
    }
    json["service_type"] = serviceTypesArray;
    
    return json;
}

bool ServersController::isServerFromApiAlreadyExists(const QString &userCountryCode, const QString &serviceType, const QString &serviceProtocol) const
{
    QVector<ServerConfig> servers = m_serversRepository->servers();
    for (const ServerConfig& serverConfig : servers) {
        if (ServerConfigUtils::isApiV2Config(serverConfig)) {
            const ApiV2ServerConfig& apiV2 = ServerConfigUtils::asApiV2(serverConfig);
            if (apiV2.apiConfig.userCountryCode == userCountryCode
                && apiV2.serviceType() == serviceType
                && apiV2.serviceProtocol() == serviceProtocol) {
                return true;
            }
        }
    }
    return false;
}

bool ServersController::hasInstalledContainers(int serverIndex) const
{
    ServerConfig serverConfig = m_serversRepository->server(serverIndex);
    QMap<DockerContainer, ContainerConfig> containers = ServerConfigUtils::containers(serverConfig);
    for (auto it = containers.begin(); it != containers.end(); ++it) {
        DockerContainer container = it.key();
        if (ContainerProps::containerService(container) == ServiceType::Vpn) {
            return true;
        }
        if (container == DockerContainer::SSXray) {
            return true;
        }
    }
    return false;
}

