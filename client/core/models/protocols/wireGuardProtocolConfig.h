#ifndef WIREGUARDPROTOCOLCONFIG_H
#define WIREGUARDPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <optional>

namespace amnezia
{

struct WireGuardServerConfig {
    QString port;
    QString transportProto;
    QString subnetAddress;
    QString subnetMask;
    QString subnetCidr;
    bool isThirdPartyConfig = false;
    
    QJsonObject toJson() const;
    static WireGuardServerConfig fromJson(const QJsonObject& json);
    
    bool hasEqualServerSettings(const WireGuardServerConfig& other) const;
};

struct WireGuardClientConfig {
    QString nativeConfig;
    QString hostName;
    int port;
    QString clientIp;
    QString clientPrivateKey;
    QString clientPublicKey;
    QString serverPublicKey;
    QString presharedKey;
    QString clientId;
    QStringList allowedIps;
    QString persistentKeepAlive;
    QString mtu;
    bool isObfuscationEnabled = false;

    // AmneziaWG obfuscation parameters. A native WireGuard config imported with
    // "Enable WireGuard obfuscation" is stored in the WireGuard container with
    // these set; they must round-trip so an already-obfuscated profile keeps
    // working after upgrade instead of degrading to plain WireGuard.
    QString junkPacketCount;
    QString junkPacketMinSize;
    QString junkPacketMaxSize;
    QString initPacketJunkSize;
    QString responsePacketJunkSize;
    QString cookieReplyPacketJunkSize;
    QString transportPacketJunkSize;
    QString initPacketMagicHeader;
    QString responsePacketMagicHeader;
    QString underloadPacketMagicHeader;
    QString transportPacketMagicHeader;
    QString specialJunk1;
    QString specialJunk2;
    QString specialJunk3;
    QString specialJunk4;
    QString specialJunk5;
    QString headerProtectionKey;
    QString contentPaddingAddition;
    QString rekeyAfterTime;
    QString rekeyTimeout;
    QString rejectAfterTime;
    QString keepaliveTimeout;
    QString maxHandshakeAttempts;

    QJsonObject toJson() const;
    static WireGuardClientConfig fromJson(const QJsonObject& json);
};

struct WireGuardProtocolConfig {
    WireGuardServerConfig serverConfig;
    std::optional<WireGuardClientConfig> clientConfig;
    
    QJsonObject toJson() const;
    static WireGuardProtocolConfig fromJson(const QJsonObject& json);
    
    bool hasClientConfig() const;
    void setClientConfig(const WireGuardClientConfig& config);
    void clearClientConfig();
};

} // namespace amnezia

#endif // WIREGUARDPROTOCOLCONFIG_H

