// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for the native MasterDnsVPN engine — per-layer, no network I/O.
// Exercises crypto, base codecs, DNS framing, wire framing, packed control
// blocks, and the ARQ state machine in isolation. Network-dependent paths
// (resolver pool, full session handshake) belong in an `--ignored`
// integration suite that runs against a real server.

#include "masterdnsvpn/arq.h"
#include "masterdnsvpn/crypto.h"
#include "masterdnsvpn/dnsframing.h"
#include "masterdnsvpn/pingpacer.h"
#include "masterdnsvpn/wireframing.h"

#include <QByteArray>
#include <QJsonArray>
#include <QSignalSpy>
#include <QTest>
#include <QVector>

using namespace amnezia::masterdnsvpn;

class TestMasterDnsVpnEngine : public QObject
{
    Q_OBJECT

private slots:
    // ----- Crypto -----------------------------------------------------------

    void cipherNoneIsPassthrough()
    {
        Cipher c;
        QVERIFY(c.init(CipherMethod::None, {}));
        QByteArray sealed;
        QVERIFY(c.seal(QByteArrayLiteral("hello"), {}, {}, sealed));
        QCOMPARE(sealed, QByteArrayLiteral("hello"));
        QByteArray opened;
        QVERIFY(c.open(sealed, {}, {}, opened));
        QCOMPARE(opened, QByteArrayLiteral("hello"));
    }

    void cipherXorIsInvolutive()
    {
        // Per the spec, XOR uses the raw key zero-padded to 32 bytes. The
        // operator passphrase is treated as UTF-8 bytes, not hex-decoded.
        const QString pass = QStringLiteral("operator-shared-secret");
        const QByteArray key = deriveKey(CipherMethod::Xor, pass);
        QCOMPARE(key.size(), 32);

        Cipher c;
        QVERIFY(c.init(CipherMethod::Xor, key));

        const QByteArray pt = QByteArrayLiteral("the quick brown fox jumps over the lazy dog");
        QByteArray sealed;
        QVERIFY(c.seal(pt, {}, {}, sealed));
        QVERIFY(sealed != pt); // some byte must change
        QCOMPARE(sealed.size(), pt.size());

        QByteArray opened;
        QVERIFY(c.open(sealed, {}, {}, opened));
        QCOMPARE(opened, pt);
    }

    void cipherChaCha20RoundTrip()
    {
        const QString pass = QStringLiteral("chacha-passphrase");
        const QByteArray key = deriveKey(CipherMethod::ChaCha20, pass);
        QCOMPARE(key.size(), 32);

        Cipher sealer;
        Cipher opener;
        QVERIFY(sealer.init(CipherMethod::ChaCha20, key));
        QVERIFY(opener.init(CipherMethod::ChaCha20, key));

        // 16-byte nonce per the spec (4 LE counter + 12 ChaCha20 nonce).
        QByteArray nonce(16, '\0');
        for (int i = 0; i < nonce.size(); ++i) {
            nonce[i] = static_cast<char>(i);
        }

        const QByteArray pt = QByteArrayLiteral("Lorem ipsum dolor sit amet, "
                                                "consectetur adipiscing elit");
        QByteArray sealed;
        QVERIFY(sealer.seal(pt, nonce, {}, sealed));
        QCOMPARE(sealed.size(), pt.size()); // stream cipher; no MAC

        QByteArray opened;
        QVERIFY(opener.open(sealed, nonce, {}, opened));
        QCOMPARE(opened, pt);
    }

