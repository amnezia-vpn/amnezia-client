#ifndef XRAYPROTOCOLCONFIG_H
#define XRAYPROTOCOLCONFIG_H

#include <QJsonObject>
#include "core/utils/constants/protocolConstants.h"
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
    QString placement = protocols::xray::defaultXPaddingPlacement; // xPaddingPlacement: Cookie|Header|Query|Body
    QString method = protocols::xray::defaultXPaddingMethod;       // xPaddingMethod: Repeat-x|Random|Zero

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
    QString mode             = protocols::xray::defaultXhttpMode;  // Auto|Packet-up|Stream-up|Stream-one
    QString host             = protocols::xray::defaultXhttpHost;
    QString path;
    QString headersTemplate  = protocols::xray::defaultXhttpHeadersTemplate;  // HTTP|None
    QString uplinkMethod     = protocols::xray::defaultXhttpUplinkMethod;  // POST|PUT|PATCH
    bool    disableGrpc      = true;
    bool    disableSse       = true;

    // Session & Sequence
    QString sessionPlacement = protocols::xray::defaultXhttpSessionPlacement;
    QString sessionKey       = protocols::xray::defaultXhttpSessionKey;
    QString seqPlacement     = protocols::xray::defaultXhttpSeqPlacement;
    QString seqKey;
    QString uplinkDataPlacement = protocols::xray::defaultXhttpUplinkDataPlacement;
    QString uplinkDataKey;

    // Traffic Shaping
    QString uplinkChunkSize       = protocols::xray::defaultXhttpUplinkChunkSize;
    QString scMaxBufferedPosts;
    QString scMaxEachPostBytesMin = protocols::xray::defaultXhttpScMaxEachPostBytesMin;
    QString scMaxEachPostBytesMax = protocols::xray::defaultXhttpScMaxEachPostBytesMax;
    QString scMinPostsIntervalMsMin = protocols::xray::defaultXhttpScMinPostsIntervalMsMin;
    QString scMinPostsIntervalMsMax = protocols::xray::defaultXhttpScMinPostsIntervalMsMax;
    QString scStreamUpServerSecsMin = protocols::xray::defaultXhttpScStreamUpServerSecsMin;
    QString scStreamUpServerSecsMax = protocols::xray::defaultXhttpScStreamUpServerSecsMax;

    XrayXPaddingConfig xPadding;
    XrayXmuxConfig     xmux;

    QJsonObject toJson() const;
    /// Reads only keys present in JSON (no Amnezia UI defaults). Use XrayConfigModel::applyDefaultsToServerConfig for UI.
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
    QString port;
    QString transportProto;
    QString subnetAddress;
    QString site;
    bool isThirdPartyConfig = false;

    QString security;
    QString flow;
    QString fingerprint;
    QString sni;
    QString alpn;

    QString transport;
    XrayXhttpConfig xhttp;
    XrayMkcpConfig mkcp;

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
    XrayServerConfig serverConfig;
    std::optional<XrayClientConfig> clientConfig;

    QJsonObject toJson() const;
    static XrayProtocolConfig fromJson(const QJsonObject &json);

    bool hasClientConfig() const;
    void setClientConfig(const XrayClientConfig &config);
    void clearClientConfig();

    bool needsClientHydration = false;

    bool hydrateServerConfigFromClientNative();
};

} // namespace amnezia

#endif // XRAYPROTOCOLCONFIG_H
