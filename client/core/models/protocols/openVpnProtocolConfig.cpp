#include "openVpnProtocolConfig.h"

#include <QJsonDocument>

#include "../../../core/utils/protocolEnum.h"
#include "../../../core/protocols/protocolUtils.h"
#include "../../../core/utils/constants/configKeys.h"
#include "../../../core/utils/constants/protocolConstants.h"

using namespace amnezia;
using namespace ProtocolUtils;
namespace amnezia
{

QJsonObject OpenVpnServerConfig::toJson() const
{
    QJsonObject obj;
    
    if (!port.isEmpty()) {
        obj[configKey::port] = port;
    }
    if (!transportProto.isEmpty()) {
        obj[configKey::transportProto] = transportProto;
    }
    if (!subnetAddress.isEmpty()) {
        obj[configKey::subnetAddress] = subnetAddress;
    }
    if (!subnetMask.isEmpty()) {
        obj[configKey::subnetMask] = subnetMask;
    }
    if (!subnetCidr.isEmpty()) {
        obj[configKey::subnetCidr] = subnetCidr;
    }
    if (!cipher.isEmpty()) {
        obj[configKey::cipher] = cipher;
    }
    if (!hash.isEmpty()) {
        obj[configKey::hash] = hash;
    }
    if (ncpDisable != false) {
        obj[configKey::ncpDisable] = ncpDisable;
    }
    if (tlsAuth != true) {
        obj[configKey::tlsAuth] = tlsAuth;
    }
    if (!additionalClientConfig.isEmpty()) {
        obj[configKey::additionalClientConfig] = additionalClientConfig;
    }
    if (!additionalServerConfig.isEmpty()) {
        obj[configKey::additionalServerConfig] = additionalServerConfig;
    }
    
    if (isThirdPartyConfig) {
        obj[configKey::isThirdPartyConfig] = isThirdPartyConfig;
    }
    
    return obj;
}

OpenVpnServerConfig OpenVpnServerConfig::fromJson(const QJsonObject& json)
{
    OpenVpnServerConfig config;
    
    config.port = json.value(configKey::port).toString();
    config.transportProto = json.value(configKey::transportProto).toString();
    config.subnetAddress = json.value(configKey::subnetAddress).toString();
    config.subnetMask = json.value(configKey::subnetMask).toString();
    config.subnetCidr = json.value(configKey::subnetCidr).toString();
    config.cipher = json.value(configKey::cipher).toString();
    config.hash = json.value(configKey::hash).toString();
    config.ncpDisable = json.value(configKey::ncpDisable).toBool(false);
    config.tlsAuth = json.value(configKey::tlsAuth).toBool(true);
    config.additionalClientConfig = json.value(configKey::additionalClientConfig).toString();
    config.additionalServerConfig = json.value(configKey::additionalServerConfig).toString();
    
    config.isThirdPartyConfig = json.value(configKey::isThirdPartyConfig).toBool(false);
    
    return config;
}

bool OpenVpnServerConfig::hasEqualServerSettings(const OpenVpnServerConfig& other) const
{
    return subnetAddress == other.subnetAddress && 
           port == other.port && 
           transportProto == other.transportProto &&
           cipher == other.cipher &&
           hash == other.hash &&
           ncpDisable == other.ncpDisable &&
           tlsAuth == other.tlsAuth &&
           additionalServerConfig == other.additionalServerConfig;
}

QJsonObject OpenVpnClientConfig::toJson() const
{
    QJsonObject obj;
    
    if (!nativeConfig.isEmpty()) {
        obj[configKey::config] = nativeConfig;
    }
    if (!clientId.isEmpty()) {
        obj[configKey::clientId] = clientId;
    }
    obj[configKey::blockOutsideDns] = blockOutsideDns;
    
    return obj;
}

OpenVpnClientConfig OpenVpnClientConfig::fromJson(const QJsonObject& json)
{
    OpenVpnClientConfig config;
    
    config.nativeConfig = json.value(configKey::config).toString();
    config.clientId = json.value(configKey::clientId).toString();
    config.blockOutsideDns = json.value(configKey::blockOutsideDns).toBool(false);
    
    return config;
}

QJsonObject OpenVpnProtocolConfig::toJson() const
{
    QJsonObject obj = serverConfig.toJson();
    
    if (clientConfig.has_value()) {
        QJsonObject clientJson = clientConfig->toJson();
        obj[configKey::lastConfig] = QString::fromUtf8(QJsonDocument(clientJson).toJson(QJsonDocument::Compact));
    }
    
    return obj;
}

OpenVpnProtocolConfig OpenVpnProtocolConfig::fromJson(const QJsonObject& json)
{
    OpenVpnProtocolConfig config;
    
    config.serverConfig = OpenVpnServerConfig::fromJson(json);
    
    QString lastConfigStr = json.value(configKey::lastConfig).toString();
    if (!lastConfigStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(lastConfigStr.toUtf8());
        if (doc.isObject()) {
            config.clientConfig = OpenVpnClientConfig::fromJson(doc.object());
        }
    }
    
    return config;
}

bool OpenVpnProtocolConfig::hasClientConfig() const
{
    return clientConfig.has_value();
}

void OpenVpnProtocolConfig::setClientConfig(const OpenVpnClientConfig& config)
{
    clientConfig = config;
}

void OpenVpnProtocolConfig::clearClientConfig()
{
    clientConfig.reset();
}

} // namespace amnezia