    void cipherAesGcmRoundTripAndTagFailure()
    {
        const QString pass = QStringLiteral("aes-passphrase");
        const QByteArray key = deriveKey(CipherMethod::Aes256Gcm, pass);
        QCOMPARE(key.size(), 32);

        Cipher sealer;
        Cipher opener;
        QVERIFY(sealer.init(CipherMethod::Aes256Gcm, key));
        QVERIFY(opener.init(CipherMethod::Aes256Gcm, key));

        QByteArray nonce(12, '\0');
        for (int i = 0; i < nonce.size(); ++i) {
            nonce[i] = static_cast<char>(0x80 + i);
        }
        const QByteArray pt = QByteArrayLiteral("AES-256-GCM authenticated payload");

        QByteArray sealed;
        QVERIFY(sealer.seal(pt, nonce, {}, sealed));
        QCOMPARE(sealed.size(), pt.size() + authTagBytes(CipherMethod::Aes256Gcm));

        QByteArray opened;
        QVERIFY(opener.open(sealed, nonce, {}, opened));
        QCOMPARE(opened, pt);

        // Flip one byte in the ciphertext — open() must reject.
        sealed[3] = sealed[3] ^ 0x01;
        QByteArray tampered;
        QVERIFY(!opener.open(sealed, nonce, {}, tampered));
    }

    void aes128UsesMd5KeyDerivation()
    {
        // Spec §5 / §5.4 — Method 3 is *MD5* of the raw key, not SHA-256.
        // Implementations that get this wrong will produce different
        // ciphertexts than the upstream server and the tunnel won't open.
        const QByteArray key = deriveKey(CipherMethod::Aes128Gcm, QStringLiteral("test"));
        QCOMPARE(key.size(), 16);
        // Known MD5 of ASCII "test": 098f6bcd4621d373cade4e832627b4f6
        QCOMPARE(key.toHex(), QByteArrayLiteral("098f6bcd4621d373cade4e832627b4f6"));
    }

    void aes192PadsRawKey()
    {
        // Spec §5 — Method 4 zero-pads the UTF-8 key to 24 B (or truncates
        // if longer). MD5/SHA aren't involved.
        const QByteArray key = deriveKey(CipherMethod::Aes192Gcm, QStringLiteral("abc"));
        QCOMPARE(key.size(), 24);
        QCOMPARE(key.left(3), QByteArrayLiteral("abc"));
        // Bytes 3..23 must be zero-padded.
        for (int i = 3; i < 24; ++i) {
            QCOMPARE(static_cast<quint8>(key[i]), quint8 { 0 });
        }
    }

    // ----- Base codecs -----------------------------------------------------

    void base36RoundTripsAllTailLengths()
    {
        // Every (block + tail) shape must round-trip. The spec's tail-byte
        // table is encoder-side; we verify by encoding random data of each
        // possible mod-7 length and confirming decode reverses it.
        for (int len = 0; len < 25; ++len) {
            QByteArray raw(len, '\0');
            for (int i = 0; i < len; ++i) {
                raw[i] = static_cast<char>(i * 7 + 3);
            }
            const QByteArray encoded = encodeBase36(raw);
            const auto decoded = decodeBase36(encoded);
            QVERIFY2(decoded.has_value(),
                     qPrintable(QString("base36 decode failed for len %1").arg(len)));
            QCOMPARE(*decoded, raw);
        }
    }

    void base36DecoderRejectsInvalidLength()
    {
        // §5.6: encoded lengths 1, 3, 6, 9 modulo 11 are illegal.
        const QList<int> illegal { 1, 3, 6, 9, 12, 14, 17, 20 };
        for (int n : illegal) {
            QByteArray garbage(n, 'a');
            QVERIFY2(!decodeBase36(garbage).has_value(),
                     qPrintable(QString("base36 should reject length %1").arg(n)));
        }
    }

    void base36DecoderIsCaseInsensitive()
    {
        const QByteArray raw = QByteArrayLiteral("hello, world");
        const QByteArray encoded = encodeBase36(raw);
        const QByteArray upper = encoded.toUpper();
        const auto decoded = decodeBase36(upper);
        QVERIFY(decoded.has_value());
        QCOMPARE(*decoded, raw);
    }

    void base32RoundTrips()
    {
        for (int len = 0; len <= 16; ++len) {
            QByteArray raw(len, '\0');
            for (int i = 0; i < len; ++i) {
                raw[i] = static_cast<char>(0xA0 + i);
            }
            const QByteArray encoded = encodeBase32(raw);
            const auto decoded = decodeBase32(encoded);
            QVERIFY2(decoded.has_value(),
                     qPrintable(QString("base32 decode failed for len %1").arg(len)));
            QCOMPARE(*decoded, raw);
        }
    }

