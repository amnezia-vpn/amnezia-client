// SPDX-License-Identifier: GPL-3.0-or-later

#include "dnscache.h"

#include <algorithm>

namespace amnezia::masterdnsvpn {

// ---------------------------------------------------------------------
// DnsLocalCache
// ---------------------------------------------------------------------

DnsLocalCache::DnsLocalCache() = default;

void DnsLocalCache::setTtlMs(qint64 ttlMs)
{
    if (ttlMs > 0) {
        m_ttlMs = ttlMs;
    }
}

DnsCacheStatus DnsLocalCache::lookupOrCreatePending(const DnsCacheKey &key, qint64 nowMs)
{
    auto it = m_entries.find(key);
    if (it == m_entries.end()) {
        Entry e;
        e.status = Status::Pending;
        e.firstSeenMs = nowMs;
        e.expiresAtMs = nowMs + m_ttlMs;
        m_entries.insert(key, e);
        return DnsCacheStatus::Miss;
    }
    if (it->status == Status::Ready) {
        if (it->expiresAtMs <= nowMs) {
            // Stale — purge and re-create as pending.
            Entry e;
            e.status = Status::Pending;
            e.firstSeenMs = nowMs;
            e.expiresAtMs = nowMs + m_ttlMs;
            *it = e;
            return DnsCacheStatus::Miss;
        }
        return DnsCacheStatus::Ready;
    }
    return DnsCacheStatus::Pending;
}

QByteArray DnsLocalCache::readyResponseFor(const DnsCacheKey &key) const
{
    auto it = m_entries.find(key);
    if (it == m_entries.end() || it->status != Status::Ready) {
        return {};
    }
    return it->response;
}

void DnsLocalCache::setReady(const DnsCacheKey &key, const QByteArray &response, qint64 nowMs)
{
    auto it = m_entries.find(key);
    if (it == m_entries.end()) {
        Entry e;
        e.status = Status::Ready;
        e.response = response;
        e.firstSeenMs = nowMs;
        e.expiresAtMs = nowMs + m_ttlMs;
        m_entries.insert(key, e);
        return;
    }
    it->status = Status::Ready;
    it->response = response;
    it->expiresAtMs = nowMs + m_ttlMs;
}

int DnsLocalCache::sweepExpired(qint64 nowMs)
{
    int purged = 0;
    for (auto it = m_entries.begin(); it != m_entries.end();) {
        if (it->expiresAtMs <= nowMs) {
            it = m_entries.erase(it);
            ++purged;
        } else {
            ++it;
        }
    }
    return purged;
}

// ---------------------------------------------------------------------
// DnsReassemblyStore
// ---------------------------------------------------------------------

void DnsReassemblyStore::track(quint16 seq, const DnsInFlight &inflight)
{
    m_inFlight.insert(seq, inflight);
}

bool DnsReassemblyStore::addFragment(quint16 seq,
                                     quint8 fragId,
                                     quint8 total,
                                     const QByteArray &payload,
                                     DnsInFlight &outInflight,
                                     QByteArray &assembledOut)
{
    auto it = m_inFlight.find(seq);
    if (it == m_inFlight.end()) {
        return false;
    }
    // Single-fragment fast path: `total == 1` means the whole response
    // is in this packet. Skip the array dance.
    if (total <= 1) {
        outInflight = *it;
        assembledOut = payload;
        m_inFlight.erase(it);
        return true;
    }
    if (it->totalFragments == 0) {
        it->totalFragments = total;
        it->fragments.resize(total);
    }
    if (fragId >= total || fragId >= it->fragments.size()) {
        return false; // out-of-range; drop the fragment
    }
    if (it->fragments[fragId].isEmpty()) {
        it->fragments[fragId] = payload;
    }
    // Check completion: every slot must be non-empty (note: this means
    // empty-payload fragments aren't supported, which matches upstream
    // — DNS responses always have at least 12 bytes of header).
    for (const QByteArray &f : it->fragments) {
        if (f.isEmpty()) {
            return false;
        }
    }
    // Reassemble.
    QByteArray full;
    int totalSize = 0;
    for (const QByteArray &f : it->fragments) totalSize += f.size();
    full.reserve(totalSize);
    for (const QByteArray &f : it->fragments) full.append(f);
    outInflight = *it;
    assembledOut = full;
    m_inFlight.erase(it);
    return true;
}

std::optional<DnsInFlight> DnsReassemblyStore::take(quint16 seq)
{
    auto it = m_inFlight.find(seq);
    if (it == m_inFlight.end()) {
        return std::nullopt;
    }
    DnsInFlight result = *it;
    m_inFlight.erase(it);
    return result;
}

int DnsReassemblyStore::sweepExpired(qint64 nowMs, qint64 ttlMs)
{
    int purged = 0;
    for (auto it = m_inFlight.begin(); it != m_inFlight.end();) {
        if (nowMs - it->createdMs > ttlMs) {
            it = m_inFlight.erase(it);
            ++purged;
        } else {
            ++it;
        }
    }
    return purged;
}

} // namespace amnezia::masterdnsvpn
