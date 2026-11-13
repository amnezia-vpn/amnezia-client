/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <functional>
#include <memory>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QTimer>
#include <QUdpSocket>

#include "daemon/dnswildcardrouter.h"
#include "daemon/wildcardroutetable.h"
#include "daemon/wildcardsite.h"

namespace {

QByteArray encodeName(const QList<QByteArray> &labels)
{
    QByteArray wire;
    for (const QByteArray &label : labels) {
        wire.append(static_cast<char>(label.size()));
        wire.append(label);
    }
    wire.append('\0');
    return wire;
}

QByteArray u16(quint16 value)
{
    QByteArray out;
    out.append(static_cast<char>((value >> 8) & 0xFF));
    out.append(static_cast<char>(value & 0xFF));
    return out;
}

QByteArray u32(quint32 value)
{
    QByteArray out;
    out.append(static_cast<char>((value >> 24) & 0xFF));
    out.append(static_cast<char>((value >> 16) & 0xFF));
    out.append(static_cast<char>((value >> 8) & 0xFF));
    out.append(static_cast<char>(value & 0xFF));
    return out;
}

QByteArray header(quint16 id, quint16 flags, quint16 qd, quint16 an, quint16 ns, quint16 ar)
{
    return u16(id) + u16(flags) + u16(qd) + u16(an) + u16(ns) + u16(ar);
}

QByteArray record(const QByteArray &owner, quint16 type, quint16 klass, quint32 ttl,
                  const QByteArray &rdata)
{
    return owner + u16(type) + u16(klass) + u32(ttl) + u16(static_cast<quint16>(rdata.size()))
            + rdata;
}

QByteArray ipv4(quint8 a, quint8 b, quint8 c, quint8 d)
{
    QByteArray out;
    out.append(static_cast<char>(a));
    out.append(static_cast<char>(b));
    out.append(static_cast<char>(c));
    out.append(static_cast<char>(d));
    return out;
}

constexpr quint16 FLAGS_RESPONSE = 0x8180;
constexpr quint16 FLAGS_QUERY = 0x0100;
constexpr quint16 TYPE_A = 1;
constexpr quint16 TYPE_CNAME = 5;
constexpr quint16 TYPE_OPT = 41;
constexpr quint16 CLASS_IN = 1;

// A NOERROR answer to `query` with one A record for the queried name.
QByteArray answerFor(const QByteArray &query)
{
    DnsWildcardRouter::Envelope envelope;
    if (!DnsWildcardRouter::parseEnvelope(query, &envelope)) {
        return QByteArray();
    }
    QByteArray owner;
    for (const QByteArray &label : envelope.qname) {
        owner.append(static_cast<char>(label.size()));
        owner.append(label);
    }
    owner.append('\0');

    QByteArray reply = query.left(envelope.questionEnd);
    reply[2] = static_cast<char>(0x81);
    reply[3] = static_cast<char>(0x80);
    reply[6] = 0;
    reply[7] = 1;
    reply[10] = 0;
    reply[11] = 0;
    reply.append(record(owner, TYPE_A, CLASS_IN, 300, ipv4(203, 0, 113, 7)));
    return reply;
}

QByteArray queryFor(quint16 id, const QByteArray &first)
{
    return header(id, FLAGS_QUERY, 1, 0, 0, 0) + encodeName({ first, "watched", "example" })
            + u16(TYPE_A) + u16(CLASS_IN);
}

// Starts `router` on a free loopback port that forwards to `upstreamPort`.
quint16 startOnLoopback(DnsWildcardRouter &router, quint16 upstreamPort)
{
    for (quint16 candidate = 15453; candidate < 15553; ++candidate) {
        if (router.start({ "*.watched.example" }, { QHostAddress::LocalHost },
                         QHostAddress::LocalHost, candidate, upstreamPort)) {
            return candidate;
        }
    }
    return 0;
}

void waitFor(const std::function<bool()> &done, int msec)
{
    QElapsedTimer elapsed;
    elapsed.start();
    while (!done() && elapsed.elapsed() < msec) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
}

}

class TestDnsWildcardRouter : public QObject
{
    Q_OBJECT

private slots:
    void testCanonicalize_data();
    void testCanonicalize();
    void testMatchesLabels_data();
    void testMatchesLabels();
    void testLiteralDotLabelCannotStraddleABoundary();

    void testParseEnvelope();
    void testParseEnvelopeRejectsTwoQuestions();
    void testAnswersRejectsWrongIdAndQuestion();
    void testMayLearnFromRejectsTruncatedAndFailed();

    void testCnameChainCarriesTheShortestTtl();
    void testUnrelatedRecordIsNotCollected();
    void testEmptyCnameRdataIsRejected();
    void testLyingSectionCountIsRejected();
    void testExtendedRcodeBlocksLearning();
    void testPointerLoopIsRejected();
    void testOutputUntouchedWhenMalformed();

