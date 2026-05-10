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

    // Free-form JSON object merged into the wire config the operator side
    // emits, carried through for round-trip integrity. The native engine
    // does not introspect this — keys outside the documented schema flow
    // through unchanged so the operator can extend the format independently
    // of an Amnezia release.
    QJsonObject additionalConfig;

    // True when imported from a third-party "vpn config" string (i.e. operator
    // doesn't have shell access to install the server via Amnezia's docker flow).
    bool isThirdPartyConfig = false;

    QJsonObject toJson() const;
    static MasterDnsVpnServerConfig fromJson(const QJsonObject &json);
};

// Per-client config — what gets handed to a single end user's MasterDnsVPN
// peer. Shares the singleton encryption key from the server config; differs
// in resolver selection, local listening port, and runtime tuning.
//
// Everything is structured JSON — the native engine consumes these fields
// directly. No TOML round-trip, no subprocess hand-off. The operator-side
// renderer (e.g. awg-easy-rs) produces this exact shape; users without an
// operator-side panel can synthesise it from the documented schema.
struct MasterDnsVpnClientConfig
{
    // Local SOCKS5 listen port the engine binds for user-app traffic.
    QString listenPort;

    // Optional local SOCKS5 auth — empty disables.
    QString socks5User;
    QString socks5Pass;

    // Public DNS resolvers the engine talks through. JSON array of strings,
    // each "ip[:port]" or "[v6]:port".
    QJsonArray resolvers;

    // RESOLVER_BALANCING_STRATEGY (1..8) — matches upstream values:
    //   1 = Random, 2 = Round Robin, 3 = Least Loss,
    //   4 = Lowest Latency, 5 = Hybrid Score,
    //   6 = Loss Then Latency, 7 = Least-Loss-Top-Random,
    //   8 = Least-Loss-Top-Round-Robin.
    int balancingStrategy = 5;

    // Packet duplication factor for normal data packets (clamped 1..10
    // by the engine) and setup packets (clamped to [packetDuplication, 12]).
    int packetDuplication = 3;
    int setupPacketDuplication = 4;

    // Compression negotiation. 0 = off, 1 = ZSTD, 2 = LZ4, 3 = ZLIB.
    int uploadCompression = 0;
    int downloadCompression = 0;

    // Free-form JSON object passed through verbatim. The engine surfaces
    // unrecognised keys via debug logging; they don't break operation.
    QJsonObject additionalConfig;

    // Stable identity slot used by Amnezia's per-peer revoke / expiry UX.
    // Mirrors XrayClientConfig::id.
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
