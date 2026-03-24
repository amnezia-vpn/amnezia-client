#ifndef XRAYPROTOCOLCONFIG_H
#define XRAYPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>
#include <optional>

namespace amnezia
{

// ── xPadding ─────────────────────────────────────────────────────────────────
struct XrayXPaddingConfig {
    QString bytesMin;                   // xPaddingBytes min
    QString bytesMax;                   // xPaddingBytes max
    bool    obfsMode = true;            // xPaddingObfsMode
    QString key;                        // xPaddingKey
    QString header;                     // xPaddingHeader
    QString placement = "Cookie";       // xPaddingPlacement: Cookie|Header|Query|Body
    QString method    = "Repeat-x";     // xPaddingMethod: Repeat-x|Random|Zero

    QJsonObject toJson() const;
    static XrayXPaddingConfig fromJson(const QJsonObject &json);
};

// ── xmux ─────────────────────────────────────────────────────────────────────
struct XrayXmuxConfig {
    bool    enabled = true;

    QString maxConcurrencyMin   = "0";
    QString maxConcurrencyMax   = "0";
    QString maxConnectionsMin   = "0";
    QString maxConnectionsMax   = "0";
    QString cMaxReuseTimesMin   = "0";
    QString cMaxReuseTimesMax   = "0";
    QString hMaxRequestTimesMin = "0";
    QString hMaxRequestTimesMax = "0";
    QString hMaxReusableSecsMin = "0";
    QString hMaxReusableSecsMax = "0";
    QString hKeepAlivePeriod;

    QJsonObject toJson() const;
    static XrayXmuxConfig fromJson(const QJsonObject &json);
};

// ── XHTTP transport ───────────────────────────────────────────────────────────
struct XrayXhttpConfig {
    QString mode             = "Auto";  // Auto|Packet-up|Stream-up|Stream-one
    QString host             = "www.googletagmanager.com";
    QString path;
    QString headersTemplate  = "HTTP";  // HTTP|None
    QString uplinkMethod     = "POST";  // POST|PUT|PATCH
    bool    disableGrpc      = true;
    bool    disableSse       = true;

    // Session & Sequence
    QString sessionPlacement = "Path";  // Path|Header|Cookie|None
    QString sessionKey       = "Path";
    QString seqPlacement     = "Path";
    QString seqKey;
    QString uplinkDataPlacement = "Body"; // Body|Query
    QString uplinkDataKey;

    // Traffic Shaping
    QString uplinkChunkSize       = "0";
    QString scMaxBufferedPosts;
    QString scMaxEachPostBytesMin = "1";
    QString scMaxEachPostBytesMax = "100";
    QString scMinPostsIntervalMsMin = "100";
    QString scMinPostsIntervalMsMax = "800";
    QString scStreamUpServerSecsMin = "1";
    QString scStreamUpServerSecsMax = "100";

    XrayXPaddingConfig xPadding;
    XrayXmuxConfig     xmux;

    QJsonObject toJson() const;
    static XrayXhttpConfig fromJson(const QJsonObject &json);
};

// ── mKCP transport ────────────────────────────────────────────────────────────
struct XrayMkcpConfig {
    QString tti;
    QString uplinkCapacity;
    QString downlinkCapacity;
    QString readBufferSize;
    QString writeBufferSize;
    bool    congestion = true;

    QJsonObject toJson() const;
    static XrayMkcpConfig fromJson(const QJsonObject &json);
};

// ── Server config (settings editable by user) ─────────────────────────────────
struct XrayServerConfig {
    // Existing fields
    QString port;
    QString transportProto;
    QString subnetAddress;
    QString site;
    bool    isThirdPartyConfig = false;

    // New: Security
    QString security    = "reality";    // none|tls|reality
    QString flow        = "xtls-rprx-vision"; // ""|xtls-rprx-vision|xtls-rprx-vision-udp443
    QString fingerprint = "Mozilla/5.0";
    QString sni         = "cdn.example.com";
    QString alpn        = "HTTP/2";     // TLS only: HTTP/2|HTTP/1.1|HTTP/2,HTTP/1.1

    // New: Transport
    QString transport = "raw";          // raw|xhttp|mkcp
    XrayXhttpConfig xhttp;
    XrayMkcpConfig  mkcp;

    QJsonObject toJson() const;
    static XrayServerConfig fromJson(const QJsonObject &json);

    bool hasEqualServerSettings(const XrayServerConfig &other) const;
};

// ── Client config (generated, not edited by user) ─────────────────────────────
struct XrayClientConfig {
    QString nativeConfig;
    QString localPort;
    QString id;

    QJsonObject toJson() const;
    static XrayClientConfig fromJson(const QJsonObject &json);
};

// ── Top-level protocol config ──────────────────────────────────────────────────
struct XrayProtocolConfig {
    XrayServerConfig              serverConfig;
    std::optional<XrayClientConfig> clientConfig;

    QJsonObject toJson() const;
    static XrayProtocolConfig fromJson(const QJsonObject &json);

    bool hasClientConfig() const;
    void setClientConfig(const XrayClientConfig &config);
    void clearClientConfig();
};

} // namespace amnezia

#endif // XRAYPROTOCOLCONFIG_H
