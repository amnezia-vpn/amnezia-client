#include <QTest>
#include <QJsonObject>
#include <QUuid>
#include <QSignalSpy>

#include "core/controllers/coreController.h"
#include "core/repositories/secureServersRepository.h"
#include "core/models/serverDescription.h"
#include "core/models/selfhosted/selfHostedAdminServerConfig.h"
#include "vpnConnection.h"
#include "secureQSettings.h"
#include "core/utils/serverConfigUtils.h"

using namespace amnezia;

class TestServerEdgeCases : public QObject
{
    Q_OBJECT

private:
    CoreController* m_coreController;
    SecureQSettings* m_settings;

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
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), QString());
        }
    }

    void testInvalidIndexOperations() {
        QString awgKey = "vpn://AAABFHjadZBBT4QwEIX_ipkzS2wBJdyMB1cPXvbgwRgyQnclgZa0RTYS_rszXRa52Mt77TfzOu0EldEeG62sg-J9AhxPUEywF1CAuF3WTl4dRLCXhJIVpVuUEMpWdLdFKaH7FeUb9Mx3scpFk0XTRbOLvlSkKZsOz-Gi4BsdRiV_EGEydhwlg0tWynEZmd5Yz1bkoaK3xpvKtOU3_UFjOE3SsRs-tfIl1rVVzoWQOI9FzC3eonYcU4ZmgkPdwxz9fSYdYafVT4M7-lEJ80cEtTri0PrH_2q4wlW26f1lioe3p5uDsjQWoS_j_Ct2ipvGU6zO2PWtiivT8RPQudHYmqBXzl-3Yn2slBEMTtklgYt4C_Mv3ROMwA";

        auto importResult = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult.config);

        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 1, "Should have 1 server");

        QSignalSpy serverRemovedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverRemoved);
        QSignalSpy serverEditedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverEdited);
        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);

        m_coreController->m_serversController->removeServer(m_coreController->m_serversController->getServerId(-1));
        QVERIFY2(serverRemovedSpy.count() == 0, "serverRemoved should NOT be emitted for invalid index");

        m_coreController->m_serversController->removeServer(m_coreController->m_serversController->getServerId(10));
        QVERIFY2(serverRemovedSpy.count() == 0, "serverRemoved should NOT be emitted for invalid index");

        m_coreController->m_serversController->removeServer(m_coreController->m_serversController->getServerId(100));
        QVERIFY2(serverRemovedSpy.count() == 0, "serverRemoved should NOT be emitted for invalid index");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 1, "Server count should remain 1");

        const QString validServerId = m_coreController->m_serversController->getServerId(0);
        const serverConfigUtils::ConfigType editKind =
            m_coreController->m_serversRepository->serverKind(validServerId);

        m_coreController->m_serversRepository->editServer(m_coreController->m_serversController->getServerId(-1),
                                                          QJsonObject(), editKind);
        QVERIFY2(serverEditedSpy.count() == 0, "serverEdited should NOT be emitted for invalid index");

        m_coreController->m_serversRepository->editServer(m_coreController->m_serversController->getServerId(10),
                                                        QJsonObject(), editKind);
        QVERIFY2(serverEditedSpy.count() == 0, "serverEdited should NOT be emitted for invalid index");

        m_coreController->m_serversController->setDefaultServer(m_coreController->m_serversController->getServerId(-1));
        QVERIFY2(defaultServerChangedSpy.count() == 0, "defaultServerChanged should NOT be emitted for invalid index");

        m_coreController->m_serversController->setDefaultServer(m_coreController->m_serversController->getServerId(10));
        QVERIFY2(defaultServerChangedSpy.count() == 0, "defaultServerChanged should NOT be emitted for invalid index");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "Default server index should remain 0");
    }

    void testEmptyRepositoryOperations() {
        QSignalSpy serverRemovedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverRemoved);
        QSignalSpy serverEditedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverEdited);
        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);

        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 0, "Should start with 0 servers");

        m_coreController->m_serversController->removeServer(m_coreController->m_serversController->getServerId(0));
        QVERIFY2(serverRemovedSpy.count() == 0, "serverRemoved should NOT be emitted for empty repository");

        m_coreController->m_serversRepository->editServer(m_coreController->m_serversController->getServerId(0),
                                                            SelfHostedAdminServerConfig {}.toJson(),
                                                            serverConfigUtils::ConfigType::SelfHostedAdmin);
        QVERIFY2(serverEditedSpy.count() == 0, "serverEdited should NOT be emitted for empty repository");

        m_coreController->m_serversController->setDefaultServer(m_coreController->m_serversController->getServerId(0));
        QVERIFY2(defaultServerChangedSpy.count() == 0, "defaultServerChanged should NOT be emitted for empty repository");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "Default server index should be 0 for empty repository");

        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 0, "Server count should remain 0");
    }
};

QTEST_MAIN(TestServerEdgeCases)
#include "testServerEdgeCases.moc"

