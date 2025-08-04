#include "openvpnProtocolConfig.h"

#include "protocols/protocols_defs.h"
#include <QJsonDocument>

using namespace amnezia;

OpenVpnProtocolConfig::OpenVpnProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName)
    : ProtocolConfig(protocolName)
{
    serverProtocolConfig.subnetAddress = protocolConfigObject.value(config_key::subnet_address).toString(protocols::openvpn::defaultSubnetAddress);
    serverProtocolConfig.transportProto = protocolConfigObject.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto);
    serverProtocolConfig.port = protocolConfigObject.value(config_key::port).toString(protocols::openvpn::defaultPort);
    serverProtocolConfig.ncpDisable = protocolConfigObject.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable);
    serverProtocolConfig.hash = protocolConfigObject.value(config_key::hash).toString(protocols::openvpn::defaultHash);
    serverProtocolConfig.cipher = protocolConfigObject.value(config_key::cipher).toString(protocols::openvpn::defaultCipher);
    serverProtocolConfig.tlsAuth = protocolConfigObject.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth);
    serverProtocolConfig.blockOutsideDns = protocolConfigObject.value(config_key::block_outside_dns).toBool(protocols::openvpn::defaultBlockOutsideDns);
    serverProtocolConfig.additionalClientConfig = protocolConfigObject.value(config_key::additional_client_config).toString(protocols::openvpn::defaultAdditionalClientConfig);
    serverProtocolConfig.additionalServerConfig = protocolConfigObject.value(config_key::additional_server_config).toString(protocols::openvpn::defaultAdditionalServerConfig);

    auto clientProtocolString = protocolConfigObject.value(config_key::last_config).toString();
    if (!clientProtocolString.isEmpty()) {
        clientProtocolConfig.isEmpty = false;

        QJsonObject clientProtocolConfigObject = QJsonDocument::fromJson(clientProtocolString.toUtf8()).object();

        clientProtocolConfig.clientId = clientProtocolConfigObject.value(config_key::clientId).toString();
        clientProtocolConfig.nativeConfig = clientProtocolConfigObject.value(config_key::config).toString();
    }
}

QJsonObject OpenVpnProtocolConfig::toJson() const
{
    QJsonObject json;

    if (!serverProtocolConfig.subnetAddress.isEmpty()) {
        json[config_key::subnet_address] = serverProtocolConfig.subnetAddress;
    }
    if (!serverProtocolConfig.transportProto.isEmpty()) {
        json[config_key::transport_proto] = serverProtocolConfig.transportProto;
    }
    if (!serverProtocolConfig.port.isEmpty()) {
        json[config_key::port] = serverProtocolConfig.port;
    }
    json[config_key::ncp_disable] = serverProtocolConfig.ncpDisable;
    if (!serverProtocolConfig.hash.isEmpty()) {
        json[config_key::hash] = serverProtocolConfig.hash;
    }
    if (!serverProtocolConfig.cipher.isEmpty()) {
        json[config_key::cipher] = serverProtocolConfig.cipher;
    }
    json[config_key::tls_auth] = serverProtocolConfig.tlsAuth;
    json[config_key::block_outside_dns] = serverProtocolConfig.blockOutsideDns;
    if (!serverProtocolConfig.additionalClientConfig.isEmpty()) {
        json[config_key::additional_client_config] = serverProtocolConfig.additionalClientConfig;
    }
    if (!serverProtocolConfig.additionalServerConfig.isEmpty()) {
        json[config_key::additional_server_config] = serverProtocolConfig.additionalServerConfig;
    }

    if (!clientProtocolConfig.isEmpty) {
        QJsonObject clientConfigJson;

        if (!clientProtocolConfig.clientId.isEmpty()) {
            clientConfigJson[config_key::clientId] = clientProtocolConfig.clientId;
        }
        if (!clientProtocolConfig.nativeConfig.isEmpty()) {
            clientConfigJson[config_key::config] = clientProtocolConfig.nativeConfig;
        }

        if (!clientConfigJson.isEmpty()) {
            json[config_key::last_config] = QString(QJsonDocument(clientConfigJson).toJson());
        }
    }

    return json;
}

bool OpenVpnProtocolConfig::hasEqualServerSettings(const OpenVpnProtocolConfig &other) const
{
    if (serverProtocolConfig.subnetAddress != other.serverProtocolConfig.subnetAddress
        || serverProtocolConfig.transportProto != other.serverProtocolConfig.transportProto
        || serverProtocolConfig.port != other.serverProtocolConfig.port
        || serverProtocolConfig.ncpDisable != other.serverProtocolConfig.ncpDisable
        || serverProtocolConfig.hash != other.serverProtocolConfig.hash || serverProtocolConfig.cipher != other.serverProtocolConfig.cipher
        || serverProtocolConfig.tlsAuth != other.serverProtocolConfig.tlsAuth
        || serverProtocolConfig.blockOutsideDns != other.serverProtocolConfig.blockOutsideDns
        || serverProtocolConfig.additionalClientConfig != other.serverProtocolConfig.additionalClientConfig
        || serverProtocolConfig.additionalServerConfig != other.serverProtocolConfig.additionalServerConfig) {
        return false;
    }
    return true;
}

void OpenVpnProtocolConfig::clearClientSettings()
{
    clientProtocolConfig = openvpn::ClientProtocolConfig();
}
