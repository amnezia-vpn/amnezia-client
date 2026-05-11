// SPDX-License-Identifier: GPL-3.0-or-later
//
// Local DNS cache + DNS_QUERY_RES fragment reassembly.
//
// Two related but distinct pieces of state, packaged together because
// both belong to the same per-session DNS-tunnel layer:
//
//   * **DnsLocalCache** — (name, type, class) → response. Repeats of
//     the same lookup get answered from this store instead of round-
//     tripping the tunnel. Both for performance and for OPSEC: a local
//     observer watching loopback should see "DNS forwarder with cache",
//     not "everything always takes a tunnel round-trip" (which would
//     stand out from `dnsmasq` / `systemd-resolved`).
//
//   * **DnsReassemblyStore** — `(sequenceNum) → fragment array`.
//     DNS_QUERY_RES packets from the tunnel arrive in N fragments
//     (FragmentID + TotalFragments wire fields). We accumulate them in
//     this store until all `total` arrive, then concatenate and emit
//     the full response.
//
// Neither piece is a hot-path data structure (a single client makes
// O(10) DNS queries per minute under normal browser load), so we use
// QHash + a single TTL-sweep QTimer rather than anything fancy.

#ifndef MASTERDNSVPN_DNSCACHE_H
#define MASTERDNSVPN_DNSCACHE_H

#include <QByteArray>
#include <QHash>
#include <QHostAddress>
#include <QString>
#include <functional>
#include <optional>

namespace amnezia::masterdnsvpn {

// Cache key: lowercase question name + numeric type + class. Equality
// uses all three.
struct DnsCacheKey {
    QString name;
    quint16 type = 0;
    quint16 cls = 0;

    bool operator==(const DnsCacheKey &o) const
    {
        return type == o.type && cls == o.cls && name == o.name;
    }
};

inline size_t qHash(const DnsCacheKey &k, size_t seed = 0) noexcept
{
    return qHashMulti(seed, k.name, k.type, k.cls);
}

// Status of a cache lookup. Mirrors upstream's tri-state result so the
// caller can decide whether to dispatch to the tunnel.
enum class DnsCacheStatus {
    Miss,    // no entry; caller should dispatch + this lookup created a Pending entry
    Pending, // entry exists but tunnel response hasn't arrived yet
    Ready,   // entry exists and response bytes are available
};

class DnsLocalCache
{
public:
    DnsLocalCache();

    // Default per-entry TTL (ms). Repeated queries within this window are
    // served from the cache. Configurable per-session via setTtlMs().
    static constexpr qint64 kDefaultTtlMs = 60'000;

    void setTtlMs(qint64 ttlMs);
    qint64 ttlMs() const { return m_ttlMs; }

    // Look up an entry by key. Side effect: if no entry exists, creates
    // a Pending one (so future lookups for the same name see Pending
    // instead of Miss while the tunnel is in flight). The first caller
    // gets Miss and is responsible for dispatching the tunnel query.
    DnsCacheStatus lookupOrCreatePending(const DnsCacheKey &key, qint64 nowMs);

    // Read a cached response. Returns the raw bytes (without txid
    // patching — caller passes through patchDnsTxid). Returns empty
    // QByteArray when the entry is Pending or absent.
    QByteArray readyResponseFor(const DnsCacheKey &key) const;

    // Store the response bytes (from the tunnel's DNS_QUERY_RES) under
    // `key`, transitioning the entry to Ready. Sets the new expiry.
    void setReady(const DnsCacheKey &key, const QByteArray &response, qint64 nowMs);

    // Remove entries whose expiry has passed. Returns the number purged.
    int sweepExpired(qint64 nowMs);

    // Diagnostics.
    int size() const { return m_entries.size(); }
    void clear() { m_entries.clear(); }

private:
    enum class Status { Pending, Ready };
    struct Entry {
        Status status = Status::Pending;
        QByteArray response;
        qint64 expiresAtMs = 0;
        qint64 firstSeenMs = 0;
    };
    QHash<DnsCacheKey, Entry> m_entries;
    qint64 m_ttlMs = kDefaultTtlMs;
};

// ---------------------------------------------------------------------
// DNS_QUERY_RES fragment reassembly
// ---------------------------------------------------------------------

// One in-flight DNS query. Owns the reply route back to the local SOCKS5
// client (UDP address + port) and the accumulator for response fragments.
struct DnsInFlight {
    QHostAddress replyAddr;
    quint16 replyPort = 0;
    quint16 clientTxid = 0;          // for response txid patching
    DnsCacheKey cacheKey;            // for setReady on completion
    QVector<QByteArray> fragments;   // indexed by fragId; missing entries are empty
    quint8 totalFragments = 0;
    qint64 createdMs = 0;
};

class DnsReassemblyStore
{
public:
    static constexpr qint64 kDefaultTtlMs = 10'000;

    // Begin tracking a tunnel-dispatched query. `seq` is the
    // DNS_QUERY_REQ wire sequence number — also expected on the matching
    // DNS_QUERY_RES packets.
    void track(quint16 seq, const DnsInFlight &inflight);

    // Add an incoming fragment. Returns true + sets `assembledOut` to
    // the full reassembled response when this fragment completes the
    // set. Returns false (and leaves assembledOut untouched) while
    // more fragments are pending.
    //
    // If the seq isn't tracked (orphan/late fragment) returns false and
    // does nothing. Drops the entry on completion.
    bool addFragment(quint16 seq,
                     quint8 fragId,
                     quint8 total,
                     const QByteArray &payload,
                     DnsInFlight &outInflight,
                     QByteArray &assembledOut);

    // Pop the in-flight entry without checking completeness. Useful for
    // cancellation paths (session reset, association teardown).
    std::optional<DnsInFlight> take(quint16 seq);

    // Sweep entries that have been pending longer than `ttlMs`.
    int sweepExpired(qint64 nowMs, qint64 ttlMs = kDefaultTtlMs);

    int size() const { return m_inFlight.size(); }
    void clear() { m_inFlight.clear(); }
    bool contains(quint16 seq) const { return m_inFlight.contains(seq); }

private:
    QHash<quint16, DnsInFlight> m_inFlight;
};

} // namespace amnezia::masterdnsvpn

#endif // MASTERDNSVPN_DNSCACHE_H
