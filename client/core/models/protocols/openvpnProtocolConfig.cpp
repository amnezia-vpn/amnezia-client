#include "openvpnProtocolConfig.h"

#include <QJsonDocument>
#include "protocols/protocols_defs.h"

using namespace amnezia;

OpenVpnProtocolConfig::OpenVpnProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.subnetAddress = protocolConfigObject.value(config_key::subnet_address).toString();
    serverProtocolConfig.transportProto = protocolConfigObject.value(config_key::transport_proto).toString();
    serverProtocolConfig.port = protocolConfigObject.value(config_key::port).toString();
    serverProtocolConfig.ncpDisable = protocolConfigObject.value(config_key::ncp_disable).toString();
    serverProtocolConfig.hash = protocolConfigObject.value(config_key::hash).toString();
    serverProtocolConfig.cipher = protocolConfigObject.value(config_key::cipher).toString();
    serverProtocolConfig.tlsAuth = protocolConfigObject.value(config_key::tls_auth).toString();
    serverProtocolConfig.blockOutsideDns = protocolConfigObject.value(config_key::block_outside_dns).toString();
    serverProtocolConfig.additionalClientConfig = protocolConfigObject.value(config_key::additional_client_config).toString();
    serverProtocolConfig.additionalServerConfig = protocolConfigObject.value(config_key::additional_server_config).toString();

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
    if (!serverProtocolConfig.ncpDisable.isEmpty()) {
        json[config_key::ncp_disable] = serverProtocolConfig.ncpDisable;
    }
    if (!serverProtocolConfig.hash.isEmpty()) {
        json[config_key::hash] = serverProtocolConfig.hash;
    }
    if (!serverProtocolConfig.cipher.isEmpty()) {
        json[config_key::cipher] = serverProtocolConfig.cipher;
    }
    if (!serverProtocolConfig.tlsAuth.isEmpty()) {
        json[config_key::tls_auth] = serverProtocolConfig.tlsAuth;
    }
    if (!serverProtocolConfig.blockOutsideDns.isEmpty()) {
        json[config_key::block_outside_dns] = serverProtocolConfig.blockOutsideDns;
    }
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
