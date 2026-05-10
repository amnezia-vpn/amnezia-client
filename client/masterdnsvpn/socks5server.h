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
};

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
