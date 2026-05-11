// SPDX-License-Identifier: GPL-3.0-or-later
//
// Per-stream ARQ (Automatic Repeat reQuest) reliability layer.
//
// Each stream gets its own ArqStream instance. The instance manages a
// sliding window of in-flight `STREAM_DATA` packets, dispatches retransmits
// when an RTO fires, and reorders incoming packets through `rcvBuf` until
// they can be delivered contiguously to the application layer.
//
// **ArqStream is pure**: it owns no sockets, no timers, no Qt main-loop
// anything. Time is supplied by the caller (`tickMs`), and outbound packets
// are delivered to the caller via a `Sink` callback. This makes the state
// machine trivially unit-testable without faking I/O.
//
// The dispatcher (session.cpp, written later) is responsible for:
//   * calling tickMs(now) periodically (or on incoming data),
//   * routing the Sink-emitted packets to the resolver pool,
//   * delivering incoming wireframing::Packet objects to onPacketReceived().
//
// All algorithm-level constants follow §6 of docs/masterdnsvpn-wire-spec.md.

#ifndef MASTERDNSVPN_ARQ_H
#define MASTERDNSVPN_ARQ_H

#include "wireframing.h"

#include <QByteArray>
#include <QMap>
#include <QVector>
#include <cstdint>
#include <functional>

// Forward declaration so ArqStream's `friend class TestMasterDnsVpnEngine`
// resolves. The test class lives in the default namespace (it's the QTest
// main class in client/tests/testMasterDnsVpnEngine.cpp).
class TestMasterDnsVpnEngine;

namespace amnezia::masterdnsvpn {

// Tunable knobs lifted from operator config (server policy clamps via
// SESSION_ACCEPT — caller applies clamping before passing the struct in).
struct ArqConfig {
    int windowSize = 1000;          // ARQ_WINDOW_SIZE; floor 300 inside ctor
    qint64 initialDataRtoMs = 500;
    qint64 maxDataRtoMs = 3000;
    qint64 initialControlRtoMs = 500;
    qint64 maxControlRtoMs = 2000;
    int maxDataRetries = 1200;       // ARQ_MAX_DATA_RETRIES floor
    int maxControlRetries = 400;     // ARQ_MAX_CONTROL_RETRIES floor
    qint64 inactivityTimeoutMs = 1'800'000;
    qint64 dataPacketTtlMs = 2'400'000;
    qint64 controlPacketTtlMs = 1'200'000;
    int dataNackMaxGap = 32;         // ARQ_DATA_NACK_MAX_GAP
    qint64 dataNackInitialDelayMs = 0; // upstream Config zero default; raise via cfg if desired
    qint64 dataNackRepeatMs = 800;
    qint64 terminalDrainMs = 60'000;
    qint64 terminalAckWaitMs = 30'000;

    // Upstream-parity knobs (mirrors `internal/arq/arq.go::Config`). These
    // are referenced by the translated test suite; the C++ engine doesn't
    // yet act on all of them — failing tests document the implementation
    // gap rather than being silently masked.
    bool enableControlReliability = false;
    bool isClient = false;
    bool isVirtual = false;
    bool startPaused = false;
    quint8 compressionType = 0;
};

// Outgoing packet emitted by the ARQ machine. The dispatcher wraps this in
// crypto + DNS framing and ships it via the resolver pool.
struct ArqOutbound {
    Packet packet;
    bool isRetransmit = false; // emitted as STREAM_RESEND vs STREAM_DATA when applicable
};

// Bytes ready for delivery to the application layer (SOCKS5 socket).
// Always in stream order; the ARQ instance buffers out-of-order packets
// in `rcvBuf` until contiguous prefix is available.
struct ArqDelivery {
    QByteArray bytes;
    bool endOfStream = false; // signalled when the peer half-closed write
};

// Stream-level lifecycle state (§6.6).
enum class ArqState {
    Open,
    HalfClosedLocal,   // sent CLOSE_READ
    HalfClosedRemote,  // received CLOSE_READ
    Closing,
    Draining,
    TimeWait,
    Reset,
    Closed,
};

class ArqStream
{
    // The upstream Go test suite (`internal/arq/arq_test.go`) lives in the
    // same package as the implementation and freely manipulates internal
    // state (sndBuf, sndNxt, rcvNxt, dataNackInitialDelay, etc.). To
    // translate those tests faithfully — without "tailoring tests to fit
    // our existing code", per project policy — the QTest harness needs
    // equivalent access. `TestMasterDnsVpnEngine` is in the default
    // namespace (the test file's class), forward-declared in tests.
    friend class ::TestMasterDnsVpnEngine;

public:
    // Sink receives outbound packets. The dispatcher is responsible for
    // adding session id / cookie before transmission — we set sessionId=0
    // here as a placeholder.
    using Sink = std::function<void(const ArqOutbound &)>;

