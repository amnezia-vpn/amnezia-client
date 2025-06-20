#include "awgProtocolConfig.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "protocols/protocols_defs.h"

using namespace amnezia;

AwgProtocolConfig::AwgProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.port = protocolConfigObject.value(config_key::port).toString();
    serverProtocolConfig.transportProto = protocolConfigObject.value(config_key::transport_proto).toString();
    serverProtocolConfig.subnetAddress = protocolConfigObject.value(config_key::subnet_address).toString();

    serverProtocolConfig.awgData.junkPacketCount = protocolConfigObject.value(config_key::junkPacketCount).toString();
    serverProtocolConfig.awgData.junkPacketMinSize = protocolConfigObject.value(config_key::junkPacketMinSize).toString();
    serverProtocolConfig.awgData.junkPacketMaxSize = protocolConfigObject.value(config_key::junkPacketMaxSize).toString();
    serverProtocolConfig.awgData.initPacketJunkSize = protocolConfigObject.value(config_key::initPacketJunkSize).toString();
    serverProtocolConfig.awgData.responsePacketJunkSize = protocolConfigObject.value(config_key::responsePacketJunkSize).toString();
    serverProtocolConfig.awgData.initPacketMagicHeader = protocolConfigObject.value(config_key::initPacketMagicHeader).toString();
    serverProtocolConfig.awgData.responsePacketMagicHeader = protocolConfigObject.value(config_key::responsePacketMagicHeader).toString();
    serverProtocolConfig.awgData.underloadPacketMagicHeader = protocolConfigObject.value(config_key::underloadPacketMagicHeader).toString();
    serverProtocolConfig.awgData.transportPacketMagicHeader = protocolConfigObject.value(config_key::transportPacketMagicHeader).toString();

    auto clientProtocolString = protocolConfigObject.value(config_key::last_config).toString();
    if (!clientProtocolString.isEmpty()) {
        clientProtocolConfig.isEmpty = false;

        QJsonObject clientProtocolConfigObject = QJsonDocument::fromJson(clientProtocolString.toUtf8()).object();

        clientProtocolConfig.awgData.junkPacketCount = clientProtocolConfigObject.value(config_key::junkPacketCount).toString();
        clientProtocolConfig.awgData.junkPacketMinSize = clientProtocolConfigObject.value(config_key::junkPacketMinSize).toString();
        clientProtocolConfig.awgData.junkPacketMaxSize = clientProtocolConfigObject.value(config_key::junkPacketMaxSize).toString();
        clientProtocolConfig.awgData.initPacketJunkSize = clientProtocolConfigObject.value(config_key::initPacketJunkSize).toString();
        clientProtocolConfig.awgData.responsePacketJunkSize = clientProtocolConfigObject.value(config_key::responsePacketJunkSize).toString();
        clientProtocolConfig.awgData.initPacketMagicHeader = clientProtocolConfigObject.value(config_key::initPacketMagicHeader).toString();
        clientProtocolConfig.awgData.responsePacketMagicHeader =
                clientProtocolConfigObject.value(config_key::responsePacketMagicHeader).toString();
        clientProtocolConfig.awgData.underloadPacketMagicHeader =
                clientProtocolConfigObject.value(config_key::underloadPacketMagicHeader).toString();
        clientProtocolConfig.awgData.transportPacketMagicHeader =
                clientProtocolConfigObject.value(config_key::transportPacketMagicHeader).toString();

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

QJsonObject AwgProtocolConfig::toJson() const
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

    if (!serverProtocolConfig.awgData.junkPacketCount.isEmpty()) {
        json[config_key::junkPacketCount] = serverProtocolConfig.awgData.junkPacketCount;
    }
    if (!serverProtocolConfig.awgData.junkPacketMinSize.isEmpty()) {
        json[config_key::junkPacketMinSize] = serverProtocolConfig.awgData.junkPacketMinSize;
    }
    if (!serverProtocolConfig.awgData.junkPacketMaxSize.isEmpty()) {
        json[config_key::junkPacketMaxSize] = serverProtocolConfig.awgData.junkPacketMaxSize;
    }
    if (!serverProtocolConfig.awgData.initPacketJunkSize.isEmpty()) {
        json[config_key::initPacketJunkSize] = serverProtocolConfig.awgData.initPacketJunkSize;
    }
    if (!serverProtocolConfig.awgData.responsePacketJunkSize.isEmpty()) {
        json[config_key::responsePacketJunkSize] = serverProtocolConfig.awgData.responsePacketJunkSize;
    }
    if (!serverProtocolConfig.awgData.initPacketMagicHeader.isEmpty()) {
        json[config_key::initPacketMagicHeader] = serverProtocolConfig.awgData.initPacketMagicHeader;
    }
    if (!serverProtocolConfig.awgData.responsePacketMagicHeader.isEmpty()) {
        json[config_key::responsePacketMagicHeader] = serverProtocolConfig.awgData.responsePacketMagicHeader;
    }
    if (!serverProtocolConfig.awgData.underloadPacketMagicHeader.isEmpty()) {
        json[config_key::underloadPacketMagicHeader] = serverProtocolConfig.awgData.underloadPacketMagicHeader;
    }
    if (!serverProtocolConfig.awgData.transportPacketMagicHeader.isEmpty()) {
        json[config_key::transportPacketMagicHeader] = serverProtocolConfig.awgData.transportPacketMagicHeader;
    }

    if (!clientProtocolConfig.isEmpty) {
        QJsonObject clientConfigJson;

        if (!clientProtocolConfig.clientId.isEmpty()) {
            clientConfigJson[config_key::clientId] = clientProtocolConfig.clientId;
        }

        if (!clientProtocolConfig.awgData.junkPacketCount.isEmpty()) {
            clientConfigJson[config_key::junkPacketCount] = clientProtocolConfig.awgData.junkPacketCount;
        }
        if (!clientProtocolConfig.awgData.junkPacketMinSize.isEmpty()) {
            clientConfigJson[config_key::junkPacketMinSize] = clientProtocolConfig.awgData.junkPacketMinSize;
        }
        if (!clientProtocolConfig.awgData.junkPacketMaxSize.isEmpty()) {
            clientConfigJson[config_key::junkPacketMaxSize] = clientProtocolConfig.awgData.junkPacketMaxSize;
        }
        if (!clientProtocolConfig.awgData.initPacketJunkSize.isEmpty()) {
            clientConfigJson[config_key::initPacketJunkSize] = clientProtocolConfig.awgData.initPacketJunkSize;
        }
        if (!clientProtocolConfig.awgData.responsePacketJunkSize.isEmpty()) {
            clientConfigJson[config_key::responsePacketJunkSize] = clientProtocolConfig.awgData.responsePacketJunkSize;
        }
        if (!clientProtocolConfig.awgData.initPacketMagicHeader.isEmpty()) {
            clientConfigJson[config_key::initPacketMagicHeader] = clientProtocolConfig.awgData.initPacketMagicHeader;
        }
        if (!clientProtocolConfig.awgData.responsePacketMagicHeader.isEmpty()) {
            clientConfigJson[config_key::responsePacketMagicHeader] = clientProtocolConfig.awgData.responsePacketMagicHeader;
        }
        if (!clientProtocolConfig.awgData.underloadPacketMagicHeader.isEmpty()) {
            clientConfigJson[config_key::underloadPacketMagicHeader] = clientProtocolConfig.awgData.underloadPacketMagicHeader;
        }
        if (!clientProtocolConfig.awgData.transportPacketMagicHeader.isEmpty()) {
            clientConfigJson[config_key::transportPacketMagicHeader] = clientProtocolConfig.awgData.transportPacketMagicHeader;
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
            json[config_key::last_config] = QString(QJsonDocument(clientConfigJson).toJson(QJsonDocument::Compact));
        }
    }

    return json;
}