    void testFramingAcrossReadsAndCoalesced();
    void testBuildServFail();

    void testRouteTableInstallRenewAndExpiry();
    void testRouteTableRefusesReservedAndCaps();
    void testRouteTableBudgetsArePerPattern();

    void testLiveUdpPathLearnsAndDelivers();
    void testLiveUdpPathRefusesExtendedRcode();
    void testLiveUdpRetransmitsToASilentUpstream();
    void testLiveUdpTakesALateAnswerToAnEarlierAttempt();
    void testLiveTcpReconnectsWhenTheUpstreamCloses();

    void testLocalSourceAcceptsAttachedSubnetsOnly();

private:
    bool runLiveQuery(quint8 extendedRcode, int *learned, QByteArray *delivered);
};

void TestDnsWildcardRouter::testCanonicalize_data()
{
    QTest::addColumn<QString>("entry");
    QTest::addColumn<QString>("expected");

    QTest::newRow("plain") << "*.example.com" << "*.example.com";
    QTest::newRow("upper case") << "*.Example.COM" << "*.example.com";
    QTest::newRow("single label") << "*.ru" << "*.ru";
    QTest::newRow("trailing dot") << "*.example.com." << "*.example.com";
    QTest::newRow("path") << "*.discord.gg/channels" << QString();
    QTest::newRow("bare star") << "*." << QString();
    QTest::newRow("double star") << "*.*" << QString();
    QTest::newRow("empty label") << "*..com" << QString();
    QTest::newRow("not a pattern") << "example.com" << QString();
    QTest::newRow("cidr") << "10.0.0.0/8" << QString();
    QTest::newRow("idn") << QString::fromUtf8("*.пример.рф")
                         << "*.xn--e1afmkfd.xn--p1ai";
}

void TestDnsWildcardRouter::testCanonicalize()
{
    QFETCH(QString, entry);
    QFETCH(QString, expected);

    QCOMPARE(WildcardSite::canonicalize(entry), expected);
}

void TestDnsWildcardRouter::testMatchesLabels_data()
{
    QTest::addColumn<QByteArrayList>("labels");
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<bool>("expected");

    QTest::newRow("subdomain") << QByteArrayList { "gateway", "discord", "gg" }
                               << "*.discord.gg" << true;
    QTest::newRow("apex is covered") << QByteArrayList { "discord", "gg" }
                                     << "*.discord.gg" << true;
    QTest::newRow("deep") << QByteArrayList { "a", "b", "discord", "gg" }
                          << "*.discord.gg" << true;
    QTest::newRow("same letters") << QByteArrayList { "notdiscord", "gg" }
                                  << "*.discord.gg" << false;
    QTest::newRow("tld") << QByteArrayList { "a", "ya", "ru" } << "*.ru" << true;
    QTest::newRow("other tld") << QByteArrayList { "ya", "rutracker", "org" }
                               << "*.ru" << false;
    QTest::newRow("case") << QByteArrayList { "YA", "RU" } << "*.ru" << true;
    QTest::newRow("shorter than base") << QByteArrayList { "gg" }
                                       << "*.discord.gg" << false;
    QTest::newRow("empty") << QByteArrayList {} << "*.ru" << false;
}

void TestDnsWildcardRouter::testMatchesLabels()
{
    QFETCH(QByteArrayList, labels);
    QFETCH(QString, pattern);
    QFETCH(bool, expected);

    QCOMPARE(WildcardSite::matchesLabels(labels, pattern), expected);
}

void TestDnsWildcardRouter::testLiteralDotLabelCannotStraddleABoundary()
{
    const QByteArrayList oneLabel { QByteArray("x.example.com") };
    QVERIFY(!WildcardSite::matchesLabels(oneLabel, "*.example.com"));

    const QByteArrayList threeLabels { "x", "example", "com" };
    QVERIFY(WildcardSite::matchesLabels(threeLabels, "*.example.com"));
}

void TestDnsWildcardRouter::testParseEnvelope()
{
    const QByteArray wire = header(0x1234, FLAGS_RESPONSE, 1, 0, 0, 0)
            + encodeName({ "gateway", "discord", "gg" }) + u16(TYPE_A) + u16(CLASS_IN);

    DnsWildcardRouter::Envelope envelope;
    QVERIFY(DnsWildcardRouter::parseEnvelope(wire, &envelope));
    QCOMPARE(envelope.id, quint16(0x1234));
    QVERIFY(envelope.isResponse);
    QCOMPARE(envelope.qtype, TYPE_A);
    QCOMPARE(envelope.qname.size(), 3);
    QCOMPARE(envelope.qname.at(0), QByteArray("gateway"));
    QCOMPARE(envelope.questionEnd, wire.size());
}

