#include "configController.h"

#include "core/models/containers/containers_defs.h"
#include "core/models/servers/apiV1ServerConfig.h"
#include "core/models/servers/apiV2ServerConfig.h"
#include "core/models/servers/selfHostedServerConfig.h"
#include "core/networkUtilities.h"
#include "protocols/protocols_defs.h"
#include "settings.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/wireguardProtocolConfig.h"
#include "core/models/protocols/openvpnProtocolConfig.h"
#include "core/models/protocols/shadowsocksProtocolConfig.h"
#include "core/models/protocols/cloakProtocolConfig.h"

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

// Removed API-specific helpers; moved to ApiConfigsController

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

bool ConfigController::isDefaultServerDefaultContainerHasSplitTunneling() const
{
    int defaultServerIndex = m_settings->defaultServerIndex();
    auto servers = m_settings->serversArray();
    if (defaultServerIndex >= servers.size()) return false;

    auto serverConfig = ServerConfig::createServerConfig(servers.at(defaultServerIndex).toObject());
    if (!serverConfig->containerConfigs.contains(serverConfig->defaultContainer)) {
        return false;
    }

    const auto &containerConfig = serverConfig->containerConfigs[serverConfig->defaultContainer];
    return checkSplitTunnelingInContainer(containerConfig, serverConfig->defaultContainer);
}

bool ConfigController::checkSplitTunnelingInContainer(const ContainerConfig &containerConfig, const QString &defaultContainer) const
{
    const DockerContainer containerType = ContainerProps::containerFromString(defaultContainer);

    auto isWireguardHasSplit = [](const QString &nativeConfig, const QStringList &allowedIps) -> bool {
        if (nativeConfig.contains("AllowedIPs") && !nativeConfig.contains("AllowedIPs = 0.0.0.0/0, ::/0")) {
            return true;
        }
        if (!allowedIps.isEmpty() && !allowedIps.contains("0.0.0.0/0")) {
            return true;
        }
        return false;
    };

    if (containerType == DockerContainer::Awg || containerType == DockerContainer::WireGuard) {
        const auto protocolConfig = containerConfig.protocolConfigs.value(defaultContainer);
        if (!protocolConfig) return false;
        auto variant = ProtocolConfig::getProtocolConfigVariant(protocolConfig);
        return std::visit(
            [&](const auto &ptr) -> bool {
                using T = std::decay_t<decltype(ptr)>;
                if constexpr (std::is_same_v<T, QSharedPointer<AwgProtocolConfig>> || std::is_same_v<T, QSharedPointer<WireGuardProtocolConfig>>) {
                    return isWireguardHasSplit(ptr->clientProtocolConfig.nativeConfig,
                                               ptr->clientProtocolConfig.wireGuardData.allowedIps);
                }
                return false;
            },
            variant);
    }

    if (containerType == DockerContainer::Cloak || containerType == DockerContainer::OpenVpn || containerType == DockerContainer::ShadowSocks) {
        const auto &protocolConfig = containerConfig.protocolConfigs.value(ContainerProps::containerTypeToString(DockerContainer::OpenVpn));
        if (!protocolConfig) return false;
        auto variant = ProtocolConfig::getProtocolConfigVariant(protocolConfig);
        return std::visit(
            [&](const auto &ptr) -> bool {
                using T = std::decay_t<decltype(ptr)>;
                if constexpr (std::is_same_v<T, QSharedPointer<OpenVpnProtocolConfig>> ||
                              std::is_same_v<T, QSharedPointer<ShadowsocksProtocolConfig>> ||
                              std::is_same_v<T, QSharedPointer<CloakProtocolConfig>>) {
                    const auto nativeConfig = ptr->clientProtocolConfig.nativeConfig;
                    return (!nativeConfig.isEmpty() && !nativeConfig.contains("redirect-gateway"));
                }
                return false;
            },
            variant);
    }

    return false;
}
