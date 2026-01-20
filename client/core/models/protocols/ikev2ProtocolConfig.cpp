#include "ikev2ProtocolConfig.h"

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

QJsonObject Ikev2ServerConfig::toJson() const
{
    QJsonObject obj;
    
    if (!hostName.isEmpty()) {
        obj[config_key::hostName] = hostName;
    }
    
    if (isThirdPartyConfig) {
        obj[config_key::isThirdPartyConfig] = isThirdPartyConfig;
    }
    
    return obj;
}

Ikev2ServerConfig Ikev2ServerConfig::fromJson(const QJsonObject& json)
{
    Ikev2ServerConfig config;
    
    config.hostName = json.value(config_key::hostName).toString();
    config.isThirdPartyConfig = json.value(config_key::isThirdPartyConfig).toBool(false);
    
    return config;
}

QJsonObject Ikev2ClientConfig::toJson() const
{
    QJsonObject obj;
    
    if (!nativeConfig.isEmpty()) {
        obj[config_key::config] = nativeConfig;
    }
    if (!hostName.isEmpty()) {
        obj[config_key::hostName] = hostName;
    }
    if (!userName.isEmpty()) {
        obj[config_key::userName] = userName;
    }
    if (!cert.isEmpty()) {
        obj[config_key::cert] = cert;
    }
    if (!password.isEmpty()) {
        obj[config_key::password] = password;
    }
    if (!clientId.isEmpty()) {
        obj[config_key::clientId] = clientId;
    }
    
    return obj;
}

Ikev2ClientConfig Ikev2ClientConfig::fromJson(const QJsonObject& json)
{
    Ikev2ClientConfig config;
    
    config.nativeConfig = json.value(config_key::config).toString();
    config.hostName = json.value(config_key::hostName).toString();
    config.userName = json.value(config_key::userName).toString();
    config.cert = json.value(config_key::cert).toString();
    config.password = json.value(config_key::password).toString();
    config.clientId = json.value(config_key::clientId).toString();
    
    return config;
}

QJsonObject Ikev2ProtocolConfig::toJson() const
{
    QJsonObject obj = serverConfig.toJson();
    
    if (clientConfig.has_value()) {
        QJsonObject clientJson = clientConfig->toJson();
        obj[config_key::last_config] = QString::fromUtf8(QJsonDocument(clientJson).toJson(QJsonDocument::Compact));
    }
    
    return obj;
}

Ikev2ProtocolConfig Ikev2ProtocolConfig::fromJson(const QJsonObject& json)
{
    Ikev2ProtocolConfig config;
    
    config.serverConfig = Ikev2ServerConfig::fromJson(json);
    
    QString lastConfigStr = json.value(config_key::last_config).toString();
    if (!lastConfigStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(lastConfigStr.toUtf8());
        if (doc.isObject()) {
            config.clientConfig = Ikev2ClientConfig::fromJson(doc.object());
        }
    }
    
    return config;
}

bool Ikev2ProtocolConfig::hasClientConfig() const
{
    return clientConfig.has_value();
}

void Ikev2ProtocolConfig::setClientConfig(const Ikev2ClientConfig& config)
{
    clientConfig = config;
}

void Ikev2ProtocolConfig::clearClientConfig()
{
    clientConfig.reset();
}

} // namespace amnezia