void TestDnsWildcardRouter::testParseEnvelopeRejectsTwoQuestions()
{
    const QByteArray wire = header(1, FLAGS_QUERY, 2, 0, 0, 0)
            + encodeName({ "a", "test" }) + u16(TYPE_A) + u16(CLASS_IN)
            + encodeName({ "b", "test" }) + u16(TYPE_A) + u16(CLASS_IN);

    DnsWildcardRouter::Envelope envelope;
    QVERIFY(!DnsWildcardRouter::parseEnvelope(wire, &envelope));
}

void TestDnsWildcardRouter::testAnswersRejectsWrongIdAndQuestion()
{
    DnsWildcardRouter::Envelope query;
    query.id = 0x1111;
    query.isResponse = false;
    query.qname = { "watched", "example" };
    query.qtype = TYPE_A;
    query.qclass = CLASS_IN;

    DnsWildcardRouter::Envelope good = query;
    good.isResponse = true;
    QVERIFY(DnsWildcardRouter::answers(good, query));

    DnsWildcardRouter::Envelope wrongId = good;
    wrongId.id = 0x9999;
    QVERIFY(!DnsWildcardRouter::answers(wrongId, query));

    DnsWildcardRouter::Envelope wrongName = good;
    wrongName.qname = { "other", "example" };
    QVERIFY(!DnsWildcardRouter::answers(wrongName, query));

    DnsWildcardRouter::Envelope wrongType = good;
    wrongType.qtype = 28;
    QVERIFY(!DnsWildcardRouter::answers(wrongType, query));

    DnsWildcardRouter::Envelope notAResponse = good;
    notAResponse.isResponse = false;
    QVERIFY(!DnsWildcardRouter::answers(notAResponse, query));
}

void TestDnsWildcardRouter::testMayLearnFromRejectsTruncatedAndFailed()
{
    DnsWildcardRouter::Envelope reply;
    reply.isResponse = true;
    reply.opcode = 0;
    reply.rcode = 0;
    reply.truncated = false;
    QVERIFY(DnsWildcardRouter::mayLearnFrom(reply));

    DnsWildcardRouter::Envelope truncated = reply;
    truncated.truncated = true;
    QVERIFY(!DnsWildcardRouter::mayLearnFrom(truncated));

    DnsWildcardRouter::Envelope failed = reply;
    failed.rcode = 2;
    QVERIFY(!DnsWildcardRouter::mayLearnFrom(failed));
}

void TestDnsWildcardRouter::testCnameChainCarriesTheShortestTtl()
{
    const QByteArray watched = encodeName({ "watched", "example" });
    const QByteArray target = encodeName({ "target", "example" });

    const QByteArray wire = header(7, FLAGS_RESPONSE, 1, 2, 0, 0) + watched
            + u16(TYPE_A) + u16(CLASS_IN)
            + record(watched, TYPE_CNAME, CLASS_IN, 30, target)
            + record(target, TYPE_A, CLASS_IN, 3600, ipv4(203, 0, 113, 7));

    DnsWildcardRouter::Message message;
    QVERIFY(DnsWildcardRouter::parseMessage(wire, &message));
    QCOMPARE(message.addresses.size(), 1);
    QCOMPARE(message.addresses.at(0).address, QHostAddress("203.0.113.7"));
    QCOMPARE(message.addresses.at(0).ttl, 30u);
}

void TestDnsWildcardRouter::testUnrelatedRecordIsNotCollected()
{
    const QByteArray watched = encodeName({ "watched", "example" });
    const QByteArray other = encodeName({ "other", "example" });

    const QByteArray wire = header(8, FLAGS_RESPONSE, 1, 2, 0, 0) + watched
            + u16(TYPE_A) + u16(CLASS_IN)
            + record(watched, TYPE_A, CLASS_IN, 300, ipv4(198, 51, 100, 1))
            + record(other, TYPE_A, CLASS_IN, 300, ipv4(203, 0, 113, 9));

    DnsWildcardRouter::Message message;
    QVERIFY(DnsWildcardRouter::parseMessage(wire, &message));
    QCOMPARE(message.addresses.size(), 1);
    QCOMPARE(message.addresses.at(0).address, QHostAddress("198.51.100.1"));
}

void TestDnsWildcardRouter::testEmptyCnameRdataIsRejected()
{
    const QByteArray watched = encodeName({ "watched", "example" });
    const QByteArray other = encodeName({ "other", "example" });

    const QByteArray wire = header(9, FLAGS_RESPONSE, 1, 2, 0, 0) + watched
            + u16(TYPE_A) + u16(CLASS_IN)
            + record(watched, TYPE_CNAME, CLASS_IN, 300, QByteArray())
            + record(other, TYPE_A, CLASS_IN, 300, ipv4(203, 0, 113, 9));

    DnsWildcardRouter::Message message;
    QVERIFY(!DnsWildcardRouter::parseMessage(wire, &message));
}

