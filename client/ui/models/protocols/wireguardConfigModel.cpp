#include "wireguardConfigModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;
using namespace ProtocolUtils;

WireGuardConfigModel::WireGuardConfigModel(QObject *parent) : QObject(parent)
{
}

QString WireGuardConfigModel::subnetAddress() const
{
    return m_protocolConfig.serverConfig.subnetAddress;
}

void WireGuardConfigModel::setSubnetAddress(const QString &value)
{
    if (m_protocolConfig.serverConfig.subnetAddress == value) {
        return;
    }
    m_protocolConfig.serverConfig.subnetAddress = value;
    emit changed();
}

QString WireGuardConfigModel::port() const
{
    return m_protocolConfig.serverConfig.port;
}

void WireGuardConfigModel::setPort(const QString &value)
{
    if (m_protocolConfig.serverConfig.port == value) {
        return;
    }
    m_protocolConfig.serverConfig.port = value;
    emit changed();
}

QString WireGuardConfigModel::clientMtu() const
{
    if (m_protocolConfig.clientConfig.has_value()) {
        return m_protocolConfig.clientConfig->mtu;
    }
    return QString(protocols::wireguard::defaultMtu);
}

void WireGuardConfigModel::setClientMtu(const QString &value)
{
    if (!m_protocolConfig.clientConfig.has_value()) {
        m_protocolConfig.clientConfig = amnezia::WireGuardClientConfig{};
    }
    if (m_protocolConfig.clientConfig->mtu == value) {
        return;
    }
    m_protocolConfig.clientConfig->mtu = value;
    emit changed();
}

void WireGuardConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::WireGuardProtocolConfig &protocolConfig)
{
    m_container = container;
    
    m_protocolConfig = protocolConfig;
    
    applyDefaultsToServerConfig(m_protocolConfig.serverConfig);

    if (!m_protocolConfig.clientConfig.has_value()) {
        m_protocolConfig.clientConfig = amnezia::WireGuardClientConfig{};
    }
    applyDefaultsToClientConfig(m_protocolConfig.clientConfig.value());
    
    m_originalProtocolConfig = m_protocolConfig;
    
    emit changed();
}

void WireGuardConfigModel::applyDefaultsToServerConfig(amnezia::WireGuardServerConfig& config)
{
    if (config.subnetAddress.isEmpty()) {
        config.subnetAddress = protocols::wireguard::defaultSubnetAddress;
    }
}

void WireGuardConfigModel::applyDefaultsToClientConfig(amnezia::WireGuardClientConfig& config)
{
    if (config.mtu.isEmpty()) {
        config.mtu = protocols::wireguard::defaultMtu;
    }
}

amnezia::WireGuardProtocolConfig WireGuardConfigModel::getProtocolConfig()
{
    bool serverSettingsChanged = !m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig);
    
    if (serverSettingsChanged) {
        m_protocolConfig.clearClientConfig();
    }
    
    return m_protocolConfig;
}

bool WireGuardConfigModel::isServerSettingsEqual()
{
    return m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig);
}
