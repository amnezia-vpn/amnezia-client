// SPDX-License-Identifier: GPL-3.0-or-later
//
// Local SOCKS5 listener — what the user's apps connect to. RFC 1928 / 1929
// (USERNAME/PASSWORD auth). Supports the CONNECT command only (no BIND, no
// UDP ASSOCIATE — neither makes sense for a DNS-tunnel transport).
//
// On accept, the listener parses the SOCKS handshake + the destination
// address, then hands the upgraded TCP socket to a `StreamSink` callback.
// The Engine wires that sink to the multiplexer/ARQ stack; this file knows
// nothing about the rest of the protocol.
//
// Address family support: IPv4 (atyp=1), IPv4-mapped or numeric IPv6
// (atyp=4), and DOMAINNAME (atyp=3, the common case for a browser sending
// "www.example.com:443"). Domain names are passed through unresolved —
// resolution happens at the remote end (i.e. the operator's mdnsvpn server),
// which is correct for a tunnel that's supposed to keep DNS traffic inside.

#ifndef MASTERDNSVPN_SOCKS5SERVER_H
#define MASTERDNSVPN_SOCKS5SERVER_H

#include <QHostAddress>
#include <QObject>
#include <QString>
#include <QTcpServer>
#include <functional>
#include <optional>

class QTcpSocket;

namespace amnezia::masterdnsvpn {

// Destination of a SOCKS5 CONNECT request, in the form the upstream tunnel
// understands. `host` is either an IPv4 / IPv6 textual address or a DNS
// name; `isDomainName` tells the consumer which.
struct Socks5Destination
{
    QString host;
    quint16 port = 0;
    bool isDomainName = false;
    // Wire-level address type — preserved separately from `isDomainName`
    // so callers can distinguish IPv4 (0x01) from IPv6 (0x04) when they
    // matter (e.g. UDP-associate response framing). Matches upstream's
    // `Target.AddressType` (internal/socksproto/target.go:29).
    quint8 addressType = 0;
};

// SOCKS5 ATYP byte values (RFC 1928 §5).
constexpr quint8 kSocks5AtypIPv4   = 0x01;
constexpr quint8 kSocks5AtypDomain = 0x03;
constexpr quint8 kSocks5AtypIPv6   = 0x04;

// Standalone parser for the SOCKS5 target tail (ATYP + addr + PORT), as
// used by the CONNECT request's address bytes and the UDP-associate
// datagram header. Returns std::nullopt for any structural error
// (truncation, unsupported ATYP, zero-length domain). Mirrors upstream
// `ParseTargetPayload` (internal/socksproto/target.go:34).
//
// `consumedBytes` (if non-null) is set on success to the number of bytes
// consumed from `payload` — useful when the buffer contains additional
// trailing data (UDP datagram payload, framing).
std::optional<Socks5Destination> parseTargetPayload(const QByteArray &payload,
                                                    int *consumedBytes = nullptr);

// Inverse of parseTargetPayload — emits the ATYP+addr+port bytes for the
// given destination. Used by the UDP datagram builder and by any future
// upstream-direction encoder. Mirrors upstream `BuildTargetPayload`
// (internal/socksproto/udp.go:90).
QByteArray buildTargetPayload(const Socks5Destination &dest);

// ----- SOCKS5 UDP ASSOCIATE datagram codec (RFC 1928 §7) ---------------
//
// Datagram layout: RSV(2) + FRAG(1) + ATYP + ADDR + PORT + DATA.
// The C++ engine doesn't yet bind a UDP-ASSOCIATE listener — Socks5Server
// is TCP CONNECT only — but the codec is useful standalone (tests,
// future UDP wiring). Mirrors upstream internal/socksproto/udp.go.

struct Socks5UdpDatagram
{
    Socks5Destination target;
    QByteArray payload;
};

// Parse an inbound UDP-ASSOCIATE datagram. Returns std::nullopt for:
//   * fewer than 4 header bytes,
//   * FRAG != 0 (fragmented; upstream returns ErrUDPFragmented),
//   * truncation in the target tail.
std::optional<Socks5UdpDatagram> parseUdpDatagram(const QByteArray &packet);

// Emit a UDP-ASSOCIATE datagram for `target` carrying `payload`. RSV and
// FRAG are zeroed. Mirrors upstream `BuildUDPDatagram`.
QByteArray buildUdpDatagram(const Socks5Destination &target,
                            const QByteArray &payload);

// Auth profile applied to the listener. Empty username = no authentication
// required (NOAUTH); operator config can populate it for shared-host setups.
struct Socks5Auth
{
    QString username;
    QString password;
    bool isEnabled() const { return !username.isEmpty(); }
};

class Socks5Server : public QObject
{
    Q_OBJECT

public:
    // Called when an incoming SOCKS5 CONNECT has been negotiated successfully.
    // The callback OWNS the socket — it must arrange for cleanup. The server
    // has already sent the SOCKS5 success reply; the socket is in pure
    // pass-through mode after the callback returns.
    using StreamSink = std::function<void(QTcpSocket *socket,
                                          const Socks5Destination &dest)>;

    explicit Socks5Server(QObject *parent = nullptr);
    ~Socks5Server() override;

    // Bind 127.0.0.1:port and start accepting. Returns false on bind failure.
    // `port == 0` asks the OS to pick — read back via listenPort().
    bool start(quint16 port, const Socks5Auth &auth, StreamSink sink);
    void stop();

    bool isListening() const;
    quint16 listenPort() const;

signals:
    void clientFailed(const QString &reason);

private:
    Q_DISABLE_COPY_MOVE(Socks5Server)

    void onIncomingConnection();
    void handleClient(QTcpSocket *socket);

    QTcpServer m_listener;
    Socks5Auth m_auth;
    StreamSink m_sink;
};

} // namespace amnezia::masterdnsvpn

#endif // MASTERDNSVPN_SOCKS5SERVER_H
