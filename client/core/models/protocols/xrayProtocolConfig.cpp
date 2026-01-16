#include "xrayProtocolConfig.h"

#include <QJsonDocument>
#include <QJsonArray>

#include "../../../protocols/protocols_defs.h"

namespace amnezia
{

QJsonObject XrayServerConfig::toJson() const
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
    if (!site.isEmpty()) {
        obj[config_key::site] = site;
    }
    
    if (isThirdPartyConfig) {
        obj[config_key::isThirdPartyConfig] = isThirdPartyConfig;
    }
    
    return obj;
}

XrayServerConfig XrayServerConfig::fromJson(const QJsonObject& json)
{
    XrayServerConfig config;
    
    config.port = json.value(config_key::port).toString();
    config.transportProto = json.value(config_key::transport_proto).toString();
    config.subnetAddress = json.value(config_key::subnet_address).toString();
    config.site = json.value(config_key::site).toString();
    
    config.isThirdPartyConfig = json.value(config_key::isThirdPartyConfig).toBool(false);
    
    return config;
}

QJsonObject XrayClientConfig::toJson() const
{
    QJsonObject obj;
    
    if (!nativeConfig.isEmpty()) {
        obj[config_key::config] = nativeConfig;
    }
    if (!localPort.isEmpty()) {
        obj[config_key::local_port] = localPort;
    }
    if (!id.isEmpty()) {
        obj[config_key::clientId] = id;
    }
    
    return obj;
}

XrayClientConfig XrayClientConfig::fromJson(const QJsonObject& json)
{
    XrayClientConfig config;
    
    config.nativeConfig = json.value(config_key::config).toString();
    config.localPort = json.value(config_key::local_port).toString();
    config.id = json.value(config_key::clientId).toString();
    
    if (config.id.isEmpty() && !config.nativeConfig.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(config.nativeConfig.toUtf8());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject configObj = doc.object();
            if (configObj.contains("outbounds")) {
                QJsonArray outbounds = configObj.value("outbounds").toArray();
                if (!outbounds.isEmpty()) {
                    QJsonObject outbound = outbounds[0].toObject();
                    if (outbound.contains("settings")) {
                        QJsonObject settings = outbound["settings"].toObject();
                        if (settings.contains("vnext")) {
                            QJsonArray vnext = settings["vnext"].toArray();
                            if (!vnext.isEmpty()) {
                                QJsonObject vnextObj = vnext[0].toObject();
                                if (vnextObj.contains("users")) {
                                    QJsonArray users = vnextObj["users"].toArray();
                                    if (!users.isEmpty()) {
                                        QJsonObject user = users[0].toObject();
                                        if (user.contains("id")) {
                                            config.id = user["id"].toString();
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
        QJsonObject clientJson = clientConfig->toJson();
        obj[config_key::last_config] = QString::fromUtf8(QJsonDocument(clientJson).toJson(QJsonDocument::Compact));
    }
    
    return obj;
}

XrayProtocolConfig XrayProtocolConfig::fromJson(const QJsonObject& json)
{
    XrayProtocolConfig config;
    
    config.serverConfig = XrayServerConfig::fromJson(json);
    
    QString lastConfigStr = json.value(config_key::last_config).toString();
    if (!lastConfigStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(lastConfigStr.toUtf8());
        if (doc.isObject()) {
            config.clientConfig = XrayClientConfig::fromJson(doc.object());
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

