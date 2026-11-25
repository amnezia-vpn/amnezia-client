#include <QUuid>
#include <QSignalSpy>
#include <QTest>

#include "vpnConnection.h"
#include "secureQSettings.h"

#include "utils/testCoreController.h"
#include "ui/controllers/serversUiController.h"

using namespace amnezia;

class TestServersUiController : public QObject
{
    Q_OBJECT

private:
    TestCoreController *m_coreController = nullptr;
    SecureQSettings *m_settings = nullptr;

private slots:
    void initTestCase()
    {
        const QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);
        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);
        m_coreController = new TestCoreController(vpnConnection, m_settings, nullptr, this);
    }

    void cleanupTestCase()
    {
        if (m_settings) {
            m_settings->clearSettings();
        }

        delete m_coreController;
        m_coreController = nullptr;

        delete m_settings;
        m_settings = nullptr;
    }

    void init()
    {
        m_coreController->m_serversRepository->clearServers();
        m_coreController->m_serversUiController->setProcessedServerId(QString());
        m_coreController->m_serversUiController->updateModel();
    }

    void testEmptyServerList()
    {
        QCOMPARE(m_coreController->m_serversUiController->getServersCount(), 0);
        QCOMPARE(m_coreController->m_serversUiController->getServerId(0), QString());
        QCOMPARE(m_coreController->m_serversUiController->getServerIndexById(QStringLiteral("unknown")), -1);
        QVERIFY(!m_coreController->m_serversUiController->hasServersFromGatewayApi());
    }

    void testGetServerIdWithInvalidIndex()
    {
        QCOMPARE(m_coreController->m_serversUiController->getServerId(-1), QString());
        QCOMPARE(m_coreController->m_serversUiController->getServerId(0), QString());
        QCOMPARE(m_coreController->m_serversUiController->getServerId(1), QString());
    }

    void testGetServerIndexByIdWithEmptyId()
    {
        QCOMPARE(m_coreController->m_serversUiController->getServerIndexById(QString()), -1);
    }

    void testProcessedServerIdWithUnknownServer()
    {
        QSignalSpy spy(m_coreController->m_serversUiController, &ServersUiController::processedServerIdChanged);

        m_coreController->m_serversUiController->setProcessedServerId(QStringLiteral("unknown-server"));

        QCOMPARE(m_coreController->m_serversUiController->getProcessedServerId(), QString());
        QCOMPARE(spy.count(), 0);
    }

    void testProcessedServerIdCleared()
    {
        QSignalSpy spy(m_coreController->m_serversUiController, &ServersUiController::processedServerIdChanged);

        m_coreController->m_serversUiController->setProcessedServerId(QString());

        QCOMPARE(m_coreController->m_serversUiController->getProcessedServerId(), QString());
        QCOMPARE(spy.count(), 0);
    }

    void testDefaultServerPropertiesWithoutServers()
    {
        QCOMPARE(m_coreController->m_serversUiController->getDefaultServerId(), QString());
        QCOMPARE(m_coreController->m_serversUiController->getDefaultServerName(), QString());
        QCOMPARE(m_coreController->m_serversUiController->getDefaultServerDefaultContainerName(), QString());
        QCOMPARE(m_coreController->m_serversUiController->getDefaultServerDescriptionCollapsed(), QString());
        QCOMPARE(m_coreController->m_serversUiController->getDefaultServerDescriptionExpanded(), QString());
        QCOMPARE(m_coreController->m_serversUiController->getDefaultServerImagePathCollapsed(), QString());
        QVERIFY(!m_coreController->m_serversUiController->isDefaultServerFromApi());
        QVERIFY(!m_coreController->m_serversUiController->isAdVisible());
        QVERIFY(m_coreController->m_serversUiController->adHeader().isEmpty());
        QVERIFY(m_coreController->m_serversUiController->adDescription().isEmpty());
    }

    void testGetAllInstalledServicesNameWithInvalidIndex()
    {
        const QStringList services = m_coreController->m_serversUiController->getAllInstalledServicesName(-1);

        QVERIFY(services.isEmpty());
    }
};

QTEST_MAIN(TestServersUiController)
#include "testServersUiController.moc"
