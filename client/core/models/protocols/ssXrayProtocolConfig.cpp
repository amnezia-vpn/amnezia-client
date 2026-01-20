#include "ssXrayProtocolConfig.h"

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

QJsonObject SSXrayServerConfig::toJson() const
{
    QJsonObject obj;
    
    if (!port.isEmpty()) {
        obj[config_key::port] = port;
    }
    if (!transportProto.isEmpty()) {
        obj[config_key::transport_proto] = transportProto;
    }
    if (!site.isEmpty()) {
        obj[config_key::site] = site;
    }
    
    if (isThirdPartyConfig) {
        obj[config_key::isThirdPartyConfig] = isThirdPartyConfig;
    }
    
    return obj;
}

SSXrayServerConfig SSXrayServerConfig::fromJson(const QJsonObject& json)
{
    SSXrayServerConfig config;
    
    config.port = json.value(config_key::port).toString();
    config.transportProto = json.value(config_key::transport_proto).toString();
    config.site = json.value(config_key::site).toString();
    
    config.isThirdPartyConfig = json.value(config_key::isThirdPartyConfig).toBool(false);
    
    return config;
}

QJsonObject SSXrayClientConfig::toJson() const
{
    QJsonObject obj;
    
    if (!nativeConfig.isEmpty()) {
        obj[config_key::config] = nativeConfig;
    }
    if (!localPort.isEmpty()) {
        obj[config_key::local_port] = localPort;
    }
    
    return obj;
}

SSXrayClientConfig SSXrayClientConfig::fromJson(const QJsonObject& json)
{
    SSXrayClientConfig config;
    
    config.nativeConfig = json.value(config_key::config).toString();
    config.localPort = json.value(config_key::local_port).toString();
    
    return config;
}

QJsonObject SSXrayProtocolConfig::toJson() const
{
    QJsonObject obj = serverConfig.toJson();
    
    if (clientConfig.has_value()) {
        QJsonObject clientJson = clientConfig->toJson();
        obj[config_key::last_config] = QString::fromUtf8(QJsonDocument(clientJson).toJson(QJsonDocument::Compact));
    }
    
    return obj;
}

SSXrayProtocolConfig SSXrayProtocolConfig::fromJson(const QJsonObject& json)
{
    SSXrayProtocolConfig config;
    
    config.serverConfig = SSXrayServerConfig::fromJson(json);
    
    QString lastConfigStr = json.value(config_key::last_config).toString();
    if (!lastConfigStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(lastConfigStr.toUtf8());
        if (doc.isObject()) {
            config.clientConfig = SSXrayClientConfig::fromJson(doc.object());
        }
    }
    
    return config;
}

bool SSXrayProtocolConfig::hasClientConfig() const
{
    return clientConfig.has_value();
}

void SSXrayProtocolConfig::setClientConfig(const SSXrayClientConfig& config)
{
    clientConfig = config;
}

void SSXrayProtocolConfig::clearClientConfig()
{
    clientConfig.reset();
}

} // namespace amnezia