    // ----- DNS framing -----------------------------------------------------

    void dnsQueryHasExpectedShape()
    {
        const QByteArray encoded = encodeBase36(QByteArrayLiteral("payload"));
        const QByteArray wire = buildQuery(0x1234, encoded, QStringLiteral("v.example.com"));

        QVERIFY(!wire.isEmpty());
        // Header: ID, flags, QDCount=1, ANCount=0, NSCount=0, ARCount=1 (OPT).
        QCOMPARE(static_cast<quint8>(wire[0]), quint8 { 0x12 });
        QCOMPARE(static_cast<quint8>(wire[1]), quint8 { 0x34 });
        QCOMPARE(static_cast<quint8>(wire[2]), quint8 { 0x01 }); // flags hi
        QCOMPARE(static_cast<quint8>(wire[3]), quint8 { 0x00 }); // flags lo
        QCOMPARE(static_cast<quint8>(wire[5]), quint8 { 0x01 }); // QDCount lo
        QCOMPARE(static_cast<quint8>(wire[11]), quint8 { 0x01 }); // ARCount lo
    }

    void maxFrameBytesIsConservative()
    {
        // For a 13-char domain ("v.example.com") the QNAME budget is
        // ~239 bytes after subtracting label-length overhead; base36
        // gives roughly 7/11 of the encoded budget back as raw bytes.
        // Bounds-check rather than pinning to an exact value.
        const int budget = maxFrameBytes(QStringLiteral("v.example.com"), false);
        QVERIFY(budget > 100);
        QVERIFY(budget < 200);
    }

    // ----- Wire framing ----------------------------------------------------

    void wirePacketRoundTripsSimpleAck()
    {
        Packet ack;
        ack.sessionId = 0x42;
        ack.type = PacketType::StreamDataAck;
        ack.streamId = 0x1234;
        ack.sequenceNum = 0x5678;
        ack.cookie = 0x99;

        const QByteArray wire = encode(ack);
        // 2 base + 4 (S+N extensions for StreamDataAck) + 2 footer = 8 bytes
        QCOMPARE(wire.size(), 8);

        const auto decoded = decode(wire);
        QVERIFY(decoded.has_value());
        QCOMPARE(decoded->sessionId, ack.sessionId);
        QCOMPARE(static_cast<int>(decoded->type), static_cast<int>(ack.type));
        QCOMPARE(decoded->streamId.value_or(0), ack.streamId.value_or(0));
        QCOMPARE(decoded->sequenceNum.value_or(0), ack.sequenceNum.value_or(0));
        QCOMPARE(decoded->cookie, ack.cookie);
    }

    void wirePacketRoundTripsStreamData()
    {
        Packet data;
        data.sessionId = 0x7F;
        data.type = PacketType::StreamData;
        data.streamId = 1;
        data.sequenceNum = 42;
        data.fragmentId = 2;
        data.totalFragments = 5;
        data.compression = 0;
        data.cookie = 0x10;
        data.payload = QByteArrayLiteral("payload-bytes");

        const QByteArray wire = encode(data);
        const auto decoded = decode(wire);
        QVERIFY(decoded.has_value());
        QCOMPARE(decoded->payload, data.payload);
        QCOMPARE(*decoded->fragmentId, *data.fragmentId);
        QCOMPARE(*decoded->totalFragments, *data.totalFragments);
    }

    void wirePacketDecodeRejectsTamperedCheckByte()
    {
        Packet p;
        p.sessionId = 1;
        p.type = PacketType::Ping;
        p.cookie = 1;
        QByteArray wire = encode(p);
        // Flip the trailing check byte; decode must reject.
        wire[wire.size() - 1] = wire[wire.size() - 1] ^ 0x01;
        QVERIFY(!decode(wire).has_value());
    }

