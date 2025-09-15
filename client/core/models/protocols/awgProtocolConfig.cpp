#include "awgProtocolConfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QSet>
#include <limits>

#include "protocols/protocols_defs.h"

using namespace amnezia;

AwgProtocolConfig::AwgProtocolConfig(const QString &protocolName) : ProtocolConfig(protocolName)
{
}

AwgProtocolConfig::AwgProtocolConfig(const QString &protocolName, int port, TransportProto transportProto) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.port = QString::number(port);
    serverProtocolConfig.transportProto = ProtocolProps::transportProtoToString(transportProto, ProtocolProps::protoFromString(protocolName));

    // Generate random AWG configuration parameters
    serverProtocolConfig.awgData.junkPacketCount = QString::number(QRandomGenerator::global()->bounded(2, 5));
    serverProtocolConfig.awgData.junkPacketMinSize = QString::number(10);
    serverProtocolConfig.awgData.junkPacketMaxSize = QString::number(50);

    int s1 = QRandomGenerator::global()->bounded(15, 150);
    int s2 = QRandomGenerator::global()->bounded(15, 150);

    // Ensure all values are unique and don't create equal packet sizes
    QSet<int> usedValues;
    usedValues.insert(s1);

    while (usedValues.contains(s2) || s1 + awg::messageInitiationSize == s2 + awg::messageResponseSize) {
        s2 = QRandomGenerator::global()->bounded(15, 150);
    }
    usedValues.insert(s2);

    serverProtocolConfig.awgData.initPacketJunkSize = QString::number(s1);
    serverProtocolConfig.awgData.responsePacketJunkSize = QString::number(s2);

    // Generate unique magic headers
    QSet<QString> headersValue;
    while (headersValue.size() != 4) {
        auto max = (std::numeric_limits<qint32>::max)();
        headersValue.insert(QString::number(QRandomGenerator::global()->bounded(5, max)));
    }

    auto headersValueList = headersValue.values();
    serverProtocolConfig.awgData.initPacketMagicHeader = headersValueList.at(0);
    serverProtocolConfig.awgData.responsePacketMagicHeader = headersValueList.at(1);
    serverProtocolConfig.awgData.underloadPacketMagicHeader = headersValueList.at(2);
    serverProtocolConfig.awgData.transportPacketMagicHeader = headersValueList.at(3);
}

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

AwgProtocolConfig::AwgProtocolConfig(const AwgProtocolConfig &other) : ProtocolConfig(other.protocolName)
{
    serverProtocolConfig = other.serverProtocolConfig;
    clientProtocolConfig = other.clientProtocolConfig;
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

bool AwgProtocolConfig::hasEqualServerSettings(const AwgProtocolConfig &other) const
{
    if (serverProtocolConfig.subnetAddress != other.serverProtocolConfig.subnetAddress
        || serverProtocolConfig.port != other.serverProtocolConfig.port
        || serverProtocolConfig.awgData.junkPacketCount != other.serverProtocolConfig.awgData.junkPacketCount
        || serverProtocolConfig.awgData.junkPacketMinSize != other.serverProtocolConfig.awgData.junkPacketMinSize
        || serverProtocolConfig.awgData.junkPacketMaxSize != other.serverProtocolConfig.awgData.junkPacketMaxSize
        || serverProtocolConfig.awgData.initPacketJunkSize != other.serverProtocolConfig.awgData.initPacketJunkSize
        || serverProtocolConfig.awgData.responsePacketJunkSize != other.serverProtocolConfig.awgData.responsePacketJunkSize
        || serverProtocolConfig.awgData.initPacketMagicHeader != other.serverProtocolConfig.awgData.initPacketMagicHeader
        || serverProtocolConfig.awgData.responsePacketMagicHeader != other.serverProtocolConfig.awgData.responsePacketMagicHeader
        || serverProtocolConfig.awgData.underloadPacketMagicHeader != other.serverProtocolConfig.awgData.underloadPacketMagicHeader
        || serverProtocolConfig.awgData.transportPacketMagicHeader != other.serverProtocolConfig.awgData.transportPacketMagicHeader) {
        return false;
    }
    return true;
}

bool AwgProtocolConfig::hasEqualClientSettings(const AwgProtocolConfig &other) const
{
    if (clientProtocolConfig.wireGuardData.mtu != other.clientProtocolConfig.wireGuardData.mtu
        || clientProtocolConfig.awgData.junkPacketCount != other.clientProtocolConfig.awgData.junkPacketCount
        || clientProtocolConfig.awgData.junkPacketMinSize != other.clientProtocolConfig.awgData.junkPacketMinSize
        || clientProtocolConfig.awgData.junkPacketMaxSize != other.clientProtocolConfig.awgData.junkPacketMaxSize) {
        return false;
    }
    return true;
}

ScriptVars AwgProtocolConfig::getScriptVars() const
{
    ScriptVars vars;

    if (!serverProtocolConfig.port.isEmpty()) {
        vars.append({ "$AWG_SERVER_PORT", serverProtocolConfig.port });
    }
    if (!serverProtocolConfig.subnetAddress.isEmpty()) {
        vars.append({ "$AWG_SUBNET_IP", serverProtocolConfig.subnetAddress });
    }

    if (!serverProtocolConfig.awgData.junkPacketCount.isEmpty()) {
        vars.append({ "$JUNK_PACKET_COUNT", serverProtocolConfig.awgData.junkPacketCount });
    }
    if (!serverProtocolConfig.awgData.junkPacketMinSize.isEmpty()) {
        vars.append({ "$JUNK_PACKET_MIN_SIZE", serverProtocolConfig.awgData.junkPacketMinSize });
    }
    if (!serverProtocolConfig.awgData.junkPacketMaxSize.isEmpty()) {
        vars.append({ "$JUNK_PACKET_MAX_SIZE", serverProtocolConfig.awgData.junkPacketMaxSize });
    }
    if (!serverProtocolConfig.awgData.initPacketJunkSize.isEmpty()) {
        vars.append({ "$INIT_PACKET_JUNK_SIZE", serverProtocolConfig.awgData.initPacketJunkSize });
    }
    if (!serverProtocolConfig.awgData.responsePacketJunkSize.isEmpty()) {
        vars.append({ "$RESPONSE_PACKET_JUNK_SIZE", serverProtocolConfig.awgData.responsePacketJunkSize });
    }
    if (!serverProtocolConfig.awgData.initPacketMagicHeader.isEmpty()) {
        vars.append({ "$INIT_PACKET_MAGIC_HEADER", serverProtocolConfig.awgData.initPacketMagicHeader });
    }
    if (!serverProtocolConfig.awgData.responsePacketMagicHeader.isEmpty()) {
        vars.append({ "$RESPONSE_PACKET_MAGIC_HEADER", serverProtocolConfig.awgData.responsePacketMagicHeader });
    }
    if (!serverProtocolConfig.awgData.underloadPacketMagicHeader.isEmpty()) {
        vars.append({ "$UNDERLOAD_PACKET_MAGIC_HEADER", serverProtocolConfig.awgData.underloadPacketMagicHeader });
    }
    if (!serverProtocolConfig.awgData.transportPacketMagicHeader.isEmpty()) {
        vars.append({ "$TRANSPORT_PACKET_MAGIC_HEADER", serverProtocolConfig.awgData.transportPacketMagicHeader });
    }

    return vars;
}

void AwgProtocolConfig::clearClientSettings()
{
    clientProtocolConfig = awg::ClientProtocolConfig();
}