void TestDnsWildcardRouter::testLyingSectionCountIsRejected()
{
    const QByteArray watched = encodeName({ "watched", "example" });

    const QByteArray wire = header(10, FLAGS_RESPONSE, 1, 1, 0, 1) + watched
            + u16(TYPE_A) + u16(CLASS_IN)
            + record(watched, TYPE_A, CLASS_IN, 300, ipv4(198, 51, 100, 2));

    DnsWildcardRouter::Message message;
    QVERIFY(!DnsWildcardRouter::parseMessage(wire, &message));
}

void TestDnsWildcardRouter::testExtendedRcodeBlocksLearning()
{
    const QByteArray watched = encodeName({ "watched", "example" });
    const QByteArray opt = record(QByteArray(1, '\0'), TYPE_OPT, 4096,
                                  0x01000000, QByteArray());

    const QByteArray wire = header(11, FLAGS_RESPONSE, 1, 1, 0, 1) + watched
            + u16(TYPE_A) + u16(CLASS_IN)
            + record(watched, TYPE_A, CLASS_IN, 300, ipv4(198, 51, 100, 3))
            + opt;

    DnsWildcardRouter::Message message;
    QVERIFY(DnsWildcardRouter::parseMessage(wire, &message));
    QVERIFY(!DnsWildcardRouter::mayLearnFrom(message.envelope));
}

void TestDnsWildcardRouter::testPointerLoopIsRejected()
{
    QByteArray wire = header(12, FLAGS_RESPONSE, 1, 0, 0, 0);
    wire.append(static_cast<char>(0xC0));
    wire.append(static_cast<char>(0x0C));
    wire.append(u16(TYPE_A));
    wire.append(u16(CLASS_IN));

    DnsWildcardRouter::Envelope envelope;
    QVERIFY(!DnsWildcardRouter::parseEnvelope(wire, &envelope));
}

void TestDnsWildcardRouter::testOutputUntouchedWhenMalformed()
{
    DnsWildcardRouter::Message message;
    message.envelope.id = 0xBEEF;
    message.addresses.append(DnsWildcardRouter::Record { QHostAddress("10.0.0.1"), 5 });

    QVERIFY(!DnsWildcardRouter::parseMessage(QByteArray::fromHex("1234"), &message));
    QCOMPARE(message.envelope.id, quint16(0xBEEF));
    QCOMPARE(message.addresses.size(), 1);
}

void TestDnsWildcardRouter::testFramingAcrossReadsAndCoalesced()
{
    const QByteArray first = header(1, FLAGS_QUERY, 1, 0, 0, 0)
            + encodeName({ "a", "test" }) + u16(TYPE_A) + u16(CLASS_IN);
    const QByteArray second = header(2, FLAGS_QUERY, 1, 0, 0, 0)
            + encodeName({ "b", "test" }) + u16(TYPE_A) + u16(CLASS_IN);

    QByteArray stream = u16(static_cast<quint16>(first.size())) + first
            + u16(static_cast<quint16>(second.size())) + second;

    using FrameResult = DnsWildcardRouter::FrameResult;

    QByteArray taken;
    QCOMPARE(DnsWildcardRouter::takeFrame(stream, &taken), FrameResult::Taken);
    QCOMPARE(taken, first);
    QCOMPARE(DnsWildcardRouter::takeFrame(stream, &taken), FrameResult::Taken);
    QCOMPARE(taken, second);
    QCOMPARE(DnsWildcardRouter::takeFrame(stream, &taken), FrameResult::NeedMore);
    QVERIFY(stream.isEmpty());

    QByteArray split = u16(static_cast<quint16>(first.size()));
    QCOMPARE(DnsWildcardRouter::takeFrame(split, &taken), FrameResult::NeedMore);
    split.append(first.left(3));
    QCOMPARE(DnsWildcardRouter::takeFrame(split, &taken), FrameResult::NeedMore);
    split.append(first.mid(3));
    QCOMPARE(DnsWildcardRouter::takeFrame(split, &taken), FrameResult::Taken);
    QCOMPARE(taken, first);

    QByteArray zero = u16(0) + first;
    QCOMPARE(DnsWildcardRouter::takeFrame(zero, &taken), FrameResult::Broken);
}

