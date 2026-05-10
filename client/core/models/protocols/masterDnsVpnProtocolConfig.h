#ifndef MASTERDNSVPNPROTOCOLCONFIG_H
#define MASTERDNSVPNPROTOCOLCONFIG_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <optional>

namespace amnezia
{

// Server-side singleton — every MasterDnsVPN client of a given operator shares the
// same encryption key and dialing parameters. Per-client variation lives in
// MasterDnsVpnClientConfig (resolvers, local SOCKS5 listen port, etc.).
struct MasterDnsVpnServerConfig
{
    // JSON array of NS-delegated FQDNs the server is authoritative for
    // (e.g. ["v.example.com"]). Stored as a QJsonArray to round-trip cleanly
    // through the wire JSON without re-parsing string forms.
    QJsonArray domains;

    // UDP port the server's mdnsvpn binary binds. Default 53.
    QString port;

    // Server bind address. Almost always "0.0.0.0".
    QString bind;

    // 0..5 — see protocols::masterDnsVpn encryptionMethod* constants.
    int encryptionMethod = 0;

    // Lower-case hex shared secret. Same value baked into every client config.
    QString encryptionKey;

    // "SOCKS5" (clients pick destination per stream) or "TCP" (every connection
    // forwards to forwardIp:forwardPort).
    QString protocolType;

    // Comma- or array-encoded list of upstream resolvers used to satisfy
    // DNS_QUERY_REQ tunnel envelopes. Stored as JSON array.
    QJsonArray dnsUpstreamServers;

    // Used in TCP mode (every connection forwards here) OR in SOCKS5 mode when
    // useExternalSocks5=true (server chains through an upstream SOCKS5 proxy).
    QString forwardIp;
    int forwardPort = 0;

    bool useExternalSocks5 = false;
    bool socks5Auth = false;
    QString socks5User;
    QString socks5Pass;

    // Free-form TOML appended verbatim to server_config.toml on the operator side.
    // Carried for round-trip integrity; the desktop client doesn't manipulate it.
    QString additionalConfig;

    // True when imported from a third-party "vpn config" string (i.e. operator
    // doesn't have shell access to install the server via Amnezia's docker flow).
    bool isThirdPartyConfig = false;

    QJsonObject toJson() const;
    static MasterDnsVpnServerConfig fromJson(const QJsonObject &json);
};

// Per-client config — what gets handed to a single end user's mdnsvpn client.
// Shares the singleton encryption key from the server config; differs only in
// resolver selection, local listening port, and optional local SOCKS5 auth.
struct MasterDnsVpnClientConfig
{
    // The full client_config.toml body the user feeds to `mdnsvpn -config <path>`.
    // Round-tripped verbatim so that the operator's exact upstream-format file
    // is what reaches the user (no Amnezia-side reformatting).
    QString nativeConfig;

    // Local SOCKS5 listen port for the mdnsvpn client. tun2socks dials this.
    QString listenPort;

    // Optional local SOCKS5 auth — empty disables.
    QString socks5User;
    QString socks5Pass;

    // Stable identity slot used by Amnezia's per-peer revoke / expiry UX.
    // Mirrors XrayClientConfig::id; for mdnsvpn it's a free-form name.
    QString id;

    QJsonObject toJson() const;
    static MasterDnsVpnClientConfig fromJson(const QJsonObject &json);
};

struct MasterDnsVpnProtocolConfig
{
    MasterDnsVpnServerConfig serverConfig;
    std::optional<MasterDnsVpnClientConfig> clientConfig;

    QJsonObject toJson() const;
    static MasterDnsVpnProtocolConfig fromJson(const QJsonObject &json);

    bool hasClientConfig() const;
    void setClientConfig(const MasterDnsVpnClientConfig &config);
    void clearClientConfig();
};

} // namespace amnezia

#endif // MASTERDNSVPNPROTOCOLCONFIG_H
