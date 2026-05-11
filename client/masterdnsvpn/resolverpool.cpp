// SPDX-License-Identifier: GPL-3.0-or-later

#include "resolverpool.h"

#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>
#include <QSet>
#include <QTimer>
#include <QUdpSocket>
#include <QtEndian>
#include <algorithm>
#include <atomic>

namespace amnezia::masterdnsvpn {

// ---------------------------------------------------------------------------
// Per-resolver connection state — one socket, plus rolling health counters.
// ---------------------------------------------------------------------------

class ResolverConnection : public QObject
{
    Q_OBJECT
public:
    ResolverConnection(int index, const ResolverSpec &spec, QObject *parent = nullptr)
        : QObject(parent), m_index(index), m_spec(spec)
    {
        m_socket = new QUdpSocket(this);
        connect(m_socket, &QUdpSocket::readyRead, this, &ResolverConnection::onReadyRead);
        m_socket->bind(QHostAddress(QHostAddress::AnyIPv4), 0);
    }

    int index() const { return m_index; }
    const ResolverSpec &spec() const { return m_spec; }
    bool isActive() const { return m_active; }
    void setActive(bool a) { m_active = a; }

    int uploadMtu() const { return m_uploadMtu; }
    int downloadMtu() const { return m_downloadMtu; }
    void setUploadMtu(int m) { m_uploadMtu = m; }
    void setDownloadMtu(int m) { m_downloadMtu = m; }

    qint64 sent() const { return m_sent; }
    qint64 acked() const { return m_acked; }
    qint64 lost() const { return m_lost; }
    qint64 rttMicrosSum() const { return m_rttMicrosSum; }
    qint64 rttCount() const { return m_rttCount; }

    bool send(const QByteArray &bytes)
    {
        const qint64 written = m_socket->writeDatagram(bytes, m_spec.address, m_spec.port);
        if (written < 0) {
            return false;
        }
        ++m_sent;
        decayCounters();
        return true;
    }

    // Health counter updates — called by ResolverPool when ACKs / timeouts
    // are observed at higher layers (the inner ARQ / DNS-response timeout).
    void recordAcked(qint64 rttMicros)
    {
        ++m_acked;
        m_rttMicrosSum += rttMicros;
        ++m_rttCount;
        decayCounters();
    }

    void recordLost()
    {
        ++m_lost;
        decayCounters();
    }

    // Increment the "sent" counter without emitting a packet. Used by the
    // dispatcher feedback path (Balancer.ReportSend equivalent) so the
    // loss-score denominator is fed when the dispatcher tracks the send
    // path itself rather than calling send() here. The actual send()
    // method below also bumps m_sent inline.
    void markSent()
    {
        ++m_sent;
        decayCounters();
    }

signals:
    void incoming(int resolverIndex, quint16 transactionId, const QByteArray &bytes);

private slots:
    void onReadyRead()
    {
        while (m_socket->hasPendingDatagrams()) {
            const qint64 size = m_socket->pendingDatagramSize();
            QByteArray buf(size, '\0');
            const qint64 read = m_socket->readDatagram(buf.data(), buf.size());
            if (read <= 0 || buf.size() < 2) {
                continue;
            }
            const quint16 txId = qFromBigEndian<quint16>(buf.constData());
            emit incoming(m_index, txId, buf);
        }
    }

private:
    // §9.4 — when any counter exceeds 1000, halve all five for exponential
    // decay. Approximates a long-running EWMA without floating point.
    void decayCounters()
    {
        if (m_sent <= 1000 && m_acked <= 1000 && m_lost <= 1000 && m_rttCount <= 1000) {
            return;
        }
        m_sent /= 2;
        m_acked /= 2;
        m_lost /= 2;
        m_rttMicrosSum /= 2;
        m_rttCount /= 2;
    }

    int m_index = -1;
    ResolverSpec m_spec;
    QUdpSocket *m_socket = nullptr;
    bool m_active = true;

    int m_uploadMtu = 0;
    int m_downloadMtu = 0;

