#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <QUuid>

#include "core/utils/constants/configKeys.h"
#include "secureQSettings.h"
#include "utils/testCoreController.h"
#include "vpnConnection.h"

using namespace amnezia;

namespace
{
    QString wireGuardConfig(const QString &address, const QString &allowedIps, const QString &endpoint)
    {
        return QStringLiteral("[Interface]\n"
                              "PrivateKey = AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=\n"
                              "Address = %1\n"
                              "DNS = 1.1.1.1, 1.0.0.1\n"
                              "\n"
                              "[Peer]\n"
                              "PublicKey = BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB=\n"
                              "AllowedIPs = %2\n"
                              "Endpoint = %3\n")
                .arg(address, allowedIps, endpoint);
    }
}

class TestWireGuardEndpointImport : public QObject
{
    Q_OBJECT

private:
    TestCoreController *m_coreController = nullptr;
    SecureQSettings *m_settings = nullptr;

private slots:
    void initTestCase()
    {
        m_settings = new SecureQSettings("AmneziaVPN-Test-" + QUuid::createUuid().toString(), "amnezia-client", nullptr, false);
        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);
        m_coreController = new TestCoreController(vpnConnection, m_settings, nullptr, this);
    }

    void cleanupTestCase()
    {
        m_settings->clearSettings();
        delete m_coreController;
        delete m_settings;
    }

    void importsEndpoint_data()
    {
        QTest::addColumn<QString>("address");
        QTest::addColumn<QString>("allowedIps");
        QTest::addColumn<QString>("endpoint");
        QTest::addColumn<QString>("hostName");
        QTest::addColumn<int>("port");
        QTest::addColumn<QStringList>("expectedAllowedIps");

        QTest::newRow("ipv4 endpoint, ipv4 traffic")
                << "10.8.1.2/32" << "0.0.0.0/0" << "203.0.113.10:51820" << "203.0.113.10" << 51820
                << QStringList { "0.0.0.0/0" };
        QTest::newRow("ipv6 endpoint, ipv4 traffic")
                << "10.8.1.2/32" << "0.0.0.0/0" << "[2001:db8::10]:51820" << "2001:db8::10" << 51820
                << QStringList { "0.0.0.0/0" };
        QTest::newRow("ipv6 endpoint, ipv6 traffic")
                << "fd00::2/128" << "::/0" << "[2001:db8::10]:51820" << "2001:db8::10" << 51820
                << QStringList { "::/0" };
        QTest::newRow("ipv6 endpoint, dual-stack traffic")
                << "10.8.1.2/32, fd00::2/128" << "0.0.0.0/0, ::/0" << "[2001:db8::10]:443" << "2001:db8::10" << 443
                << QStringList { "0.0.0.0/0", "::/0" };
        QTest::newRow("ipv6 endpoint without port")
                << "10.8.1.2/32" << "0.0.0.0/0" << "2001:db8::10" << "2001:db8::10" << 51820
                << QStringList { "0.0.0.0/0" };
        QTest::newRow("domain endpoint")
                << "10.8.1.2/32" << "0.0.0.0/0" << "vpn.example.com:51820" << "vpn.example.com" << 51820
                << QStringList { "0.0.0.0/0" };
    }

    void importsEndpoint()
    {
        QFETCH(QString, address);
        QFETCH(QString, allowedIps);
        QFETCH(QString, endpoint);
        QFETCH(QString, hostName);
        QFETCH(int, port);
        QFETCH(QStringList, expectedAllowedIps);

        const QString config = wireGuardConfig(address, allowedIps, endpoint);
        const auto result = m_coreController->m_importCoreController->extractConfigFromData(config);
        QCOMPARE(result.errorCode, ErrorCode::NoError);
        QCOMPARE(result.config.value(configKey::hostName).toString(), hostName);

        const QJsonObject container = result.config.value(configKey::containers).toArray().first().toObject();
        const QJsonObject protocolConfig = container.value(configKey::wireguard).toObject();
        QCOMPARE(protocolConfig.value(configKey::port).toString(), QString::number(port));

        const QJsonObject lastConfig =
                QJsonDocument::fromJson(protocolConfig.value(configKey::lastConfig).toString().toUtf8()).object();
        QCOMPARE(lastConfig.value(configKey::hostName).toString(), hostName);
        QCOMPARE(lastConfig.value(configKey::port).toInt(), port);
        QCOMPARE(lastConfig.value(configKey::clientIp).toString(), address);
        QCOMPARE(lastConfig.value(configKey::allowedIps).toVariant().toStringList(), expectedAllowedIps);
    }

    void rejectsInvalidEndpoint_data()
    {
        QTest::addColumn<QString>("endpoint");

        QTest::newRow("unbracketed ipv6 with port") << "2001:db8::10:51820";
        QTest::newRow("invalid port") << "[2001:db8::10]:70000";
        QTest::newRow("missing") << "";
    }

    void rejectsInvalidEndpoint()
    {
        QFETCH(QString, endpoint);

        const auto result = m_coreController->m_importCoreController->extractConfigFromData(
                wireGuardConfig("10.8.1.2/32", "0.0.0.0/0", endpoint));
        QCOMPARE(result.errorCode, ErrorCode::ImportInvalidConfigError);
    }
};

QTEST_MAIN(TestWireGuardEndpointImport)
#include "testWireGuardEndpointImport.moc"
