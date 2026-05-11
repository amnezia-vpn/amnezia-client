// SPDX-License-Identifier: GPL-3.0-or-later

#include "socks5server.h"

#include <QHostAddress>
#include <QTcpSocket>
#include <QtEndian>
#include <cstring>

namespace amnezia::masterdnsvpn {

namespace {

// SOCKS5 protocol constants (RFC 1928 / 1929).
constexpr quint8 kVer5 = 0x05;
constexpr quint8 kAuthVer = 0x01;
constexpr quint8 kAuthNone = 0x00;
constexpr quint8 kAuthUserPass = 0x02;
constexpr quint8 kAuthNoneAcceptable = 0xFF;

constexpr quint8 kCmdConnect = 0x01;
// ATYP values come from the public header (kSocks5AtypIPv4 etc.). The
// short aliases below stay for source-readability inside this file.
constexpr quint8 kAtypIpv4 = kSocks5AtypIPv4;
constexpr quint8 kAtypDomain = kSocks5AtypDomain;
constexpr quint8 kAtypIpv6 = kSocks5AtypIPv6;

constexpr quint8 kRepSucceeded = 0x00;
constexpr quint8 kRepGeneralFailure = 0x01;
constexpr quint8 kRepCmdUnsupported = 0x07;
constexpr quint8 kRepAtypUnsupported = 0x08;

// How long we wait at each handshake step before giving up. SOCKS5 clients
// (browsers, system proxy stacks) speak the handshake in milliseconds; 5 s
// is generous enough to absorb network hiccups and tight enough that a
// wedged client doesn't park a socket forever.
constexpr int kStepTimeoutMs = 5'000;

// Block-on-read helper. Returns the requested number of bytes or empty on
// timeout / disconnect. Built on QTcpSocket's blocking API; safe inside the
// per-client handler because each client runs its handshake on the calling
// thread (Engine spins us up on its own QThread, so the GUI loop is fine).
QByteArray readExact(QTcpSocket *socket, qsizetype n)
{
    QByteArray out;
    out.reserve(n);
    while (out.size() < n) {
        if (!socket->bytesAvailable()) {
            if (!socket->waitForReadyRead(kStepTimeoutMs)) {
                return {};
            }
        }
        const QByteArray chunk = socket->read(n - out.size());
        if (chunk.isEmpty()) {
            // Peer closed during handshake — abandon.
            return {};
        }
        out.append(chunk);
    }
    return out;
}

bool writeAll(QTcpSocket *socket, const QByteArray &data)
{
    qint64 written = 0;
    while (written < data.size()) {
        const qint64 n = socket->write(data.constData() + written, data.size() - written);
        if (n < 0) {
            return false;
        }
        written += n;
        if (!socket->waitForBytesWritten(kStepTimeoutMs)) {
            return false;
        }
    }
    return true;
}

// Build the SOCKS5 reply preamble (VER REP RSV ATYP BND.ADDR BND.PORT).
// We always reply with BND = 0.0.0.0:0 because the tunnel's egress address
// isn't observable from the client side, and SOCKS clients ignore BND in
// practice when CONNECT succeeds.
QByteArray buildReply(quint8 rep)
{
    QByteArray r;
    r.append(static_cast<char>(kVer5));
    r.append(static_cast<char>(rep));
    r.append(static_cast<char>(0x00)); // RSV
    r.append(static_cast<char>(kAtypIpv4));
    r.append(QByteArray(4, '\0')); // BND.ADDR = 0.0.0.0
    r.append(static_cast<char>(0x00)); // BND.PORT high
    r.append(static_cast<char>(0x00)); // BND.PORT low
    return r;
}

} // namespace

// ---------------------------------------------------------------------------
// Standalone target-payload parser
// ---------------------------------------------------------------------------
//
// Mirrors upstream `internal/socksproto/target.go::ParseTargetPayload`.
// The C++ Socks5Server's CONNECT handler reads from a streaming socket;
// this helper handles the case where the ATYP+addr+port tail is already
// in a buffer (TCP CONNECT after the 4-byte header is read, or UDP
// ASSOCIATE datagram framing). On success, `*consumedBytes` is set to
// the number of bytes parsed; on error returns std::nullopt.
std::optional<Socks5Destination>
parseTargetPayload(const QByteArray &payload, int *consumedBytes)
{
    if (payload.size() < 3) {
        return std::nullopt; // ATYP + at-least-1 byte of addr + 0 port = 3
    }

    Socks5Destination dest;
    dest.addressType = static_cast<quint8>(payload[0]);
    int offset = 1;

    switch (dest.addressType) {
    case kSocks5AtypIPv4: {
        if (payload.size() < offset + 4 + 2) {
            return std::nullopt;
        }
        const quint32 raw = qFromBigEndian<quint32>(payload.constData() + offset);
        dest.host = QHostAddress(raw).toString();
        dest.isDomainName = false;
        offset += 4;
        break;
    }
    case kSocks5AtypDomain: {
        if (payload.size() < offset + 1) {
            return std::nullopt;
        }
        const int domainLen = static_cast<quint8>(payload[offset]);
        ++offset;
        if (domainLen < 1 || payload.size() < offset + domainLen + 2) {
            return std::nullopt;
        }
        dest.host = QString::fromLatin1(payload.constData() + offset, domainLen);
        dest.isDomainName = true;
        offset += domainLen;
        break;
    }
    case kSocks5AtypIPv6: {
        if (payload.size() < offset + 16 + 2) {
            return std::nullopt;
        }
        Q_IPV6ADDR raw{};
        std::memcpy(&raw, payload.constData() + offset, 16);
        dest.host = QHostAddress(raw).toString();
        dest.isDomainName = false;
        offset += 16;
        break;
    }
    default:
        return std::nullopt; // unsupported ATYP
    }

    dest.port = qFromBigEndian<quint16>(payload.constData() + offset);
    offset += 2;
    if (consumedBytes != nullptr) {
        *consumedBytes = offset;
    }
    return dest;
}

// ---------------------------------------------------------------------------
// Target / UDP datagram codecs
// ---------------------------------------------------------------------------

QByteArray buildTargetPayload(const Socks5Destination &dest)
{
    QByteArray out;
    const quint8 atyp = dest.addressType != 0
            ? dest.addressType
            : (dest.isDomainName ? kSocks5AtypDomain
                                 : (dest.host.contains(QChar(':'))
                                            ? kSocks5AtypIPv6
                                            : kSocks5AtypIPv4));

    out.append(static_cast<char>(atyp));
    switch (atyp) {
    case kSocks5AtypIPv4: {
        const QHostAddress addr(dest.host);
        const quint32 raw = addr.toIPv4Address();
        char be[4];
        qToBigEndian<quint32>(raw, be);
        out.append(be, 4);
        break;
    }
    case kSocks5AtypIPv6: {
        const QHostAddress addr(dest.host);
        const Q_IPV6ADDR raw = addr.toIPv6Address();
        out.append(reinterpret_cast<const char *>(&raw), 16);
        break;
    }
    case kSocks5AtypDomain: {
        const QByteArray h = dest.host.toLatin1();
        out.append(static_cast<char>(h.size() & 0xFF));
        out.append(h);
        break;
    }
    default:
        return {};
    }
    char beport[2];
    qToBigEndian<quint16>(dest.port, beport);
    out.append(beport, 2);
    return out;
}

std::optional<Socks5UdpDatagram> parseUdpDatagram(const QByteArray &packet)
{
    if (packet.size() < 4) {
        return std::nullopt;
    }
    // FRAG must be 0 — RFC 1928 §7. Upstream returns ErrUDPFragmented when
    // packet[2] != 0.
    if (static_cast<quint8>(packet[2]) != 0) {
        return std::nullopt;
    }
    int consumed = 0;
    const auto target = parseTargetPayload(packet.mid(3), &consumed);
    if (!target) {
        return std::nullopt;
    }
    Socks5UdpDatagram dgram;
    dgram.target = *target;
    dgram.payload = packet.mid(3 + consumed);
    return dgram;
}

QByteArray buildUdpDatagram(const Socks5Destination &target, const QByteArray &payload)
{
    QByteArray out;
    out.append('\0').append('\0').append('\0'); // RSV(2) + FRAG(1)
    out.append(buildTargetPayload(target));
    out.append(payload);
    return out;
}

Socks5Server::Socks5Server(QObject *parent) : QObject(parent)
{
    connect(&m_listener, &QTcpServer::newConnection, this, &Socks5Server::onIncomingConnection);
}

Socks5Server::~Socks5Server()
{
    stop();
}

bool Socks5Server::start(quint16 port, const Socks5Auth &auth, StreamSink sink)
{
    if (m_listener.isListening()) {
        return true;
    }
    m_auth = auth;
    m_sink = std::move(sink);

    if (!m_listener.listen(QHostAddress::LocalHost, port)) {
        emit clientFailed(QStringLiteral("listen failed: %1").arg(m_listener.errorString()));
        return false;
    }
    return true;
}

void Socks5Server::stop()
{
    if (m_listener.isListening()) {
        m_listener.close();
    }
    m_sink = nullptr;
    m_auth = {};
}

bool Socks5Server::isListening() const
{
    return m_listener.isListening();
}

quint16 Socks5Server::listenPort() const
{
    return m_listener.serverPort();
}

void Socks5Server::onIncomingConnection()
{
    while (m_listener.hasPendingConnections()) {
        QTcpSocket *socket = m_listener.nextPendingConnection();
        if (!socket) {
            continue;
        }
        // Negotiate inline. Each client is short-lived during the handshake,
        // so we don't fork a thread per connection — the listener is bound to
        // the engine's worker thread anyway.
        handleClient(socket);
    }
}

void Socks5Server::handleClient(QTcpSocket *socket)
{
    auto fail = [&](quint8 rep, const char *why) {
        if (rep != 0xFF) {
            // Best-effort error reply; ignore write failures, we're tearing down.
            (void)writeAll(socket, buildReply(rep));
        }
        emit clientFailed(QString::fromLatin1(why));
        socket->disconnectFromHost();
        socket->deleteLater();
    };

    // ---- Method-selection (greeting) ----
    QByteArray greet = readExact(socket, 2);
    if (greet.size() != 2 || static_cast<quint8>(greet[0]) != kVer5) {
        return fail(0xFF, "bad greeting");
    }
    const int nMethods = static_cast<quint8>(greet[1]);
    if (nMethods <= 0) {
        return fail(0xFF, "no methods advertised");
    }
    QByteArray methods = readExact(socket, nMethods);
    if (methods.size() != nMethods) {
        return fail(0xFF, "short methods list");
    }

    quint8 chosen = kAuthNoneAcceptable;
    if (m_auth.isEnabled()) {
        if (methods.contains(static_cast<char>(kAuthUserPass))) {
            chosen = kAuthUserPass;
        }
    } else if (methods.contains(static_cast<char>(kAuthNone))) {
        chosen = kAuthNone;
    }

    QByteArray methodReply;
    methodReply.append(static_cast<char>(kVer5));
    methodReply.append(static_cast<char>(chosen));
    if (!writeAll(socket, methodReply) || chosen == kAuthNoneAcceptable) {
        return fail(0xFF, "no acceptable auth method");
    }

    // ---- USERNAME/PASSWORD sub-negotiation (RFC 1929) ----
    if (chosen == kAuthUserPass) {
        QByteArray hdr = readExact(socket, 2);
        if (hdr.size() != 2 || static_cast<quint8>(hdr[0]) != kAuthVer) {
            return fail(0xFF, "bad auth version");
        }
        const int ulen = static_cast<quint8>(hdr[1]);
        QByteArray username = readExact(socket, ulen);
        QByteArray plenByte = readExact(socket, 1);
        if (plenByte.size() != 1) {
            return fail(0xFF, "short auth plen");
        }
        const int plen = static_cast<quint8>(plenByte[0]);
        QByteArray password = readExact(socket, plen);

        const bool ok = (QString::fromUtf8(username) == m_auth.username
                         && QString::fromUtf8(password) == m_auth.password);
        QByteArray authReply;
        authReply.append(static_cast<char>(kAuthVer));
        authReply.append(static_cast<char>(ok ? 0x00 : 0x01));
        if (!writeAll(socket, authReply) || !ok) {
            return fail(0xFF, "auth failed");
        }
    }

    // ---- CONNECT request ----
    QByteArray req = readExact(socket, 4);
    if (req.size() != 4 || static_cast<quint8>(req[0]) != kVer5) {
        return fail(kRepGeneralFailure, "bad connect header");
    }
    if (static_cast<quint8>(req[1]) != kCmdConnect) {
        return fail(kRepCmdUnsupported, "only CONNECT supported");
    }
    const quint8 atyp = static_cast<quint8>(req[3]);

    Socks5Destination dest;
    switch (atyp) {
    case kAtypIpv4: {
        QByteArray addr = readExact(socket, 4);
        if (addr.size() != 4) {
            return fail(kRepGeneralFailure, "short v4 addr");
        }
        QHostAddress h(qFromBigEndian<quint32>(addr.constData()));
        dest.host = h.toString();
        dest.isDomainName = false;
        break;
    }
    case kAtypIpv6: {
        QByteArray addr = readExact(socket, 16);
        if (addr.size() != 16) {
            return fail(kRepGeneralFailure, "short v6 addr");
        }
        Q_IPV6ADDR raw;
        std::memcpy(&raw, addr.constData(), 16);
        QHostAddress h(raw);
        dest.host = h.toString();
        dest.isDomainName = false;
        break;
    }
    case kAtypDomain: {
        QByteArray lenByte = readExact(socket, 1);
        if (lenByte.size() != 1) {
            return fail(kRepGeneralFailure, "short domain len");
        }
        const int dlen = static_cast<quint8>(lenByte[0]);
        QByteArray name = readExact(socket, dlen);
        if (name.size() != dlen) {
            return fail(kRepGeneralFailure, "short domain");
        }
        dest.host = QString::fromLatin1(name);
        dest.isDomainName = true;
        break;
    }
    default:
        return fail(kRepAtypUnsupported, "unknown ATYP");
    }

    QByteArray portBytes = readExact(socket, 2);
    if (portBytes.size() != 2) {
        return fail(kRepGeneralFailure, "short port");
    }
    dest.port = qFromBigEndian<quint16>(portBytes.constData());

    if (!m_sink) {
        return fail(kRepGeneralFailure, "no sink configured");
    }

    // Send the success reply *before* handing off to the sink. The sink
    // takes ownership of the now-tunnel-ready socket and is responsible
    // for plumbing both directions.
    if (!writeAll(socket, buildReply(kRepSucceeded))) {
        socket->deleteLater();
        return;
    }

    m_sink(socket, dest);
}

} // namespace amnezia::masterdnsvpn
