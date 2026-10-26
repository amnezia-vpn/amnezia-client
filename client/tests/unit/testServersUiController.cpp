#include <QSignalSpy>
#include <QTest>

#include "utils/testCoreController.h"
#include "ui/controllers/serversUiController.h"

using namespace amnezia;

class TestServersUiController : public QObject
{
    Q_OBJECT

private:
    TestCoreController *m_coreController = nullptr;
    ServersUiController *m_controller = nullptr;

private slots:
    void initTestCase()
    {
        m_coreController = new TestCoreController;
        m_controller = m_coreController->m_serversUiController;

        QVERIFY(m_controller != nullptr);
    }

    void cleanupTestCase()
    {
        delete m_coreController;
        m_coreController = nullptr;
        m_controller = nullptr;
    }

    void init()
    {
        m_coreController->m_serversRepository->clearServers();
        m_controller->setProcessedServerId(QString());
        m_controller->updateModel();
    }

    void testEmptyServerList()
    {
        QCOMPARE(m_controller->getServersCount(), 0);
        QCOMPARE(m_controller->getServerId(0), QString());
        QCOMPARE(m_controller->getServerIndexById(QStringLiteral("unknown")), -1);
        QVERIFY(!m_controller->hasServersFromGatewayApi());
    }

    void testGetServerIdWithInvalidIndex()
    {
        QCOMPARE(m_controller->getServerId(-1), QString());
        QCOMPARE(m_controller->getServerId(0), QString());
        QCOMPARE(m_controller->getServerId(1), QString());
    }

    void testGetServerIndexByIdWithEmptyId()
    {
        QCOMPARE(m_controller->getServerIndexById(QString()), -1);
    }

    void testProcessedServerIdWithUnknownServer()
    {
        QSignalSpy spy(m_controller, &ServersUiController::processedServerIdChanged);

        m_controller->setProcessedServerId(QStringLiteral("unknown-server"));

        QCOMPARE(m_controller->getProcessedServerId(), QString());
        QCOMPARE(spy.count(), 0);
    }

    void testProcessedServerIdCleared()
    {
        QSignalSpy spy(m_controller, &ServersUiController::processedServerIdChanged);

        m_controller->setProcessedServerId(QString());

        QCOMPARE(m_controller->getProcessedServerId(), QString());
        QCOMPARE(spy.count(), 0);
    }

    void testDefaultServerPropertiesWithoutServers()
    {
        QCOMPARE(m_controller->getDefaultServerId(), QString());
        QCOMPARE(m_controller->getDefaultServerName(), QString());
        QCOMPARE(m_controller->getDefaultServerDefaultContainerName(), QString());
        QCOMPARE(m_controller->getDefaultServerDescriptionCollapsed(), QString());
        QCOMPARE(m_controller->getDefaultServerDescriptionExpanded(), QString());
        QCOMPARE(m_controller->getDefaultServerImagePathCollapsed(), QString());
        QVERIFY(!m_controller->isDefaultServerFromApi());
        QVERIFY(!m_controller->isAdVisible());
        QVERIFY(m_controller->adHeader().isEmpty());
        QVERIFY(m_controller->adDescription().isEmpty());
    }

    void testGetAllInstalledServicesNameWithInvalidIndex()
    {
        const QStringList services = m_controller->getAllInstalledServicesName(-1);

        QVERIFY(services.isEmpty());
    }
};

QTEST_MAIN(TestServersUiController)
#include "testServersUiController.moc"
