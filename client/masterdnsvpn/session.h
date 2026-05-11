// SPDX-License-Identifier: GPL-3.0-or-later
//
// Session — the per-tunnel orchestrator. Owns:
//
//   * one ResolverPool (which owns the QUdpSockets to public resolvers),
//   * one Cipher pair (encrypt + decrypt with the operator-chosen method),
//   * one Socks5Server (local user-app traffic in),
//   * one ArqStream per active SOCKS5 connection,
//   * the SESSION_INIT handshake state + (sessionId, sessionCookie),
//   * the ping/keepalive timer + tiered pacing,
//   * the outstanding-DNS-query map (for matching responses to requests).
//
// The Session is the bridge between Engine (façade visible to the rest of
// Amnezia) and the lower-layer building blocks. Engine instantiates one
// Session per start() call; stop() destroys it.
//
// All work happens on a single Qt event loop (the engine's worker thread).
// No shared state between sessions; tearing one down + spinning a fresh
// one is the supported reset path.

#ifndef MASTERDNSVPN_SESSION_H
#define MASTERDNSVPN_SESSION_H

#include "arq.h"
#include "crypto.h"
#include "resolverpool.h"
#include "socks5server.h"

#include <QHash>
#include <QHostAddress>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <memory>

class QTcpSocket;

namespace amnezia::masterdnsvpn {

class Session : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Idle,
        Initialising,    // sockets up, MTU baseline known, doing SESSION_INIT
        Authenticating,  // SESSION_INIT sent, awaiting SESSION_ACCEPT
        Established,     // SOCKS5 listener up, traffic flowing
        TearingDown,
        Stopped,
        Failed,
    };
    Q_ENUM(State)

    explicit Session(QObject *parent = nullptr);
    ~Session() override;

    // Build everything from the structured JSON the model emits. Returns
    // false synchronously if the config is malformed; async failures arrive
    // via stateChanged(Failed).
    bool start(const QJsonObject &config);
    void stop();

    State state() const { return m_state; }
    QString lastError() const { return m_lastError; }
    quint16 socksPort() const;

    quint64 bytesReceived() const { return m_bytesRx; }
    quint64 bytesSent() const { return m_bytesTx; }

signals:
    void stateChanged(State newState);
    void bytesChanged(quint64 receivedDelta, quint64 sentDelta);

private:
    Q_DISABLE_COPY_MOVE(Session)

    // ---- Lifecycle ----
    void setState(State s);
    void fail(const QString &reason);
    void onResolverPoolReady();

    // ---- Outbound ----
    // Wraps a wireframing::Packet in encryption + DNS framing and ships it
    // through the resolver pool. Honours the configured packet-duplication
    // factor for normal vs setup packets.
    void sendPacket(const Packet &packet, bool isSetupPacket);

    // Encrypt the wire frame per the negotiated cipher method, prepend the
    // nonce, base-encode for label safety. Returns the encoded ASCII run
    // ready to feed into dnsframing::buildQuery().
    QByteArray sealAndEncode(const QByteArray &plaintext);

    // Inverse of sealAndEncode — base-decode then strip nonce + decrypt.
    // Returns std::nullopt on AEAD tag failure / decode failure.
    std::optional<QByteArray> decodeAndOpen(const QByteArray &encoded);

    // ---- Inbound ----
    void onResolverResponse(int resolverIndex, quint16 transactionId, const QByteArray &bytes);

    // Dispatch a decoded inner Packet to the right per-stream ARQ instance
    // or the session-level handler (SESSION_ACCEPT, PING, etc.).
    void onInnerPacket(const Packet &packet);

    // ---- SOCKS5 → tunnel glue ----
    // Called by Socks5Server for each accepted CONNECT. We allocate a
    // fresh stream id, send PACKET_SOCKS5_SYN, and bridge the TCP socket's
    // read traffic into the matching ArqStream.
    void onSocks5Accepted(QTcpSocket *socket, const Socks5Destination &dest);

    // Per-stream ArqStream::Sink callback — packs into outer wire frame.
    void onArqOutbound(quint16 streamId, const ArqOutbound &out);

    // Per-stream ArqStream::DeliverySink — push reassembled bytes to the
    // matching SOCKS5 client socket.
    void onArqDelivery(quint16 streamId, const ArqDelivery &delivery);

    // ---- Periodic tick (ARQ + ping pacing) ----
    void onTick();

    // ---- Handshake ----
    void sendSessionInit();
    void onSessionAccept(const Packet &packet);
    void onSessionBusy(const Packet &packet);

    // ---- Helpers ----
    quint16 allocStreamId();
    quint16 nextTransactionId();

    State m_state = State::Idle;
    QString m_lastError;

    // Operator config snapshot.
    QStringList m_tunnelDomains;
    QString m_encryptionPassphrase;
    CipherMethod m_cipherMethod = CipherMethod::None;
    QByteArray m_derivedKey;
    Cipher m_cipherSeal;
    Cipher m_cipherOpen;
    quint16 m_listenPort = 18000;
    Socks5Auth m_socks5Auth;
    int m_uploadCompression = 0;
    int m_downloadCompression = 0;

    // Components.
    std::unique_ptr<ResolverPool> m_resolvers;
    std::unique_ptr<Socks5Server> m_socks5;
    QTimer *m_tickTimer = nullptr;

    // Session id + cookie filled by SESSION_ACCEPT. Until then, packets use
    // (0, 0) for SESSION_INIT only.
    quint8 m_sessionId = 0;
    quint8 m_sessionCookie = 0;
    QByteArray m_initVerifyCode; // 4 bytes random; echoed back by server

    // Stream-id allocator + map of active streams.
    quint16 m_nextStreamId = 1;
    QHash<quint16, std::unique_ptr<ArqStream>> m_streams;
    QHash<quint16, QPointer<QTcpSocket>> m_streamSockets;

    // Outstanding DNS query map: dns-tx-id -> resolver index. Used by the
    // tick to time out queries that never see a response.
    QHash<quint16, int> m_outstandingQueries;
    quint16 m_dnsTxIdCounter = 0;

    // Stats.
    quint64 m_bytesRx = 0;
    quint64 m_bytesTx = 0;
};

} // namespace amnezia::masterdnsvpn

#endif // MASTERDNSVPN_SESSION_H
