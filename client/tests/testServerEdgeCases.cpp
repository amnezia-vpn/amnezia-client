#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QSignalSpy>
#include <QTest>

#include "core/controllers/coreController.h"
#include "core/models/serverConfig.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

using namespace amnezia;

class TestServerEdgeCases : public QObject
{
    Q_OBJECT

private:
    CoreController* m_coreController;
    SecureQSettings* m_settings;

    QString getValueFromIni(const QString &key)
    {
        QSettings settings("test_vars.ini", QSettings::IniFormat);
        return settings.value(key).toString();
    }

private slots:
    void initTestCase() {
        QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);
        
        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);
        m_coreController = new CoreController(vpnConnection, m_settings, nullptr, this);
    }

    void cleanupTestCase() {
        m_settings->clearSettings();
        delete m_coreController;
        delete m_settings;
    }

    void init() {
        m_settings->clearSettings();
        m_coreController->m_serversRepository->invalidateCache();
        if (m_coreController->m_serversModel) {
            m_coreController->m_serversModel->updateModel(QVector<ServerConfig>(), -1, false);
        }
    }

    void testInvalidIndexOperations() {
        QString awgKey = getValueFromIni("configs/TEST_CONFIG_AWG");

        auto importResult = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult.config);

        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 1, "Should have 1 server");

        QSignalSpy serverRemovedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverRemoved);
        QSignalSpy serverEditedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverEdited);
        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);

        m_coreController->m_serversController->removeServer(-1);
        QVERIFY2(serverRemovedSpy.count() == 0, "serverRemoved should NOT be emitted for invalid index");

        m_coreController->m_serversController->removeServer(10);
        QVERIFY2(serverRemovedSpy.count() == 0, "serverRemoved should NOT be emitted for invalid index");

        m_coreController->m_serversController->removeServer(100);
        QVERIFY2(serverRemovedSpy.count() == 0, "serverRemoved should NOT be emitted for invalid index");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 1, "Server count should remain 1");

        ServerConfig serverConfig = m_coreController->m_serversController->getServerConfig(0);
        m_coreController->m_serversController->editServer(-1, serverConfig);
        QVERIFY2(serverEditedSpy.count() == 0, "serverEdited should NOT be emitted for invalid index");

        m_coreController->m_serversController->editServer(10, serverConfig);
        QVERIFY2(serverEditedSpy.count() == 0, "serverEdited should NOT be emitted for invalid index");

        m_coreController->m_serversController->setDefaultServerIndex(-1);
        QVERIFY2(defaultServerChangedSpy.count() == 0, "defaultServerChanged should NOT be emitted for invalid index");

        m_coreController->m_serversController->setDefaultServerIndex(10);
        QVERIFY2(defaultServerChangedSpy.count() == 0, "defaultServerChanged should NOT be emitted for invalid index");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "Default server index should remain 0");
    }

    void testEmptyRepositoryOperations() {
        QSignalSpy serverRemovedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverRemoved);
        QSignalSpy serverEditedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverEdited);
        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);

        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 0, "Should start with 0 servers");

        ServerConfig emptyConfig = SelfHostedServerConfig{};
        m_coreController->m_serversController->removeServer(0);
        QVERIFY2(serverRemovedSpy.count() == 0, "serverRemoved should NOT be emitted for empty repository");

        m_coreController->m_serversController->editServer(0, emptyConfig);
        QVERIFY2(serverEditedSpy.count() == 0, "serverEdited should NOT be emitted for empty repository");

        m_coreController->m_serversController->setDefaultServerIndex(0);
        QVERIFY2(defaultServerChangedSpy.count() == 0, "defaultServerChanged should NOT be emitted for empty repository");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "Default server index should be 0 for empty repository");

        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 0, "Server count should remain 0");
    }
};

QTEST_MAIN(TestServerEdgeCases)
#include "testServerEdgeCases.moc"

