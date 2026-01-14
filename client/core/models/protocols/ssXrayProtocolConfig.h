#ifndef SSXRAYPROTOCOLCONFIG_H
#define SSXRAYPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>
#include <optional>

namespace amnezia
{

struct SSXrayServerConfig {
    QString port;
    QString transportProto;
    QString site;
    bool isThirdPartyConfig;
    
    QJsonObject toJson() const;
    static SSXrayServerConfig fromJson(const QJsonObject& json);
};

struct SSXrayClientConfig {
    QString nativeConfig;
    QString localPort;
    
    QJsonObject toJson() const;
    static SSXrayClientConfig fromJson(const QJsonObject& json);
};

struct SSXrayProtocolConfig {
    SSXrayServerConfig serverConfig;
    std::optional<SSXrayClientConfig> clientConfig;
    
    QJsonObject toJson() const;
    static SSXrayProtocolConfig fromJson(const QJsonObject& json);
    
    bool hasClientConfig() const;
    void setClientConfig(const SSXrayClientConfig& config);
    void clearClientConfig();
};

} // namespace amnezia

#endif // SSXRAYPROTOCOLCONFIG_H

