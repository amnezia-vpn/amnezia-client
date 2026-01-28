#include "openVpnProtocolConfig.h"

#include <QJsonDocument>

#include "../../../core/utils/protocolEnum.h"
#include "../../../core/protocols/protocolUtils.h"
#include "../../../core/utils/constants/configKeys.h"
#include "../../../core/utils/constants/protocolConstants.h"

using namespace amnezia;
using namespace ProtocolUtils;
using namespace config_key;

namespace amnezia
{

QJsonObject OpenVpnServerConfig::toJson() const
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
    if (!cipher.isEmpty()) {
        obj[config_key::cipher] = cipher;
    }
    if (!hash.isEmpty()) {
        obj[config_key::hash] = hash;
    }
    if (ncpDisable != false) {
        obj[config_key::ncp_disable] = ncpDisable;
    }
    if (tlsAuth != true) {
        obj[config_key::tls_auth] = tlsAuth;
    }
    if (!additionalClientConfig.isEmpty()) {
        obj[config_key::additional_client_config] = additionalClientConfig;
    }
    if (!additionalServerConfig.isEmpty()) {
        obj[config_key::additional_server_config] = additionalServerConfig;
    }
    
    if (isThirdPartyConfig) {
        obj[config_key::isThirdPartyConfig] = isThirdPartyConfig;
    }
    
    return obj;
}

OpenVpnServerConfig OpenVpnServerConfig::fromJson(const QJsonObject& json)
{
    OpenVpnServerConfig config;
    
    config.port = json.value(config_key::port).toString();
    config.transportProto = json.value(config_key::transport_proto).toString();
    config.subnetAddress = json.value(config_key::subnet_address).toString();
    config.subnetMask = json.value(config_key::subnet_mask).toString();
    config.subnetCidr = json.value(config_key::subnet_cidr).toString();
    config.cipher = json.value(config_key::cipher).toString();
    config.hash = json.value(config_key::hash).toString();
    config.ncpDisable = json.value(config_key::ncp_disable).toBool(false);
    config.tlsAuth = json.value(config_key::tls_auth).toBool(true);
    config.additionalClientConfig = json.value(config_key::additional_client_config).toString();
    config.additionalServerConfig = json.value(config_key::additional_server_config).toString();
    
    config.isThirdPartyConfig = json.value(config_key::isThirdPartyConfig).toBool(false);
    
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
        obj[config_key::config] = nativeConfig;
    }
    if (!clientId.isEmpty()) {
        obj[config_key::clientId] = clientId;
    }
    obj[config_key::block_outside_dns] = blockOutsideDns;
    
    return obj;
}

OpenVpnClientConfig OpenVpnClientConfig::fromJson(const QJsonObject& json)
{
    OpenVpnClientConfig config;
    
    config.nativeConfig = json.value(config_key::config).toString();
    config.clientId = json.value(config_key::clientId).toString();
    config.blockOutsideDns = json.value(config_key::block_outside_dns).toBool(false);
    
    return config;
}

QJsonObject OpenVpnProtocolConfig::toJson() const
{
    QJsonObject obj = serverConfig.toJson();
    
    if (clientConfig.has_value()) {
        QJsonObject clientJson = clientConfig->toJson();
        obj[config_key::last_config] = QString::fromUtf8(QJsonDocument(clientJson).toJson(QJsonDocument::Compact));
    }
    
    return obj;
}

OpenVpnProtocolConfig OpenVpnProtocolConfig::fromJson(const QJsonObject& json)
{
    OpenVpnProtocolConfig config;
    
    config.serverConfig = OpenVpnServerConfig::fromJson(json);
    
    QString lastConfigStr = json.value(config_key::last_config).toString();
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

