#ifndef AWGPROTOCOLCONFIG_H
#define AWGPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <optional>

namespace amnezia
{

struct AwgServerConfig {
    QString port;
    QString transportProto;
    QString protocolVersion;
    QString subnetAddress;
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
    bool isThirdPartyConfig;
    
    QJsonObject toJson() const;
    static AwgServerConfig fromJson(const QJsonObject& json);
};

struct AwgClientConfig {
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
    bool isObfuscationEnabled;
    
    QJsonObject toJson() const;
    static AwgClientConfig fromJson(const QJsonObject& json);
};

struct AwgProtocolConfig {
    AwgServerConfig serverConfig;
    std::optional<AwgClientConfig> clientConfig;
    
    QJsonObject toJson() const;
    static AwgProtocolConfig fromJson(const QJsonObject& json);
    
    bool hasClientConfig() const;
    void setClientConfig(const AwgClientConfig& config);
    void clearClientConfig();
};

} // namespace amnezia

#endif // AWGPROTOCOLCONFIG_H

