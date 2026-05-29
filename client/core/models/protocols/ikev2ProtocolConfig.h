#ifndef IKEV2PROTOCOLCONFIG_H
#define IKEV2PROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>
#include <QByteArray>
#include <optional>

namespace amnezia
{

struct Ikev2ServerConfig {
    QString hostName;
    bool isThirdPartyConfig = false;
    
    QJsonObject toJson() const;
    static Ikev2ServerConfig fromJson(const QJsonObject& json);
};

struct Ikev2ClientConfig {
    QString nativeConfig;
    QString hostName;
    QString userName;
    QString cert;
    QString password;
    QString clientId;
    
    QJsonObject toJson() const;
    static Ikev2ClientConfig fromJson(const QJsonObject& json);
};

struct Ikev2ProtocolConfig {
    Ikev2ServerConfig serverConfig;
    std::optional<Ikev2ClientConfig> clientConfig;
    
    QJsonObject toJson() const;
    static Ikev2ProtocolConfig fromJson(const QJsonObject& json);
    
    bool hasClientConfig() const;
    void setClientConfig(const Ikev2ClientConfig& config);
    void clearClientConfig();
};

} // namespace amnezia

#endif // IKEV2PROTOCOLCONFIG_H


