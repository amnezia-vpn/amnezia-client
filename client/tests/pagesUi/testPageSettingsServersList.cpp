#include <QTest>
#include <QQuickItem>
#include <QSignalSpy>

#include "core/controllers/selfhosted/importController.h"
#include "core/models/serverDescription.h"
#include "ui/models/serversModel.h"
#include "ui/controllers/serversUiController.h"

#include "pagesUiTestFixture.h"

using namespace amnezia;

class TestPageSettingsServersList : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

    void addServer(const QString &serverId,
                   const QString &name,
                   const QString &hostName)
    {
        ServerDescription server;
        server.serverId = serverId;
        server.serverName = name;
        server.hostName = hostName;

        QVector<ServerDescription> descriptions;
        descriptions.append(server);

        m_fixture.core()->m_serversModel->updateModel(descriptions, serverId);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_fixture.initialize(this));
    }

    void init()
    {
        m_fixture.core()->m_serversModel->updateModel(QVector<ServerDescription>(), QString());
    }

    void cleanup()
    {
        m_fixture.closePage();
    }

    void cleanupTestCase()
    {
        m_fixture.cleanup();
    }

    void pageLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsServersList")));
        QVERIFY(m_fixture.page());
    }

    void emptyServerListIsDisplayed()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsServersList")));
        QCOMPARE(m_fixture.core()->m_serversModel->rowCount(), 0);
    }

    void serverRowsAreDisplayed()
    {
        addServer(
            QStringLiteral("test-server-1"),
            QStringLiteral("Test server"),
            QStringLiteral("test.example.com"));

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsServersList")));

        QQuickItem *serverItem = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Test server"));

        QVERIFY2(serverItem, "Server name was not found in the list");

        QQuickItem *serverHost = m_fixture.visibleItem(QStringLiteral("subtitle"), QStringLiteral("test.example.com"));

        QVERIFY2(serverHost, "Server hostname was not found in the list");
    }

    void multipleServersAreDisplayed()
    {
        ServerDescription firstServer;
        firstServer.serverId = QStringLiteral("server-1");
        firstServer.serverName = QStringLiteral("First server");
        firstServer.hostName = QStringLiteral("first.example.com");

        ServerDescription secondServer;
        secondServer.serverId = QStringLiteral("server-2");
        secondServer.serverName = QStringLiteral("Second server");
        secondServer.hostName = QStringLiteral("second.example.com");

        QVector<ServerDescription> descriptions;
        descriptions.append(firstServer);
        descriptions.append(secondServer);

        m_fixture.core()->m_serversModel->updateModel(descriptions, QStringLiteral("server-1"));

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsServersList")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("First server")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Second server")));
    }

    
void clickingServerSetsProcessedServerId()
    {
        QSignalSpy importFinishedSpy(m_fixture.core()->m_importCoreController, &ImportController::importFinished);

        m_fixture.core()->m_importCoreController->importConfig(createAwg2Config());

        QCOMPARE(importFinishedSpy.count(), 1);

        m_fixture.core()->m_serversUiController->updateModel();

        QCOMPARE(m_fixture.core()->m_serversModel->rowCount(), 1);

        const QString serverId = m_fixture.core()->m_serversUiController->getServerId(0);

        QVERIFY2(!serverId.isEmpty(), "Imported server ID is empty");
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsServersList")));

        QQuickItem *serverItem = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("AWG2 Test Server"));

        QVERIFY2(serverItem, "Server item was not found");
        QCOMPARE(m_fixture.core()->m_serversUiController->getProcessedServerId(), QString());

        m_fixture.click(serverItem);

        QTRY_COMPARE(m_fixture.core()->m_serversUiController->getProcessedServerId(), serverId);
    }
};

int runPageSettingsServersListTests(int argc, char *argv[])
{
    TestPageSettingsServersList test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSettingsServersList.moc"
