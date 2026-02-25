#include "ikev2ProtocolConfig.h"

#include <QJsonDocument>

#include "../../../core/utils/protocolEnum.h"
#include "../../../core/protocols/protocolUtils.h"
#include "../../../core/utils/constants/configKeys.h"
#include "../../../core/utils/constants/protocolConstants.h"

using namespace amnezia;
using namespace ProtocolUtils;
namespace amnezia
{

QJsonObject Ikev2ServerConfig::toJson() const
{
    QJsonObject obj;
    
    if (!hostName.isEmpty()) {
        obj[configKey::hostName] = hostName;
    }
    
    if (isThirdPartyConfig) {
        obj[configKey::isThirdPartyConfig] = isThirdPartyConfig;
    }
    
    return obj;
}

Ikev2ServerConfig Ikev2ServerConfig::fromJson(const QJsonObject& json)
{
    Ikev2ServerConfig config;
    
    config.hostName = json.value(configKey::hostName).toString();
    config.isThirdPartyConfig = json.value(configKey::isThirdPartyConfig).toBool(false);
    
    return config;
}

QJsonObject Ikev2ClientConfig::toJson() const
{
    QJsonObject obj;
    
    if (!nativeConfig.isEmpty()) {
        obj[configKey::config] = nativeConfig;
    }
    if (!hostName.isEmpty()) {
        obj[configKey::hostName] = hostName;
    }
    if (!userName.isEmpty()) {
        obj[configKey::userName] = userName;
    }
    if (!cert.isEmpty()) {
        obj[configKey::cert] = cert;
    }
    if (!password.isEmpty()) {
        obj[configKey::password] = password;
    }
    if (!clientId.isEmpty()) {
        obj[configKey::clientId] = clientId;
    }
    
    return obj;
}

Ikev2ClientConfig Ikev2ClientConfig::fromJson(const QJsonObject& json)
{
    Ikev2ClientConfig config;
    
    config.nativeConfig = json.value(configKey::config).toString();
    config.hostName = json.value(configKey::hostName).toString();
    config.userName = json.value(configKey::userName).toString();
    config.cert = json.value(configKey::cert).toString();
    config.password = json.value(configKey::password).toString();
    config.clientId = json.value(configKey::clientId).toString();
    
    return config;
}

QJsonObject Ikev2ProtocolConfig::toJson() const
{
    QJsonObject obj = serverConfig.toJson();
    
    if (clientConfig.has_value()) {
        QJsonObject clientJson = clientConfig->toJson();
        obj[configKey::lastConfig] = QString::fromUtf8(QJsonDocument(clientJson).toJson(QJsonDocument::Compact));
    }
    
    return obj;
}

Ikev2ProtocolConfig Ikev2ProtocolConfig::fromJson(const QJsonObject& json)
{
    Ikev2ProtocolConfig config;
    
    config.serverConfig = Ikev2ServerConfig::fromJson(json);
    
    QString lastConfigStr = json.value(configKey::lastConfig).toString();
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

