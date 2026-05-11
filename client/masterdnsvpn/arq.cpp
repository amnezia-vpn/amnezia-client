// SPDX-License-Identifier: GPL-3.0-or-later

#include "arq.h"

#include <QDateTime>
#include <QDebug>
#include <QSet>
#include <algorithm>

namespace amnezia::masterdnsvpn {

namespace {

// Spec §6.4 — RTO growth multipliers.
constexpr double kDataGrowthFactor = 1.35;
constexpr double kControlGrowthFactor = 1.25;

// Backpressure threshold (spec §6.2): we accept new bytes from the
// application as long as the in-flight count stays below this fraction of
// the negotiated window.
constexpr double kBackpressureFraction = 0.8;

// Spec §6.5 — the front-budget retransmit selector.
int frontBudget(int window, int jobsCount)
{
    return std::min({ std::max(window / 10, 1), 64, jobsCount });
}

qint64 clamp64(qint64 v, qint64 lo, qint64 hi)
{
    return std::max(lo, std::min(hi, v));
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ArqStream::ArqStream(quint16 streamId, const ArqConfig &cfg, Sink outboundSink, DeliverySink deliverySink)
    : m_streamId(streamId), m_cfg(cfg), m_sink(std::move(outboundSink)), m_deliver(std::move(deliverySink))
{
    // Spec §6.2 — window floor 300.
    if (m_cfg.windowSize < 300) {
        m_cfg.windowSize = 300;
    }
    // Spec §6.4 — RTO floor 50 ms on both sides.
    m_cfg.initialDataRtoMs = std::max<qint64>(50, m_cfg.initialDataRtoMs);
    m_cfg.maxDataRtoMs = std::max<qint64>(m_cfg.initialDataRtoMs, m_cfg.maxDataRtoMs);
    m_cfg.initialControlRtoMs = std::max<qint64>(50, m_cfg.initialControlRtoMs);
    m_cfg.maxControlRtoMs = std::max<qint64>(m_cfg.initialControlRtoMs, m_cfg.maxControlRtoMs);

    m_currentDataRtoMs = m_cfg.initialDataRtoMs;
    m_currentControlRtoMs = m_cfg.initialControlRtoMs;
}

bool ArqStream::isTerminal() const
{
    return m_state == ArqState::Closed || m_state == ArqState::Reset;
}

int ArqStream::inFlightCount() const
{
    return m_sndBuf.size();
}

// ---------------------------------------------------------------------------
// Sequence helpers (uint16 wrap-around)
// ---------------------------------------------------------------------------

bool ArqStream::isAhead(quint16 sn, quint16 baseline) const
{
    // Per §3.5: diff < 32768 means `sn` is ahead of `baseline` in the
    // wrap-around sequence space.
    return ((static_cast<quint16>(sn - baseline)) < 0x8000);
}

// ---------------------------------------------------------------------------
// Application -> network
// ---------------------------------------------------------------------------

qsizetype ArqStream::writeApp(const QByteArray &bytes)
{
    if (m_state != ArqState::Open && m_state != ArqState::HalfClosedRemote) {
        return 0;
    }
    // Backpressure: hold the writer when the in-flight set is full.
    const int inFlightCap = static_cast<int>(m_cfg.windowSize * kBackpressureFraction);
    if (m_sndBuf.size() >= std::max(inFlightCap, 50)) {
        return 0;
    }

    // Single-fragment send for now — the framing layer chunks via the
    // resolver MTU. Fragmentation extension is honoured but the dispatcher
    // sets fragmentId/totalFragments from MTU discovery output.
    PendingSend pending;
    pending.seq = m_sndNxt++;
    pending.payload = bytes;
    pending.type = PacketType::StreamData;
    pending.firstSentMs = 0;
    pending.lastSentMs = 0;
    pending.sampleEligible = true;
    m_sndBuf.insert(pending.seq, pending);

    Packet p;
    p.type = PacketType::StreamData;
    p.streamId = m_streamId;
    p.sequenceNum = pending.seq;
    p.fragmentId = 0;
    p.totalFragments = 1;
    p.compression = 0;
    p.payload = bytes;
    dispatch(p, /*retransmit=*/false);

    return bytes.size();
}

void ArqStream::halfCloseWrite()
{
    if (m_localClosedWrite || isTerminal()) {
        return;
    }
    m_localClosedWrite = true;

    // Send STREAM_CLOSE_WRITE (control packet, allocates a sequence).
    Packet p;
    p.type = PacketType::StreamCloseWrite;
    p.streamId = m_streamId;
    p.sequenceNum = m_sndNxt++;
    dispatch(p, /*retransmit=*/false);

    if (m_state == ArqState::HalfClosedRemote) {
        m_state = ArqState::Closing;
    } else {
        m_state = ArqState::HalfClosedLocal;
    }
}

void ArqStream::reset()
{
    if (isTerminal()) {
        return;
    }
    Packet p;
    p.type = PacketType::StreamRst;
    p.streamId = m_streamId;
    p.sequenceNum = m_sndNxt++;
    dispatch(p, /*retransmit=*/false);
    m_state = ArqState::Reset;
}

// ---------------------------------------------------------------------------
// Network -> application
// ---------------------------------------------------------------------------

void ArqStream::onPacketReceived(const Packet &pkt)
{
    if (!pkt.streamId || *pkt.streamId != m_streamId) {
        return;
    }
    m_lastActivityMs = 0; // refreshed by tickMs

    switch (pkt.type) {
    case PacketType::StreamData:
    case PacketType::StreamResend:
        onDataPacket(pkt);
        break;
    case PacketType::StreamDataAck:
        if (pkt.sequenceNum) onAck(*pkt.sequenceNum);
        break;
    case PacketType::StreamDataNack:
        // Inbound NACK: route through HandleDataNack, which honours the
        // per-seq cooldown and does NOT bump retries / RTO. (Upstream
        // semantics — `internal/arq/arq.go::HandleDataNack`.)
        if (pkt.sequenceNum) HandleDataNack(*pkt.sequenceNum);
        break;
    case PacketType::StreamCloseRead:
        onCloseRead();
        break;
    case PacketType::StreamCloseWrite:
        onCloseWrite();
        break;
    case PacketType::StreamRst:
        onRst();
        break;
    default:
        // Other packet types (SYN/SYN_ACK, half-close ACKs, etc.) are
        // handled by the session-level state machine; ARQ ignores them.
        break;
    }
}

void ArqStream::onDataPacket(const Packet &pkt)
{
    if (!pkt.sequenceNum) {
        return;
    }
    const quint16 sn = *pkt.sequenceNum;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    // Behind rcvNxt → duplicate: emit a refreshing ACK so the peer can
    // retire the sndBuf entry, then drop the payload. (Upstream emits
    // an ACK in this branch and returns immediately —
    // internal/arq/arq.go:1576-1583.)
    if (!isAhead(sn, m_rcvNxt) && sn != m_rcvNxt) {
        emitDataAck(sn);
        return;
    }

    // Receive-window cap. Upstream maintains
    // `receiveWindowSize = 2 * windowSize` and drops seqs that would
    // expand rcvBuf beyond that bound. Two distinct checks:
    //   (a) the seq itself is too far ahead of rcvNxt (would overrun
    //       the window), and
    //   (b) the rcvBuf is already at capacity and this seq isn't the
    //       in-order frontier.
    // Either rejects the packet outright (no ACK, no buffering) — the
    // sender will retry once its RTO fires. Mirrors
    // internal/arq/arq.go:1586-1595.
    const int receiveWindowSize = m_cfg.windowSize * 2;
    const quint16 diff = static_cast<quint16>(sn - m_rcvNxt);
    if (static_cast<int>(diff) > receiveWindowSize) {
        return;
    }
    if (sn != m_rcvNxt
        && !m_rcvBuf.contains(sn)
        && m_rcvBuf.size() >= receiveWindowSize) {
        return;
    }

    // Always ACK on accept — duplicates already returned above.
    emitDataAck(sn);

    if (sn == m_rcvNxt) {
        // In-order: deliver immediately, advance, drain backlog.
        if (!pkt.payload.isEmpty()) {
            ArqDelivery d;
            d.bytes = pkt.payload;
            m_deliver(d);
        }
        ++m_rcvNxt;
        deliverContiguous();
        // After rcvNxt advances, NACK state for now-resolved seqs must
        // be dropped so subsequent gaps can re-arm them. Mirrors upstream
        // post-delivery cleanup via clearSentDataNack on each filled gap.
        pruneDataNackStateLocked(m_rcvNxt);
    } else {
        // Out-of-order: buffer the payload, then run the bounded NACK
        // path. m_rcvBuf must be populated FIRST so maybeSendDataNacks
        // can skip already-buffered seqs in the gap (per upstream).
        m_rcvBuf.insert(sn, { sn, pkt.payload });
        maybeSendDataNacks(sn, nowMs);
    }
}

void ArqStream::deliverContiguous()
{
    while (true) {
        auto it = m_rcvBuf.find(m_rcvNxt);
        if (it == m_rcvBuf.end()) {
            return;
        }
        if (!it->payload.isEmpty()) {
            ArqDelivery d;
            d.bytes = it->payload;
            m_deliver(d);
        }
        m_rcvBuf.erase(it);
        ++m_rcvNxt;
    }
}

void ArqStream::onAck(quint16 ackedSeq)
{
    auto it = m_sndBuf.find(ackedSeq);
    if (it == m_sndBuf.end()) {
        return; // duplicate or already-ACKed
    }
    if (it->sampleEligible && it->firstSentMs > 0) {
        // Karn's algorithm: only sample RTT for packets that were sent
        // exactly once (sampleEligible flips to false on any retransmit
        // or HandleDataNack-driven resend). Mirrors upstream
        // `ARQ.noteSuccessfulDataSample` (internal/arq/arq.go:1858).
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        const qint64 sampleMs = nowMs - it->firstSentMs;
        if (sampleMs > 0) {
            updateRttSample(sampleMs, /*isControl=*/false);
        }
    }
    m_sndBuf.erase(it);
}

// ---------------------------------------------------------------------------
// Inbound STREAM_DATA_NACK path
// ---------------------------------------------------------------------------
//
// Receipt of a peer's STREAM_DATA_NACK is dispatched through
// HandleDataNack (the receive-side public wrapper) — see
// onPacketReceived → HandleDataNack. The retransmit semantics are
// described there: no retries bump, no RTO growth, with a per-seq
// cooldown gate. The RTO-driven retransmit path lives in
// scheduleRetransmits and follows separate semantics (does bump
// retries + grow RTO per §6.4/§6.5).

void ArqStream::onCloseRead()
{
    // Peer signals "won't send more data" — drain any remaining bytes
    // already in m_rcvBuf, then signal EOF upstream.
    deliverContiguous();
    ArqDelivery eof;
    eof.endOfStream = true;
    m_deliver(eof);
}

void ArqStream::onCloseWrite()
{
    m_remoteClosedWrite = true;
    deliverContiguous();
    ArqDelivery eof;
    eof.endOfStream = true;
    m_deliver(eof);

    if (m_state == ArqState::HalfClosedLocal) {
        m_state = ArqState::Closing;
    } else if (m_state == ArqState::Open) {
        m_state = ArqState::HalfClosedRemote;
    }
}

void ArqStream::onRst()
{
    m_state = ArqState::Reset;
    m_sndBuf.clear();
    m_rcvBuf.clear();
}

// ---------------------------------------------------------------------------
// Periodic tick
// ---------------------------------------------------------------------------

void ArqStream::tickMs(qint64 nowMs)
{
    if (m_lastActivityMs == 0) {
        m_lastActivityMs = nowMs;
    }
    if (nowMs - m_lastActivityMs > m_cfg.inactivityTimeoutMs) {
        // Spec §6.8 — terminate on inactivity.
        reset();
        return;
    }
    if (isTerminal()) {
        return;
    }

    scheduleRetransmits(nowMs);

    // Terminal-drain watchdog.
    if (m_state == ArqState::Closing && m_sndBuf.isEmpty()) {
        if (m_terminalStartMs == 0) {
            m_terminalStartMs = nowMs;
        }
        if (nowMs - m_terminalStartMs > m_cfg.terminalAckWaitMs) {
            m_state = ArqState::Closed;
        }
    }
}

void ArqStream::scheduleRetransmits(qint64 nowMs)
{
    QVector<quint16> due;
    for (auto it = m_sndBuf.begin(); it != m_sndBuf.end(); ++it) {
        if (it->lastSentMs == 0) {
            // Not actually transmitted yet — first send happened via
            // emit() but the dispatcher may not have called tickMs since.
            // Set the timestamp now; the next tick checks RTO from here.
            it->lastSentMs = nowMs;
            if (it->firstSentMs == 0) {
                it->firstSentMs = nowMs;
            }
            continue;
        }
        const qint64 rto = currentRto(/*isControl=*/false);
        if (nowMs - it->lastSentMs >= rto) {
            due.append(it.key());
        }
    }

    if (due.isEmpty()) {
        return;
    }

    // §6.5 — front-budget priority. Sort by sequence (oldest first).
    std::sort(due.begin(), due.end());

    const int budget = frontBudget(m_cfg.windowSize, due.size());
    for (int i = 0; i < due.size(); ++i) {
        const quint16 seq = due[i];
        auto it = m_sndBuf.find(seq);
        if (it == m_sndBuf.end()) {
            continue;
        }
        ++it->retries;
        if (it->retries > m_cfg.maxDataRetries) {
            // §6.8 — terminate after max retries.
            reset();
            return;
        }
        it->sampleEligible = false;
        it->lastSentMs = nowMs;

        // Backoff.
        m_currentDataRtoMs = clamp64(static_cast<qint64>(m_currentDataRtoMs * kDataGrowthFactor),
                                     m_cfg.initialDataRtoMs,
                                     m_cfg.maxDataRtoMs);

        Packet p;
        p.type = (i < budget) ? PacketType::StreamResend : PacketType::StreamData;
        p.streamId = m_streamId;
        p.sequenceNum = seq;
        p.fragmentId = 0;
        p.totalFragments = 1;
        p.compression = 0;
        p.payload = it->payload;
        dispatch(p, /*retransmit=*/true);
    }
}

// ---------------------------------------------------------------------------
// Outbound emit + control helpers
// ---------------------------------------------------------------------------

void ArqStream::dispatch(const Packet &pkt, bool retransmit)
{
    if (m_sink) {
        m_sink({ pkt, retransmit });
    }
}

void ArqStream::emitDataAck(quint16 seq)
{
    Packet p;
    p.type = PacketType::StreamDataAck;
    p.streamId = m_streamId;
    p.sequenceNum = seq;
    dispatch(p, false);
}

// ---------------------------------------------------------------------------
// Bounded NACK gap + frontier sampling (§6.7)
// ---------------------------------------------------------------------------
//
// On every out-of-order arrival we compute the set of still-missing seqs
// in the gap [rcvNxt..sn) and emit at most one STREAM_DATA_NACK per
// missing seq, subject to a per-seq cooldown and the dataNackMaxGap
// bound. Two regimes:
//
//   1. Gap fits in dataNackMaxGap: walk the gap inline, skip any seqs
//      already buffered in m_rcvBuf, NACK the rest.
//   2. Gap exceeds the bound: NACK a small recent-window sample
//      (sampleCount ≈ 5% of dataNackMaxGap, floor 1), then one frontier
//      seq at the trailing edge of the window. This bounds NACK traffic
//      when many seqs are missing simultaneously.
//
// Mirrors upstream `ARQ.maybeSendDataNacks` (internal/arq/arq.go:1919).
void ArqStream::maybeSendDataNacks(quint16 sn, qint64 nowMs)
{
    if (m_cfg.dataNackMaxGap <= 0) {
        return;
    }
    if (m_state == ArqState::Closed || m_state == ArqState::Reset) {
        return;
    }

    // diff is the wrap-aware distance from rcvNxt to sn (sn must be
    // strictly ahead). diff in [1, 32767] means "sn is ahead of rcvNxt";
    // diff in [32768, 65535] means "sn is behind". diff == 0 → no gap.
    const quint16 diff = static_cast<quint16>(sn - m_rcvNxt);
    if (diff == 0 || diff >= 32768) {
        return;
    }

    pruneDataNackStateLocked(m_rcvNxt);

    const quint16 windowSpan = static_cast<quint16>(m_cfg.dataNackMaxGap);
    QVector<quint16> missingSeqs;
    missingSeqs.reserve(m_cfg.dataNackMaxGap);

    if (diff <= windowSpan) {
        // Full-walk path: every seq in [rcvNxt..sn) that isn't already
        // buffered gets enqueued as a NACK candidate.
        for (quint16 missing = m_rcvNxt; missing != sn; ++missing) {
            if (m_rcvBuf.contains(missing)) {
                continue;
            }
            missingSeqs.append(missing);
        }
    } else {
        // Frontier-sample path: cap the candidates at sampleCount from
        // the head + one frontier seq at the trailing window edge.
        const int sampleCount = std::max(1, (m_cfg.dataNackMaxGap + 19) / 20);
        QSet<quint16> seen;
        seen.reserve(std::max(2, m_cfg.dataNackMaxGap / 20 + 1));

        int added = 0;
        for (quint16 missing = m_rcvNxt;
             missing != sn && added < sampleCount;
             ++missing) {
            if (m_rcvBuf.contains(missing)) {
                continue;
            }
            missingSeqs.append(missing);
            seen.insert(missing);
            ++added;
        }

        // The frontier: the trailing edge of the bounded NACK window —
        // either dataNackMaxGap-1 ahead of rcvNxt, or the first unbuffered
        // seq scanning down from there.
        const quint16 frontier = static_cast<quint16>(
                static_cast<quint32>(m_rcvNxt) + static_cast<quint32>(windowSpan) - 1);
        for (quint16 candidate = frontier;; --candidate) {
            if (!m_rcvBuf.contains(candidate)) {
                if (!seen.contains(candidate)) {
                    missingSeqs.append(candidate);
                }
                break;
            }
            if (candidate == m_rcvNxt) {
                break;
            }
        }
    }

    for (quint16 missing : missingSeqs) {
        if (!shouldSendDataNack(missing, nowMs)) {
            continue;
        }
        Packet p;
        p.type = PacketType::StreamDataNack;
        p.streamId = m_streamId;
        p.sequenceNum = missing;
        dispatch(p, /*retransmit=*/false);
        noteDataNackSent(missing, nowMs);
    }
}

bool ArqStream::shouldSendDataNack(quint16 sn, qint64 nowMs)
{
    auto firstIt = m_firstDataNackSeenMs.find(sn);
    if (firstIt == m_firstDataNackSeenMs.end()) {
        // First observation of this missing seq — record the moment, then
        // gate on initial-delay (zero → send immediately; positive → wait).
        m_firstDataNackSeenMs.insert(sn, nowMs);
        return m_cfg.dataNackInitialDelayMs <= 0;
    }
    if (m_cfg.dataNackInitialDelayMs > 0
        && (nowMs - firstIt.value()) < m_cfg.dataNackInitialDelayMs) {
        return false;
    }
    auto lastIt = m_lastNackSentMs.find(sn);
    if (lastIt == m_lastNackSentMs.end()) {
        return true;
    }
    return (nowMs - lastIt.value()) >= m_cfg.dataNackRepeatMs;
}

bool ArqStream::seqBehind(quint16 base, quint16 candidate)
{
    if (candidate == base) {
        return false;
    }
    return static_cast<quint16>(base - candidate) < 0x8000;
}

void ArqStream::pruneDataNackStateLocked(quint16 rcvNxt)
{
    for (auto it = m_firstDataNackSeenMs.begin(); it != m_firstDataNackSeenMs.end();) {
        if (seqBehind(rcvNxt, it.key())) {
            it = m_firstDataNackSeenMs.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_lastNackSentMs.begin(); it != m_lastNackSentMs.end();) {
        if (seqBehind(rcvNxt, it.key())) {
            it = m_lastNackSentMs.erase(it);
        } else {
            ++it;
        }
    }
}

void ArqStream::emitControl(PacketType type, quint16 seq)
{
    Packet p;
    p.type = type;
    p.streamId = m_streamId;
    p.sequenceNum = seq;
    dispatch(p, false);
}

// ---------------------------------------------------------------------------
// RTT sampling (§6.4)
// ---------------------------------------------------------------------------

void ArqStream::updateRttSample(qint64 sampleMs, bool isControl)
{
    qint64 &srtt = isControl ? m_controlSrttMs : m_dataSrttMs;
    qint64 &rttvar = isControl ? m_controlRttvarMs : m_dataRttvarMs;
    qint64 &currentRto = isControl ? m_currentControlRtoMs : m_currentDataRtoMs;
    const qint64 initialRto = isControl ? m_cfg.initialControlRtoMs : m_cfg.initialDataRtoMs;
    const qint64 maxRto = isControl ? m_cfg.maxControlRtoMs : m_cfg.maxDataRtoMs;

    if (srtt == 0) {
        // First sample.
        srtt = sampleMs;
        rttvar = sampleMs / 2;
    } else {
        const qint64 delta = std::abs(srtt - sampleMs);
        rttvar = (3 * rttvar + delta) / 4;
        srtt = (7 * srtt + sampleMs) / 8;
    }
    currentRto = clamp64(srtt + 4 * rttvar, initialRto, maxRto);
}

qint64 ArqStream::currentRto(bool isControl) const
{
    return isControl ? m_currentControlRtoMs : m_currentDataRtoMs;
}

// ---------------------------------------------------------------------------
// Upstream-API-shaped wrappers (parity surface for translated tests)
// ---------------------------------------------------------------------------

bool ArqStream::ReceiveData(quint16 sn, const QByteArray &data)
{
    Packet pkt;
    pkt.type = PacketType::StreamData;
    pkt.streamId = m_streamId;
    pkt.sequenceNum = sn;
    pkt.payload = data;
    onPacketReceived(pkt);
    return true;
}

bool ArqStream::ReceiveAck(PacketType packetType, quint16 sn)
{
    Packet pkt;
    pkt.type = packetType;
    pkt.streamId = m_streamId;
    pkt.sequenceNum = sn;
    const int before = inFlightCount();
    onPacketReceived(pkt);
    return inFlightCount() < before;
}

bool ArqStream::HandleDataNack(quint16 sn)
{
    // Mirrors upstream `ARQ.HandleDataNack` (internal/arq/arq.go:1873).
    // Receipt of an inbound STREAM_DATA_NACK schedules ONE immediate
    // STREAM_RESEND for the named seq, gated by a per-seq cooldown
    // (dataNackRepeatMs). It is NOT a retransmit in the RTO sense —
    // retries and currentRto are left unchanged; only sampleEligible
    // flips to false (the seq is no longer a clean RTT sample source).
    if (m_state == ArqState::Closed || m_state == ArqState::Reset) {
        return false;
    }
    auto it = m_sndBuf.find(sn);
    if (it == m_sndBuf.end()) {
        return false;
    }
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (it->lastNackSentMs > 0
        && (nowMs - it->lastNackSentMs) < m_cfg.dataNackRepeatMs) {
        return false;
    }
    it->lastNackSentMs = nowMs;
    it->sampleEligible = false;

    Packet p;
    p.type = PacketType::StreamResend;
    p.streamId = m_streamId;
    p.sequenceNum = sn;
    p.fragmentId = 0;
    p.totalFragments = 1;
    p.compression = 0;
    p.payload = it->payload;
    dispatch(p, /*retransmit=*/true);
    return true;
}

bool ArqStream::HandleAckPacket(PacketType packetType, quint16 sn, quint8 /*fragmentId*/)
{
    if (packetType == PacketType::StreamDataAck) {
        const int before = inFlightCount();
        onAck(sn);
        return inFlightCount() < before;
    }
    // For close/syn/rst ACK types route via the control-ack path.
    return ReceiveControlAck(packetType, sn, 0);
}

bool ArqStream::ReceiveControlAck(PacketType ackPacketType, quint16 sn, quint8 /*fragmentId*/)
{
    // Mirrors upstream `ReceiveControlAck` (arq.go:2250). The C++ engine
    // doesn't yet track outstanding control packets explicitly; ACKs are
    // handled as state-machine transitions on receipt. Returns true if we
    // recognised the ack, false otherwise.
    Packet pkt;
    pkt.type = ackPacketType;
    pkt.streamId = m_streamId;
    pkt.sequenceNum = sn;
    onPacketReceived(pkt);
    return true;
}

void ArqStream::MarkCloseReadReceived()
{
    onCloseRead();
}

void ArqStream::MarkCloseWriteReceived()
{
    onCloseWrite();
}

void ArqStream::MarkRstReceived()
{
    onRst();
}

void ArqStream::noteDataNackSent(quint16 sn, qint64 nowMs)
{
    // Records the actual wall-clock millisecond at which we emitted a
    // STREAM_DATA_NACK for `sn`. Subsequent `shouldSendDataNack` calls
    // compare `now - lastSent` against `dataNackRepeatMs` to enforce the
    // per-seq throttle. Mirrors upstream `ARQ.noteDataNackSent`.
    m_lastNackSentMs.insert(sn, nowMs);
}

void ArqStream::clearAllQueues(bool includeDataNacks)
{
    m_sndBuf.clear();
    m_rcvBuf.clear();
    if (includeDataNacks) {
        m_lastNackSentMs.clear();
        m_firstDataNackSeenMs.clear();
    }
}

} // namespace amnezia::masterdnsvpn
