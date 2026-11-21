#include <QJsonArray>
#include <QStandardPaths>
#include <QTest>
#include <QUuid>
#include <memory>

#include "core/controllers/selfhosted/importController.h"
#include "core/models/api/apiV2ServerConfig.h"
#include "core/models/selfhosted/nativeServerConfig.h"
#include "core/models/selfhosted/selfHostedAdminServerConfig.h"
#include "core/models/selfhosted/selfHostedUserServerConfig.h"
#include "core/utils/serverConfigUtils.h"

using namespace amnezia;

class TestWireGuardDnsImport : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<SecureQSettings> m_settings;
    std::unique_ptr<SecureServersRepository> m_servers;
    std::unique_ptr<SecureAppSettingsRepository> m_appSettings;
    std::unique_ptr<ImportController> m_import;

    static QString testKey(char value)
    {
        return QString::fromLatin1(QByteArray(32, value).toBase64());
    }

    static QString configuration(const QString &dnsLines, bool awg)
    {
        return QStringLiteral("[Interface]\nPrivateKey = %1\nAddress = 10.23.0.2/32\nMTU = 1380\n%2%3\n"
                              "[Peer]\nPublicKey = %4\nPresharedKey = %5\nAllowedIPs = 0.0.0.0/0\n"
                              "Endpoint = 192.0.2.1:51820\nPersistentKeepalive = 25\n")
                .arg(testKey('a'), awg ? QStringLiteral("Jc = 4\nJmin = 10\nJmax = 50\n") : QString(), dnsLines,
                     testKey('b'), testKey('c'));
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        m_settings = std::make_unique<SecureQSettings>(
                QStringLiteral("AmneziaDnsImportTest-%1").arg(QUuid::createUuid().toString()),
                QStringLiteral("amnezia-client"), nullptr, false);
        m_servers = std::make_unique<SecureServersRepository>(m_settings.get());
        m_appSettings = std::make_unique<SecureAppSettingsRepository>(m_settings.get());
        m_import = std::make_unique<ImportController>(m_servers.get(), m_appSettings.get());
    }

    void init()
    {
        m_settings->clearSettings();
        m_servers->invalidateCache();
        m_appSettings->setPrimaryDns(QStringLiteral("1.1.1.1"));
        m_appSettings->setSecondaryDns(QStringLiteral("1.0.0.1"));
    }

    void cleanupTestCase()
    {
        m_settings->clearSettings();
    }

    void importDns_data()
    {
        QTest::addColumn<bool>("awg");
        QTest::addColumn<QString>("dnsLines");
        QTest::addColumn<QString>("dns1");
        QTest::addColumn<QString>("dns2");

        for (bool awg : { false, true }) {
            const QByteArray prefix = awg ? "awg-" : "wg-";
            const auto row = [&](const char *name, const QString &lines, const QString &first = { },
                                 const QString &second = { }) {
                QTest::newRow((prefix + name).constData()) << awg << lines << first << second;
            };
            row("single", "DNS = 10.23.0.53", "10.23.0.53");
            row("pair", "DNS = 10.23.0.53, 10.23.0.54", "10.23.0.53", "10.23.0.54");
            row("no-spaces", "DNS=10.23.0.53", "10.23.0.53");
            row("whitespace", "\t DNS \t=\t 10.23.0.53 \t,\t10.23.0.54 \t", "10.23.0.53", "10.23.0.54");
            row("order", "DNS=10.23.0.54,10.23.0.53", "10.23.0.54", "10.23.0.53");
            row("absent", "");
            row("empty", "DNS = \t");
            row("invalid", "DNS = 999.23.0.53, 10.23.0.999");
            row("invalid-first", "DNS = invalid, 10.23.0.53", "10.23.0.53");
            row("invalid-second", "DNS = 10.23.0.53, invalid", "10.23.0.53");
            row("comment", "# DNS = 10.23.0.53, 10.23.0.54\n# note: DNS = 10.23.0.55, 10.23.0.56");
            row("other-key", "NotDNS = 10.23.0.53, 10.23.0.54");
            row("inline-comment", "DNS = 10.23.0.53 # DNS = 10.23.0.54, 10.23.0.55", "10.23.0.53");
            row("comment-only", "DNS = # 10.23.0.53, 10.23.0.54");
            row("extra-resolver", "DNS = 10.23.0.53, 10.23.0.54, 10.23.0.55", "10.23.0.53", "10.23.0.54");
            row("ipv6-unsupported", "DNS = 2001:db8::53");
        }
    }

    void importDns()
    {
        QFETCH(bool, awg);
        QFETCH(QString, dnsLines);
        QFETCH(QString, dns1);
        QFETCH(QString, dns2);

        const QString data = configuration(dnsLines, awg);
        const auto result = m_import->extractConfigFromData(data, QStringLiteral("synthetic.conf"));
        QCOMPARE(result.errorCode, ErrorCode::NoError);
        QVERIFY(!result.config.isEmpty());
        QVERIFY(result.configType == (awg ? ConfigTypes::Awg : ConfigTypes::WireGuard));
        QCOMPARE(result.isNativeWireGuardConfig, !awg);
        QCOMPARE(result.config.value(configKey::dns1).toString(), dns1);
        QCOMPARE(result.config.value(configKey::dns2).toString(), dns2);
        QCOMPARE(result.config.contains(configKey::dns1), !dns1.isEmpty());
        QCOMPARE(result.config.contains(configKey::dns2), !dns2.isEmpty());
        QCOMPARE(serverConfigUtils::configTypeFromJson(result.config), serverConfigUtils::ConfigType::Native);

        const auto native = NativeServerConfig::fromJson(result.config);
        QCOMPARE(native.defaultContainer, awg ? DockerContainer::Awg : DockerContainer::WireGuard);
        QCOMPARE(native.dns1, dns1);
        QCOMPARE(native.dns2, dns2);
        QCOMPARE(native.hostName, QStringLiteral("192.0.2.1"));

        const auto protocol = native.containerConfig(native.defaultContainer).protocolConfig;
        const auto client = protocol.getClientConfigJson();
        QCOMPARE(protocol.nativeConfig(), data);
        QCOMPARE(client.value(configKey::clientPrivKey).toString(), testKey('a'));
        QCOMPARE(client.value(configKey::serverPubKey).toString(), testKey('b'));
        QCOMPARE(client.value(configKey::pskKey).toString(), testKey('c'));
        QCOMPARE(client.value(configKey::clientIp).toString(), QStringLiteral("10.23.0.2/32"));
        QCOMPARE(client.value(configKey::mtu).toString(), QStringLiteral("1380"));
        QCOMPARE(client.value(configKey::port).toInt(), 51820);
        QCOMPARE(client.value(configKey::allowedIps).toArray(), QJsonArray { QStringLiteral("0.0.0.0/0") });
        QCOMPARE(client.value(configKey::persistentKeepAlive).toString(), QStringLiteral("25"));
        if (awg) {
            QCOMPARE(client.value(configKey::junkPacketCount).toString(), QStringLiteral("4"));
        }

        const QPair<QString, QString> expected { dns1.isEmpty() ? m_appSettings->primaryDns() : dns1,
                                                 dns1.isEmpty() ? m_appSettings->secondaryDns() : dns2 };
        QCOMPARE(native.getDnsPair(m_appSettings->primaryDns(), m_appSettings->secondaryDns()), expected);
        const auto restored = NativeServerConfig::fromJson(native.toJson());
        QCOMPARE(restored.getDnsPair(m_appSettings->primaryDns(), m_appSettings->secondaryDns()), expected);
    }

    void nativeDnsFallback_data()
    {
        QTest::addColumn<int>("containerValue");
        QTest::addColumn<QString>("dns1");
        QTest::addColumn<QString>("dns2");
        QTest::addColumn<QString>("expected1");
        QTest::addColumn<QString>("expected2");

        for (DockerContainer container : { DockerContainer::WireGuard, DockerContainer::Awg, DockerContainer::Awg2 }) {
            const QByteArray prefix = ContainerUtils::containerToString(container).toLatin1() + '-';
            const auto row = [&](const char *name, const QString &first, const QString &second,
                                 const QString &expected1, const QString &expected2) {
                QTest::newRow((prefix + name).constData())
                        << static_cast<int>(container) << first << second << expected1 << expected2;
            };
            row("single", "10.23.0.53", "", "10.23.0.53", "");
            row("pair", "10.23.0.53", "10.23.0.54", "10.23.0.53", "10.23.0.54");
            row("absent", "", "", "1.1.1.1", "1.0.0.1");
            row("invalid-primary", "invalid", "", "1.1.1.1", "1.0.0.1");
            row("invalid-secondary", "10.23.0.53", "invalid", "10.23.0.53", "1.0.0.1");
            row("secondary-only", "", "10.23.0.54", "1.1.1.1", "10.23.0.54");
        }
        QTest::newRow("openvpn-single") << static_cast<int>(DockerContainer::OpenVpn) << QStringLiteral("10.23.0.53")
                                        << QString() << QStringLiteral("10.23.0.53") << QStringLiteral("1.0.0.1");
        QTest::newRow("xray-single") << static_cast<int>(DockerContainer::Xray) << QStringLiteral("10.23.0.53")
                                     << QString() << QStringLiteral("10.23.0.53") << QStringLiteral("1.0.0.1");
    }

    void nativeDnsFallback()
    {
        QFETCH(int, containerValue);
        QFETCH(QString, dns1);
        QFETCH(QString, dns2);
        QFETCH(QString, expected1);
        QFETCH(QString, expected2);

        NativeServerConfig config;
        config.defaultContainer = static_cast<DockerContainer>(containerValue);
        config.dns1 = dns1;
        config.dns2 = dns2;
        const auto actual = config.getDnsPair(m_appSettings->primaryDns(), m_appSettings->secondaryDns());
        QCOMPARE(actual.first, expected1);
        QCOMPARE(actual.second, expected2);
    }

    void nonNativeFallbackUnchanged()
    {
        const auto verify = [&](const auto &config) {
            const auto dns = config.getDnsPair(m_appSettings->primaryDns(), m_appSettings->secondaryDns());
            QCOMPARE(dns.first, QStringLiteral("10.23.0.53"));
            QCOMPARE(dns.second, QStringLiteral("1.0.0.1"));
        };
        SelfHostedUserServerConfig user;
        user.dns1 = QStringLiteral("10.23.0.53");
        verify(user);
        ApiV2ServerConfig api;
        api.dns1 = QStringLiteral("10.23.0.53");
        verify(api);
        SelfHostedAdminServerConfig admin;
        admin.dns1 = QStringLiteral("10.23.0.53");
        const auto dns = admin.getDnsPair(false, m_appSettings->primaryDns(), m_appSettings->secondaryDns());
        QCOMPARE(dns.first, QStringLiteral("10.23.0.53"));
        QCOMPARE(dns.second, QStringLiteral("1.0.0.1"));
    }
};

QTEST_GUILESS_MAIN(TestWireGuardDnsImport)
#include "testWireGuardDnsImport.moc"
