#include "wireguardProtocolConfig.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "protocols/protocols_defs.h"

using namespace amnezia;

WireGuardProtocolConfig::WireGuardProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName)
    : ProtocolConfig(protocolName)
{
    serverProtocolConfig.port = protocolConfigObject.value(config_key::port).toString();
    serverProtocolConfig.transportProto = protocolConfigObject.value(config_key::transport_proto).toString();
    serverProtocolConfig.subnetAddress = protocolConfigObject.value(config_key::subnet_address).toString();

    auto clientProtocolString = protocolConfigObject.value(config_key::last_config).toString();
    if (!clientProtocolString.isEmpty()) {
        clientProtocolConfig.isEmpty = false;

        QJsonObject clientProtocolConfigObject = QJsonDocument::fromJson(clientProtocolString.toUtf8()).object();

        clientProtocolConfig.clientId = clientProtocolConfigObject.value(config_key::clientId).toString();

        clientProtocolConfig.wireGuardData.clientIp = clientProtocolConfigObject.value(config_key::client_ip).toString();
        clientProtocolConfig.wireGuardData.clientPrivateKey = clientProtocolConfigObject.value(config_key::client_priv_key).toString();
        clientProtocolConfig.wireGuardData.clientPublicKey = clientProtocolConfigObject.value(config_key::client_pub_key).toString();
        clientProtocolConfig.wireGuardData.persistentKeepAlive =
                clientProtocolConfigObject.value(config_key::persistent_keep_alive).toString();
        clientProtocolConfig.wireGuardData.pskKey = clientProtocolConfigObject.value(config_key::psk_key).toString();
        clientProtocolConfig.wireGuardData.serverPubKey = clientProtocolConfigObject.value(config_key::server_pub_key).toString();
        clientProtocolConfig.wireGuardData.mtu = clientProtocolConfigObject.value(config_key::mtu).toString();

        clientProtocolConfig.hostname = clientProtocolConfigObject.value(config_key::hostName).toString();
        clientProtocolConfig.port = clientProtocolConfigObject.value(config_key::port).toInt(0);

        clientProtocolConfig.nativeConfig = clientProtocolConfigObject.value(config_key::config).toString();

        if (clientProtocolConfigObject.contains(config_key::allowed_ips)
            && clientProtocolConfigObject.value(config_key::allowed_ips).isArray()) {
            auto allowedIpsArray = clientProtocolConfigObject.value(config_key::allowed_ips).toArray();
            for (const auto &ip : allowedIpsArray) {
                clientProtocolConfig.wireGuardData.allowedIps.append(ip.toString());
            }
        }
    }
}

QJsonObject WireGuardProtocolConfig::toJson() const
{
    QJsonObject json;

    if (!serverProtocolConfig.port.isEmpty()) {
        json[config_key::port] = serverProtocolConfig.port;
    }
    if (!serverProtocolConfig.transportProto.isEmpty()) {
        json[config_key::transport_proto] = serverProtocolConfig.transportProto;
    }
    if (!serverProtocolConfig.subnetAddress.isEmpty()) {
        json[config_key::subnet_address] = serverProtocolConfig.subnetAddress;
    }

    if (!clientProtocolConfig.isEmpty) {
        QJsonObject clientConfigJson;

        if (!clientProtocolConfig.clientId.isEmpty()) {
            clientConfigJson[config_key::clientId] = clientProtocolConfig.clientId;
        }

        if (!clientProtocolConfig.wireGuardData.clientIp.isEmpty()) {
            clientConfigJson[config_key::client_ip] = clientProtocolConfig.wireGuardData.clientIp;
        }
        if (!clientProtocolConfig.wireGuardData.clientPrivateKey.isEmpty()) {
            clientConfigJson[config_key::client_priv_key] = clientProtocolConfig.wireGuardData.clientPrivateKey;
        }
        if (!clientProtocolConfig.wireGuardData.clientPublicKey.isEmpty()) {
            clientConfigJson[config_key::client_pub_key] = clientProtocolConfig.wireGuardData.clientPublicKey;
        }
        if (!clientProtocolConfig.wireGuardData.persistentKeepAlive.isEmpty()) {
            clientConfigJson[config_key::persistent_keep_alive] = clientProtocolConfig.wireGuardData.persistentKeepAlive;
        }
        if (!clientProtocolConfig.wireGuardData.pskKey.isEmpty()) {
            clientConfigJson[config_key::psk_key] = clientProtocolConfig.wireGuardData.pskKey;
        }
        if (!clientProtocolConfig.wireGuardData.serverPubKey.isEmpty()) {
            clientConfigJson[config_key::server_pub_key] = clientProtocolConfig.wireGuardData.serverPubKey;
        }
        if (!clientProtocolConfig.wireGuardData.mtu.isEmpty()) {
            clientConfigJson[config_key::mtu] = clientProtocolConfig.wireGuardData.mtu;
        }

        if (!clientProtocolConfig.wireGuardData.allowedIps.isEmpty()) {
            QJsonArray allowedIpsArray;
            for (const auto &ip : clientProtocolConfig.wireGuardData.allowedIps) {
                if (!ip.isEmpty()) {
                    allowedIpsArray.append(ip);
                }
            }
            if (!allowedIpsArray.isEmpty()) {
                clientConfigJson[config_key::allowed_ips] = allowedIpsArray;
            }
        }

        if (!clientProtocolConfig.hostname.isEmpty()) {
            clientConfigJson[config_key::hostName] = clientProtocolConfig.hostname;
        }
        if (clientProtocolConfig.port) {
            clientConfigJson[config_key::port] = clientProtocolConfig.port;
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
