#include "serversController.h"
#include "core/utils/networkUtilities.h"
#include "core/utils/api/apiEnums.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <AmneziaVPN-Swift.h>
#endif


ServersController::ServersController(SecureServersRepository* serversRepository, 
                                      SecureAppSettingsRepository* appSettingsRepository,
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

void ServersController::clearCachedProfile(int serverIndex, DockerContainer container)
{
    m_serversRepository->clearLastConnectionConfig(serverIndex, container);
}

QJsonArray ServersController::getServersArray() const
{
    QJsonArray result;
    QVector<ServerConfig> servers = m_serversRepository->servers();
    for (const ServerConfig& server : servers) {
        result.append(server.toJson());
    }
    return result;
}

QVector<ServerConfig> ServersController::getServers() const
{
    return m_serversRepository->servers();
}

ContainerConfig ServersController::getContainerConfig(int serverIndex, DockerContainer container) const
{
    return m_serversRepository->containerConfig(serverIndex, container);
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
    return serverConfig.getDnsPair(isAmneziaDnsEnabled,
                                   m_appSettingsRepository->primaryDns(),
                                   m_appSettingsRepository->secondaryDns());
}

ServersController::GatewayStacksData ServersController::gatewayStacks() const
{
    return m_gatewayStacks;
}

void ServersController::recomputeGatewayStacks()
{
    GatewayStacksData computed;
    bool hasNewTags = false;
    QVector<ServerConfig> servers = m_serversRepository->servers();

    for (const ServerConfig& serverConfig : servers) {
        if (serverConfig.isApiV2()) {
            const ApiV2ServerConfig* apiV2 = serverConfig.as<ApiV2ServerConfig>();
            if (!apiV2) continue;
            const QString userCountryCode = apiV2->apiConfig.userCountryCode;
            const QString serviceType = apiV2->serviceType();

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

bool ServersController::GatewayStacksData::operator==(const GatewayStacksData &other) const
{
    return userCountryCodes == other.userCountryCodes && serviceTypes == other.serviceTypes;
}

QJsonObject ServersController::GatewayStacksData::toJson() const
{
    QJsonObject json;
    
    QJsonArray userCountryCodesArray;
    for (const QString &code : userCountryCodes) {
        userCountryCodesArray.append(code);
    }
    json[apiDefs::key::userCountryCode] = userCountryCodesArray;
    
    QJsonArray serviceTypesArray;
    for (const QString &type : serviceTypes) {
        serviceTypesArray.append(type);
    }
    json[apiDefs::key::serviceType] = serviceTypesArray;
    
    return json;
}

bool ServersController::isServerFromApiAlreadyExists(const QString &userCountryCode, const QString &serviceType, const QString &serviceProtocol) const
{
    QVector<ServerConfig> servers = m_serversRepository->servers();
    for (const ServerConfig& serverConfig : servers) {
        if (serverConfig.isApiV2()) {
            const ApiV2ServerConfig* apiV2 = serverConfig.as<ApiV2ServerConfig>();
            if (!apiV2) return false;
            if (apiV2->apiConfig.userCountryCode == userCountryCode
                && apiV2->serviceType() == serviceType
                && apiV2->serviceProtocol() == serviceProtocol) {
                return true;
            }
        }
    }
    return false;
}

bool ServersController::hasInstalledContainers(int serverIndex) const
{
    ServerConfig serverConfig = m_serversRepository->server(serverIndex);
    QMap<DockerContainer, ContainerConfig> containers = serverConfig.containers();
    for (auto it = containers.begin(); it != containers.end(); ++it) {
        DockerContainer container = it.key();
        if (ContainerUtils::containerService(container) == ServiceType::Vpn) {
            return true;
        }
        if (container == DockerContainer::SSXray) {
            return true;
        }
    }
    return false;
}