void TestDnsWildcardRouter::testBuildServFail()
{
    const QByteArray query = header(0x4321, FLAGS_QUERY, 1, 0, 0, 0)
            + encodeName({ "watched", "example" }) + u16(TYPE_A) + u16(CLASS_IN);

    const QByteArray reply = DnsWildcardRouter::buildServFail(query);
    QVERIFY(!reply.isEmpty());

    DnsWildcardRouter::Envelope envelope;
    QVERIFY(DnsWildcardRouter::parseEnvelope(reply, &envelope));
    QCOMPARE(envelope.id, quint16(0x4321));
    QVERIFY(envelope.isResponse);
    QCOMPARE(envelope.rcode, quint8(2));
    QCOMPARE(envelope.qname.size(), 2);

    const QByteArray truncatedQuery = header(5, 0x0300, 1, 0, 0, 0)
            + encodeName({ "watched", "example" }) + u16(TYPE_A) + u16(CLASS_IN);
    DnsWildcardRouter::Envelope fromTruncated;
    QVERIFY(DnsWildcardRouter::parseEnvelope(
            DnsWildcardRouter::buildServFail(truncatedQuery), &fromTruncated));
    QVERIFY(!fromTruncated.truncated);
}

void TestDnsWildcardRouter::testRouteTableInstallRenewAndExpiry()
{
    // A learned route outlives the TTL: past it the entry is only stale, a
    // candidate for eviction, so an open connection is never cut by a timer.
    WildcardRouteTable table;
    const QString pattern = QStringLiteral("example.com");
    const QHostAddress address("203.0.113.7");
    const qint64 start = 1000;

    QCOMPARE(table.add(address, pattern, 30, start), WildcardRouteTable::AddResult::Install);
    QCOMPARE(table.add(address, pattern, 30, start), WildcardRouteTable::AddResult::Renew);
    QVERIFY(table.contains(address));
    // Stale only after the TTL plus the grace period.
    const qint64 staleAt = start + 30000 + WildcardRouteTable::StaleGraceMsec;
    QVERIFY(!table.isStale(address, staleAt - 1));
    QVERIFY(table.isStale(address, staleAt));
    QVERIFY(table.contains(address));

    // A renewal makes it fresh again; a TTL of 0 is clamped up to the minimum.
    QCOMPARE(table.add(address, pattern, 0, staleAt), WildcardRouteTable::AddResult::Renew);
    QVERIFY(!table.isStale(address, staleAt + 1000));

    // A long TTL is not cut down: a day-long record stays fresh for a day.
    const QHostAddress longLived("203.0.113.8");
    QCOMPARE(table.add(longLived, pattern, 86400, start), WildcardRouteTable::AddResult::Install);
    QVERIFY(!table.isStale(longLived, start + 86400LL * 1000));
}

void TestDnsWildcardRouter::testRouteTableRefusesReservedAndCaps()
{
    const QString pattern = QStringLiteral("example.com");
    WildcardRouteTable table(2);
    table.setReserved({ QHostAddress("1.1.1.1") },
                      { qMakePair(QHostAddress("10.8.1.2"), 24) });

    // The resolver and anything inside the tunnel's own network are refused.
    QCOMPARE(table.add(QHostAddress("1.1.1.1"), pattern, 60, 0),
             WildcardRouteTable::AddResult::Rejected);
    QCOMPARE(table.add(QHostAddress("10.8.1.1"), pattern, 60, 0),
             WildcardRouteTable::AddResult::Rejected);
    QCOMPARE(table.add(QHostAddress("0.0.0.0"), pattern, 60, 0),
             WildcardRouteTable::AddResult::Rejected);
    QCOMPARE(table.add(QHostAddress("127.0.0.1"), pattern, 60, 0),
             WildcardRouteTable::AddResult::Rejected);

    QCOMPARE(table.add(QHostAddress("198.51.100.1"), pattern, 60, 0),
             WildcardRouteTable::AddResult::Install);
    QCOMPARE(table.add(QHostAddress("198.51.100.2"), pattern, 60, 0),
             WildcardRouteTable::AddResult::Install);
    // Full and nothing stale yet: refused.
    QCOMPARE(table.add(QHostAddress("198.51.100.3"), pattern, 60, 0),
             WildcardRouteTable::AddResult::AtCapacity);

    // Once stale, the least recently seen entry makes room.
    const qint64 later = 60000 + WildcardRouteTable::StaleGraceMsec;
    QCOMPARE(table.add(QHostAddress("198.51.100.2"), pattern, 60, later - 1000),
             WildcardRouteTable::AddResult::Renew);
    QHostAddress evicted;
    QCOMPARE(table.add(QHostAddress("198.51.100.3"), pattern, 60, later, &evicted),
             WildcardRouteTable::AddResult::Install);
    QCOMPARE(evicted, QHostAddress("198.51.100.1"));
    QVERIFY(!table.contains(QHostAddress("198.51.100.1")));
    QCOMPARE(table.takeAll().size(), 2);
}

