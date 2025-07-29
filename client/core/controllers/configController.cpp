#include "configController.h"

#include "core/models/containers/containers_defs.h"
#include "core/models/servers/apiV1ServerConfig.h"
#include "core/models/servers/apiV2ServerConfig.h"
#include "core/models/servers/selfHostedServerConfig.h"
#include "core/networkUtilities.h"
#include "protocols/protocols_defs.h"
#include "settings.h"

ConfigController::ConfigController(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
}

void ConfigController::addServer(const QSharedPointer<ServerConfig> &serverConfig)
{
    m_settings->addServer(serverConfig->toJson());
    emit serverAdded(m_settings->serversCount() - 1);
}

void ConfigController::editServer(const QSharedPointer<ServerConfig> &serverConfig, int serverIndex)
{
    updateServerInSettings(serverConfig, serverIndex);
    emit serverEdited(serverIndex);
}

void ConfigController::removeServer(int serverIndex)
{
    m_settings->removeServer(serverIndex);
    emit serverRemoved(serverIndex);
}

void ConfigController::setDefaultServer(int serverIndex)
{
    m_settings->setDefaultServer(serverIndex);
    emit defaultServerChanged(serverIndex);
}

void ConfigController::setDefaultContainer(int serverIndex, int containerIndex)
{
    auto servers = m_settings->serversArray();
    if (serverIndex >= servers.size()) return;
    
    auto serverConfig = ServerConfig::createServerConfig(servers.at(serverIndex).toObject());
    auto container = static_cast<DockerContainer>(containerIndex);
    serverConfig->defaultContainer = ContainerProps::containerToString(container);
    
    updateServerInSettings(serverConfig, serverIndex);
    emit defaultContainerChanged(serverIndex, container);
}

bool ConfigController::isServerFromApiAlreadyExists(quint16 crc) const
{
    auto servers = m_settings->serversArray();
    for (const auto &server : servers) {
        auto serverConfig = ServerConfig::createServerConfig(server.toObject());
        if (static_cast<quint16>(serverConfig->crc) == crc) {
            return true;
        }
    }
    return false;
}

bool ConfigController::isServerFromApiAlreadyExists(const QString &userCountryCode, 
                                                   const QString &serviceType, 
                                                   const QString &serviceProtocol) const
{
    auto servers = m_settings->serversArray();
    for (const auto &server : servers) {
        auto serverConfig = ServerConfig::createServerConfig(server.toObject());
        
        if (serverConfig->hostName == "api.amnezia.org") {
            auto apiV2Config = serverConfig.dynamicCast<ApiV2ServerConfig>();
            if (apiV2Config && 
                apiV2Config->apiConfigData.countryCode == userCountryCode &&
                apiV2Config->apiConfigData.serviceInfo.serviceType == serviceType &&
                apiV2Config->apiConfigData.serviceInfo.serviceProtocol == serviceProtocol) {
                return true;
            }
        }
    }
    return false;
}

bool ConfigController::isApiKeyExpired(int serverIndex) const
{
    auto servers = m_settings->serversArray();
    if (serverIndex >= servers.size()) return false;
    
    auto serverConfig = ServerConfig::createServerConfig(servers.at(serverIndex).toObject());
    
    if (serverConfig->hostName == "api.amnezia.org") {
        auto apiV2Config = serverConfig.dynamicCast<ApiV2ServerConfig>();
        if (apiV2Config) {
            QDateTime expiresAt = QDateTime::fromString(apiV2Config->apiConfigData.serviceInfo.expiresAt, Qt::ISODate);
            return QDateTime::currentDateTime() > expiresAt;
        }
    }
    return false;
}

void ConfigController::removeApiConfig(int serverIndex)
{
    auto servers = m_settings->serversArray();
    if (serverIndex >= servers.size()) return;
    
    auto serverConfig = ServerConfig::createServerConfig(servers.at(serverIndex).toObject());
    
    if (serverConfig->hostName == "api.amnezia.org") {
        auto apiV2Config = serverConfig.dynamicCast<ApiV2ServerConfig>();
        if (apiV2Config) {
            apiV2Config->apiConfigData = ApiV2ServerConfig::ApiConfigData{};
            updateServerInSettings(apiV2Config, serverIndex);
        }
    }
}

QStringList ConfigController::getAllInstalledServicesName(int serverIndex) const
{
    auto servers = m_settings->serversArray();
    if (serverIndex >= servers.size()) return {};
    
    auto serverConfig = ServerConfig::createServerConfig(servers.at(serverIndex).toObject());
    
    QStringList serviceNames;
    for (auto it = serverConfig->containerConfigs.constBegin(); 
         it != serverConfig->containerConfigs.constEnd(); ++it) {
        const QString &containerName = it.key();
        serviceNames.append(containerName);
    }
    return serviceNames;
}

void ConfigController::clearCachedProfile(int serverIndex, DockerContainer container)
{
    auto servers = m_settings->serversArray();
    if (serverIndex >= servers.size()) return;
    
    auto serverConfig = ServerConfig::createServerConfig(servers.at(serverIndex).toObject());
    
    QString containerName = ContainerProps::containerToString(container);
    if (serverConfig->containerConfigs.contains(containerName)) {
        auto &containerConfig = serverConfig->containerConfigs[containerName];
        containerConfig.clearProfile();
        updateServerInSettings(serverConfig, serverIndex);
    }
}

void ConfigController::updateServerInSettings(const QSharedPointer<ServerConfig> &serverConfig, int serverIndex)
{
    m_settings->editServer(serverIndex, serverConfig->toJson());
} 