    // Delivery sink fires whenever the contiguous receive prefix grows.
    using DeliverySink = std::function<void(const ArqDelivery &)>;

    ArqStream(quint16 streamId, const ArqConfig &cfg, Sink outboundSink, DeliverySink deliverySink);

    // ---- Application -> network ----
    // Append plaintext bytes to the send queue. Returns the number of
    // bytes accepted; partial accepts mean the caller must retry later
    // (backpressure when the send window is ≥ 80% full).
    qsizetype writeApp(const QByteArray &bytes);

    // Initiate a half-close (CLOSE_WRITE).
    void halfCloseWrite();

    // Initiate a hard reset (RST). After this, the stream is terminal.
    void reset();

    // ---- Network -> application ----
    // Feed an incoming wire-decoded packet to the state machine.
    void onPacketReceived(const Packet &pkt);

    // ---- Periodic tick ----
    // Call every ~50-200 ms (or on any state change). Drives the RTO
    // expiry, NACK throttling, terminal-drain watchdog. Caller supplies
    // the current monotonic-clock millisecond stamp.
    void tickMs(qint64 nowMs);

    quint16 streamId() const { return m_streamId; }
    ArqState state() const { return m_state; }
    bool isTerminal() const;

    // Diagnostic: number of unacked packets currently on the wire.
    int inFlightCount() const;

    // ---- Upstream-API-shaped wrappers ----
    //
    // These thin wrappers expose the same external API names the Go
    // reference uses (`internal/arq/arq.go`), so the translated test
    // suite can call them as upstream does. They dispatch to the
    // existing private logic.

    // Equivalent of upstream `ARQ.ReceiveData(sn, data)` — feeds a
    // STREAM_DATA packet (with the given sequence + payload) through
    // the state machine.
    bool ReceiveData(quint16 sn, const QByteArray &data);

    // Equivalent of upstream `ARQ.ReceiveAck(packetType, sn)` — feeds
    // a STREAM_DATA_ACK (or compatible) for the given sequence.
    bool ReceiveAck(PacketType packetType, quint16 sn);

    // Equivalent of upstream `ARQ.HandleDataNack(sn)` — handles an
    // inbound STREAM_DATA_NACK for the given sequence; returns true
    // when a resend was scheduled, false when suppressed by cooldown.
    bool HandleDataNack(quint16 sn);

    // Equivalent of upstream `ARQ.Start()` / `ARQ.Close(reason, opts)`.
    // The C++ engine is purely synchronous on the Qt event loop, so
    // these are no-ops; they exist for translation parity.
    void Start() {}
    struct CloseOptions { bool Force = false; bool SendRST = false; };
    void Close(const QString & /*reason*/, const CloseOptions & /*opts*/) {}

    // Equivalent of upstream `ARQ.HandleAckPacket(packetType, sn, fragId)`.
    // Routes the ack through the type-specific handler (data vs control).
    bool HandleAckPacket(PacketType packetType, quint16 sn, quint8 fragmentId);

