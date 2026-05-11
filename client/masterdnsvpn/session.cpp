// SPDX-License-Identifier: GPL-3.0-or-later

#include "session.h"

#include "compression.h"
#include "dnsframing.h"
#include "wireframing.h"

#include <climits>
#include <cmath>
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
                                   QByteArray *verifyCode)
{
    QByteArray payload(10, '\0');
    payload[0] = 0; // raw TXT chunks; matches `BASE_ENCODE_DATA = false`
    payload[1] = static_cast<char>(((upComp & 0x0F) << 4) | (downComp & 0x0F));
    qToBigEndian<quint16>(static_cast<quint16>(uploadMtu), payload.data() + 2);
    qToBigEndian<quint16>(static_cast<quint16>(downloadMtu), payload.data() + 4);

    // Caller may pass an existing verify code to be embedded (retry path —
    // see spec §13(9): the code is persistent across SESSION_INIT attempts
    // until SESSION_ACCEPT or lifecycle reset). An empty buffer triggers a
    // fresh mint, which is written back to *verifyCode for the caller to
    // cache.
    if (verifyCode == nullptr) {
        QByteArray fresh = randomBytes(4);
        std::memcpy(payload.data() + 6, fresh.constData(), 4);
    } else if (verifyCode->size() == 4) {
        std::memcpy(payload.data() + 6, verifyCode->constData(), 4);
    } else {
        *verifyCode = randomBytes(4);
        std::memcpy(payload.data() + 6, verifyCode->constData(), 4);
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

    // ---- ARQ / ping-pacing tunables (all optional) ----
    //
    // Operators can override any of these via the JSON config; defaults
    // remain in place if a key is absent. ArqConfig's constructor clamps
    // each field to its protocol floor (windowSize >= 300, RTOs >= 50ms,
    // etc.), so out-of-range values are silently corrected rather than
    // rejected.
    if (config.contains(QStringLiteral("arqWindowSize"))) {
        m_arqCfg.windowSize =
                config.value(QStringLiteral("arqWindowSize")).toInt(m_arqCfg.windowSize);
    }
    if (config.contains(QStringLiteral("arqInitialRtoMs"))) {
        m_arqCfg.initialDataRtoMs = static_cast<qint64>(
                config.value(QStringLiteral("arqInitialRtoMs")).toInt(
                        static_cast<int>(m_arqCfg.initialDataRtoMs)));
    }
    if (config.contains(QStringLiteral("arqMaxRtoMs"))) {
        m_arqCfg.maxDataRtoMs = static_cast<qint64>(
                config.value(QStringLiteral("arqMaxRtoMs")).toInt(
                        static_cast<int>(m_arqCfg.maxDataRtoMs)));
    }
    if (config.contains(QStringLiteral("arqDataNackMaxGap"))) {
        m_arqCfg.dataNackMaxGap = config.value(QStringLiteral("arqDataNackMaxGap"))
                                          .toInt(m_arqCfg.dataNackMaxGap);
    }
    if (config.contains(QStringLiteral("arqDataNackInitialDelayMs"))) {
        m_arqCfg.dataNackInitialDelayMs = static_cast<qint64>(
                config.value(QStringLiteral("arqDataNackInitialDelayMs"))
                        .toInt(static_cast<int>(m_arqCfg.dataNackInitialDelayMs)));
    }
    if (config.contains(QStringLiteral("arqDataNackRepeatMs"))) {
        m_arqCfg.dataNackRepeatMs = static_cast<qint64>(
                config.value(QStringLiteral("arqDataNackRepeatMs"))
                        .toInt(static_cast<int>(m_arqCfg.dataNackRepeatMs)));
    }
    if (config.contains(QStringLiteral("arqEnableControlReliability"))) {
        m_arqCfg.enableControlReliability =
                config.value(QStringLiteral("arqEnableControlReliability")).toBool(false);
    }
    if (config.contains(QStringLiteral("pingAggressiveMs"))) {
        m_pingPacing.aggressiveMs = static_cast<qint64>(
                config.value(QStringLiteral("pingAggressiveMs"))
                        .toInt(static_cast<int>(m_pingPacing.aggressiveMs)));
    }
    if (config.contains(QStringLiteral("compressionMinSize"))) {
        m_compressionMinSize = std::max(1,
                config.value(QStringLiteral("compressionMinSize"))
                        .toInt(m_compressionMinSize));
    }

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

    // Cache the operator-configured duplication counts in Session — these
    // are what sendPacket() actually reads when fanning packets out (the
    // pool only consults its own Config for stat displays). Server policy
    // can later clamp them via applyServerPolicy().
    m_packetDuplication = rcfg.packetDuplicationCount;
    m_setupPacketDuplication = rcfg.setupPacketDuplicationCount;

    m_resolvers = std::make_unique<ResolverPool>();
    if (!m_resolvers->configure(resolverSpecs, rcfg)) {
        m_lastError = QStringLiteral("resolver pool configure failed");
        return false;
    }
    connect(m_resolvers.get(), &ResolverPool::socketsBound,
            this, &Session::onSocketsBound);
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

    m_initVerifyCode.clear();

    // Seed the ping FSM so we start in the aggressive tier and let staged
    // promotions push us toward cold as traffic quiets — mirrors upstream's
    // `newPingManager` (internal/client/ping_manager.go:35-47).
    m_pingState.seed(QDateTime::currentMSecsSinceEpoch());

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
    // ResolverPool::readyForUse now signals "MTU sweep finalised; synced
    // MTU is the discovered minimum" (see ResolverPool::setSyncedMtu).
    // Session::onSocketsBound drove the §9 sweep; we land here only after
    // it completes (or times out and falls back to conservative defaults).
    if (m_state != State::MtuProbing) {
        return;
    }
    setState(State::Authenticating);
    sendSessionInit();
}

void Session::onSocketsBound()
{
    if (m_state != State::Initialising) {
        return;
    }
    setState(State::MtuProbing);
    startMtuProbeSweep();
}

void Session::startMtuProbeSweep()
{
    // Spec §9 — fan out one MtuProber per resolver, run them in parallel.
    // Each prober's `nextProbe(packetType, payload, isUpload)` signal is
    // bridged here into a real wire send through this resolver only; the
    // probe response routes back via onInnerPacket using the resolverIndex
    // that ResolverPool already tags every inbound datagram with.
    const int n = m_resolvers ? m_resolvers->resolverCount() : 0;
    if (n <= 0) {
        // No resolvers — short-circuit to the conservative defaults that
        // ResolverPool::start() already published. Session can still
        // attempt SESSION_INIT and let the failure surface naturally.
        m_resolvers->setSyncedMtu(m_resolvers->syncedUploadMtu(),
                                  m_resolvers->syncedDownloadMtu());
        return;
    }

    m_probers.clear();
    m_probers.reserve(n);
    m_probeResults.clear();
    m_probeResults.resize(n);
    m_probesPending = n;

    for (int i = 0; i < n; ++i) {
        auto prober = std::make_unique<MtuProber>(this);
        const int idx = i;
        connect(prober.get(), &MtuProber::nextProbe, this,
                [this, idx](PacketType t, const QByteArray &p, bool up) {
                    onProbeNextRequested(idx, t, p, up);
                });
        connect(prober.get(), &MtuProber::finished, this,
                [this, idx](bool ok, int up, int down) {
                    onProbeFinished(idx, ok, up, down);
                });

        MtuProber::Config cfg;
        // The conservative defaults the pool published when sockets came up
        // are now the upper bounds for the search — probing only refines
        // them upward. Future commits can plumb operator-config-supplied
        // max bounds through here.
        cfg.maxUpload = std::max(m_resolvers->syncedUploadMtu(), 150);
        cfg.maxDownload = std::max(m_resolvers->syncedDownloadMtu(), 4096);
        cfg.baseEncodeReply = false; // mirrors SESSION_INIT byte 0 = 0
        m_probers.push_back(std::move(prober));
        m_probers.back()->start(cfg);
    }
}

void Session::onProbeNextRequested(int resolverIndex,
                                    PacketType type,
                                    const QByteArray &payload,
                                    bool /*isUpload*/)
{
    if (!m_resolvers || resolverIndex < 0 || resolverIndex >= m_resolvers->resolverCount()) {
        return;
    }

    // Build the inner-VPN packet per spec §9 / mtu.go:1369-1378. The MTU
    // probe phase uses a magic SessionID = 0xFF and Cookie = 0 (the
    // pre-session sentinel), StreamID/SeqNum/FragId = (1, 1, 0/1) just
    // like upstream's buildMTUProbeQuery does.
    Packet inner;
    inner.sessionId = 0xFF;
    inner.cookie = 0;
    inner.type = type;
    inner.streamId = 1;
    inner.sequenceNum = 1;
    inner.fragmentId = 0;
    inner.totalFragments = 1;
    inner.compression = 0;
    inner.payload = payload;

    const QByteArray plaintext = encode(inner);
    const QByteArray encoded = sealAndEncode(plaintext);
    if (encoded.isEmpty()) {
        return;
    }

    // Send to THIS resolver only — not duplicated across the pool. Probe
    // results are per-resolver and must not be cross-contaminated.
    const QString domain = [&]() -> QString {
        // Look up tunnel domain via a pickPrimary() round if needed;
        // for now we trust the pool to expose it inline with the send.
        // ResolverPool::pickPrimary uses balancing — but we want a
        // specific index. Fall back to the first tunnel domain the
        // operator configured.
        return m_tunnelDomains.isEmpty() ? QStringLiteral("") : m_tunnelDomains.first();
    }();
    if (domain.isEmpty()) {
        return;
    }
    const quint16 txId = nextTransactionId();
    const QByteArray dnsBytes = buildQuery(txId, encoded, domain);
    m_outstandingQueries.insert(txId, resolverIndex);
    m_resolvers->send(resolverIndex, dnsBytes);
    m_bytesTx += dnsBytes.size();
}

void Session::onProbeFinished(int resolverIndex, bool ok, int uploadMtu, int downloadMtu)
{
    if (resolverIndex < 0 || resolverIndex >= m_probeResults.size()) {
        return;
    }
    if (m_probeResults[resolverIndex].finished) {
        // Defensive — should never happen since MtuProber emits exactly
        // one terminal signal per `start()` lifecycle.
        return;
    }
    m_probeResults[resolverIndex].finished = true;
    m_probeResults[resolverIndex].ok = ok;
    m_probeResults[resolverIndex].uploadMtu = uploadMtu;
    m_probeResults[resolverIndex].downloadMtu = downloadMtu;

    if (!ok && m_resolvers) {
        // Spec §9 — resolvers that fail MTU probing get pulled from the
        // active set so the dispatcher never tries to send through a
        // resolver that can't carry our packets.
        m_resolvers->markResolverInactive(resolverIndex);
    }
    --m_probesPending;
    maybeFinaliseMtuProbeSweep();
}

void Session::maybeFinaliseMtuProbeSweep()
{
    if (m_probesPending > 0) {
        return;
    }
    // Aggregate min upload/download across the resolvers that succeeded.
    // If none succeeded, the conservative defaults published at start
    // remain in effect — SESSION_INIT will go out at the safe minimum.
    int minUp = INT_MAX;
    int minDown = INT_MAX;
    int okCount = 0;
    for (const ProbeOutcome &r : m_probeResults) {
        if (!r.ok) continue;
        ++okCount;
        if (r.uploadMtu < minUp)   minUp = r.uploadMtu;
        if (r.downloadMtu < minDown) minDown = r.downloadMtu;
    }
    if (okCount > 0) {
        m_resolvers->setSyncedMtu(minUp, minDown);
    } else {
        // Re-emit setSyncedMtu with the existing values just to fire the
        // readyForUse signal — this gates Session::onResolverPoolReady.
        m_resolvers->setSyncedMtu(m_resolvers->syncedUploadMtu(),
                                  m_resolvers->syncedDownloadMtu());
    }
    // Probers can be released — they've done their job. Clearing the
    // vector also nulls out the pointers used by onInnerPacket's MTU
    // response router, which is correct because no more probes are
    // outstanding from this point forward.
    m_probers.clear();
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

    // Spec §8: apply the negotiated upload codec to packet types that
    // carry the compression extension. `prepareOutgoingPayload` is a
    // no-op for non-eligible types, undersized payloads, or when the
    // compressed result isn't smaller than the input — matching upstream's
    // `PreparePayload` semantics (internal/vpnproto/payload.go:19-31).
    Packet outbound = packet;
    auto [payload, codec] = compression::prepareOutgoingPayload(
            outbound.type, outbound.payload,
            static_cast<quint8>(m_uploadCompression),
            m_compressionMinSize);
    outbound.payload = payload;
    outbound.compression = codec;

    const QByteArray plaintext = encode(outbound);
    const QByteArray encoded = sealAndEncode(plaintext);
    if (encoded.isEmpty()) {
        return;
    }
    const auto picks = isSetupPacket
            ? m_resolvers->pickDuplicates(m_setupPacketDuplication, /*setup=*/true)
            : m_resolvers->pickDuplicates(m_packetDuplication, /*setup=*/false);
    for (const ResolverPick &pick : picks) {
        const quint16 txId = nextTransactionId();
        const QByteArray dnsBytes = buildQuery(txId, encoded, pick.tunnelDomain);
        m_outstandingQueries.insert(txId, pick.index);
        m_resolvers->send(pick.index, dnsBytes);
        m_bytesTx += dnsBytes.size();
    }

    // Spec §12: every outbound packet feeds the tiered-pacing FSM so the
    // next PING is scheduled against actual conversation activity, not a
    // static interval.
    m_pingState.notify(packet.type, /*inbound=*/false, QDateTime::currentMSecsSinceEpoch());
}

// ---------------------------------------------------------------------------
// Inbound
// ---------------------------------------------------------------------------

void Session::onResolverResponse(int resolverIndex, quint16 transactionId, const QByteArray &bytes)
{
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

    // Spec §8: if the per-packet compression extension is non-zero, the
    // payload was compressed by the peer. Inflate it before dispatching
    // so consumers downstream operate on the original plaintext. A
    // compression byte of 0 is a pass-through. Mirrors upstream's
    // `InflatePayload` (internal/vpnproto/payload.go:34-45) — a corrupt
    // stream silently drops the packet rather than crashing.
    if (pkt->compression.has_value() && pkt->compression.value() != compression::TypeOff) {
        auto inflated = compression::tryDecompressPayload(pkt->payload, *pkt->compression);
        if (!inflated) {
            return;
        }
        pkt->payload = *inflated;
        pkt->compression = compression::TypeOff;
    }

    onInnerPacket(*pkt, resolverIndex);
}

void Session::onInnerPacket(const Packet &packet, int resolverIndex)
{
    // Spec §12: every inbound packet feeds the tiered-pacing FSM so the
    // next PING is scheduled against actual conversation activity, not a
    // static interval.
    m_pingState.notify(packet.type, /*inbound=*/true, QDateTime::currentMSecsSinceEpoch());

    // Spec §9 MTU probe responses are routed back to the prober that owns
    // the outstanding probe for this resolver. They never reach the rest
    // of the dispatch — and conversely, no other code path constructs an
    // MtuUpRes/MtuDownRes packet, so this is the only consumer.
    if (packet.type == PacketType::MtuUpRes || packet.type == PacketType::MtuDownRes) {
        if (resolverIndex >= 0 && resolverIndex < m_probers.size() && m_probers[resolverIndex]) {
            m_probers[resolverIndex]->feedResponse(packet.type, packet.payload);
        }
        return;
    }

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
            // Packed-block content never carries MTU probe responses
            // (those types aren't in the packable catalogue), so the
            // resolverIndex is irrelevant here — pass -1 to make that
            // explicit. Real MTU responses arrive as standalone packets.
            onInnerPacket(synthetic, /*resolverIndex=*/-1);
        }
        return;
    }
    case PacketType::ErrorDrop:
        // §7.6 — server says "I don't recognise this cookie". The server has
        // forgotten any prior in-flight handshake, so spec §13(9)'s
        // persistence window has closed: mint a fresh verify code on the
        // next SESSION_INIT.
        m_sessionId = 0;
        m_sessionCookie = 0;
        m_initVerifyCode.clear();
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

    auto stream = std::make_unique<ArqStream>(
            streamId, m_arqCfg,
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

    // Spec §12 tiered ping pacing. The interval is recomputed every tick
    // against live conversation timestamps; we fire a PING only when the
    // configured tier interval has elapsed since the last PING.
    if (m_state == State::Established) {
        const qint64 interval = pingNextIntervalMs(m_pingPacing, m_pingState, now);
        if (now - m_pingState.lastPingSentMs >= interval) {
            emitPing(now);
        }
    }

    // Spec §9 MTU probers are passive — they only emit `nextProbe` when
    // a response arrives. We drive their timeout deadlines here so a
    // resolver that goes silent doesn't stall the whole sweep forever.
    if (m_state == State::MtuProbing) {
        for (auto &p : m_probers) {
            if (p) {
                p->tick(now);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Handshake
// ---------------------------------------------------------------------------

void Session::sendSessionInit()
{
    const int upMtu = std::max(64, m_resolvers->syncedUploadMtu());
    const int downMtu = std::max(255, m_resolvers->syncedDownloadMtu());

    // Spec §13(9): verify code persists across SESSION_INIT retries within
    // one handshake lifecycle. A fresh value is only minted at lifecycle
    // boundaries (Session::start, ErrorDrop re-init); back-to-back retries
    // (SESSION_BUSY, no-response) reuse the cached code so any accept that
    // arrives for a prior in-flight attempt still validates.
    QByteArray payload = buildSessionInitPayload(upMtu, downMtu,
                                                  m_uploadCompression, m_downloadCompression,
                                                  &m_initVerifyCode);

    Packet init;
    init.sessionId = 0; // pre-session
    init.cookie = 0;
    init.type = PacketType::SessionInit;
    init.payload = payload;
    sendPacket(init, /*isSetupPacket=*/true);
}

void Session::onSessionAccept(const Packet &packet)
{
    const auto decoded = decodeSessionAcceptPayload(packet.payload);
    if (!decoded) {
        return;
    }
    const QByteArray echoVerify(reinterpret_cast<const char *>(decoded->verifyCode.data()), 4);
    if (echoVerify != m_initVerifyCode) {
        return;
    }
    m_sessionId = decoded->sessionId;
    m_sessionCookie = decoded->sessionCookie;

    // Server may have downgraded our compression preference; record what
    // it actually permitted so future packets honour the constraint.
    m_uploadCompression = std::min(m_uploadCompression,
                                   (decoded->compressionPair >> 4) & 0x0F);
    m_downloadCompression = std::min(m_downloadCompression,
                                     decoded->compressionPair & 0x0F);

    // §7 client-policy sync: capture the server-declared per-client caps,
    // then clamp the local config knobs against them. Streams created
    // after this point honour the clamped config; the resolver pool's
    // duplication / MTU caps are tightened immediately so the next
    // sendPacket() and pickDuplicates() see them.
    if (decoded->hasClientPolicySync) {
        m_serverPolicy = decoded->clientPolicy;
        m_hasServerPolicy = true;
        applyServerPolicy();
    }

    // Spec §13(9): retire the verify code once the handshake completes.
    m_initVerifyCode.clear();
    setState(State::Established);
}

void Session::applyServerPolicy()
{
    // Mirrors upstream `Client.applySessionClientPolicy`
    // (internal/client/session.go:182). The pattern is one-way: caps
    // tighten the existing knobs (never relax them). Each branch is
    // gated on a positive policy value so a zero-default policy field
    // is treated as "no opinion".
    const SessionAcceptClientPolicy &p = m_serverPolicy;

    // ARQ window — clamp from above.
    if (p.maxARQWindowSize > 0) {
        m_arqCfg.windowSize = std::min(m_arqCfg.windowSize, p.maxARQWindowSize);
    }
    // NACK gap — clamp from above.
    if (p.maxARQDataNackMaxGap > 0) {
        m_arqCfg.dataNackMaxGap = std::min(m_arqCfg.dataNackMaxGap, p.maxARQDataNackMaxGap);
    }
    // ARQ initial RTO — server may FLOOR (raise) the minimum.
    if (p.minARQInitialRTOSeconds > 0.0) {
        const qint64 floorMs = static_cast<qint64>(
                std::round(p.minARQInitialRTOSeconds * 1000.0));
        m_arqCfg.initialDataRtoMs = std::max(m_arqCfg.initialDataRtoMs, floorMs);
        m_arqCfg.initialControlRtoMs =
                std::max(m_arqCfg.initialControlRtoMs, floorMs);
    }
    // Ping aggressive interval — server may FLOOR.
    if (p.minPingAggressiveInterval > 0.0) {
        const qint64 floorMs = static_cast<qint64>(
                std::round(p.minPingAggressiveInterval * 1000.0));
        m_pingPacing.aggressiveMs = std::max(m_pingPacing.aggressiveMs, floorMs);
    }
    // Compression min-size — server may RAISE (so small payloads aren't
    // compressed against the server's preference).
    if (p.minCompressionMinSize > 0) {
        m_compressionMinSize = std::max(m_compressionMinSize, p.minCompressionMinSize);
    }
    // Duplication counts — server caps from above.
    if (p.maxPacketDuplicationCount > 0) {
        m_packetDuplication = std::min(m_packetDuplication, p.maxPacketDuplicationCount);
    }
    if (p.maxSetupDuplicationCount > 0) {
        m_setupPacketDuplication = std::min(m_setupPacketDuplication,
                                            p.maxSetupDuplicationCount);
        // Setup count must still cover packet count — match
        // pool::configure() clamp semantics.
        m_setupPacketDuplication = std::max(m_setupPacketDuplication, m_packetDuplication);
    }
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

// ---------------------------------------------------------------------------
// §12 ping pacing — synthesises the PING packet; tier-selection math + the
// notify bookkeeping live in pingpacer.h so they're unit-testable.
// ---------------------------------------------------------------------------

void Session::emitPing(qint64 /*now*/)
{
    Packet ping;
    ping.sessionId = m_sessionId;
    ping.cookie = m_sessionCookie;
    ping.type = PacketType::Ping;
    // PING is in `kNone` extensions per §3.4, so no stream/seq is serialised
    // even if set — the encoder drops the optional fields by packet type.

    // §3.4 PING payload: 7 bytes — `P`, `O`, `:`, then 4 random bytes.
    QByteArray payload;
    payload.reserve(7);
    payload.append('P');
    payload.append('O');
    payload.append(':');
    payload.append(randomBytes(4));
    ping.payload = payload;

    sendPacket(ping, /*isSetupPacket=*/false);
    // `sendPacket` -> notifyPacket(Ping, outbound) already advances
    // `m_pingState.lastPingSentMs`; no manual update needed here.
}

} // namespace amnezia::masterdnsvpn