    qint64 m_sent = 0;
    qint64 m_acked = 0;
    qint64 m_lost = 0;
    qint64 m_rttMicrosSum = 0;
    qint64 m_rttCount = 0;
};

// ---------------------------------------------------------------------------
// ResolverPool
// ---------------------------------------------------------------------------

ResolverPool::ResolverPool(QObject *parent) : QObject(parent) {}

ResolverPool::~ResolverPool() = default;

bool ResolverPool::configure(const QVector<ResolverSpec> &resolvers, const Config &cfg)
{
    if (m_started) {
        qWarning("masterdnsvpn::ResolverPool: configure() called while running");
        return false;
    }
    if (resolvers.isEmpty()) {
        return false;
    }
    m_cfg = cfg;

    // Spec §9.6 — clamp duplication counts.
    m_cfg.packetDuplicationCount = std::clamp(m_cfg.packetDuplicationCount, 1, 10);
    m_cfg.setupPacketDuplicationCount = std::clamp(m_cfg.setupPacketDuplicationCount,
                                                   m_cfg.packetDuplicationCount, 12);

    m_connections.clear();
    m_connections.reserve(resolvers.size());
    for (int i = 0; i < resolvers.size(); ++i) {
        auto conn = std::make_unique<ResolverConnection>(i, resolvers[i]);
        connect(conn.get(), &ResolverConnection::incoming, this, &ResolverPool::onIncoming);
        m_connections.push_back(std::move(conn));
    }
    return true;
}

void ResolverPool::start()
{
    if (m_started) {
        return;
    }
    m_started = true;

    // Initial MTU baseline — conservative floors that let the dispatcher
    // ship SESSION_INIT and basic packets before §9 probing refines them.
    // Session::onSocketsBound runs the probe sweep, then calls
    // setSyncedMtu() to publish the discovered values.
    m_syncedUploadMtu = std::min(64, m_cfg.maxUploadMtu);
    m_syncedDownloadMtu = std::min(255, m_cfg.maxDownloadMtu);
    QTimer::singleShot(0, this, [this]() { emit socketsBound(); });
}

void ResolverPool::setSyncedMtu(int uploadMtu, int downloadMtu)
{
    // Floor-clamp against the conservative defaults so a partial probe
    // sweep (e.g. one resolver succeeded, others timed out) can never
    // narrow the working MTU below what we already knew was safe.
    m_syncedUploadMtu = std::max(m_syncedUploadMtu, std::min(uploadMtu, m_cfg.maxUploadMtu));
    m_syncedDownloadMtu = std::max(m_syncedDownloadMtu, std::min(downloadMtu, m_cfg.maxDownloadMtu));
    QTimer::singleShot(0, this, [this]() { emit readyForUse(); });
}

void ResolverPool::reportSend(int index)
{
    if (index < 0 || index >= m_connections.size()) {
        return;
    }
    m_connections[index]->markSent();
}

void ResolverPool::reportSuccess(int index, qint64 rttMicros)
{
    if (index < 0 || index >= m_connections.size()) {
        return;
    }
    m_connections[index]->recordAcked(rttMicros);
}

void ResolverPool::reportTimeout(int index)
{
    if (index < 0 || index >= m_connections.size()) {
        return;
    }
    m_connections[index]->recordLost();
}

qint64 ResolverPool::resolverSentForTesting(int index) const
{
    if (index < 0 || index >= m_connections.size()) return 0;
    return m_connections[index]->sent();
}

qint64 ResolverPool::resolverAckedForTesting(int index) const
{
    if (index < 0 || index >= m_connections.size()) return 0;
    return m_connections[index]->acked();
}

qint64 ResolverPool::resolverLostForTesting(int index) const
{
    if (index < 0 || index >= m_connections.size()) return 0;
    return m_connections[index]->lost();
}

qint64 ResolverPool::resolverRttCountForTesting(int index) const
{
    if (index < 0 || index >= m_connections.size()) return 0;
    return m_connections[index]->rttCount();
}

bool ResolverPool::resolverActive(int index) const
{
    if (index < 0 || index >= m_connections.size()) return false;
    return m_connections[index]->isActive();
}

void ResolverPool::markResolverInactive(int index)
{
    if (index < 0 || index >= m_connections.size()) {
        return;
    }
    auto &conn = m_connections[index];
    if (!conn->isActive()) {
        return;
    }
    conn->setActive(false);
    emit resolverStateChanged(index, false);
}

void ResolverPool::stop()
{
    if (!m_started) {
        return;
    }
    m_connections.clear();
    m_started = false;
    m_syncedUploadMtu = 0;
    m_syncedDownloadMtu = 0;
}

int ResolverPool::syncedUploadMtu() const
{
    return m_syncedUploadMtu;
}

int ResolverPool::syncedDownloadMtu() const
{
    return m_syncedDownloadMtu;
}

// ---- Picker ----

namespace {

// Loss score per §9.3 strategy 3 / 5.
//  - sent < 5 → 200 (probation default)
//  - else (lost*1000)/sent
int lossScore(const ResolverConnection &c)
{
    if (c.sent() < 5) return 200;
    if (c.sent() == 0) return 200;
    return static_cast<int>((c.lost() * 1000) / c.sent());
}

// Average RTT (in microseconds) per strategy 4 / 5.
int avgLatencyMicros(const ResolverConnection &c)
{
    if (c.rttCount() < 5) return 999'000;
    return static_cast<int>(c.rttMicrosSum() / c.rttCount());
}

} // namespace

ResolverPick ResolverPool::pickPrimary()
{
    QVector<ResolverConnection *> active;
    active.reserve(m_connections.size());
    for (auto &c : m_connections) {
        if (c->isActive()) {
            active.append(c.get());
        }
    }
    if (active.isEmpty()) {
        return {};
    }

    auto pickRandom = [&]() {
        const int idx = QRandomGenerator::global()->bounded(active.size());
        ResolverPick p;
        p.index = active[idx]->index();
        p.tunnelDomain = active[idx]->spec().tunnelDomain;
        return p;
    };

    auto pickRoundRobin = [&]() {
        const int idx = m_roundRobinCursor++ % active.size();
        ResolverPick p;
        p.index = active[idx]->index();
        p.tunnelDomain = active[idx]->spec().tunnelDomain;
        return p;
    };

    BalancingStrategy s = m_cfg.strategy;
    if (s == BalancingStrategy::Default) {
        s = BalancingStrategy::RoundRobin;
    }

    switch (s) {
    case BalancingStrategy::Default:
    case BalancingStrategy::RoundRobin:
        return pickRoundRobin();

    case BalancingStrategy::Random:
        return pickRandom();

    case BalancingStrategy::LeastLoss: {
        // Spec §9.3 strategy 3: lowest loss score. Fall back to round-robin
        // if no resolver has yet hit the 5-sample probation threshold.
        bool anyHasSamples = false;
        int bestScore = INT_MAX;
        ResolverConnection *best = nullptr;
        for (auto *c : active) {
            const int score = lossScore(*c);
            if (c->sent() >= 5) {
                anyHasSamples = true;
            }
            if (score < bestScore) {
                bestScore = score;
                best = c;
            }
        }
        if (!anyHasSamples || !best) {
            return pickRoundRobin();
        }
        return { best->index(), best->spec().tunnelDomain };
    }

    case BalancingStrategy::LowestLatency: {
        bool anyHasSamples = false;
        int bestRtt = INT_MAX;
        ResolverConnection *best = nullptr;
        for (auto *c : active) {
            const int rtt = avgLatencyMicros(*c);
            if (c->rttCount() >= 5) {
                anyHasSamples = true;
            }
            if (rtt < bestRtt) {
                bestRtt = rtt;
                best = c;
            }
        }
        if (!anyHasSamples || !best) {
            return pickRoundRobin();
        }
        return { best->index(), best->spec().tunnelDomain };
    }

    case BalancingStrategy::HybridScore: {
        // Spec §9.3 strategy 5: lossScore * 8 + clamp(latencyMs, 0, 1000).
        // Latency defaults to 200ms when unknown. Falls back to round-robin
        // until at least one resolver crosses the 5-sample probation
        // threshold (otherwise all scores tie at 1800 and we'd always pick
        // the first resolver — upstream's parity behavior is RR fallback).
        bool anyHasSamples = false;
        int bestScore = INT_MAX;
        ResolverConnection *best = nullptr;
        for (auto *c : active) {
            if (c->sent() >= 5 || c->rttCount() >= 5) {
                anyHasSamples = true;
            }
            const int loss = lossScore(*c);
            int latencyMs = (c->rttCount() < 5)
                    ? 200
                    : static_cast<int>((c->rttMicrosSum() / c->rttCount()) / 1000);
            latencyMs = std::clamp(latencyMs, 0, 1000);
            const int score = loss * 8 + latencyMs;
            if (score < bestScore) {
                bestScore = score;
                best = c;
            }
        }
        if (!anyHasSamples || !best) {
            return pickRoundRobin();
        }
        return { best->index(), best->spec().tunnelDomain };
    }

    case BalancingStrategy::LossThenLatency: {
        // Spec §9.3 strategy 6: shortlist by loss tolerance, then by latency
        // tolerance, then random pick.
        if (active.isEmpty()) return pickRoundRobin();
        int bestLoss = INT_MAX;
        for (auto *c : active) {
            bestLoss = std::min(bestLoss, lossScore(*c));
        }
        const int lossTolerance = (bestLoss < 200) ? 25 : 0;
        QVector<ResolverConnection *> shortlist;
        for (auto *c : active) {
            if (lossScore(*c) <= bestLoss + lossTolerance) {
                shortlist.append(c);
            }
        }
        if (shortlist.isEmpty()) {
            return pickRoundRobin();
        }
        int bestLatencyMs = INT_MAX;
        for (auto *c : shortlist) {
            const int latencyMs = (c->rttCount() < 5)
                    ? 200
                    : static_cast<int>((c->rttMicrosSum() / c->rttCount()) / 1000);
            bestLatencyMs = std::min(bestLatencyMs, latencyMs);
        }
        const int latencyTolerance = (bestLatencyMs < 200)
                ? std::clamp(bestLatencyMs / 4, 2, 25)
                : 0;
        QVector<ResolverConnection *> survivors;
        for (auto *c : shortlist) {
            const int latencyMs = (c->rttCount() < 5)
                    ? 200
                    : static_cast<int>((c->rttMicrosSum() / c->rttCount()) / 1000);
            if (latencyMs <= bestLatencyMs + latencyTolerance) {
                survivors.append(c);
            }
        }
        if (survivors.isEmpty()) {
            return pickRoundRobin();
        }
        const int idx = QRandomGenerator::global()->bounded(survivors.size());
        return { survivors[idx]->index(), survivors[idx]->spec().tunnelDomain };
    }

    case BalancingStrategy::LeastLossTopRandom:
    case BalancingStrategy::LeastLossTopRoundRobin: {
        // Spec §9.3 strategies 7/8. Sort by loss; take the top max(2, ⌈N/10⌉).
        // Falls back to plain round-robin when no resolver has crossed the
        // 5-sample probation threshold yet — matches upstream behavior.
        bool anyHasSamples = false;
        for (auto *c : active) {
            if (c->sent() >= 5) { anyHasSamples = true; break; }
        }
        if (!anyHasSamples) {
            return pickRoundRobin();
        }
        QVector<ResolverConnection *> sorted = active;
        std::sort(sorted.begin(), sorted.end(),
                  [](ResolverConnection *a, ResolverConnection *b) {
                      return lossScore(*a) < lossScore(*b);
                  });
        const int top = std::max<int>(2, (sorted.size() + 9) / 10);
        const int len = std::min(top, sorted.size());
        if (len == 0) {
            return {};
        }
        int idx = 0;
        if (s == BalancingStrategy::LeastLossTopRandom) {
            idx = QRandomGenerator::global()->bounded(len);
        } else {
            idx = m_roundRobinCursor++ % len;
        }
        return { sorted[idx]->index(), sorted[idx]->spec().tunnelDomain };
    }
    }
    return pickRoundRobin();
}

QVector<ResolverPick> ResolverPool::pickDuplicates(int count, bool setup)
{
    Q_UNUSED(setup);
    QVector<ResolverPick> picks;
    QSet<int> chosen;
    int attempts = 0;
    while (picks.size() < count && attempts < count * 4) {
        ++attempts;
        ResolverPick pick = pickPrimary();
        if (pick.index < 0) {
            break;
        }
        if (!chosen.contains(pick.index)) {
            chosen.insert(pick.index);
            picks.append(pick);
        }
    }
    return picks;
}

bool ResolverPool::send(int index, const QByteArray &queryBytes)
{
    if (index < 0 || index >= m_connections.size()) {
        return false;
    }
    const bool ok = m_connections[index]->send(queryBytes);
    recordSendResult(index, ok);
    return ok;
}

void ResolverPool::onIncoming(int index, const QByteArray &bytes, quint16 transactionId)
{
    Q_UNUSED(index);
    emit responseReceived(index, transactionId, bytes);
}

void ResolverPool::recordSendResult(int index, bool ok)
{
    if (!ok && index >= 0 && index < m_connections.size()) {
        // Single send failure → bump lost counter; auto-disable kicks in
        // when the window-tracker rolls (see ResolverConnection::recordLost).
        m_connections[index]->recordLost();
    }
}

} // namespace amnezia::masterdnsvpn

#include "resolverpool.moc"