void TestDnsWildcardRouter::testRouteTableBudgetsArePerPattern()
{
    // One busy pattern cannot use up the room of another.
    WildcardRouteTable table(10, 2);
    const QString broad = QStringLiteral("ru");
    const QString bank = QStringLiteral("mybank.example");

    QCOMPARE(table.add(QHostAddress("198.51.100.1"), broad, 3600, 0),
             WildcardRouteTable::AddResult::Install);
    QCOMPARE(table.add(QHostAddress("198.51.100.2"), broad, 3600, 0),
             WildcardRouteTable::AddResult::Install);
    QCOMPARE(table.add(QHostAddress("198.51.100.3"), broad, 3600, 0),
             WildcardRouteTable::AddResult::AtCapacity);
    QCOMPARE(table.add(QHostAddress("203.0.113.1"), bank, 3600, 0),
             WildcardRouteTable::AddResult::Install);
    QCOMPARE(table.countForPattern(broad), 2);
    QCOMPARE(table.countForPattern(bank), 1);

    // Evicting for one pattern never takes another pattern's entry.
    QHostAddress evicted;
    QCOMPARE(table.add(QHostAddress("198.51.100.3"), broad, 60, 8000000, &evicted),
             WildcardRouteTable::AddResult::Install);
    QVERIFY(evicted == QHostAddress("198.51.100.1") || evicted == QHostAddress("198.51.100.2"));
    QVERIFY(table.contains(QHostAddress("203.0.113.1")));
}

