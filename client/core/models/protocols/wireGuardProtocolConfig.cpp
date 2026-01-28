#include "wireGuardProtocolConfig.h"

#include <QJsonDocument>
#include <QJsonArray>

#include "../../../core/utils/protocolEnum.h"
#include "../../../core/protocols/protocolUtils.h"
#include "../../../core/utils/constants/configKeys.h"
#include "../../../core/utils/constants/protocolConstants.h"

using namespace amnezia;
using namespace ProtocolUtils;
using namespace config_key;

namespace amnezia
{

QJsonObject WireGuardServerConfig::toJson() const
{
    QJsonObject obj;
    
    if (!port.isEmpty()) {
        obj[config_key::port] = port;
    }
    if (!transportProto.isEmpty()) {
        obj[config_key::transport_proto] = transportProto;
    }
    if (!subnetAddress.isEmpty()) {
        obj[config_key::subnet_address] = subnetAddress;
    }
    if (!subnetMask.isEmpty()) {
        obj[config_key::subnet_mask] = subnetMask;
    }
    if (!subnetCidr.isEmpty()) {
        obj[config_key::subnet_cidr] = subnetCidr;
    }
    
    if (isThirdPartyConfig) {
        obj[config_key::isThirdPartyConfig] = isThirdPartyConfig;
    }
    
    return obj;
}

WireGuardServerConfig WireGuardServerConfig::fromJson(const QJsonObject& json)
{
    WireGuardServerConfig config;
    
    config.port = json.value(config_key::port).toString();
    config.transportProto = json.value(config_key::transport_proto).toString();
    config.subnetAddress = json.value(config_key::subnet_address).toString();
    config.subnetMask = json.value(config_key::subnet_mask).toString();
    config.subnetCidr = json.value(config_key::subnet_cidr).toString();
    
    config.isThirdPartyConfig = json.value(config_key::isThirdPartyConfig).toBool(false);
    
    return config;
}

bool WireGuardServerConfig::hasEqualServerSettings(const WireGuardServerConfig& other) const
{
    return subnetAddress == other.subnetAddress && port == other.port;
}

QJsonObject WireGuardClientConfig::toJson() const
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
    
    if (isObfuscationEnabled) {
        obj[config_key::isObfuscationEnabled] = isObfuscationEnabled;
    }
    
    return obj;
}

WireGuardClientConfig WireGuardClientConfig::fromJson(const QJsonObject& json)
{
    WireGuardClientConfig config;
    
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
    
    config.isObfuscationEnabled = json.value(config_key::isObfuscationEnabled).toBool(false);
    
    return config;
}

QJsonObject WireGuardProtocolConfig::toJson() const
{
    QJsonObject obj = serverConfig.toJson();
    
    if (clientConfig.has_value()) {
        QJsonObject clientJson = clientConfig->toJson();
        obj[config_key::last_config] = QString::fromUtf8(QJsonDocument(clientJson).toJson(QJsonDocument::Compact));
    }
    
    return obj;
}

WireGuardProtocolConfig WireGuardProtocolConfig::fromJson(const QJsonObject& json)
{
    WireGuardProtocolConfig config;
    
    config.serverConfig = WireGuardServerConfig::fromJson(json);
    
    QString lastConfigStr = json.value(config_key::last_config).toString();
    if (!lastConfigStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(lastConfigStr.toUtf8());
        if (doc.isObject()) {
            config.clientConfig = WireGuardClientConfig::fromJson(doc.object());
        }
    }
    
    return config;
}

bool WireGuardProtocolConfig::hasClientConfig() const
{
    return clientConfig.has_value();
}

void WireGuardProtocolConfig::setClientConfig(const WireGuardClientConfig& config)
{
    clientConfig = config;
}

void WireGuardProtocolConfig::clearClientConfig()
{
    clientConfig.reset();
}

} // namespace amnezia

