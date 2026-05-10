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
    qint64 dataNackInitialDelayMs = 100;
    qint64 dataNackRepeatMs = 800;
    qint64 terminalDrainMs = 60'000;
    qint64 terminalAckWaitMs = 30'000;
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

private:
    Q_DISABLE_COPY_MOVE(ArqStream)

    struct PendingSend {
        quint16 seq;
        QByteArray payload;
        PacketType type = PacketType::StreamData;
        qint64 firstSentMs = 0;
        qint64 lastSentMs = 0;
        int retries = 0;
        bool sampleEligible = true; // per §6.4 — false after a retransmit
    };

    struct PendingReceive {
        quint16 seq;
        QByteArray payload;
    };

    void dispatch(const Packet &pkt, bool retransmit);
    void emitDataAck(quint16 seq);
    void emitDataNack(quint16 seq);
    void emitControl(PacketType type, quint16 seq);
    void scheduleRetransmits(qint64 nowMs);
    void deliverContiguous();
    void onDataPacket(const Packet &pkt);
    void onAck(quint16 ackedSeq);
    void onNack(quint16 nackedSeq);
    void onCloseRead();
    void onCloseWrite();
    void onRst();
    void updateRttSample(qint64 sampleMs, bool isControl);
    qint64 currentRto(bool isControl) const;
    bool isAhead(quint16 sn, quint16 baseline) const;

    quint16 m_streamId;
    ArqConfig m_cfg;
    Sink m_sink;
    DeliverySink m_deliver;

    ArqState m_state = ArqState::Open;
    qint64 m_lastActivityMs = 0;

    // Send-side state
    quint16 m_sndNxt = 1;          // initial sequence per spec (clients use 1+)
    QMap<quint16, PendingSend> m_sndBuf; // seq -> pending
    qint64 m_dataSrttMs = 0;
    qint64 m_dataRttvarMs = 0;
    qint64 m_currentDataRtoMs;
    qint64 m_controlSrttMs = 0;
    qint64 m_controlRttvarMs = 0;
    qint64 m_currentControlRtoMs;

    // Receive-side state
    quint16 m_rcvNxt = 1;
    QMap<quint16, PendingReceive> m_rcvBuf;
    QMap<quint16, qint64> m_lastNackSentMs; // throttling per missing seq

    // Terminal lifecycle bookkeeping
    qint64 m_terminalStartMs = 0;
    bool m_remoteClosedWrite = false;
    bool m_localClosedWrite = false;
};

} // namespace amnezia::masterdnsvpn

#endif // MASTERDNSVPN_ARQ_H