bool TestDnsWildcardRouter::runLiveQuery(quint8 extendedRcode, int *learned,
                                         QByteArray *delivered)
{
    QUdpSocket resolver;
    if (!resolver.bind(QHostAddress::LocalHost, 0)) {
        return false;
    }
    const quint16 resolverPort = resolver.localPort();

    DnsWildcardRouter router;
    quint16 listenPort = 0;
    for (quint16 candidate = 15353; candidate < 15453; ++candidate) {
        if (router.start({ "*.watched.example" }, { QHostAddress::LocalHost },
                         QHostAddress::LocalHost, candidate, resolverPort)) {
            listenPort = candidate;
            break;
        }
    }
    if (listenPort == 0) {
        return false;
    }

    QSignalSpy spy(&router, &DnsWildcardRouter::addressesResolved);

    QObject::connect(&resolver, &QUdpSocket::readyRead, &resolver, [&]() {
        while (resolver.hasPendingDatagrams()) {
            QByteArray query;
            query.resize(static_cast<int>(resolver.pendingDatagramSize()));
            QHostAddress from;
            quint16 fromPort = 0;
            resolver.readDatagram(query.data(), query.size(), &from, &fromPort);

            DnsWildcardRouter::Envelope envelope;
            if (!DnsWildcardRouter::parseEnvelope(query, &envelope)) {
                continue;
            }

            QByteArray owner;
            for (const QByteArray &label : envelope.qname) {
                owner.append(static_cast<char>(label.size()));
                owner.append(label);
            }
            owner.append('\0');

            QByteArray reply = query.left(envelope.questionEnd);
            reply[2] = static_cast<char>(0x81);
            reply[3] = static_cast<char>(0x80);
            reply[6] = 0;
            reply[7] = 1;
            reply[10] = 0;
            reply[11] = 1;
            reply.append(record(owner, TYPE_A, CLASS_IN, 300, ipv4(203, 0, 113, 7)));
            reply.append(record(QByteArray(1, '\0'), TYPE_OPT, 4096,
                                static_cast<quint32>(extendedRcode) << 24,
                                QByteArray()));
            resolver.writeDatagram(reply, from, fromPort);
        }
    });

    QUdpSocket client;
    if (!client.bind(QHostAddress::LocalHost, 0)) {
        return false;
    }

    QByteArray response;
    QObject::connect(&client, &QUdpSocket::readyRead, &client, [&]() {
        while (client.hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(static_cast<int>(client.pendingDatagramSize()));
            client.readDatagram(datagram.data(), datagram.size());
            response = datagram;
        }
    });

    const QByteArray query = header(0x2468, FLAGS_QUERY, 1, 0, 0, 0)
            + encodeName({ "a", "watched", "example" }) + u16(TYPE_A) + u16(CLASS_IN);
    client.writeDatagram(query, QHostAddress::LocalHost, listenPort);

    QElapsedTimer elapsed;
    elapsed.start();
    while (response.isEmpty() && elapsed.elapsed() < 5000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    if (response.isEmpty()) {
        return false;
    }

    QCoreApplication::processEvents();

    *learned = spy.count();
    *delivered = response;
    return true;
}

void TestDnsWildcardRouter::testLiveUdpPathLearnsAndDelivers()
{
    int learned = -1;
    QByteArray delivered;
    QVERIFY(runLiveQuery(0, &learned, &delivered));

    QCOMPARE(learned, 1);

    DnsWildcardRouter::Envelope envelope;
    QVERIFY(DnsWildcardRouter::parseEnvelope(delivered, &envelope));
    QCOMPARE(envelope.id, quint16(0x2468));
    QVERIFY(envelope.isResponse);
}

void TestDnsWildcardRouter::testLiveUdpPathRefusesExtendedRcode()
{
    int learned = -1;
    QByteArray delivered;
    QVERIFY(runLiveQuery(1, &learned, &delivered));

    QCOMPARE(learned, 0);

    DnsWildcardRouter::Envelope envelope;
    QVERIFY(DnsWildcardRouter::parseEnvelope(delivered, &envelope));
    QCOMPARE(envelope.id, quint16(0x2468));
}

// One upstream that drops the first datagram: the query is retransmitted to
// it after the attempt interval and answered, instead of failing outright.
void TestDnsWildcardRouter::testLiveUdpRetransmitsToASilentUpstream()
{
    QUdpSocket resolver;
    QVERIFY(resolver.bind(QHostAddress::LocalHost, 0));

    DnsWildcardRouter router;
    const quint16 listenPort = startOnLoopback(router, resolver.localPort());
    QVERIFY(listenPort != 0);

    int seen = 0;
    QObject::connect(&resolver, &QUdpSocket::readyRead, &resolver, [&]() {
        while (resolver.hasPendingDatagrams()) {
            QByteArray query;
            query.resize(static_cast<int>(resolver.pendingDatagramSize()));
            QHostAddress from;
            quint16 fromPort = 0;
            resolver.readDatagram(query.data(), query.size(), &from, &fromPort);
            if (++seen == 1) {
                continue;
            }
            resolver.writeDatagram(answerFor(query), from, fromPort);
        }
    });

    QUdpSocket client;
    QVERIFY(client.bind(QHostAddress::LocalHost, 0));
    QByteArray response;
    QObject::connect(&client, &QUdpSocket::readyRead, &client, [&]() {
        while (client.hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(static_cast<int>(client.pendingDatagramSize()));
            client.readDatagram(datagram.data(), datagram.size());
            response = datagram;
        }
    });

    client.writeDatagram(queryFor(0x1357, "a"), QHostAddress::LocalHost, listenPort);
    waitFor([&]() { return !response.isEmpty(); }, 7000);

    QVERIFY(!response.isEmpty());
    QVERIFY(seen >= 2);
    DnsWildcardRouter::Envelope envelope;
    QVERIFY(DnsWildcardRouter::parseEnvelope(response, &envelope));
    QCOMPARE(envelope.id, quint16(0x1357));
    QCOMPARE(envelope.rcode, quint8(0));
}

// The first attempt is answered only after the retransmit went out; that
// answer still reaches the client, because earlier attempts stay open.
void TestDnsWildcardRouter::testLiveUdpTakesALateAnswerToAnEarlierAttempt()
{
    QUdpSocket resolver;
    QVERIFY(resolver.bind(QHostAddress::LocalHost, 0));

    DnsWildcardRouter router;
    const quint16 listenPort = startOnLoopback(router, resolver.localPort());
    QVERIFY(listenPort != 0);
    QSignalSpy spy(&router, &DnsWildcardRouter::addressesResolved);

    int seen = 0;
    QHostAddress firstFrom;
    quint16 firstPort = 0;
    QByteArray firstQuery;
    QObject::connect(&resolver, &QUdpSocket::readyRead, &resolver, [&]() {
        while (resolver.hasPendingDatagrams()) {
            QByteArray query;
            query.resize(static_cast<int>(resolver.pendingDatagramSize()));
            QHostAddress from;
            quint16 fromPort = 0;
            resolver.readDatagram(query.data(), query.size(), &from, &fromPort);
            if (++seen == 1) {
                firstQuery = query;
                firstFrom = from;
                firstPort = fromPort;
                QTimer::singleShot(1500, &resolver, [&]() {
                    resolver.writeDatagram(answerFor(firstQuery), firstFrom, firstPort);
                });
            }
        }
    });

    QUdpSocket client;
    QVERIFY(client.bind(QHostAddress::LocalHost, 0));
    QByteArray response;
    QObject::connect(&client, &QUdpSocket::readyRead, &client, [&]() {
        while (client.hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(static_cast<int>(client.pendingDatagramSize()));
            client.readDatagram(datagram.data(), datagram.size());
            response = datagram;
        }
    });

    client.writeDatagram(queryFor(0x2469, "b"), QHostAddress::LocalHost, listenPort);
    waitFor([&]() { return !response.isEmpty(); }, 7000);
    QCoreApplication::processEvents();

    QVERIFY(!response.isEmpty());
    QVERIFY(seen >= 2);
    DnsWildcardRouter::Envelope envelope;
    QVERIFY(DnsWildcardRouter::parseEnvelope(response, &envelope));
    QCOMPARE(envelope.id, quint16(0x2469));
    QCOMPARE(envelope.rcode, quint8(0));
    QCOMPARE(spy.count(), 1);
}

// The upstream answers the first of two pipelined queries and closes. The
// forwarder reconnects to the same resolver and the second one is answered,
// rather than failed as if the resolver were dead.
void TestDnsWildcardRouter::testLiveTcpReconnectsWhenTheUpstreamCloses()
{
    QTcpServer resolver;
    QVERIFY(resolver.listen(QHostAddress::LocalHost, 0));

    DnsWildcardRouter router;
    const quint16 listenPort = startOnLoopback(router, resolver.serverPort());
    QVERIFY(listenPort != 0);

    int connections = 0;
    QObject::connect(&resolver, &QTcpServer::newConnection, &resolver, [&]() {
        while (resolver.hasPendingConnections()) {
            QTcpSocket *upstream = resolver.nextPendingConnection();
            const int number = ++connections;
            auto buffer = std::make_shared<QByteArray>();
            QObject::connect(upstream, &QTcpSocket::readyRead, upstream,
                             [upstream, buffer, number]() {
                buffer->append(upstream->readAll());
                QByteArray query;
                while (DnsWildcardRouter::takeFrame(*buffer, &query)
                       == DnsWildcardRouter::FrameResult::Taken) {
                    upstream->write(DnsWildcardRouter::frame(answerFor(query)));
                    if (number == 1) {
                        // Short of buffers: one answer, then the door.
                        upstream->disconnectFromHost();
                        return;
                    }
                }
            });
            QObject::connect(upstream, &QTcpSocket::disconnected, upstream,
                             &QObject::deleteLater);
        }
    });

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, listenPort);
    QVERIFY(client.waitForConnected(3000));
    client.write(DnsWildcardRouter::frame(queryFor(0x0101, "c"))
                 + DnsWildcardRouter::frame(queryFor(0x0202, "d")));

    QByteArray stream;
    QList<QByteArray> answers;
    QObject::connect(&client, &QTcpSocket::readyRead, &client, [&]() {
        stream.append(client.readAll());
        QByteArray message;
        while (DnsWildcardRouter::takeFrame(stream, &message)
               == DnsWildcardRouter::FrameResult::Taken) {
            answers.append(message);
        }
    });
    waitFor([&]() { return answers.size() >= 2; }, 7000);

    QCOMPARE(answers.size(), 2);
    QVERIFY(connections >= 2);
    QList<quint16> ids;
    for (const QByteArray &answer : answers) {
        DnsWildcardRouter::Envelope envelope;
        QVERIFY(DnsWildcardRouter::parseEnvelope(answer, &envelope));
        QCOMPARE(envelope.rcode, quint8(0));
        ids.append(envelope.id);
    }
    QVERIFY(ids.contains(quint16(0x0101)));
    QVERIFY(ids.contains(quint16(0x0202)));
}

