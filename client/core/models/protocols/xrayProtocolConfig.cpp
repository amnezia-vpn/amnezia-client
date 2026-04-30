#include "xrayProtocolConfig.h"

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

QJsonObject XrayServerConfig::toJson() const
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
    if (!site.isEmpty()) {
        obj[configKey::site] = site;
    }
    
    if (isThirdPartyConfig) {
        obj[configKey::isThirdPartyConfig] = isThirdPartyConfig;
    }
    
    return obj;
}

XrayServerConfig XrayServerConfig::fromJson(const QJsonObject& json)
{
    XrayServerConfig config;
    
    config.port = json.value(configKey::port).toString();
    config.transportProto = json.value(configKey::transportProto).toString();
    config.subnetAddress = json.value(configKey::subnetAddress).toString();
    config.site = json.value(configKey::site).toString();
    
    config.isThirdPartyConfig = json.value(configKey::isThirdPartyConfig).toBool(false);
    
    return config;
}

bool XrayServerConfig::hasEqualServerSettings(const XrayServerConfig& other) const
{
    return port == other.port && site == other.site;
}

QJsonObject XrayClientConfig::toJson() const
{
    QJsonObject obj;
    
    if (!nativeConfig.isEmpty()) {
        obj[configKey::config] = nativeConfig;
    }
    if (!localPort.isEmpty()) {
        obj[configKey::localPort] = localPort;
    }
    if (!id.isEmpty()) {
        obj[configKey::clientId] = id;
    }
    
    return obj;
}

XrayClientConfig XrayClientConfig::fromJson(const QJsonObject& json)
{
    XrayClientConfig config;
    
    config.nativeConfig = json.value(configKey::config).toString();
    config.localPort = json.value(configKey::localPort).toString();
    config.id = json.value(configKey::clientId).toString();
    
    if (config.id.isEmpty() && !config.nativeConfig.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(config.nativeConfig.toUtf8());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject configObj = doc.object();
            if (configObj.contains(protocols::xray::outbounds)) {
                QJsonArray outbounds = configObj.value(protocols::xray::outbounds).toArray();
                if (!outbounds.isEmpty()) {
                    QJsonObject outbound = outbounds[0].toObject();
                    if (outbound.contains(protocols::xray::settings)) {
                        QJsonObject settings = outbound[protocols::xray::settings].toObject();
                        if (settings.contains(protocols::xray::vnext)) {
                            QJsonArray vnext = settings[protocols::xray::vnext].toArray();
                            if (!vnext.isEmpty()) {
                                QJsonObject vnextObj = vnext[0].toObject();
                                if (vnextObj.contains(protocols::xray::users)) {
                                    QJsonArray users = vnextObj[protocols::xray::users].toArray();
                                    if (!users.isEmpty()) {
                                        QJsonObject user = users[0].toObject();
                                        if (user.contains(protocols::xray::id)) {
                                            config.id = user[protocols::xray::id].toString();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    return config;
}

QJsonObject XrayProtocolConfig::toJson() const
{
    QJsonObject obj = serverConfig.toJson();
    
    if (clientConfig.has_value()) {
        // Third-party import: nativeConfig is raw Xray JSON (inbounds/outbounds)
        QJsonDocument doc = QJsonDocument::fromJson(clientConfig->nativeConfig.toUtf8());
        if (!doc.isNull() && doc.isObject() && doc.object().contains(protocols::xray::outbounds)
                && !doc.object().contains(configKey::config)) {
            obj[configKey::lastConfig] = clientConfig->nativeConfig;
        } else {
            QJsonObject clientJson = clientConfig->toJson();
            obj[configKey::lastConfig] = QString::fromUtf8(QJsonDocument(clientJson).toJson(QJsonDocument::Compact));
        }
    }
    
    return obj;
}

XrayProtocolConfig XrayProtocolConfig::fromJson(const QJsonObject& json)
{
    XrayProtocolConfig config;
    
    config.serverConfig = XrayServerConfig::fromJson(json);
    
    QString lastConfigStr = json.value(configKey::lastConfig).toString();
    if (!lastConfigStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(lastConfigStr.toUtf8());
        if (doc.isObject()) {
            QJsonObject parsed = doc.object();
            // Third-party import stores raw Xray config (inbounds/outbounds) directly
            if (parsed.contains(protocols::xray::outbounds) && !parsed.contains(configKey::config)) {
                XrayClientConfig clientCfg;
                clientCfg.nativeConfig = lastConfigStr;
                if (parsed.contains(protocols::xray::inbounds)) {
                    QJsonArray inbounds = parsed.value(protocols::xray::inbounds).toArray();
                    if (!inbounds.isEmpty()) {
                        QJsonObject inbound = inbounds[0].toObject();
                        if (inbound.contains(protocols::xray::port)) {
                            clientCfg.localPort = QString::number(inbound.value(protocols::xray::port).toInt());
                        }
                    }
                }
                config.clientConfig = clientCfg;
            } else {
                config.clientConfig = XrayClientConfig::fromJson(parsed);
            }
        }
    }
    
    return config;
}

bool XrayProtocolConfig::hasClientConfig() const
{
    return clientConfig.has_value();
}

void XrayProtocolConfig::setClientConfig(const XrayClientConfig& config)
{
    clientConfig = config;
}

void XrayProtocolConfig::clearClientConfig()
{
    clientConfig.reset();
}

} // namespace amnezia