    // Equivalent of upstream `ARQ.ReceiveControlAck(ackType, sn, fragId)`.
    bool ReceiveControlAck(PacketType ackPacketType, quint16 sn, quint8 fragmentId);

    // Mark the peer's half-close signals as received (mirrors upstream's
    // `Mark*Received` family — internal/arq/arq.go:775,819).
    void MarkCloseReadReceived();
    void MarkCloseWriteReceived();
    void MarkRstReceived();

    // Mirrors upstream `IsClosed()` / `IsReset()` (arq.go:312, 618).
    bool IsClosed() const { return m_state == ArqState::Closed; }
    bool IsReset() const  { return m_state == ArqState::Reset; }

    // Mirrors upstream `HasPendingSequence(sn)` (arq.go:324).
    bool HasPendingSequence(quint16 sn) const { return m_sndBuf.contains(sn); }

    // Mirrors upstream `noteDataNackSent` — records that we sent a NACK
    // for `sn` so cooldown logic suppresses duplicates.
    void noteDataNackSent(quint16 sn, qint64 nowMs);

    // Mirrors upstream `clearAllQueues(includeDataNacks)` — wipes the
    // per-stream NACK / send buffers. Used by tests verifying that
    // teardown drops remembered NACK state.
    void clearAllQueues(bool includeDataNacks);

private:
    Q_DISABLE_COPY_MOVE(ArqStream)

    struct PendingSend {
        quint16 seq;
        QByteArray payload;
        PacketType type = PacketType::StreamData;
        qint64 firstSentMs = 0;
        qint64 lastSentMs = 0;
        // Last time we emitted a STREAM_RESEND for this seq in response to an
        // inbound STREAM_DATA_NACK (HandleDataNack). Used as the per-seq
        // cooldown gate so back-to-back NACKs don't flood resends. Mirrors
        // upstream arqDataItem.LastNackSentAt (internal/arq/arq.go:1886).
        qint64 lastNackSentMs = 0;
        int retries = 0;
        bool sampleEligible = true; // per §6.4 — false after a retransmit
    };

    struct PendingReceive {
        quint16 seq;
        QByteArray payload;
    };

    void dispatch(const Packet &pkt, bool retransmit);
    void emitDataAck(quint16 seq);
    void emitControl(PacketType type, quint16 seq);
    void scheduleRetransmits(qint64 nowMs);

    // Control-plane analogue of scheduleRetransmits: walks
    // m_controlSndBuf, re-emits entries whose RTO has expired, and
    // grows the control-RTO on each retransmit. Bounded by
    // m_cfg.maxControlRetries — stream is reset when exceeded.
    void scheduleControlRetransmits(qint64 nowMs);
    void deliverContiguous();
    void onDataPacket(const Packet &pkt);
    void onAck(quint16 ackedSeq);
    void onCloseRead();
    void onCloseWrite();
    void onRst();
    void updateRttSample(qint64 sampleMs, bool isControl);
    qint64 currentRto(bool isControl) const;
    bool isAhead(quint16 sn, quint16 baseline) const;

    // §6.7 NACK-gap path. `sn` is the seq of the just-arrived out-of-order
    // packet; we compute the bounded list of still-missing seqs in the gap
    // [m_rcvNxt..sn) — either the full slice when the gap is within
    // dataNackMaxGap, or sample + frontier when it isn't — and emit a
    // STREAM_DATA_NACK per missing seq subject to the per-seq cooldown.
    void maybeSendDataNacks(quint16 sn, qint64 nowMs);

    // Decision oracle for a single missing seq. Mirrors upstream
    // `ARQ.shouldSendDataNack` (internal/arq/arq.go:1996). Side-effect: on
    // the first observation of a missing seq, records firstSeenMs[sn].
    bool shouldSendDataNack(quint16 sn, qint64 nowMs);

