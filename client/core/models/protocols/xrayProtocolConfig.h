#ifndef XRAYPROTOCOLCONFIG_H
#define XRAYPROTOCOLCONFIG_H

#include <QJsonArray>
#include <QJsonObject>
#include "core/utils/constants/protocolConstants.h"
#include <QString>
#include <QStringList>
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

struct XrayClientTemplate {
    int formatVersion = 0;

    QString fingerprint     = protocols::xray::defaultFingerprint;
    QString uplinkMethod    = protocols::xray::defaultXhttpUplinkMethod;
    QString uplinkChunkSize = protocols::xray::defaultXhttpUplinkChunkSize;
    QString scMinPostsIntervalMsMin = protocols::xray::defaultXhttpScMinPostsIntervalMsMin;
    QString scMinPostsIntervalMsMax = protocols::xray::defaultXhttpScMinPostsIntervalMsMax;

    XrayXmuxConfig xmux;

    QString updatedAt;

    QJsonObject toJson() const;
    static XrayClientTemplate fromJson(const QJsonObject &json);

    QString contentFingerprint() const;

    void materializeFromLegacy(const QJsonObject &storedServerJson);
};

// ── XHTTP transport ───────────────────────────────────────────────────────────
struct XrayXhttpConfig {
    QString mode             = protocols::xray::defaultXhttpMode;  // Auto|Packet-up|Stream-up|Stream-one
    QString host             = protocols::xray::defaultXhttpHost;
    QString path;
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
    QString scMaxBufferedPosts;
    QString scMaxEachPostBytesMin = protocols::xray::defaultXhttpScMaxEachPostBytesMin;
    QString scMaxEachPostBytesMax = protocols::xray::defaultXhttpScMaxEachPostBytesMax;
    QString scStreamUpServerSecsMin = protocols::xray::defaultXhttpScStreamUpServerSecsMin;
    QString scStreamUpServerSecsMax = protocols::xray::defaultXhttpScStreamUpServerSecsMax;

    XrayXPaddingConfig xPadding;

    QJsonObject toJson() const;
    /// Reads only keys present in JSON (no Amnezia UI defaults). Use XrayConfigModel::applyDefaultsToServerConfig for UI.
    static XrayXhttpConfig fromJson(const QJsonObject &json);
};

// ── mKCP transport ────────────────────────────────────────────────────────────
struct XrayMkcpConfig {
    QString tti;
    QString mtu;
    QString uplinkCapacity;
    QString downlinkCapacity;
    QString cwndMultiplier;
    QString maxSendingWindow;

    QJsonObject toJson() const;
    static XrayMkcpConfig fromJson(const QJsonObject &json);
};

enum class XrayStreamSide {
    Server,
    Client,
};

struct XrayServerInboundInputs {
    QJsonArray clients;
    QString realityPrivateKey;
    QString realityShortId;
};

enum class XrayServerJsonStatus {
    Ok,
    MissingInbounds,
    EmptyInbounds,
    MissingStreamSettings,
    MissingSettings,
};

enum class XrayClientListFilter {
    KeepAll,
    DropWithoutId,
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
    QString sni;
    QString alpn;

    QString transport;
    XrayXhttpConfig xhttp;
    XrayMkcpConfig mkcp;

    QJsonObject toJson() const;

    static XrayServerConfig fromJson(const QJsonObject &json);

    void applyDefaults(bool fillFlowDefault = false);

    QJsonObject streamSettingsJson(XrayStreamSide side, const XrayClientTemplate &clientTemplate) const;

    QJsonObject serverStreamSettings() const;

    QJsonObject clientStreamSettings(const XrayClientTemplate &clientTemplate) const;

    QJsonObject toServerInboundJson(const XrayServerInboundInputs &inputs) const;

    static XrayServerJsonStatus fromServerInboundJson(const QJsonObject &serverJson, XrayServerConfig &outServerConfig,
                                                      XrayClientTemplate &outClientTemplate);

    static QJsonArray clientsFromServerInboundJson(const QJsonObject &serverJson);
    static XrayServerJsonStatus setClientsInServerInboundJson(QJsonObject &serverJson, const QJsonArray &clients);

    static QJsonObject makeClientEntry(const QString &clientId, const QString &flowValue);
    static QJsonObject applyFlowToClient(const QJsonObject &client, const QString &flowValue);
    static int indexOfClient(const QJsonArray &clients, const QString &clientId);
    static QString firstClientId(const QJsonArray &clients);
    static QJsonArray applyFlowToClients(const QJsonArray &clients, const QString &flowValue,
                                         XrayClientListFilter filter = XrayClientListFilter::KeepAll);

    QJsonObject serverView() const;

    QJsonObject issuedConfigView() const;

    bool hasEqualServerSettings(const XrayServerConfig &other) const;

    bool breaksIssuedConfigs(const XrayServerConfig &other) const;
};

namespace xrayEffective
{
    QString xhttpMode(const QString &mode);
    QString sessionSeqPlacement(const QString &placement);
    QString uplinkDataPlacement(const QString &placement);
    QString xPaddingPlacement(const QString &placement);
    QString xPaddingMethod(const QString &method);
    QString range(const QString &minV, const QString &maxV);
    void putRangeIfAny(QJsonObject &obj, const char *key, QString minV, QString maxV, const char *fallbackMin,
                       const char *fallbackMax);

    QString security(const XrayServerConfig &srv);
    QString clientFlow(const XrayServerConfig &srv);
    QString network(const XrayServerConfig &srv);
    QString xhttpModeSent(const XrayServerConfig &srv);
}

struct XrayClientOutboundInputs {
    QString serverAddress;
    QString clientId;
    QString realityPublicKey;
    QString realityShortId;
    QString tlsPinnedPeerCertSha256;
};

// ── Client config (generated, not edited by user) ─────────────────────────────
struct XrayClientConfig {
    QString nativeConfig;
    QString localPort;
    QString id;
    QString templateFingerprint;

    QJsonObject toJson() const;
    static XrayClientConfig fromJson(const QJsonObject &json);

    static QString idFromNativeJson(const QJsonObject &nativeJson);
    static QString localPortFromNativeJson(const QJsonObject &nativeJson);
};

// ── Top-level protocol config ──────────────────────────────────────────────────
struct XrayProtocolConfig {
    XrayServerConfig serverConfig;
    XrayClientTemplate clientTemplate;
    std::optional<XrayClientConfig> clientConfig;

    QJsonObject toJson() const;
    static XrayProtocolConfig fromJson(const QJsonObject &json);

    bool hasClientConfig() const;
    void setClientConfig(const XrayClientConfig &config);
    void clearClientConfig();

    bool needsClientHydration = false;
    bool needsTemplateMaterialization = false;

    QJsonObject toClientOutboundJson(const XrayClientOutboundInputs &inputs) const;

    bool fromClientOutboundJson(const QJsonObject &nativeJson);

    bool hydrateServerConfigFromClientNative();

    bool stampClientTemplateFormatVersion();
};

} // namespace amnezia

#endif // XRAYPROTOCOLCONFIG_H
