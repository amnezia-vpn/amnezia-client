#include <QTest>

#include "core/utils/networkUtilities.h"

class TestHostPortParsing : public QObject
{
    Q_OBJECT

private slots:
    void parsesValidInput_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("host");
        QTest::addColumn<int>("port");

        QTest::newRow("ipv4") << "203.0.113.10" << "203.0.113.10" << -1;
        QTest::newRow("ipv4 with port") << "203.0.113.10:51820" << "203.0.113.10" << 51820;
        QTest::newRow("ipv6") << "2001:db8::10" << "2001:db8::10" << -1;
        QTest::newRow("ipv6 full form") << "2001:0db8:0000:0000:0000:0000:0000:0010"
                                        << "2001:0db8:0000:0000:0000:0000:0000:0010" << -1;
        QTest::newRow("ipv6 loopback") << "::1" << "::1" << -1;
        QTest::newRow("unbracketed ipv6 is never split") << "2001:db8::10:5182" << "2001:db8::10:5182" << -1;
        QTest::newRow("ipv6 bracketed") << "[2001:db8::10]" << "2001:db8::10" << -1;
        QTest::newRow("ipv6 with port") << "[2001:db8::10]:51820" << "2001:db8::10" << 51820;
        QTest::newRow("ipv6 mapped ipv4 with port") << "[::ffff:203.0.113.10]:22" << "::ffff:203.0.113.10" << 22;
        QTest::newRow("ipv6 with scope") << "[fe80::1%en0]:22" << "fe80::1%en0" << 22;
        QTest::newRow("domain") << "vpn.example.com" << "vpn.example.com" << -1;
        QTest::newRow("domain with port") << "vpn.example.com:51820" << "vpn.example.com" << 51820;
        QTest::newRow("single label host") << "myserver:2222" << "myserver" << 2222;
        QTest::newRow("surrounding spaces") << "  [2001:db8::10]:443  " << "2001:db8::10" << 443;
        QTest::newRow("max port") << "203.0.113.10:65535" << "203.0.113.10" << 65535;
    }

    void parsesValidInput()
    {
        QFETCH(QString, input);
        QFETCH(QString, host);
        QFETCH(int, port);

        QString parsedHost;
        int parsedPort = 0;
        QVERIFY(NetworkUtilities::parseHostPort(input, parsedHost, parsedPort));
        QCOMPARE(parsedHost, host);
        QCOMPARE(parsedPort, port);
    }

    void rejectsInvalidInput_data()
    {
        QTest::addColumn<QString>("input");

        QTest::newRow("empty") << "";
        QTest::newRow("only port") << ":22";
        QTest::newRow("unbracketed ipv6 with port") << "2001:db8::10:51820";
        QTest::newRow("unclosed bracket") << "[2001:db8::10:51820";
        QTest::newRow("bracketed ipv4") << "[203.0.113.10]:22";
        QTest::newRow("bracketed domain") << "[vpn.example.com]:22";
        QTest::newRow("garbage after bracket") << "[2001:db8::10]x22";
        QTest::newRow("empty port") << "203.0.113.10:";
        QTest::newRow("empty port ipv6") << "[2001:db8::10]:";
        QTest::newRow("zero port") << "203.0.113.10:0";
        QTest::newRow("port too large") << "203.0.113.10:65536";
        QTest::newRow("negative port") << "203.0.113.10:-1";
        QTest::newRow("signed port") << "203.0.113.10:+22";
        QTest::newRow("non numeric port") << "vpn.example.com:ssh";
        QTest::newRow("invalid ipv6") << "2001:db8:::10";
        QTest::newRow("space in host") << "vpn example.com";
        QTest::newRow("slash in host") << "vpn.example.com/path";
        QTest::newRow("user in host") << "root@vpn.example.com";
        QTest::newRow("ipv4 octet out of range") << "999.1.1.1:22";
        QTest::newRow("truncated ipv4") << "203.0.113:22";
    }

    void rejectsInvalidInput()
    {
        QFETCH(QString, input);

        QString host = "unchanged";
        int port = 7;
        QVERIFY(!NetworkUtilities::parseHostPort(input, host, port));
        QCOMPARE(host, QString("unchanged"));
        QCOMPARE(port, 7);
    }

    void formatsEndpoint_data()
    {
        QTest::addColumn<QString>("host");
        QTest::addColumn<int>("port");
        QTest::addColumn<QString>("endpointHost");
        QTest::addColumn<QString>("endpoint");

        QTest::newRow("ipv4") << "203.0.113.10" << 51820 << "203.0.113.10" << "203.0.113.10:51820";
        QTest::newRow("ipv6") << "2001:db8::10" << 51820 << "[2001:db8::10]" << "[2001:db8::10]:51820";
        QTest::newRow("domain") << "vpn.example.com" << 443 << "vpn.example.com" << "vpn.example.com:443";
    }

    void formatsEndpoint()
    {
        QFETCH(QString, host);
        QFETCH(int, port);
        QFETCH(QString, endpointHost);
        QFETCH(QString, endpoint);

        QCOMPARE(NetworkUtilities::endpointHost(host), endpointHost);
        QCOMPARE(NetworkUtilities::endpoint(host, port), endpoint);
    }

    void endpointRoundTrips_data()
    {
        QTest::addColumn<QString>("host");
        QTest::addColumn<int>("port");

        QTest::newRow("ipv4") << "203.0.113.10" << 51820;
        QTest::newRow("ipv6") << "2001:db8::10" << 51820;
        QTest::newRow("domain") << "vpn.example.com" << 1;
    }

    void endpointRoundTrips()
    {
        QFETCH(QString, host);
        QFETCH(int, port);

        QString parsedHost;
        int parsedPort = -1;
        QVERIFY(NetworkUtilities::parseHostPort(NetworkUtilities::endpoint(host, port), parsedHost, parsedPort));
        QCOMPARE(parsedHost, host);
        QCOMPARE(parsedPort, port);
    }
};

QTEST_MAIN(TestHostPortParsing)
#include "testHostPortParsing.moc"
