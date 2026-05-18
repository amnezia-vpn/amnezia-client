// SPDX-License-Identifier: GPL-3.0-or-later
//
// Engine — the public façade of the MasterDnsVPN native client engine.
//
// This is the only header that callers outside `client/masterdnsvpn/` are
// expected to include. It owns the full protocol stack:
//
//   SOCKS5 server (user apps connect here)
//        ↓
//   Stream multiplexer       ─┐
//        ↓                    │
//   ARQ reliability layer     │   "ProtocolCore"
//        ↓                    │   (cross-platform Qt-using C++)
//   Wire framing              │
//        ↓                    │
//   Crypto + (optional) compression
//        ↓                    │
//   DNS framing              ─┘
//        ↓
//   ResolverPool (multiple QUdpSockets, one per public DNS resolver)
//        ↓
//   Internet → operator's NS-delegated DNS server → server-side mdnsvpn
//
// Calling pattern from masterDnsVpnProtocol (desktop) and the JNI bridge
// (Android) is symmetric:
//
//     auto engine = std::make_unique<MasterDnsVpnEngine>();
//     QObject::connect(engine.get(), &MasterDnsVpnEngine::stateChanged, ...);
//     engine->start(configJson);
//     ...
//     engine->stop();
//
// The engine never spawns subprocesses, never bundles a foreign binary, and
// never calls into a non-Qt library beyond OpenSSL (already a dep) and zlib
// (already a dep). The wire-format implementation lives in sibling files
// (dnsframing.h / wireframing.h / arq.h / etc.); this header reveals nothing
// about it.

#ifndef MASTERDNSVPN_ENGINE_H
#define MASTERDNSVPN_ENGINE_H

#include <QJsonObject>
#include <QObject>
#include <QString>

#include <memory>

namespace amnezia::masterdnsvpn {

class EnginePrivate;

class Engine : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Idle,        // not started, or cleanly stopped
        Starting,    // start() called; doing handshake / MTU discovery
        Connected,   // SOCKS5 listener live, traffic flowing
        Stopping,    // stop() in progress
        Failed       // a fatal error stopped the engine; lastError() has detail
    };
    Q_ENUM(State)

    explicit Engine(QObject *parent = nullptr);
    ~Engine() override;

    // Spin up the protocol stack. `config` is the structured JSON the model
    // produces (see MasterDnsVpnProtocolConfig::toJson()); the engine pulls
    // out the fields it needs (domains, encryptionKey, encryptionMethod,
    // resolvers, listenPort, …). Returns false synchronously when the
    // config is structurally invalid (missing key, no domains, bad cipher
    // ID, …); asynchronous startup failures arrive via stateChanged(Failed)
    // with lastError() populated.
    bool start(const QJsonObject &config);

    // Tear down the protocol stack. Idempotent. After return the engine
    // may be reused via another start() call.
    void stop();

    State state() const;
    QString lastError() const;

    // Local SOCKS5 endpoint the engine is listening on once Connected.
    // Returns 0 when not listening yet.
    quint16 socksPort() const;

    // Best-effort traffic counters. May be 0 for transports that haven't
    // been measured yet; never negative.
    quint64 bytesReceived() const;
    quint64 bytesSent() const;

signals:
    void stateChanged(amnezia::masterdnsvpn::Engine::State newState);
    void bytesChanged(quint64 receivedDelta, quint64 sentDelta);

private:
    Q_DISABLE_COPY_MOVE(Engine)
    std::unique_ptr<EnginePrivate> d;
};

} // namespace amnezia::masterdnsvpn

#endif // MASTERDNSVPN_ENGINE_H