    void packedControlBlocksRoundTrip()
    {
        QVector<PackedBlock> blocks;
        PackedBlock b1 { PacketType::StreamDataAck, 100, 200, 0, 1 };
        PackedBlock b2 { PacketType::Socks5ConnectedAck, 101, 0, 0, 1 };
        PackedBlock b3 { PacketType::StreamDataNack, 100, 199, 0, 1 };
        blocks << b1 << b2 << b3;

        const QByteArray packed = packBlocks(blocks);
        QCOMPARE(packed.size(), 3 * 7);

        const auto unpacked = unpackBlocks(packed);
        QCOMPARE(unpacked.size(), 3);
        QCOMPARE(static_cast<int>(unpacked[0].type), static_cast<int>(b1.type));
        QCOMPARE(unpacked[0].streamId, b1.streamId);
        QCOMPARE(unpacked[0].sequenceNum, b1.sequenceNum);
        QCOMPARE(static_cast<int>(unpacked[1].type), static_cast<int>(b2.type));
        QCOMPARE(unpacked[1].streamId, b2.streamId);
        QCOMPARE(static_cast<int>(unpacked[2].type), static_cast<int>(b3.type));
    }

    void packedBlockUnpackIgnoresPartialTail()
    {
        // Append 5 trailing bytes — receiver must silently discard.
        QByteArray packed = packBlocks(QVector<PackedBlock> { { PacketType::Ping, 0, 0, 0, 1 } });
        packed.append("xxxxx", 5);
        const auto unpacked = unpackBlocks(packed);
        QCOMPARE(unpacked.size(), 1);
    }

    void packableTypeCatalogue()
    {
        QVERIFY(isPackableControl(PacketType::StreamDataAck));
        QVERIFY(isPackableControl(PacketType::StreamDataNack));
        QVERIFY(isPackableControl(PacketType::Socks5ConnectedAck));
        QVERIFY(isPackableControl(PacketType::DnsQueryReqAck));
        // Data packets are NOT packable.
        QVERIFY(!isPackableControl(PacketType::StreamData));
        QVERIFY(!isPackableControl(PacketType::SessionInit));
        QVERIFY(!isPackableControl(PacketType::Ping));
    }

    // ----- ARQ state machine -----------------------------------------------

    void arqInOrderDataDelivers()
    {
        QVector<Packet> sent;
        QVector<QByteArray> delivered;
        ArqConfig cfg;
        ArqStream stream(
                1, cfg,
                [&sent](const ArqOutbound &o) { sent.append(o.packet); },
                [&delivered](const ArqDelivery &d) {
                    if (!d.bytes.isEmpty()) delivered.append(d.bytes);
                });

        // Simulate incoming STREAM_DATA seq=1 ... seq=3 in order.
        for (quint16 seq = 1; seq <= 3; ++seq) {
            Packet p;
            p.type = PacketType::StreamData;
            p.streamId = 1;
            p.sequenceNum = seq;
            p.payload = QByteArray(1, 'a' + seq - 1);
            stream.onPacketReceived(p);
        }
        QCOMPARE(delivered.size(), 3);
        QCOMPARE(delivered[0], QByteArrayLiteral("a"));
        QCOMPARE(delivered[1], QByteArrayLiteral("b"));
        QCOMPARE(delivered[2], QByteArrayLiteral("c"));

        // Each data packet should produce one STREAM_DATA_ACK.
        int ackCount = 0;
        for (const Packet &p : sent) {
            if (p.type == PacketType::StreamDataAck) ++ackCount;
        }
        QCOMPARE(ackCount, 3);
    }

    void arqOutOfOrderBuffersUntilContiguous()
    {
        QVector<QByteArray> delivered;
        ArqConfig cfg;
        ArqStream stream(
                1, cfg,
                [](const ArqOutbound &) {},
                [&delivered](const ArqDelivery &d) {
                    if (!d.bytes.isEmpty()) delivered.append(d.bytes);
                });

        // seq=3 arrives before seq=1 and seq=2.
        Packet third;
        third.type = PacketType::StreamData;
        third.streamId = 1;
        third.sequenceNum = 3;
        third.payload = QByteArrayLiteral("c");
        stream.onPacketReceived(third);
        QCOMPARE(delivered.size(), 0); // held in rcvBuf

        Packet first;
        first.type = PacketType::StreamData;
        first.streamId = 1;
        first.sequenceNum = 1;
        first.payload = QByteArrayLiteral("a");
        stream.onPacketReceived(first);
        QCOMPARE(delivered.size(), 1); // "a" delivered, "c" still held

        Packet second;
        second.type = PacketType::StreamData;
        second.streamId = 1;
        second.sequenceNum = 2;
        second.payload = QByteArrayLiteral("b");
        stream.onPacketReceived(second);
        QCOMPARE(delivered.size(), 3); // "b" + drained "c"
        QCOMPARE(delivered[1], QByteArrayLiteral("b"));
        QCOMPARE(delivered[2], QByteArrayLiteral("c"));
    }

