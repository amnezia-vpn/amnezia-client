#ifndef XRAYPROTOCOLCONFIG_H
#define XRAYPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>
#include <optional>

namespace amnezia
{

struct XrayServerConfig {
    QString port;
    QString transportProto;
    QString subnetAddress;
    QString site;
    bool isThirdPartyConfig = false;
    
    QJsonObject toJson() const;
    static XrayServerConfig fromJson(const QJsonObject& json);
    
    bool hasEqualServerSettings(const XrayServerConfig& other) const;
};

struct XrayClientConfig {
    QString nativeConfig;
    QString localPort;
    QString id;
    
    QJsonObject toJson() const;
    static XrayClientConfig fromJson(const QJsonObject& json);
};

struct XrayProtocolConfig {
    XrayServerConfig serverConfig;
    std::optional<XrayClientConfig> clientConfig;
    
    QJsonObject toJson() const;
    static XrayProtocolConfig fromJson(const QJsonObject& json);
    
    bool hasClientConfig() const;
    void setClientConfig(const XrayClientConfig& config);
    void clearClientConfig();
};

} // namespace amnezia

#endif // XRAYPROTOCOLCONFIG_H

