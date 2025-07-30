#include "selfhostedConfigController.h"

#include "core/models/containers/containers_defs.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/cloakProtocolConfig.h"
#include "core/models/protocols/openvpnProtocolConfig.h"
#include "core/models/protocols/shadowsocksProtocolConfig.h"
#include "core/models/protocols/wireguardProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/networkUtilities.h"
#include "settings.h"
#include "logger.h"

namespace
{
    Logger logger("SelfhostedConfigController");
}

SelfhostedConfigController::SelfhostedConfigController(std::shared_ptr<Settings> settings, QObject *parent)
    : ConfigController(settings, parent)
{
    m_isAmneziaDnsEnabled = m_settings->useAmneziaDns();
}

bool SelfhostedConfigController::isDefaultServerDefaultContainerHasSplitTunneling() const
{
    int defaultServerIndex = m_settings->defaultServerIndex();
    auto servers = m_settings->serversArray();
    if (defaultServerIndex >= servers.size()) return false;
    
    auto serverConfig = ServerConfig::createServerConfig(servers.at(defaultServerIndex).toObject());
    auto defaultContainer = ContainerProps::containerFromString(serverConfig->defaultContainer);
    
    if (!serverConfig->containerConfigs.contains(serverConfig->defaultContainer)) {
        return false;
    }
    
    const auto &containerConfig = serverConfig->containerConfigs[serverConfig->defaultContainer];
    return checkSplitTunnelingInContainer(containerConfig, serverConfig->defaultContainer);
}

void SelfhostedConfigController::toggleAmneziaDns(bool enabled)
{
    m_isAmneziaDnsEnabled = enabled;
    m_settings->setUseAmneziaDns(enabled);
    emit amneziaDnsToggled(enabled);
}

QPair<QString, QString> SelfhostedConfigController::getDnsPair(int serverIndex) const
{
    auto servers = m_settings->serversArray();
    if (serverIndex >= servers.size()) return {};
    
    auto serverConfig = ServerConfig::createServerConfig(servers.at(serverIndex).toObject());
    QPair<QString, QString> dns;
    
    bool isDnsContainerInstalled = isAmneziaDnsContainerInstalled(serverIndex);
    
    dns.first = serverConfig->dns1;
    dns.second = serverConfig->dns2;
    
    if (isDnsContainerInstalled && m_isAmneziaDnsEnabled) {
        dns.first = serverConfig->hostName;
        dns.second = "";
    }
    
    return dns;
}

bool SelfhostedConfigController::isAmneziaDnsContainerInstalled(int serverIndex) const
{
    auto servers = m_settings->serversArray();
    if (serverIndex >= servers.size()) return false;
    
    auto serverConfig = ServerConfig::createServerConfig(servers.at(serverIndex).toObject());
    
    for (const auto &container : serverConfig->containerConfigs) {
        auto containerType = ContainerProps::containerFromString(container.containerName);
        if (containerType == DockerContainer::Dns) {
            return true;
        }
    }
    return false;
}


QMap<QString, QSharedPointer<ProtocolConfig>> SelfhostedConfigController::getProtocolConfigs(const QVector<QSharedPointer<ProtocolConfig>> &protocols)
{
    QMap<QString, QSharedPointer<ProtocolConfig>> protocolConfigs;

    for (const auto &config : protocols) {
        Proto protocol = ProtocolProps::protoFromString(config->protocolName);
        
        if (isProtocolSupported(protocol)) {
            protocolConfigs.insert(config->protocolName, config);
        }
    }

    return protocolConfigs;
}

void SelfhostedConfigController::updateProtocolConfiguration(Proto protocol, const QSharedPointer<ProtocolConfig> &protocolConfig)
{
    if (!isProtocolSupported(protocol)) {
        logger.warning() << "Protocol not supported:" << ProtocolProps::protoToString(protocol);
        return;
    }

    logger.info() << "Updating protocol configuration for:" << ProtocolProps::protoToString(protocol);
    
    emit protocolConfigUpdated(protocol, protocolConfig);
}

QSharedPointer<ProtocolConfig> SelfhostedConfigController::createProtocolConfig(Proto protocol)
{
    if (!isProtocolSupported(protocol)) {
        logger.warning() << "Attempting to create unsupported protocol config:" << ProtocolProps::protoToString(protocol);
        return nullptr;
    }

    switch (protocol) {
    case Proto::Awg:
        return QSharedPointer<AwgProtocolConfig>::create();
    case Proto::Cloak:
        return QSharedPointer<CloakProtocolConfig>::create();
    case Proto::OpenVpn:
        return QSharedPointer<OpenVpnProtocolConfig>::create();
    case Proto::ShadowSocks:
        return QSharedPointer<ShadowsocksProtocolConfig>::create();
    case Proto::WireGuard:
        return QSharedPointer<WireGuardProtocolConfig>::create();
    case Proto::Xray:
        return QSharedPointer<XrayProtocolConfig>::create();
    default:
        logger.warning() << "Unknown protocol in createProtocolConfig:" << ProtocolProps::protoToString(protocol);
        return nullptr;
    }
}

bool SelfhostedConfigController::isProtocolSupported(Proto protocol) const
{
    switch (protocol) {
    case Proto::Awg:
    case Proto::Cloak:
    case Proto::OpenVpn:
    case Proto::ShadowSocks:
    case Proto::WireGuard:
    case Proto::Xray:
        return true;
    default:
        return false;
    }
}

bool SelfhostedConfigController::checkSplitTunnelingInContainer(const ContainerConfig &containerConfig, 
                                                               const QString &defaultContainer) const
{
    for (const auto &protocol : containerConfig.protocolConfigs) {
        Proto protoType = ProtocolProps::protoFromString(protocol->protocolName);
        
        switch (protoType) {
        case Proto::Awg: {
            auto awgConfig = protocol.dynamicCast<AwgProtocolConfig>();
            if (awgConfig && awgConfig->clientConfig.awgConfig.mtu != 0) {
                return true;
            }
            break;
        }
        case Proto::WireGuard: {
            auto wgConfig = protocol.dynamicCast<WireGuardProtocolConfig>();
            if (wgConfig && wgConfig->clientConfig.mtu != 0) {
                return true;
            }
            break;
        }
        default:
            break;
        }
    }
    return false;
} 
