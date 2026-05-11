// SPDX-License-Identifier: GPL-3.0-or-later

#include "session.h"

#include "dnsframing.h"
#include "wireframing.h"

#include <QDateTime>
#include <QDebug>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonValue>
#include <QRandomGenerator>
#include <QTcpSocket>
#include <QtEndian>

namespace amnezia::masterdnsvpn {

namespace {

constexpr int kTickIntervalMs = 100;

// Spec §7.1 — SESSION_INIT payload layout. We allocate exactly 10 bytes:
//
//   [0]   response mode  (0 = raw TXT chunks, 1 = base64-encoded chunks)
//   [1]   compression pair  (upload<<4 | download)
//   [2..3] client max upload MTU (uint16 BE)
//   [4..5] client max download MTU (uint16 BE)
//   [6..9] verify code  (4 bytes random)
// QRandomGenerator::generate(begin, end) requires word-aligned quint32
// pointers; QByteArray::data() is byte-aligned. Drop the bytes through a
// stack scalar to avoid the alignment hazard.
QByteArray randomBytes(int n)
{
    QByteArray out(n, '\0');
    auto *gen = QRandomGenerator::system();
    int written = 0;
    while (written < n) {
        const quint32 v = gen->generate();
        const int chunk = std::min<int>(4, n - written);
        std::memcpy(out.data() + written, &v, chunk);
        written += chunk;
    }
    return out;
}

QByteArray buildSessionInitPayload(int uploadMtu, int downloadMtu, int upComp, int downComp,
                                   QByteArray *verifyCodeOut)
{
    QByteArray payload(10, '\0');
    payload[0] = 0; // raw TXT chunks; matches `BASE_ENCODE_DATA = false`
    payload[1] = static_cast<char>(((upComp & 0x0F) << 4) | (downComp & 0x0F));
    qToBigEndian<quint16>(static_cast<quint16>(uploadMtu), payload.data() + 2);
    qToBigEndian<quint16>(static_cast<quint16>(downloadMtu), payload.data() + 4);

    QByteArray verify = randomBytes(4);
    std::memcpy(payload.data() + 6, verify.constData(), 4);

    if (verifyCodeOut) {
        *verifyCodeOut = verify;
    }
    return payload;
}

// Pull resolver entries from the JSON array the model produces. Each entry
// is "ip[:port]" or "[v6]:port". Returns the parsed pool spec; empty if
// the operator forgot to populate the resolvers slot.
QVector<ResolverSpec> parseResolvers(const QJsonArray &arr, const QStringList &tunnelDomains)
{
    QVector<ResolverSpec> out;
    if (tunnelDomains.isEmpty()) {
        return out;
    }
    int domainIdx = 0;
    for (const QJsonValue &v : arr) {
        if (!v.isString()) {
            continue;
        }
        QString s = v.toString().trimmed();
        if (s.isEmpty()) {
            continue;
        }

        ResolverSpec spec;
        spec.port = 53;
        spec.tunnelDomain = tunnelDomains[domainIdx % tunnelDomains.size()];
        ++domainIdx;

        if (s.startsWith('[')) {
            // [v6]:port
            const int rb = s.indexOf(']');
            if (rb < 0) continue;
            spec.address = QHostAddress(s.mid(1, rb - 1));
            const QString rest = s.mid(rb + 1);
            if (rest.startsWith(':')) {
                spec.port = static_cast<quint16>(rest.mid(1).toUInt());
            }
        } else if (s.contains(':')) {
            const int colon = s.lastIndexOf(':');
            spec.address = QHostAddress(s.left(colon));
            spec.port = static_cast<quint16>(s.mid(colon + 1).toUInt());
        } else {
            spec.address = QHostAddress(s);
        }
        if (spec.address.isNull()) {
            continue;
        }
        out.append(spec);
    }
    return out;
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

Session::Session(QObject *parent) : QObject(parent) {}
Session::~Session() = default;

void Session::setState(State s)
{
    if (m_state == s) {
        return;
    }
    m_state = s;
    emit stateChanged(s);
}

void Session::fail(const QString &reason)
{
    m_lastError = reason;
    qWarning() << "masterdnsvpn::Session: failure -" << reason;
    setState(State::Failed);
}

quint16 Session::socksPort() const
{
    return m_socks5 ? m_socks5->listenPort() : 0;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool Session::start(const QJsonObject &config)
{
    if (m_state != State::Idle) {
        m_lastError = QStringLiteral("Session already started");
        return false;
    }

    // Pull operator config out of the JSON. The keys here mirror those
    // documented on MasterDnsVpnProtocolConfig (which the model writes
    // out via toJson()).
    const QJsonArray domainsJson = config.value(QStringLiteral("domains")).toArray();
    for (const QJsonValue &v : domainsJson) {
        if (v.isString()) {
            m_tunnelDomains.append(v.toString());
        }
    }
    if (m_tunnelDomains.isEmpty()) {
        m_lastError = QStringLiteral("no tunnel domains configured");
        return false;
    }

    m_encryptionPassphrase = config.value(QStringLiteral("encryptionKey")).toString();
    if (m_encryptionPassphrase.isEmpty()) {
        m_lastError = QStringLiteral("no encryption key configured");
        return false;
    }
    const auto cipherOpt = cipherMethodFromInt(
            config.value(QStringLiteral("encryptionMethod")).toInt(0));
    if (!cipherOpt) {
        m_lastError = QStringLiteral("unknown encryption method");
        return false;
    }
    m_cipherMethod = *cipherOpt;
    m_derivedKey = deriveKey(m_cipherMethod, m_encryptionPassphrase);
    if (!m_cipherSeal.init(m_cipherMethod, m_derivedKey)
        || !m_cipherOpen.init(m_cipherMethod, m_derivedKey)) {
        m_lastError = QStringLiteral("cipher init failed");
        return false;
    }

    m_listenPort = static_cast<quint16>(
            config.value(QStringLiteral("listenPort")).toString().toUInt());
    if (m_listenPort == 0) {
        m_listenPort = 18000;
    }
    m_socks5Auth.username = config.value(QStringLiteral("socks5User")).toString();
    m_socks5Auth.password = config.value(QStringLiteral("socks5Pass")).toString();

    m_uploadCompression = std::clamp(
            config.value(QStringLiteral("uploadCompression")).toInt(0), 0, 3);
    m_downloadCompression = std::clamp(
            config.value(QStringLiteral("downloadCompression")).toInt(0), 0, 3);

    // ---- Resolvers ----
    const QJsonArray resolversJson = config.value(QStringLiteral("resolvers")).toArray();
    const QVector<ResolverSpec> resolverSpecs =
            parseResolvers(resolversJson, m_tunnelDomains);
    if (resolverSpecs.isEmpty()) {
        m_lastError = QStringLiteral("no usable resolvers in pool");
        return false;
    }

    ResolverPool::Config rcfg;
    const int strategyInt =
            config.value(QStringLiteral("balancingStrategy")).toInt(5);
    rcfg.strategy = static_cast<BalancingStrategy>(strategyInt);
    rcfg.packetDuplicationCount =
            config.value(QStringLiteral("packetDuplication")).toInt(3);
    rcfg.setupPacketDuplicationCount =
            config.value(QStringLiteral("setupPacketDuplication")).toInt(4);

    m_resolvers = std::make_unique<ResolverPool>();
    if (!m_resolvers->configure(resolverSpecs, rcfg)) {
        m_lastError = QStringLiteral("resolver pool configure failed");
        return false;
    }
    connect(m_resolvers.get(), &ResolverPool::readyForUse,
            this, &Session::onResolverPoolReady);
    connect(m_resolvers.get(), &ResolverPool::responseReceived,
            this, &Session::onResolverResponse);
    m_resolvers->start();

    // ---- SOCKS5 listener ----
    m_socks5 = std::make_unique<Socks5Server>();
    if (!m_socks5->start(
                m_listenPort, m_socks5Auth,
                [this](QTcpSocket *s, const Socks5Destination &d) { onSocks5Accepted(s, d); })) {
        m_lastError = QStringLiteral("SOCKS5 listen on %1 failed").arg(m_listenPort);
        return false;
    }

    // ---- Tick timer ----
    if (!m_tickTimer) {
        m_tickTimer = new QTimer(this);
        connect(m_tickTimer, &QTimer::timeout, this, &Session::onTick);
    }
    m_tickTimer->start(kTickIntervalMs);

    setState(State::Initialising);
    return true;
}

void Session::stop()
{
    if (m_state == State::Stopped || m_state == State::Idle) {
        return;
    }
    setState(State::TearingDown);

    if (m_tickTimer) {
        m_tickTimer->stop();
    }

    // Best-effort SESSION_CLOSE — the session burst at §7.5 is overkill
    // for an explicit stop, just emit one packet.
    if (m_sessionId != 0 && m_resolvers) {
        Packet close;
        close.sessionId = m_sessionId;
        close.cookie = m_sessionCookie;
        close.type = PacketType::SessionClose;
        sendPacket(close, /*isSetupPacket=*/false);
    }

    m_streams.clear();
    for (auto it = m_streamSockets.begin(); it != m_streamSockets.end(); ++it) {
        if (it.value()) {
            it.value()->disconnectFromHost();
            it.value()->deleteLater();
        }
    }
    m_streamSockets.clear();

    if (m_socks5) {
        m_socks5->stop();
    }
    m_socks5.reset();

    if (m_resolvers) {
        m_resolvers->stop();
    }
    m_resolvers.reset();

    setState(State::Stopped);
}

void Session::onResolverPoolReady()
{
    if (m_state != State::Initialising) {
        return;
    }
    setState(State::Authenticating);
    sendSessionInit();
}

// ---------------------------------------------------------------------------
// Outbound
// ---------------------------------------------------------------------------

QByteArray Session::sealAndEncode(const QByteArray &plaintext)
{
    const int nonceLen = requiredNonceBytes(m_cipherMethod);
    QByteArray nonce = nonceLen > 0 ? randomBytes(nonceLen) : QByteArray();
    QByteArray ciphertext;
    if (!m_cipherSeal.seal(plaintext, nonce, /*aad=*/{}, ciphertext)) {
        return {};
    }

    QByteArray wire = nonce;
    wire.append(ciphertext);
    return encodeBase36(wire);
}

std::optional<QByteArray> Session::decodeAndOpen(const QByteArray &encoded)
{
    auto raw = decodeBase36(encoded);
    if (!raw) {
        return std::nullopt;
    }
    const int nonceLen = requiredNonceBytes(m_cipherMethod);
    if (raw->size() < nonceLen) {
        return std::nullopt;
    }
    const QByteArray nonce = raw->left(nonceLen);
    const QByteArray ciphertext = raw->mid(nonceLen);

    QByteArray plaintext;
    if (!m_cipherOpen.open(ciphertext, nonce, /*aad=*/{}, plaintext)) {
        return std::nullopt;
    }
    return plaintext;
}

void Session::sendPacket(const Packet &packet, bool isSetupPacket)
{
    if (!m_resolvers) {
        return;
    }
    const QByteArray plaintext = encode(packet);
    const QByteArray encoded = sealAndEncode(plaintext);
    if (encoded.isEmpty()) {
        return;
    }
    const auto picks = isSetupPacket
            ? m_resolvers->pickDuplicates(4, /*setup=*/true)
            : m_resolvers->pickDuplicates(3, /*setup=*/false);
    for (const ResolverPick &pick : picks) {
        const quint16 txId = nextTransactionId();
        const QByteArray dnsBytes = buildQuery(txId, encoded, pick.tunnelDomain);
        m_outstandingQueries.insert(txId, pick.index);
        m_resolvers->send(pick.index, dnsBytes);
        m_bytesTx += dnsBytes.size();
    }
}

// ---------------------------------------------------------------------------
// Inbound
// ---------------------------------------------------------------------------

void Session::onResolverResponse(int resolverIndex, quint16 transactionId, const QByteArray &bytes)
{
    Q_UNUSED(resolverIndex);
    m_bytesRx += bytes.size();
    auto outIt = m_outstandingQueries.find(transactionId);
    if (outIt != m_outstandingQueries.end()) {
        m_outstandingQueries.erase(outIt);
    }

    auto resp = parseResponse(bytes, /*wasBase64Mode=*/false);
    if (!resp || resp->frame.isEmpty()) {
        return;
    }

    auto plaintext = decodeAndOpen(resp->frame);
    if (!plaintext) {
        return;
    }
    auto pkt = decode(*plaintext);
    if (!pkt) {
        return;
    }
    onInnerPacket(*pkt);
}

void Session::onInnerPacket(const Packet &packet)
{
    switch (packet.type) {
    case PacketType::SessionAccept:
        onSessionAccept(packet);
        return;
    case PacketType::SessionBusy:
        onSessionBusy(packet);
        return;
    case PacketType::SessionClose:
        stop();
        return;
    case PacketType::Pong:
        // Update last-pong timestamp; the tick uses it for the tiered ping
        // pacing the spec describes. Counters are bookkeeping only —
        // resolver health updates land in onArqOutbound's ACK plumbing.
        return;
    case PacketType::PackedControlBlocks: {
        const QVector<PackedBlock> blocks = unpackBlocks(packet.payload);
        for (const PackedBlock &b : blocks) {
            // Each block re-enters the dispatch as if it had arrived as
            // a standalone packet. Exterior session id / cookie still
            // applies; payload of the synthetic packet is empty.
            Packet synthetic;
            synthetic.sessionId = packet.sessionId;
            synthetic.cookie = packet.cookie;
            synthetic.type = b.type;
            synthetic.streamId = b.streamId;
            synthetic.sequenceNum = b.sequenceNum;
            const HeaderExtensions ext = headerExtensions(b.type);
            if (ext.fragment) {
                synthetic.fragmentId = b.fragmentId;
                synthetic.totalFragments = b.totalFragments;
            }
            onInnerPacket(synthetic);
        }
        return;
    }
    case PacketType::ErrorDrop:
        // §7.6 — server says "I don't recognise this cookie". Re-init.
        m_sessionId = 0;
        m_sessionCookie = 0;
        sendSessionInit();
        return;
    default:
        break;
    }

    if (packet.streamId.has_value()) {
        const quint16 sid = *packet.streamId;
        auto it = m_streams.find(sid);
        if (it != m_streams.end()) {
            (*it)->onPacketReceived(packet);
        }
    }
}

// ---------------------------------------------------------------------------
// SOCKS5 → tunnel
// ---------------------------------------------------------------------------

void Session::onSocks5Accepted(QTcpSocket *socket, const Socks5Destination &dest)
{
    if (m_state != State::Established) {
        socket->disconnectFromHost();
        socket->deleteLater();
        return;
    }
    const quint16 streamId = allocStreamId();

    ArqConfig acfg;
    auto stream = std::make_unique<ArqStream>(
            streamId, acfg,
            [this, streamId](const ArqOutbound &out) { onArqOutbound(streamId, out); },
            [this, streamId](const ArqDelivery &d) { onArqDelivery(streamId, d); });
    m_streams.insert(streamId, std::move(stream));
    m_streamSockets.insert(streamId, socket);

    // Bridge socket -> ARQ.
    connect(socket, &QTcpSocket::readyRead, this, [this, streamId, socket]() {
        const QByteArray bytes = socket->readAll();
        if (bytes.isEmpty()) {
            return;
        }
        auto it = m_streams.find(streamId);
        if (it != m_streams.end()) {
            (*it)->writeApp(bytes);
        }
    });
    connect(socket, &QTcpSocket::disconnected, this, [this, streamId]() {
        auto it = m_streams.find(streamId);
        if (it != m_streams.end()) {
            (*it)->halfCloseWrite();
        }
    });

    // Build PACKET_SOCKS5_SYN payload (§10.2).
    QByteArray synPayload;
    if (dest.isDomainName) {
        const QByteArray nameAscii = dest.host.toLatin1();
        synPayload.append(static_cast<char>(0x03));
        synPayload.append(static_cast<char>(nameAscii.size()));
        synPayload.append(nameAscii);
    } else {
        QHostAddress h(dest.host);
        if (h.protocol() == QAbstractSocket::IPv4Protocol) {
            synPayload.append(static_cast<char>(0x01));
            const quint32 v4 = qToBigEndian<quint32>(h.toIPv4Address());
            synPayload.append(reinterpret_cast<const char *>(&v4), 4);
        } else {
            synPayload.append(static_cast<char>(0x04));
            Q_IPV6ADDR raw = h.toIPv6Address();
            synPayload.append(reinterpret_cast<const char *>(&raw), 16);
        }
    }
    char portBuf[2];
    qToBigEndian<quint16>(dest.port, portBuf);
    synPayload.append(portBuf, 2);

    Packet syn;
    syn.sessionId = m_sessionId;
    syn.cookie = m_sessionCookie;
    syn.type = PacketType::Socks5Syn;
    syn.streamId = streamId;
    syn.sequenceNum = 1;
    syn.fragmentId = 0;
    syn.totalFragments = 1;
    syn.payload = synPayload;
    sendPacket(syn, /*isSetupPacket=*/true);
}

void Session::onArqOutbound(quint16 streamId, const ArqOutbound &out)
{
    Q_UNUSED(streamId);
    Packet p = out.packet;
    p.sessionId = m_sessionId;
    p.cookie = m_sessionCookie;
    sendPacket(p, /*isSetupPacket=*/false);
}

void Session::onArqDelivery(quint16 streamId, const ArqDelivery &delivery)
{
    auto it = m_streamSockets.find(streamId);
    if (it == m_streamSockets.end() || !it.value()) {
        return;
    }
    if (!delivery.bytes.isEmpty()) {
        it.value()->write(delivery.bytes);
    }
    if (delivery.endOfStream) {
        it.value()->disconnectFromHost();
    }
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void Session::onTick()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (auto it = m_streams.begin(); it != m_streams.end();) {
        (*it)->tickMs(now);
        if ((*it)->isTerminal()) {
            const quint16 sid = it.key();
            auto sock = m_streamSockets.take(sid);
            if (sock) {
                sock->disconnectFromHost();
                sock->deleteLater();
            }
            it = m_streams.erase(it);
        } else {
            ++it;
        }
    }

    // Periodic ping when established. We use the simplest constant-rate
    // policy here; tiered pacing per §12 lands as a follow-up.
    static qint64 lastPing = 0;
    if (m_state == State::Established && now - lastPing > 8'000) {
        Packet ping;
        ping.sessionId = m_sessionId;
        ping.cookie = m_sessionCookie;
        ping.type = PacketType::Ping;
        // §3.4 PING payload: 7 bytes `P` `O` `:` <4 random>.
        QByteArray pingPayload;
        pingPayload.append('P');
        pingPayload.append('O');
        pingPayload.append(':');
        pingPayload.append(randomBytes(4));
        ping.payload = pingPayload;
        sendPacket(ping, /*isSetupPacket=*/false);
        lastPing = now;
    }
}

// ---------------------------------------------------------------------------
// Handshake
// ---------------------------------------------------------------------------

void Session::sendSessionInit()
{
    const int upMtu = std::max(64, m_resolvers->syncedUploadMtu());
    const int downMtu = std::max(255, m_resolvers->syncedDownloadMtu());
    QByteArray verify;
    QByteArray payload = buildSessionInitPayload(upMtu, downMtu,
                                                  m_uploadCompression, m_downloadCompression,
                                                  &verify);
    m_initVerifyCode = verify;

    Packet init;
    init.sessionId = 0; // pre-session
    init.cookie = 0;
    init.type = PacketType::SessionInit;
    init.payload = payload;
    sendPacket(init, /*isSetupPacket=*/true);
}

void Session::onSessionAccept(const Packet &packet)
{
    if (packet.payload.size() < 7) {
        return;
    }
    const quint8 grantedSession = static_cast<quint8>(packet.payload[0]);
    const quint8 grantedCookie = static_cast<quint8>(packet.payload[1]);
    const quint8 grantedComp = static_cast<quint8>(packet.payload[2]);
    const QByteArray echoVerify = packet.payload.mid(3, 4);

    if (echoVerify != m_initVerifyCode) {
        return;
    }
    m_sessionId = grantedSession;
    m_sessionCookie = grantedCookie;

    // Server may have downgraded our compression preference; record what
    // it actually permitted so future packets honour the constraint.
    m_uploadCompression = std::min(m_uploadCompression, (grantedComp >> 4) & 0x0F);
    m_downloadCompression = std::min(m_downloadCompression, grantedComp & 0x0F);

    setState(State::Established);
}

void Session::onSessionBusy(const Packet &packet)
{
    if (packet.payload.size() < 4) {
        return;
    }
    const QByteArray echoVerify = packet.payload.left(4);
    if (echoVerify != m_initVerifyCode) {
        return;
    }
    // Spec §7.3 — back off for SESSION_INIT_BUSY_RETRY_INTERVAL_SECONDS
    // (default 60s). For now we just stop; the engine layer will retry on
    // its own schedule.
    fail(QStringLiteral("server returned SESSION_BUSY"));
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

quint16 Session::allocStreamId()
{
    quint16 sid = m_nextStreamId++;
    if (sid == 0) {
        sid = m_nextStreamId++;
    }
    return sid;
}

quint16 Session::nextTransactionId()
{
    return ++m_dnsTxIdCounter;
}

} // namespace amnezia::masterdnsvpn