void TestDnsWildcardRouter::testLocalSourceAcceptsAttachedSubnetsOnly()
{
    const QHostAddress listen("10.8.1.2");
    const QList<QHostAddress> local = { QHostAddress("192.168.1.20"),
                                        QHostAddress("172.17.0.1"), listen };
    // docker0 and the LAN are attached; the tunnel's own subnet is not listed,
    // as refreshLocalSnapshot() leaves it out.
    const QList<DnsWildcardRouter::Subnet> attached = {
        { QHostAddress("172.17.0.1"), 16 },
        { QHostAddress("192.168.1.20"), 24 },
    };

    auto accepts = [&](const char *source) {
        return DnsWildcardRouter::isLocalSource(QHostAddress(source), listen, local, attached);
    };

    QVERIFY(accepts("127.0.0.1"));
    QVERIFY(accepts("::1"));
    QVERIFY(accepts("10.8.1.2"));
    QVERIFY(accepts("192.168.1.20"));
    QVERIFY(accepts("172.17.0.5"));
    QVERIFY(accepts("172.17.255.254"));

    QVERIFY(!accepts("10.8.1.1"));
    QVERIFY(!accepts("10.8.1.7"));
    QVERIFY(!accepts("172.18.0.5"));
    QVERIFY(!accepts("203.0.113.9"));
    QVERIFY(!accepts("2001:db8::1"));
    QVERIFY(!DnsWildcardRouter::isLocalSource(QHostAddress(), listen, local, attached));
}

QTEST_MAIN(TestDnsWildcardRouter)
#include "testDnsWildcardRouter.moc"