    void arqDuplicatePacketProducesAckAndDropsPayload()
    {
        QVector<Packet> sent;
        QVector<QByteArray> delivered;
        ArqConfig cfg;
        ArqStream stream(
                1, cfg,
                [&sent](const ArqOutbound &o) { sent.append(o.packet); },
                [&delivered](const ArqDelivery &d) {
                    if (!d.bytes.isEmpty()) delivered.append(d.bytes);
                });

        Packet p;
        p.type = PacketType::StreamData;
        p.streamId = 1;
        p.sequenceNum = 1;
        p.payload = QByteArrayLiteral("hello");
        stream.onPacketReceived(p);
        stream.onPacketReceived(p); // duplicate

        QCOMPARE(delivered.size(), 1); // payload delivered exactly once
        int acks = 0;
        for (const Packet &s : sent) {
            if (s.type == PacketType::StreamDataAck) ++acks;
        }
        QCOMPARE(acks, 2); // both arrivals ACKed
    }

    void arqWriteEmitsStreamData()
    {
        QVector<Packet> sent;
        ArqConfig cfg;
        ArqStream stream(
                1, cfg,
                [&sent](const ArqOutbound &o) { sent.append(o.packet); },
                [](const ArqDelivery &) {});

        const qsizetype written = stream.writeApp(QByteArrayLiteral("hello"));
        QCOMPARE(written, 5);
        QCOMPARE(sent.size(), 1);
        QCOMPARE(static_cast<int>(sent[0].type), static_cast<int>(PacketType::StreamData));
        QCOMPARE(sent[0].payload, QByteArrayLiteral("hello"));
        QCOMPARE(stream.inFlightCount(), 1);
    }

    void arqAckClearsInFlight()
    {
        QVector<Packet> sent;
        ArqConfig cfg;
        ArqStream stream(
                1, cfg,
                [&sent](const ArqOutbound &o) { sent.append(o.packet); },
                [](const ArqDelivery &) {});

        stream.writeApp(QByteArrayLiteral("hello"));
        QCOMPARE(stream.inFlightCount(), 1);

        Packet ack;
        ack.type = PacketType::StreamDataAck;
        ack.streamId = 1;
        ack.sequenceNum = sent[0].sequenceNum;
        stream.onPacketReceived(ack);
        QCOMPARE(stream.inFlightCount(), 0);
    }

    void arqHalfCloseTransitionsState()
    {
        ArqConfig cfg;
        ArqStream stream(
                1, cfg,
                [](const ArqOutbound &) {},
                [](const ArqDelivery &) {});
        QCOMPARE(static_cast<int>(stream.state()), static_cast<int>(ArqState::Open));

        stream.halfCloseWrite();
        QCOMPARE(static_cast<int>(stream.state()),
                 static_cast<int>(ArqState::HalfClosedLocal));
    }

    // ----- §12 ping pacing FSM ----------------------------------------------

    void pingPacerStartsInAggressiveTier()
    {
        // Seeded state mimics a freshly-handshaken session — all four
        // timestamps equal `now`, so idle is 0 and we must be in the
        // aggressive tier irrespective of any threshold.
        PingPacingConfig cfg;
        PingPacingState state;
        state.seed(1'000'000);
        QCOMPARE(pingNextIntervalMs(cfg, state, 1'000'000), cfg.aggressiveMs);
    }

