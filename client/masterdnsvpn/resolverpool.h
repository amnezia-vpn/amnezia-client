// SPDX-License-Identifier: GPL-3.0-or-later
//
// Resolver pool — manages the set of public DNS resolvers used as transports
// for the tunnel envelopes. One QUdpSocket per resolver entry; each socket
// fires `responseReceived` when the operator's mdnsvpn server replies.
//
// Responsibilities:
//   * Bootstrap MTU discovery (upload + download, exponential then binary
//     search) per resolver. See §9.2.
//   * Track per-resolver health (sent / acked / lost / RTT EWMA) with the
//     "halve when any counter > 1000" exponential decay.
//   * Auto-disable resolvers that go all-timeouts inside the configured
//     window; periodically re-probe inactive ones.
//   * Apply one of the 8 balancing strategies (§9.3) when the dispatcher
//     asks "which resolver should I send this packet on?".
//   * Implement packet duplication — N copies of normal packets, M for
//     setup (SYN) packets, fanned across distinct resolvers.
//
// The pool does NOT speak the protocol — it ships opaque DNS-query bytes
// and surfaces opaque DNS-response bytes. The dispatcher composes /
// decomposes the inner protocol using wireframing + dnsframing + crypto.

#ifndef MASTERDNSVPN_RESOLVERPOOL_H
#define MASTERDNSVPN_RESOLVERPOOL_H

#include <QHostAddress>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <cstdint>
#include <memory>

namespace amnezia::masterdnsvpn {

// 8 balancing strategies + the "0 = default" alias from §9.3.
enum class BalancingStrategy : int {
    Default = 0,
    Random = 1,
    RoundRobin = 2,
    LeastLoss = 3,
    LowestLatency = 4,
    HybridScore = 5,
    LossThenLatency = 6,
    LeastLossTopRandom = 7,
    LeastLossTopRoundRobin = 8,
};

// One operator-configured resolver entry. The address+port is what we
// actually dial; `tunnelDomain` is the NS-delegated FQDN appended to each
// QNAME (per §1 / §2.1 the operator may run multiple delegations).
struct ResolverSpec {
    QHostAddress address;
    quint16 port = 53;
    QString tunnelDomain;
};

// Result of resolver selection. `index` is the position into the active
// pool; `domain` is the matching tunnel domain to use for the QNAME suffix.
struct ResolverPick {
    int index = -1; // -1 = no resolver available
    QString tunnelDomain;
};

class ResolverConnection;

class ResolverPool : public QObject
{
    Q_OBJECT

public:
    struct Config {
        BalancingStrategy strategy = BalancingStrategy::HybridScore;
        int packetDuplicationCount = 3;          // §9.6 — clamped 1..10
        int setupPacketDuplicationCount = 4;     // §9.6 — clamped to [pkt..12]
        int maxUploadMtu = 150;
        int maxDownloadMtu = 4096;
        int autoDisableWindowMs = 30'000;
        bool autoDisableEnabled = true;
        bool recheckInactiveServers = true;
    };

    explicit ResolverPool(QObject *parent = nullptr);
    ~ResolverPool() override;

    // One-shot configuration. start() then opens the UDP sockets and begins
    // MTU discovery; the pool emits readyForUse() when at least one
    // resolver has a valid (uploadMTU, downloadMTU) pair.
    bool configure(const QVector<ResolverSpec> &resolvers, const Config &cfg);

    // Begin probing. Idempotent.
    void start();

    // Stop, close all sockets, drop all health stats.
    void stop();

    // Pick a resolver per the current strategy. May return ResolverPick{-1,""}
    // when the pool has zero active resolvers (caller should retry later).
    ResolverPick pickPrimary();

    // Pick `count` resolvers without replacement for packet duplication —
    // returns up to `count` distinct picks. Caller passes setup=true for
    // SYN packets so the larger duplication count applies.
    QVector<ResolverPick> pickDuplicates(int count, bool setup);

    // Send pre-encoded DNS query bytes to the resolver at `index`. Returns
    // false on socket error (resolver auto-disabled if repeated failures
    // hit the window threshold).
    bool send(int index, const QByteArray &queryBytes);

    // Currently-discovered global MTU pair (the minimum of all active
    // resolvers — the dispatcher uses these to size frames).
    int syncedUploadMtu() const;
    int syncedDownloadMtu() const;

signals:
    // Fires when the first resolver completes MTU discovery.
    void readyForUse();

    // Per-resolver MTU update — useful for operator dashboards. `index` is
    // the active-pool index (changes as resolvers move between active /
    // inactive lists).
    void resolverMtuChanged(int index, int uploadMtu, int downloadMtu);

    // Inbound DNS response — opaque bytes the dispatcher decodes via
    // dnsframing::parseResponse(). `transactionId` is supplied for the
    // dispatcher's outstanding-query map.
    void responseReceived(int resolverIndex,
                          quint16 transactionId,
                          const QByteArray &responseBytes);

    // Resolver state changes. `active` reports whether the resolver is
    // currently in the active pool (false = auto-disabled / failed MTU).
    void resolverStateChanged(int index, bool active);

private:
    Q_DISABLE_COPY_MOVE(ResolverPool)

    void onIncoming(int index, const QByteArray &bytes, quint16 transactionId);
    void recordSendResult(int index, bool ok);

    QVector<std::unique_ptr<ResolverConnection>> m_connections;
    Config m_cfg;
    int m_roundRobinCursor = 0;
    int m_syncedUploadMtu = 0;
    int m_syncedDownloadMtu = 0;
    bool m_started = false;
};

} // namespace amnezia::masterdnsvpn

#endif // MASTERDNSVPN_RESOLVERPOOL_H
