#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaObject>
#include <QObject>
#include <QTest>
#include <QVariant>
#include <QSignalSpy>

#include "core/controllers/selfhosted/importController.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "ui/models/serversModel.h"

#include "pagesUiTestFixture.h"

using namespace amnezia;

class PageHomeTest : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

    QJsonObject createAwg2Config(const QString &serverName)
    {
        QJsonObject clientConfig;
        clientConfig[configKey::mtu] = protocols::awg::defaultMtu;
        clientConfig[configKey::junkPacketCount] = protocols::awg::defaultJunkPacketCount;
        clientConfig[configKey::junkPacketMinSize] = protocols::awg::defaultJunkPacketMinSize;
        clientConfig[configKey::junkPacketMaxSize] = protocols::awg::defaultJunkPacketMaxSize;
        clientConfig[configKey::specialJunk1] = protocols::awg::defaultSpecialJunk1;
        clientConfig[configKey::specialJunk2] = protocols::awg::defaultSpecialJunk2;
        clientConfig[configKey::specialJunk3] = protocols::awg::defaultSpecialJunk3;
        clientConfig[configKey::specialJunk4] = protocols::awg::defaultSpecialJunk4;
        clientConfig[configKey::specialJunk5] = protocols::awg::defaultSpecialJunk5;
        clientConfig[configKey::initPacketJunkSize] = QString::number(protocols::awg::initPacketJunkSizeMax);
        clientConfig[configKey::responsePacketJunkSize] = QString::number(protocols::awg::responsePacketJunkSizeMax);
        clientConfig[configKey::cookieReplyPacketJunkSize] = QString::number(protocols::awg::cookieReplyPacketJunkSizeMax);
        clientConfig[configKey::transportPacketJunkSize] = protocols::awg::defaultPadding;
        clientConfig[configKey::initPacketMagicHeader] = protocols::awg::defaultInitPacketMagicHeader;
        clientConfig[configKey::responsePacketMagicHeader] = protocols::awg::defaultResponsePacketMagicHeader;
        clientConfig[configKey::underloadPacketMagicHeader] = protocols::awg::defaultUnderloadPacketMagicHeader;
        clientConfig[configKey::transportPacketMagicHeader] = protocols::awg::defaultTransportPacketMagicHeader;
        clientConfig[configKey::clientPrivKey] = "test_client_private_key";
        clientConfig[configKey::clientPubKey] = "test_client_public_key";
        clientConfig[configKey::serverPubKey] = "test_server_public_key";
        clientConfig[configKey::pskKey] = "test_psk_key";
        clientConfig[configKey::clientIp] = "10.8.1.2";
        clientConfig[configKey::allowedIps] = QJsonArray::fromStringList({ "0.0.0.0/0" });

        QJsonObject awgConfig;
        awgConfig[configKey::lastConfig] = QString(QJsonDocument(clientConfig).toJson());
        awgConfig[configKey::port] = protocols::awg::defaultPort;
        awgConfig[configKey::transportProto] = "udp";
        awgConfig[configKey::protocolVersion] = protocols::awg::awgV2;
        awgConfig[configKey::subnetAddress] = protocols::wireguard::defaultSubnetAddress;
        awgConfig[configKey::junkPacketCount] = protocols::awg::defaultJunkPacketCount;
        awgConfig[configKey::junkPacketMinSize] = protocols::awg::defaultJunkPacketMinSize;
        awgConfig[configKey::junkPacketMaxSize] = protocols::awg::defaultJunkPacketMaxSize;
        awgConfig[configKey::initPacketJunkSize] = QString::number(protocols::awg::initPacketJunkSizeMax);
        awgConfig[configKey::responsePacketJunkSize] = QString::number(protocols::awg::responsePacketJunkSizeMax);
        awgConfig[configKey::cookieReplyPacketJunkSize] = QString::number(protocols::awg::cookieReplyPacketJunkSizeMax);
        awgConfig[configKey::transportPacketJunkSize] = protocols::awg::defaultPadding;
        awgConfig[configKey::initPacketMagicHeader] = protocols::awg::defaultInitPacketMagicHeader;
        awgConfig[configKey::responsePacketMagicHeader] = protocols::awg::defaultResponsePacketMagicHeader;
        awgConfig[configKey::underloadPacketMagicHeader] = protocols::awg::defaultUnderloadPacketMagicHeader;
        awgConfig[configKey::transportPacketMagicHeader] = protocols::awg::defaultTransportPacketMagicHeader;
        awgConfig[configKey::specialJunk1] = protocols::awg::defaultSpecialJunk1;
        awgConfig[configKey::specialJunk2] = protocols::awg::defaultSpecialJunk2;
        awgConfig[configKey::specialJunk3] = protocols::awg::defaultSpecialJunk3;
        awgConfig[configKey::specialJunk4] = protocols::awg::defaultSpecialJunk4;
        awgConfig[configKey::specialJunk5] = protocols::awg::defaultSpecialJunk5;
        awgConfig[configKey::isThirdPartyConfig] = true;

        QJsonObject container;
        container[configKey::container] = "amnezia-awg";
        container[configKey::awg] = awgConfig;

        QJsonArray containers;
        containers.append(container);

        QJsonObject config;
        config[configKey::containers] = containers;
        config[configKey::defaultContainer] = "amnezia-awg";
        config[configKey::description] = serverName;
        config[configKey::hostName] = "test.example.com";

        return config;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_fixture.initialize(this));
    }

    void cleanup()
    {
        m_fixture.closePage();
    }

    void cleanupTestCase()
    {
        m_fixture.cleanup();
    }

    void pageHomeLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageHome")));
        QVERIFY(m_fixture.page());
    }

    void apiProtocolStateIsInitiallyEmpty()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageHome")));
        QCOMPARE(m_fixture.page()->property("apiAvailableProtocols").toList(), QVariantList { });
        QCOMPARE(m_fixture.page()->property("apiCurrentProtocol").toString(), QString());
    }

    void protocolDisplayName_data()
    {
        QTest::addColumn<QString>("protocol");
        QTest::addColumn<QString>("expectedName");

        QTest::newRow("awg") << QStringLiteral("awg") << QStringLiteral("AmneziaWG");
        QTest::newRow("vless") << QStringLiteral("vless") << QStringLiteral("VLESS");
        QTest::newRow("wireguard") << QStringLiteral("wireguard") << QStringLiteral("wireguard");
        QTest::newRow("empty") << QString() << QString();
    }

    void protocolDisplayName()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageHome")));

        QFETCH(QString, protocol);
        QFETCH(QString, expectedName);

        QVariant result;

        const bool invoked =
                QMetaObject::invokeMethod(m_fixture.page(), "protocolDisplayName", Q_RETURN_ARG(QVariant, result),
                                          Q_ARG(QVariant, QVariant(protocol)));

        QVERIFY2(invoked, "Could not invoke PageHome.protocolDisplayName()");
        QCOMPARE(result.toString(), expectedName);
    }

    void selectsServerFromExpandedList()
    {
        TestCoreController *core = m_fixture.core();

        QVERIFY(core);
        QVERIFY(core->m_importCoreController);
        QVERIFY(core->m_serversRepository);
        QVERIFY(core->m_serversModel);

        // Import servers
        QSignalSpy importFinishedSpy(core->m_importCoreController, &ImportController::importFinished);

        core->m_importCoreController->importConfig(createAwg2Config(QStringLiteral("Server 1")));
        core->m_importCoreController->importConfig(createAwg2Config(QStringLiteral("Server 2")));

        QCOMPARE(importFinishedSpy.count(), 2);
        QCOMPARE(core->m_serversRepository->serversCount(), 2);
        QCOMPARE(core->m_serversModel->rowCount(), 2);

        // Find a server that is not currently selected
        const QString initialDefaultServerId = core->m_serversRepository->defaultServerId();

        QVERIFY(!initialDefaultServerId.isEmpty());

        int targetServerIndex = -1;
        QString targetServerId;
        QString targetServerName;

        for (int row = 0; row < core->m_serversModel->rowCount(); ++row) {
            const QModelIndex index = core->m_serversModel->index(row, 0);
            const QString serverId = core->m_serversModel->data(index, ServersModel::ServerIdRole).toString();

            if (serverId.isEmpty() || serverId == initialDefaultServerId) {
                continue;
            }

            targetServerIndex = row;
            targetServerId = serverId;
            targetServerName = core->m_serversModel->data(index, ServersModel::NameRole).toString();

            break;
        }

        QVERIFY2(targetServerIndex >= 0, "Could not find a server different from the default server");
        QVERIFY(!targetServerId.isEmpty());
        QVERIFY(!targetServerName.isEmpty());

        // Load PageHome
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageHome")));

        // Find the collapsed HomeServerSummary
        const QList<QQuickItem *> summaries = m_fixture.itemsOfType(QStringLiteral("HomeServerSummary"));

        QVERIFY(!summaries.isEmpty());

        QQuickItem *collapsedSummary = nullptr;

        for (QQuickItem *summary : summaries) {
            const QVariant expanded = summary->property("expanded");

            if (expanded.isValid() && !expanded.toBool()) {
                collapsedSummary = summary;
                break;
            }
        }

        QVERIFY2(collapsedSummary, "Collapsed HomeServerSummary was not found");

        // Open the server drawer
        QVERIFY(QMetaObject::invokeMethod(collapsedSummary, "expandRequested"));

        QQuickItem *targetServerItem = nullptr;

        QTRY_VERIFY((targetServerItem = m_fixture.visibleItem(QStringLiteral("text"), targetServerName)) != nullptr);
        QVERIFY(targetServerItem);
        QVERIFY(targetServerItem->isEnabled());

        // Select the target server through the UI
        m_fixture.click(targetServerItem);

        // Verify that the selected server has changed
        QTRY_COMPARE(core->m_serversRepository->defaultServerId(), targetServerId);

        // Verify that the selection is reflected in ServersModel
        const QModelIndex selectedServerIndex = core->m_serversModel->index(targetServerIndex, 0);

        QVERIFY(selectedServerIndex.isValid());
        QCOMPARE(core->m_serversModel->data(selectedServerIndex, ServersModel::IsDefaultRole).toBool(), true);
    }
};

int runPageHomeTests(int argc, char *argv[])
{
    PageHomeTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageHome.moc"