    // Drops firstSeen / lastSent entries for any seq strictly behind
    // `rcvNxt` (the gap is no longer relevant). Mirrors upstream
    // `pruneDataNackStateLocked` (arq.go:2026).
    void pruneDataNackStateLocked(quint16 rcvNxt);

    // Sequence wrap-around: returns true if `candidate` is strictly behind
    // `base` (i.e., `(base - candidate)` is in (0, 32768)). Mirrors
    // upstream `seqBehind` (arq.go:2022).
    static bool seqBehind(quint16 base, quint16 candidate);

    // Map an inbound control ACK packet type back to the originating
    // packet type it acks (SYN_ACK → SYN, CONNECTED_ACK → CONNECTED, …).
    // Returns std::nullopt for types that don't ack anything trackable.
    // Mirrors upstream `Enums.ReverseControlAckFor`.
    static std::optional<PacketType> reverseControlAckFor(PacketType ackType);

    // Track that a reliable control packet was just dispatched, so the
    // matching ACK can drop the entry and feed an RTT sample. No-op when
    // `enableControlReliability` is false (upstream parity).
    void trackControlSent(PacketType type, quint16 seq, quint8 fragId);

    // Composite key into m_controlSndBuf: PacketType << 24 | seq << 8 |
    // fragId. Mirrors upstream's controlSndBuf keying.
    static quint32 controlKey(PacketType type, quint16 seq, quint8 fragId)
    {
        return (static_cast<quint32>(type) << 24)
                | (static_cast<quint32>(seq) << 8)
                | static_cast<quint32>(fragId);
    }

    quint16 m_streamId;
    ArqConfig m_cfg;
    Sink m_sink;
    DeliverySink m_deliver;

    ArqState m_state = ArqState::Open;
    qint64 m_lastActivityMs = 0;

    // Send-side state
    quint16 m_sndNxt = 1;          // initial sequence per spec (clients use 1+)
    QMap<quint16, PendingSend> m_sndBuf; // seq -> pending

    // Outstanding control packets awaiting their type-specific ACK
    // (SYN/SYN_ACK, CONNECTED/CONNECTED_ACK, CLOSE_WRITE/_ACK etc.).
    // Keyed by (packetType << 24) | (seq << 8) | fragId to match upstream's
    // controlSndBuf encoding. The dispatcher seeds entries when sending
    // reliable control packets; receipt of the corresponding ack consumes
    // them and feeds an RTT sample to the control-plane EWMA.
    QMap<quint32, PendingSend> m_controlSndBuf;
    qint64 m_dataSrttMs = 0;
    qint64 m_dataRttvarMs = 0;
    qint64 m_currentDataRtoMs;
    qint64 m_controlSrttMs = 0;
    qint64 m_controlRttvarMs = 0;
    qint64 m_currentControlRtoMs;

    // Receive-side state
    //
    // m_rcvNxt: next contiguous in-order sequence number expected from the
    // peer. Defaults to 0 — peers begin emitting data with seq=0 after the
    // SYN handshake (matches upstream `ARQ.rcvNxt` zero default in
    // internal/arq/arq.go).
    quint16 m_rcvNxt = 0;
    QMap<quint16, PendingReceive> m_rcvBuf;

    // Per-missing-seq NACK throttle state. Values are real monotonic
    // millisecond timestamps (QDateTime::currentMSecsSinceEpoch()), not
    // sentinels. Mirrors upstream `firstDataNackSeen` / `lastDataNackSent`.
    QMap<quint16, qint64> m_firstDataNackSeenMs;
    QMap<quint16, qint64> m_lastNackSentMs;

    // Terminal lifecycle bookkeeping
    qint64 m_terminalStartMs = 0;
    bool m_remoteClosedWrite = false;
    bool m_localClosedWrite = false;
};

} // namespace amnezia::masterdnsvpn

#endif // MASTERDNSVPN_ARQ_H
