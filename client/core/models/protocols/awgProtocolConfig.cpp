#include "awgProtocolConfig.h"

#include <QJsonDocument>
#include <QJsonArray>

#include "../../../core/utils/protocolEnum.h"
#include "../../../core/protocols/protocolUtils.h"
#include "../../../core/utils/constants/configKeys.h"
#include "../../../core/utils/constants/protocolConstants.h"

using namespace amnezia;
using namespace ProtocolUtils;
namespace amnezia
{

QJsonObject AwgServerConfig::toJson() const
{
    QJsonObject obj;
    
    if (!port.isEmpty()) {
        obj[configKey::port] = this->port;
    }
    if (!transportProto.isEmpty()) {
        obj[configKey::transportProto] = transportProto;
    }
    if (!protocolVersion.isEmpty()) {
        obj[configKey::protocolVersion] = protocolVersion;
    }
    if (!subnetAddress.isEmpty()) {
        obj[configKey::subnetAddress] = subnetAddress;
    }
    if (!subnetCidr.isEmpty()) {
        obj[configKey::subnetCidr] = subnetCidr;
    }
    
    if (!junkPacketCount.isEmpty()) {
        obj[configKey::junkPacketCount] = junkPacketCount;
    }
    if (!junkPacketMinSize.isEmpty()) {
        obj[configKey::junkPacketMinSize] = junkPacketMinSize;
    }
    if (!junkPacketMaxSize.isEmpty()) {
        obj[configKey::junkPacketMaxSize] = junkPacketMaxSize;
    }
    if (!initPacketJunkSize.isEmpty()) {
        obj[configKey::initPacketJunkSize] = initPacketJunkSize;
    }
    if (!responsePacketJunkSize.isEmpty()) {
        obj[configKey::responsePacketJunkSize] = responsePacketJunkSize;
    }
    if (!cookieReplyPacketJunkSize.isEmpty()) {
        obj[configKey::cookieReplyPacketJunkSize] = cookieReplyPacketJunkSize;
    }
    if (!transportPacketJunkSize.isEmpty()) {
        obj[configKey::transportPacketJunkSize] = transportPacketJunkSize;
    }
    
    if (!initPacketMagicHeader.isEmpty()) {
        obj[configKey::initPacketMagicHeader] = initPacketMagicHeader;
    }
    if (!responsePacketMagicHeader.isEmpty()) {
        obj[configKey::responsePacketMagicHeader] = responsePacketMagicHeader;
    }
    if (!underloadPacketMagicHeader.isEmpty()) {
        obj[configKey::underloadPacketMagicHeader] = underloadPacketMagicHeader;
    }
    if (!transportPacketMagicHeader.isEmpty()) {
        obj[configKey::transportPacketMagicHeader] = transportPacketMagicHeader;
    }
    
    obj[configKey::specialJunk1] = specialJunk1;
    obj[configKey::specialJunk2] = specialJunk2;
    obj[configKey::specialJunk3] = specialJunk3;
    obj[configKey::specialJunk4] = specialJunk4;
    obj[configKey::specialJunk5] = specialJunk5;
    
    if (isThirdPartyConfig) {
        obj[configKey::isThirdPartyConfig] = isThirdPartyConfig;
    }
    
    return obj;
}

AwgServerConfig AwgServerConfig::fromJson(const QJsonObject& json)
{
    AwgServerConfig config;
    
    config.port = json.value(configKey::port).toString();
    config.transportProto = json.value(configKey::transportProto).toString();
    config.protocolVersion = json.value(configKey::protocolVersion).toString();
    config.subnetAddress = json.value(configKey::subnetAddress).toString();
    config.subnetCidr = json.value(configKey::subnetCidr).toString();
    
    config.junkPacketCount = json.value(configKey::junkPacketCount).toString();
    config.junkPacketMinSize = json.value(configKey::junkPacketMinSize).toString();
    config.junkPacketMaxSize = json.value(configKey::junkPacketMaxSize).toString();
    config.initPacketJunkSize = json.value(configKey::initPacketJunkSize).toString();
    config.responsePacketJunkSize = json.value(configKey::responsePacketJunkSize).toString();
    config.cookieReplyPacketJunkSize = json.value(configKey::cookieReplyPacketJunkSize).toString();
    config.transportPacketJunkSize = json.value(configKey::transportPacketJunkSize).toString();
    
    config.initPacketMagicHeader = json.value(configKey::initPacketMagicHeader).toString();
    config.responsePacketMagicHeader = json.value(configKey::responsePacketMagicHeader).toString();
    config.underloadPacketMagicHeader = json.value(configKey::underloadPacketMagicHeader).toString();
    config.transportPacketMagicHeader = json.value(configKey::transportPacketMagicHeader).toString();
    
    config.specialJunk1 = json.value(configKey::specialJunk1).toString();
    config.specialJunk2 = json.value(configKey::specialJunk2).toString();
    config.specialJunk3 = json.value(configKey::specialJunk3).toString();
    config.specialJunk4 = json.value(configKey::specialJunk4).toString();
    config.specialJunk5 = json.value(configKey::specialJunk5).toString();
    
    config.isThirdPartyConfig = json.value(configKey::isThirdPartyConfig).toBool(false);
    
    return config;
}

QJsonObject AwgClientConfig::toJson() const
{
    QJsonObject obj;
    
    if (!nativeConfig.isEmpty()) {
        obj[configKey::config] = nativeConfig;
    }
    if (!hostName.isEmpty()) {
        obj[configKey::hostName] = hostName;
    }
    if (port > 0) {
        obj[configKey::port] = port;
    }
    if (!clientIp.isEmpty()) {
        obj[configKey::clientIp] = clientIp;
    }
    if (!clientPrivateKey.isEmpty()) {
        obj[configKey::clientPrivKey] = clientPrivateKey;
    }
    if (!clientPublicKey.isEmpty()) {
        obj[configKey::clientPubKey] = clientPublicKey;
    }
    if (!serverPublicKey.isEmpty()) {
        obj[configKey::serverPubKey] = serverPublicKey;
    }
    if (!presharedKey.isEmpty()) {
        obj[configKey::pskKey] = presharedKey;
    }
    if (!clientId.isEmpty()) {
        obj[configKey::clientId] = clientId;
    }
    
    if (!allowedIps.isEmpty()) {
        QJsonArray arr;
        for (const QString& ip : allowedIps) {
            arr.append(ip);
        }
        obj[configKey::allowedIps] = arr;
    }
    if (!persistentKeepAlive.isEmpty()) {
        obj[configKey::persistentKeepAlive] = persistentKeepAlive;
    }
    if (!mtu.isEmpty()) {
        obj[configKey::mtu] = mtu;
    }
    
    if (!junkPacketCount.isEmpty()) {
        obj[configKey::junkPacketCount] = junkPacketCount;
    }
    if (!junkPacketMinSize.isEmpty()) {
        obj[configKey::junkPacketMinSize] = junkPacketMinSize;
    }
    if (!junkPacketMaxSize.isEmpty()) {
        obj[configKey::junkPacketMaxSize] = junkPacketMaxSize;
    }
    if (!initPacketJunkSize.isEmpty()) {
        obj[configKey::initPacketJunkSize] = initPacketJunkSize;
    }
    if (!responsePacketJunkSize.isEmpty()) {
        obj[configKey::responsePacketJunkSize] = responsePacketJunkSize;
    }
    if (!cookieReplyPacketJunkSize.isEmpty()) {
        obj[configKey::cookieReplyPacketJunkSize] = cookieReplyPacketJunkSize;
    }
    if (!transportPacketJunkSize.isEmpty()) {
        obj[configKey::transportPacketJunkSize] = transportPacketJunkSize;
    }
    
    if (!initPacketMagicHeader.isEmpty()) {
        obj[configKey::initPacketMagicHeader] = initPacketMagicHeader;
    }
    if (!responsePacketMagicHeader.isEmpty()) {
        obj[configKey::responsePacketMagicHeader] = responsePacketMagicHeader;
    }
    if (!underloadPacketMagicHeader.isEmpty()) {
        obj[configKey::underloadPacketMagicHeader] = underloadPacketMagicHeader;
    }
    if (!transportPacketMagicHeader.isEmpty()) {
        obj[configKey::transportPacketMagicHeader] = transportPacketMagicHeader;
    }
    
    obj[configKey::specialJunk1] = specialJunk1;
    obj[configKey::specialJunk2] = specialJunk2;
    obj[configKey::specialJunk3] = specialJunk3;
    obj[configKey::specialJunk4] = specialJunk4;
    obj[configKey::specialJunk5] = specialJunk5;
    
    if (isObfuscationEnabled) {
        obj[configKey::isObfuscationEnabled] = isObfuscationEnabled;
    }
    
    return obj;
}

AwgClientConfig AwgClientConfig::fromJson(const QJsonObject& json)
{
    AwgClientConfig config;
    
    config.nativeConfig = json.value(configKey::config).toString();
    config.hostName = json.value(configKey::hostName).toString();
    config.port = json.value(configKey::port).toInt(0);
    config.clientIp = json.value(configKey::clientIp).toString();
    config.clientPrivateKey = json.value(configKey::clientPrivKey).toString();
    config.clientPublicKey = json.value(configKey::clientPubKey).toString();
    config.serverPublicKey = json.value(configKey::serverPubKey).toString();
    config.presharedKey = json.value(configKey::pskKey).toString();
    config.clientId = json.value(configKey::clientId).toString();
    
    QJsonArray allowedIpsArr = json.value(configKey::allowedIps).toArray();
    for (const QJsonValue& val : allowedIpsArr) {
        config.allowedIps.append(val.toString());
    }
    config.persistentKeepAlive = json.value(configKey::persistentKeepAlive).toString();
    config.mtu = json.value(configKey::mtu).toString();
    
    config.junkPacketCount = json.value(configKey::junkPacketCount).toString();
    config.junkPacketMinSize = json.value(configKey::junkPacketMinSize).toString();
    config.junkPacketMaxSize = json.value(configKey::junkPacketMaxSize).toString();
    config.initPacketJunkSize = json.value(configKey::initPacketJunkSize).toString();
    config.responsePacketJunkSize = json.value(configKey::responsePacketJunkSize).toString();
    config.cookieReplyPacketJunkSize = json.value(configKey::cookieReplyPacketJunkSize).toString();
    config.transportPacketJunkSize = json.value(configKey::transportPacketJunkSize).toString();
    
    config.initPacketMagicHeader = json.value(configKey::initPacketMagicHeader).toString();
    config.responsePacketMagicHeader = json.value(configKey::responsePacketMagicHeader).toString();
    config.underloadPacketMagicHeader = json.value(configKey::underloadPacketMagicHeader).toString();
    config.transportPacketMagicHeader = json.value(configKey::transportPacketMagicHeader).toString();
    
    config.specialJunk1 = json.value(configKey::specialJunk1).toString();
    config.specialJunk2 = json.value(configKey::specialJunk2).toString();
    config.specialJunk3 = json.value(configKey::specialJunk3).toString();
    config.specialJunk4 = json.value(configKey::specialJunk4).toString();
    config.specialJunk5 = json.value(configKey::specialJunk5).toString();
    
    config.isObfuscationEnabled = json.value(configKey::isObfuscationEnabled).toBool(false);
    
    return config;
}

QJsonObject AwgProtocolConfig::toJson() const
{
    QJsonObject obj = serverConfig.toJson();
    
    if (clientConfig.has_value()) {
        QJsonObject clientJson = clientConfig->toJson();
        obj[configKey::lastConfig] = QString::fromUtf8(QJsonDocument(clientJson).toJson(QJsonDocument::Compact));
    }
    
    return obj;
}

AwgProtocolConfig AwgProtocolConfig::fromJson(const QJsonObject& json)
{
    AwgProtocolConfig config;
    
    config.serverConfig = AwgServerConfig::fromJson(json);
    
    QString lastConfigStr = json.value(configKey::lastConfig).toString();
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

bool AwgServerConfig::hasEqualServerSettings(const AwgServerConfig& other) const
{
    if (subnetAddress != other.subnetAddress || port != other.port || 
        junkPacketCount != other.junkPacketCount ||
        junkPacketMinSize != other.junkPacketMinSize || junkPacketMaxSize != other.junkPacketMaxSize ||
        initPacketJunkSize != other.initPacketJunkSize || responsePacketJunkSize != other.responsePacketJunkSize ||
        initPacketMagicHeader != other.initPacketMagicHeader ||
        responsePacketMagicHeader != other.responsePacketMagicHeader ||
        underloadPacketMagicHeader != other.underloadPacketMagicHeader ||
        transportPacketMagicHeader != other.transportPacketMagicHeader ||
        specialJunk1 != other.specialJunk1 || specialJunk2 != other.specialJunk2 ||
        specialJunk3 != other.specialJunk3 || specialJunk4 != other.specialJunk4 ||
        specialJunk5 != other.specialJunk5) {
        return false;
    }

    bool isV2 = protocolVersion == protocols::awg::awgV2;
    if (isV2) {
        if (cookieReplyPacketJunkSize != other.cookieReplyPacketJunkSize ||
            transportPacketJunkSize != other.transportPacketJunkSize) {
            return false;
        }
    }

    return true;
}

bool AwgProtocolConfig::isHeadersEqual(const QString &h1, const QString &h2, const QString &h3, const QString &h4)
{
    return (h1 == h2) || (h1 == h3) || (h1 == h4) || (h2 == h3) || (h2 == h4) || (h3 == h4);
}

bool AwgProtocolConfig::isPacketSizeEqual(int s1, int s2, int s3, int s4)
{
    int initSize = AwgConstant::messageInitiationSize + s1;
    int responseSize = AwgConstant::messageResponseSize + s2;
    int cookieSize = AwgConstant::messageCookieReplySize + s3;
    int transportSize = AwgConstant::messageTransportSize + s4;

    return (initSize == responseSize || initSize == cookieSize || initSize == transportSize || responseSize == cookieSize
            || responseSize == transportSize || cookieSize == transportSize);
}

} // namespace amnezia

