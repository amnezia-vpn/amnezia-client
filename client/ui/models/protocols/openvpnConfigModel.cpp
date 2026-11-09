#include "openvpnConfigModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;

OpenVpnConfigModel::OpenVpnConfigModel(QObject *parent) : QObject(parent)
{
}

QString OpenVpnConfigModel::subnetAddress() const
{
    return m_protocolConfig.serverConfig.subnetAddress;
}

void OpenVpnConfigModel::setSubnetAddress(const QString &value)
{
    if (m_protocolConfig.serverConfig.subnetAddress == value) {
        return;
    }
    m_protocolConfig.serverConfig.subnetAddress = value;
    emit changed();
}

QString OpenVpnConfigModel::transportProto() const
{
    return m_protocolConfig.serverConfig.transportProto;
}

void OpenVpnConfigModel::setTransportProto(const QString &value)
{
    if (m_protocolConfig.serverConfig.transportProto == value) {
        return;
    }
    m_protocolConfig.serverConfig.transportProto = value;
    emit changed();
}

QString OpenVpnConfigModel::port() const
{
    return m_protocolConfig.serverConfig.port;
}

void OpenVpnConfigModel::setPort(const QString &value)
{
    if (m_protocolConfig.serverConfig.port == value) {
        return;
    }
    m_protocolConfig.serverConfig.port = value;
    emit changed();
}

bool OpenVpnConfigModel::autoNegotiateEncryprion() const
{
    return !m_protocolConfig.serverConfig.ncpDisable;
}

void OpenVpnConfigModel::setAutoNegotiateEncryprion(bool value)
{
    if (m_protocolConfig.serverConfig.ncpDisable == !value) {
        return;
    }
    m_protocolConfig.serverConfig.ncpDisable = !value;
    emit changed();
}

QString OpenVpnConfigModel::hash() const
{
    return m_protocolConfig.serverConfig.hash;
}

void OpenVpnConfigModel::setHash(const QString &value)
{
    if (m_protocolConfig.serverConfig.hash == value) {
        return;
    }
    m_protocolConfig.serverConfig.hash = value;
    emit changed();
}

QString OpenVpnConfigModel::cipher() const
{
    return m_protocolConfig.serverConfig.cipher;
}

void OpenVpnConfigModel::setCipher(const QString &value)
{
    if (m_protocolConfig.serverConfig.cipher == value) {
        return;
    }
    m_protocolConfig.serverConfig.cipher = value;
    emit changed();
}

bool OpenVpnConfigModel::tlsAuth() const
{
    return m_protocolConfig.serverConfig.tlsAuth;
}

void OpenVpnConfigModel::setTlsAuth(bool value)
{
    if (m_protocolConfig.serverConfig.tlsAuth == value) {
        return;
    }
    m_protocolConfig.serverConfig.tlsAuth = value;
    emit changed();
}

bool OpenVpnConfigModel::blockDns() const
{
    if (m_protocolConfig.clientConfig.has_value()) {
        return m_protocolConfig.clientConfig->blockOutsideDns;
    }
    return protocols::openvpn::defaultBlockOutsideDns;
}

void OpenVpnConfigModel::setBlockDns(bool value)
{
    if (!m_protocolConfig.clientConfig.has_value()) {
        m_protocolConfig.clientConfig = amnezia::OpenVpnClientConfig{};
    }
    if (m_protocolConfig.clientConfig->blockOutsideDns == value) {
        return;
    }
    m_protocolConfig.clientConfig->blockOutsideDns = value;
    emit changed();
}

QString OpenVpnConfigModel::additionalClientCommands() const
{
    return m_protocolConfig.serverConfig.additionalClientConfig;
}

void OpenVpnConfigModel::setAdditionalClientCommands(const QString &value)
{
    if (m_protocolConfig.serverConfig.additionalClientConfig == value) {
        return;
    }
    m_protocolConfig.serverConfig.additionalClientConfig = value;
    emit changed();
}

QString OpenVpnConfigModel::additionalServerCommands() const
{
    return m_protocolConfig.serverConfig.additionalServerConfig;
}

void OpenVpnConfigModel::setAdditionalServerCommands(const QString &value)
{
    if (m_protocolConfig.serverConfig.additionalServerConfig == value) {
        return;
    }
    m_protocolConfig.serverConfig.additionalServerConfig = value;
    emit changed();
}

bool OpenVpnConfigModel::isPortEditable() const
{
    return m_container == DockerContainer::OpenVpn;
}

bool OpenVpnConfigModel::isTransportProtoEditable() const
{
    return m_container == DockerContainer::OpenVpn;
}

bool OpenVpnConfigModel::hasRemoveButton() const
{
    return m_container == DockerContainer::OpenVpn;
}

void OpenVpnConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::OpenVpnProtocolConfig &protocolConfig)
{
    m_container = container;
    
    m_protocolConfig = protocolConfig;
    
    applyDefaultsToServerConfig(m_protocolConfig.serverConfig);
    
    if (!m_protocolConfig.clientConfig.has_value()) {
        m_protocolConfig.clientConfig = amnezia::OpenVpnClientConfig{};
    }
    applyDefaultsToClientConfig(m_protocolConfig.clientConfig.value());
    
    m_originalProtocolConfig = m_protocolConfig;
    
    emit changed();
}

void OpenVpnConfigModel::applyDefaultsToServerConfig(amnezia::OpenVpnServerConfig& config)
{
    if (config.subnetAddress.isEmpty()) {
        config.subnetAddress = protocols::openvpn::defaultSubnetAddress;
    }
    if (config.port.isEmpty()) {
        config.port = protocols::openvpn::defaultPort;
    }
    if (config.transportProto.isEmpty()) {
        if (m_container == DockerContainer::OpenVpn) {
            config.transportProto = protocols::openvpn::defaultTransportProto;
        } else {
            config.transportProto = "tcp";
        }
    }
    if (config.cipher.isEmpty()) {
        config.cipher = protocols::openvpn::defaultCipher;
    }
    if (config.hash.isEmpty()) {
        config.hash = protocols::openvpn::defaultHash;
    }
}

void OpenVpnConfigModel::applyDefaultsToClientConfig(amnezia::OpenVpnClientConfig& config)
{
    if (!config.blockOutsideDns && !m_protocolConfig.serverConfig.additionalClientConfig.isEmpty()) {
        config.blockOutsideDns = protocols::openvpn::defaultBlockOutsideDns;
    }
}

amnezia::OpenVpnProtocolConfig OpenVpnConfigModel::getProtocolConfig()
{
    bool serverSettingsChanged = !m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig);
    
    if (serverSettingsChanged) {
        m_protocolConfig.clearClientConfig();
    }
    
    return m_protocolConfig;
}
