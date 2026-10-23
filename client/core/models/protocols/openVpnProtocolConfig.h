#ifndef OPENVPNPROTOCOLCONFIG_H
#define OPENVPNPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>
#include <optional>

namespace amnezia
{

struct OpenVpnServerConfig {
    QString port;
    QString transportProto;
    QString subnetAddress;
    QString subnetMask;
    QString subnetCidr;
    QString cipher;
    QString hash;
    bool ncpDisable = false;
    bool tlsAuth = true;
    QString additionalClientConfig;
    QString additionalServerConfig;
    bool isThirdPartyConfig = false;
    
    QJsonObject toJson() const;
    static OpenVpnServerConfig fromJson(const QJsonObject& json);
    
    bool hasEqualServerSettings(const OpenVpnServerConfig& other) const;
};

struct OpenVpnClientConfig {
    QString nativeConfig;
    QString clientId;
    bool blockOutsideDns = false;
    
    QJsonObject toJson() const;
    static OpenVpnClientConfig fromJson(const QJsonObject& json);
};

struct OpenVpnProtocolConfig {
    OpenVpnServerConfig serverConfig;
    std::optional<OpenVpnClientConfig> clientConfig;
    
    QJsonObject toJson() const;
    static OpenVpnProtocolConfig fromJson(const QJsonObject& json);
    
    bool hasClientConfig() const;
    void setClientConfig(const OpenVpnClientConfig& config);
    void clearClientConfig();
};

} // namespace amnezia

#endif // OPENVPNPROTOCOLCONFIG_H