    void pingPacerPromotesThroughTiersAsTrafficQuiets()
    {
        PingPacingConfig cfg; // 100 / 750 / 2000 / 15000 ; thresholds 8000 / 20000 / 30000
        PingPacingState state;
        state.seed(0);
        const qint64 lazyEntry     = cfg.warmThreshMs;        //  8000
        const qint64 cooldownEntry = cfg.coolThreshMs;        // 20000
        const qint64 coldEntry     = cfg.coldThreshMs;        // 30000

        QCOMPARE(pingNextIntervalMs(cfg, state, cfg.warmThreshMs - 1), cfg.aggressiveMs);
        QCOMPARE(pingNextIntervalMs(cfg, state, lazyEntry),            cfg.lazyMs);
        QCOMPARE(pingNextIntervalMs(cfg, state, cfg.coolThreshMs - 1), cfg.lazyMs);
        QCOMPARE(pingNextIntervalMs(cfg, state, cooldownEntry),        cfg.cooldownMs);
        QCOMPARE(pingNextIntervalMs(cfg, state, cfg.coldThreshMs - 1), cfg.cooldownMs);
        QCOMPARE(pingNextIntervalMs(cfg, state, coldEntry),            cfg.coldMs);
        QCOMPARE(pingNextIntervalMs(cfg, state, coldEntry + 1'000'000), cfg.coldMs);
    }

    void pingPacerNotifyResetsConversationTimers()
    {
        // After 25s of idle (cooldown tier), a single non-PING send must
        // pull us back to aggressive — the FSM is `min(idleSent, idleRecv)`
        // so reviving one direction is enough.
        PingPacingConfig cfg;
        PingPacingState state;
        state.seed(0);
        QCOMPARE(pingNextIntervalMs(cfg, state, 25'000), cfg.cooldownMs);

        state.notify(PacketType::StreamData, /*inbound=*/false, /*now=*/25'000);
        QCOMPARE(pingNextIntervalMs(cfg, state, 25'000), cfg.aggressiveMs);
    }

    void pingPacerPingPongDoNotResetConversationTimers()
    {
        // Pings/pongs are the keepalive itself — they must NOT count as
        // conversation traffic, or the tier would never advance. The FSM
        // only looks at non-ping/non-pong activity for tier selection.
        PingPacingConfig cfg;
        PingPacingState state;
        state.seed(0);
        QCOMPARE(pingNextIntervalMs(cfg, state, 25'000), cfg.cooldownMs);

        state.notify(PacketType::Ping, /*inbound=*/false, /*now=*/25'000);
        state.notify(PacketType::Pong, /*inbound=*/true,  /*now=*/25'000);
        QCOMPARE(pingNextIntervalMs(cfg, state, 25'000), cfg.cooldownMs);
    }

    void pingPacerInboundOnlyTrafficKeepsAggressive()
    {
        // Asymmetric: server sends data, client only acks. Even with no
        // outbound non-ping traffic, the warm-threshold check is an OR
        // across the two directions, so we stay aggressive while the
        // server is talking to us.
        PingPacingConfig cfg;
        PingPacingState state;
        state.seed(0);
        QCOMPARE(pingNextIntervalMs(cfg, state, 25'000), cfg.cooldownMs);

        state.notify(PacketType::StreamData, /*inbound=*/true, /*now=*/25'000);
        QCOMPARE(pingNextIntervalMs(cfg, state, 25'000), cfg.aggressiveMs);
    }

    void pingPacerLastPingTimestampUpdatedOnPingSend()
    {
        // Sending a PING must update `lastPingSentMs` but NOT
        // `lastNonPingSentMs` — otherwise the FSM would falsely interpret
        // a keepalive as conversation activity.
        PingPacingState state;
        state.seed(1'000);
        const qint64 before = state.lastNonPingSentMs;
        state.notify(PacketType::Ping, /*inbound=*/false, /*now=*/5'000);
        QCOMPARE(state.lastPingSentMs, qint64(5'000));
        QCOMPARE(state.lastNonPingSentMs, before);
    }
};

QTEST_MAIN(TestMasterDnsVpnEngine)
#include "testMasterDnsVpnEngine.moc"
