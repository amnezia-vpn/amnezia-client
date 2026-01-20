#include "awgProtocolConfig.h"

#include <QJsonDocument>
#include <QJsonArray>

#include "../../../core/protocols/protocolsDefs.h"

namespace amnezia
{

QJsonObject AwgServerConfig::toJson() const
{
    QJsonObject obj;
    
    if (!port.isEmpty()) {
        obj[config_key::port] = this->port;
    }
    if (!transportProto.isEmpty()) {
        obj[config_key::transport_proto] = transportProto;
    }
    if (!protocolVersion.isEmpty()) {
        obj[config_key::protocolVersion] = protocolVersion;
    }
    if (!subnetAddress.isEmpty()) {
        obj[config_key::subnet_address] = subnetAddress;
    }
    
    if (!junkPacketCount.isEmpty()) {
        obj[config_key::junkPacketCount] = junkPacketCount;
    }
    if (!junkPacketMinSize.isEmpty()) {
        obj[config_key::junkPacketMinSize] = junkPacketMinSize;
    }
    if (!junkPacketMaxSize.isEmpty()) {
        obj[config_key::junkPacketMaxSize] = junkPacketMaxSize;
    }
    if (!initPacketJunkSize.isEmpty()) {
        obj[config_key::initPacketJunkSize] = initPacketJunkSize;
    }
    if (!responsePacketJunkSize.isEmpty()) {
        obj[config_key::responsePacketJunkSize] = responsePacketJunkSize;
    }
    if (!cookieReplyPacketJunkSize.isEmpty()) {
        obj[config_key::cookieReplyPacketJunkSize] = cookieReplyPacketJunkSize;
    }
    if (!transportPacketJunkSize.isEmpty()) {
        obj[config_key::transportPacketJunkSize] = transportPacketJunkSize;
    }
    
    if (!initPacketMagicHeader.isEmpty()) {
        obj[config_key::initPacketMagicHeader] = initPacketMagicHeader;
    }
    if (!responsePacketMagicHeader.isEmpty()) {
        obj[config_key::responsePacketMagicHeader] = responsePacketMagicHeader;
    }
    if (!underloadPacketMagicHeader.isEmpty()) {
        obj[config_key::underloadPacketMagicHeader] = underloadPacketMagicHeader;
    }
    if (!transportPacketMagicHeader.isEmpty()) {
        obj[config_key::transportPacketMagicHeader] = transportPacketMagicHeader;
    }
    
    obj[config_key::specialJunk1] = specialJunk1;
    obj[config_key::specialJunk2] = specialJunk2;
    obj[config_key::specialJunk3] = specialJunk3;
    obj[config_key::specialJunk4] = specialJunk4;
    obj[config_key::specialJunk5] = specialJunk5;
    
    if (isThirdPartyConfig) {
        obj[config_key::isThirdPartyConfig] = isThirdPartyConfig;
    }
    
    return obj;
}

AwgServerConfig AwgServerConfig::fromJson(const QJsonObject& json)
{
    AwgServerConfig config;
    
    config.port = json.value(config_key::port).toString();
    config.transportProto = json.value(config_key::transport_proto).toString();
    config.protocolVersion = json.value(config_key::protocolVersion).toString();
    config.subnetAddress = json.value(config_key::subnet_address).toString();
    
    config.junkPacketCount = json.value(config_key::junkPacketCount).toString();
    config.junkPacketMinSize = json.value(config_key::junkPacketMinSize).toString();
    config.junkPacketMaxSize = json.value(config_key::junkPacketMaxSize).toString();
    config.initPacketJunkSize = json.value(config_key::initPacketJunkSize).toString();
    config.responsePacketJunkSize = json.value(config_key::responsePacketJunkSize).toString();
    config.cookieReplyPacketJunkSize = json.value(config_key::cookieReplyPacketJunkSize).toString();
    config.transportPacketJunkSize = json.value(config_key::transportPacketJunkSize).toString();
    
    config.initPacketMagicHeader = json.value(config_key::initPacketMagicHeader).toString();
    config.responsePacketMagicHeader = json.value(config_key::responsePacketMagicHeader).toString();
    config.underloadPacketMagicHeader = json.value(config_key::underloadPacketMagicHeader).toString();
    config.transportPacketMagicHeader = json.value(config_key::transportPacketMagicHeader).toString();
    
    config.specialJunk1 = json.value(config_key::specialJunk1).toString();
    config.specialJunk2 = json.value(config_key::specialJunk2).toString();
    config.specialJunk3 = json.value(config_key::specialJunk3).toString();
    config.specialJunk4 = json.value(config_key::specialJunk4).toString();
    config.specialJunk5 = json.value(config_key::specialJunk5).toString();
    
    config.isThirdPartyConfig = json.value(config_key::isThirdPartyConfig).toBool(false);
    
    return config;
}

QJsonObject AwgClientConfig::toJson() const
{
    QJsonObject obj;
    
    if (!nativeConfig.isEmpty()) {
        obj[config_key::config] = nativeConfig;
    }
    if (!hostName.isEmpty()) {
        obj[config_key::hostName] = hostName;
    }
    if (port > 0) {
        obj[config_key::port] = port;
    }
    if (!clientIp.isEmpty()) {
        obj[config_key::client_ip] = clientIp;
    }
    if (!clientPrivateKey.isEmpty()) {
        obj[config_key::client_priv_key] = clientPrivateKey;
    }
    if (!clientPublicKey.isEmpty()) {
        obj[config_key::client_pub_key] = clientPublicKey;
    }
    if (!serverPublicKey.isEmpty()) {
        obj[config_key::server_pub_key] = serverPublicKey;
    }
    if (!presharedKey.isEmpty()) {
        obj[config_key::psk_key] = presharedKey;
    }
    if (!clientId.isEmpty()) {
        obj[config_key::clientId] = clientId;
    }
    
    if (!allowedIps.isEmpty()) {
        QJsonArray arr;
        for (const QString& ip : allowedIps) {
            arr.append(ip);
        }
        obj[config_key::allowed_ips] = arr;
    }
    if (!persistentKeepAlive.isEmpty()) {
        obj[config_key::persistent_keep_alive] = persistentKeepAlive;
    }
    if (!mtu.isEmpty()) {
        obj[config_key::mtu] = mtu;
    }
    
    if (!junkPacketCount.isEmpty()) {
        obj[config_key::junkPacketCount] = junkPacketCount;
    }
    if (!junkPacketMinSize.isEmpty()) {
        obj[config_key::junkPacketMinSize] = junkPacketMinSize;
    }
    if (!junkPacketMaxSize.isEmpty()) {
        obj[config_key::junkPacketMaxSize] = junkPacketMaxSize;
    }
    if (!initPacketJunkSize.isEmpty()) {
        obj[config_key::initPacketJunkSize] = initPacketJunkSize;
    }
    if (!responsePacketJunkSize.isEmpty()) {
        obj[config_key::responsePacketJunkSize] = responsePacketJunkSize;
    }
    if (!cookieReplyPacketJunkSize.isEmpty()) {
        obj[config_key::cookieReplyPacketJunkSize] = cookieReplyPacketJunkSize;
    }
    if (!transportPacketJunkSize.isEmpty()) {
        obj[config_key::transportPacketJunkSize] = transportPacketJunkSize;
    }
    
    if (!initPacketMagicHeader.isEmpty()) {
        obj[config_key::initPacketMagicHeader] = initPacketMagicHeader;
    }
    if (!responsePacketMagicHeader.isEmpty()) {
        obj[config_key::responsePacketMagicHeader] = responsePacketMagicHeader;
    }
    if (!underloadPacketMagicHeader.isEmpty()) {
        obj[config_key::underloadPacketMagicHeader] = underloadPacketMagicHeader;
    }
    if (!transportPacketMagicHeader.isEmpty()) {
        obj[config_key::transportPacketMagicHeader] = transportPacketMagicHeader;
    }
    
    obj[config_key::specialJunk1] = specialJunk1;
    obj[config_key::specialJunk2] = specialJunk2;
    obj[config_key::specialJunk3] = specialJunk3;
    obj[config_key::specialJunk4] = specialJunk4;
    obj[config_key::specialJunk5] = specialJunk5;
    
    if (isObfuscationEnabled) {
        obj[config_key::isObfuscationEnabled] = isObfuscationEnabled;
    }
    
    return obj;
}

AwgClientConfig AwgClientConfig::fromJson(const QJsonObject& json)
{
    AwgClientConfig config;
    
    config.nativeConfig = json.value(config_key::config).toString();
    config.hostName = json.value(config_key::hostName).toString();
    config.port = json.value(config_key::port).toInt(0);
    config.clientIp = json.value(config_key::client_ip).toString();
    config.clientPrivateKey = json.value(config_key::client_priv_key).toString();
    config.clientPublicKey = json.value(config_key::client_pub_key).toString();
    config.serverPublicKey = json.value(config_key::server_pub_key).toString();
    config.presharedKey = json.value(config_key::psk_key).toString();
    config.clientId = json.value(config_key::clientId).toString();
    
    QJsonArray allowedIpsArr = json.value(config_key::allowed_ips).toArray();
    for (const QJsonValue& val : allowedIpsArr) {
        config.allowedIps.append(val.toString());
    }
    config.persistentKeepAlive = json.value(config_key::persistent_keep_alive).toString();
    config.mtu = json.value(config_key::mtu).toString();
    
    config.junkPacketCount = json.value(config_key::junkPacketCount).toString();
    config.junkPacketMinSize = json.value(config_key::junkPacketMinSize).toString();
    config.junkPacketMaxSize = json.value(config_key::junkPacketMaxSize).toString();
    config.initPacketJunkSize = json.value(config_key::initPacketJunkSize).toString();
    config.responsePacketJunkSize = json.value(config_key::responsePacketJunkSize).toString();
    config.cookieReplyPacketJunkSize = json.value(config_key::cookieReplyPacketJunkSize).toString();
    config.transportPacketJunkSize = json.value(config_key::transportPacketJunkSize).toString();
    
    config.initPacketMagicHeader = json.value(config_key::initPacketMagicHeader).toString();
    config.responsePacketMagicHeader = json.value(config_key::responsePacketMagicHeader).toString();
    config.underloadPacketMagicHeader = json.value(config_key::underloadPacketMagicHeader).toString();
    config.transportPacketMagicHeader = json.value(config_key::transportPacketMagicHeader).toString();
    
    config.specialJunk1 = json.value(config_key::specialJunk1).toString();
    config.specialJunk2 = json.value(config_key::specialJunk2).toString();
    config.specialJunk3 = json.value(config_key::specialJunk3).toString();
    config.specialJunk4 = json.value(config_key::specialJunk4).toString();
    config.specialJunk5 = json.value(config_key::specialJunk5).toString();
    
    config.isObfuscationEnabled = json.value(config_key::isObfuscationEnabled).toBool(false);
    
    return config;
}

QJsonObject AwgProtocolConfig::toJson() const
{
    QJsonObject obj = serverConfig.toJson();
    
    if (clientConfig.has_value()) {
        QJsonObject clientJson = clientConfig->toJson();
        obj[config_key::last_config] = QString::fromUtf8(QJsonDocument(clientJson).toJson(QJsonDocument::Compact));
    }
    
    return obj;
}

AwgProtocolConfig AwgProtocolConfig::fromJson(const QJsonObject& json)
{
    AwgProtocolConfig config;
    
    config.serverConfig = AwgServerConfig::fromJson(json);
    
    QString lastConfigStr = json.value(config_key::last_config).toString();
    if (!lastConfigStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(lastConfigStr.toUtf8());
        if (doc.isObject()) {
            config.clientConfig = AwgClientConfig::fromJson(doc.object());
        }
    }
    
    return config;
}

bool AwgProtocolConfig::hasClientConfig() const
{
    return clientConfig.has_value();
}

void AwgProtocolConfig::setClientConfig(const AwgClientConfig& config)
{
    clientConfig = config;
}

void AwgProtocolConfig::clearClientConfig()
{
    clientConfig.reset();
}

} // namespace amnezia

