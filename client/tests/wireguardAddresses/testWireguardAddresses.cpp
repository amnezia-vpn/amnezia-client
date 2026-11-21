#include <QtTest>

#include "../../daemon/wireguardaddressutils.h"
#include "../../platforms/macos/daemon/macosfirewall.h"

class TestWireguardAddresses : public QObject
{
    Q_OBJECT

private slots:
    void ipv6FirewallRules()
    {
        const QString block = QStringLiteral("block return out inet6 flags any no state");
        QCOMPARE(MacOSFirewall::ipv6Rules("2001:db8::10", 51820),
                 (QStringList { block, "pass out inet6 proto udp to 2001:db8::10 port 51820 no state" }));
        for (const QString &address : { QString(), QStringLiteral("192.0.2.10"), QStringLiteral("invalid") }) {
            QCOMPARE(MacOSFirewall::ipv6Rules(address, 51820), QStringList { block });
        }
        QCOMPARE(MacOSFirewall::ipv6Rules("2001:db8::10", 0), QStringList { block });
        QCOMPARE(MacOSFirewall::ipv6Rules("2001:db8::10", 65536), QStringList { block });
        QCOMPARE(MacOSFirewall::ipv6Rules("fe80::1%en0", 51820),
                 (QStringList { block, "pass out inet6 proto udp to fe80::1 port 51820 no state" }));
    }

    void parseDeviceAddresses_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<bool>("valid");
        QTest::addColumn<QString>("ipv4");
        QTest::addColumn<QString>("ipv6");
        QTest::newRow("ipv4-only") << "10.99.0.2/32" << true << "10.99.0.2/32" << QString();
        QTest::newRow("dual-stack") << "10.99.0.2/32, fd99:99::2/128" << true << "10.99.0.2/32" << "fd99:99::2/128";
        QTest::newRow("reversed") << " fd99:99::2/128 , 10.99.0.2/32 " << true << "10.99.0.2/32" << "fd99:99::2/128";
        QTest::newRow("ipv6-only") << "fd99:99::2/128" << true << QString() << "fd99:99::2/128";
        QTest::newRow("empty") << "" << false << QString() << QString();
        QTest::newRow("invalid") << "10.99.0.2/32, invalid" << false << QString() << QString();
        QTest::newRow("invalid-prefix") << "10.99.0.2/99" << false << QString() << QString();
        QTest::newRow("duplicate-ipv4") << "10.99.0.2/32, 10.99.0.3/32" << false << QString() << QString();
        QTest::newRow("duplicate-ipv6") << "fd99:99::2/128, fd99:99::3/128" << false << QString() << QString();
        QTest::newRow("trailing-comma") << "10.99.0.2/32," << false << QString() << QString();
    }

    void parseDeviceAddresses()
    {
        QFETCH(QString, input);
        QFETCH(bool, valid);
        QFETCH(QString, ipv4);
        QFETCH(QString, ipv6);
        const auto actual = WireguardAddressUtils::parseDeviceAddresses(input);
        QCOMPARE(actual.has_value(), valid);
        if (actual) {
            QCOMPARE(actual->ipv4, ipv4);
            QCOMPARE(actual->ipv6, ipv6);
        }
    }

    void formatEndpoint_data()
    {
        QTest::addColumn<QString>("address");
        QTest::addColumn<QString>("endpoint");
        QTest::newRow("ipv4") << "192.0.2.10" << "192.0.2.10:34541";
        QTest::newRow("ipv6") << "2001:db8:1234::" << "[2001:db8:1234::]:34541";
        QTest::newRow("nat64") << "64:ff9b::c000:20a" << "[64:ff9b::c000:20a]:34541";
        QTest::newRow("scoped-ipv6") << "fe80::1%en0" << "[fe80::1%en0]:34541";
        QTest::newRow("hostname") << "vpn.example.net" << "vpn.example.net:34541";
    }

    void formatEndpoint()
    {
        QFETCH(QString, address);
        QFETCH(QString, endpoint);
        QCOMPARE(WireguardAddressUtils::formatEndpoint(address, 34541), endpoint);
    }
};

QTEST_GUILESS_MAIN(TestWireguardAddresses)
#include "testWireguardAddresses.moc"
