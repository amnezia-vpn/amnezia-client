#include "openvpnConfigModel.h"

#include "protocols/protocols_defs.h"

OpenVpnConfigModel::OpenVpnConfigModel(QObject *parent)
    : QObject(parent)
{
}

QString OpenVpnConfigModel::subnetAddress() const
{
    return m_protocolConfig.value(amnezia::config_key::subnet_address).toString(amnezia::protocols::openvpn::defaultSubnetAddress);
}

void OpenVpnConfigModel::setSubnetAddress(const QString &subnetAddress)
{
    m_protocolConfig.insert(amnezia::config_key::subnet_address, subnetAddress);
}

QString OpenVpnConfigModel::transportProto() const
{
    return m_protocolConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto);
}

void OpenVpnConfigModel::setTransportProto(const QString &transportProto)
{
    m_protocolConfig.insert(config_key::transport_proto, transportProto);
}

QString OpenVpnConfigModel::port() const
{
    return m_protocolConfig.value(config_key::port).toString(protocols::openvpn::defaultPort);
}

void OpenVpnConfigModel::setPort(const QString &port)
{
    m_protocolConfig.insert(config_key::port, port);
}

bool OpenVpnConfigModel::autoNegotiateEncryption() const
{
    return !m_protocolConfig.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable);
}

void OpenVpnConfigModel::setAutoNegotiateEncryption(bool enabled)
{
    m_protocolConfig.insert(config_key::ncp_disable, !enabled);
}

QString OpenVpnConfigModel::hash() const
{
    return m_protocolConfig.value(config_key::hash).toString(protocols::openvpn::defaultHash);
}

void OpenVpnConfigModel::setHash(const QString &hash)
{
    m_protocolConfig.insert(config_key::hash, hash);
}

QString OpenVpnConfigModel::cipher() const
{
    return m_protocolConfig.value(config_key::cipher).toString(protocols::openvpn::defaultCipher);
}

void OpenVpnConfigModel::setCipher(const QString &cipher)
{
    m_protocolConfig.insert(config_key::cipher, cipher);
}

bool OpenVpnConfigModel::tlsAuth() const
{
    return m_protocolConfig.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth);
}

void OpenVpnConfigModel::setTlsAuth(bool enabled)
{
    m_protocolConfig.insert(config_key::tls_auth, enabled);
}

bool OpenVpnConfigModel::blockDns() const
{
    return m_protocolConfig.value(config_key::block_outside_dns).toBool(protocols::openvpn::defaultBlockOutsideDns);
}

void OpenVpnConfigModel::setBlockDns(bool enabled)
{
    m_protocolConfig.insert(config_key::block_outside_dns, enabled);
}

QString OpenVpnConfigModel::additionalClientCommands() const
{
    return m_protocolConfig.value(config_key::additional_client_config).toString(protocols::openvpn::defaultAdditionalClientConfig);
}

void OpenVpnConfigModel::setAdditionalClientCommands(const QString &commands)
{
    m_protocolConfig.insert(config_key::additional_client_config, commands);
}

QString OpenVpnConfigModel::additionalServerCommands() const
{
    return m_protocolConfig.value(config_key::additional_server_config).toString(protocols::openvpn::defaultAdditionalServerConfig);
}

void OpenVpnConfigModel::setAdditionalServerCommands(const QString &commands)
{
    m_protocolConfig.insert(config_key::additional_server_config, commands);
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

void OpenVpnConfigModel::updateModel(const QJsonObject &config)
{
    m_container = ContainerProps::containerFromString(config.value(config_key::container).toString());

    m_fullConfig = config;
    QJsonObject protocolConfig = config.value(config_key::openvpn).toObject();

    m_protocolConfig.insert(
            config_key::subnet_address,
            protocolConfig.value(amnezia::config_key::subnet_address).toString(amnezia::protocols::openvpn::defaultSubnetAddress));

    QString transportProto;
    if (m_container == DockerContainer::OpenVpn) {
        transportProto = protocolConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto);
    } else {
        transportProto = "tcp";
    }

    m_protocolConfig.insert(config_key::transport_proto, transportProto);

    m_protocolConfig.insert(config_key::ncp_disable,
                            protocolConfig.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable));
    m_protocolConfig.insert(config_key::cipher, protocolConfig.value(config_key::cipher).toString(protocols::openvpn::defaultCipher));
    m_protocolConfig.insert(config_key::hash, protocolConfig.value(config_key::hash).toString(protocols::openvpn::defaultHash));
    m_protocolConfig.insert(config_key::block_outside_dns,
                            protocolConfig.value(config_key::block_outside_dns).toBool(protocols::openvpn::defaultBlockOutsideDns));
    m_protocolConfig.insert(config_key::port, protocolConfig.value(config_key::port).toString(protocols::openvpn::defaultPort));
    m_protocolConfig.insert(config_key::tls_auth, protocolConfig.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth));
    m_protocolConfig.insert(
            config_key::additional_client_config,
            protocolConfig.value(config_key::additional_client_config).toString(protocols::openvpn::defaultAdditionalClientConfig));
    m_protocolConfig.insert(
            config_key::additional_server_config,
            protocolConfig.value(config_key::additional_server_config).toString(protocols::openvpn::defaultAdditionalServerConfig));
}

QJsonObject OpenVpnConfigModel::getConfig()
{
    m_fullConfig.insert(config_key::openvpn, m_protocolConfig);
    return m_fullConfig;
}
