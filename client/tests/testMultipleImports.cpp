#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QUuid>
#include <QSignalSpy>

#include "core/controllers/coreController.h"
#include "core/models/serverDescription.h"
#include "tests/testServerRepositoryHelpers.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

using namespace amnezia;

class TestMultipleImports : public QObject
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

    void testMultipleImports() {
        QString awgKey = "vpn://AAABFHjadZBBT4QwEIX_ipkzS2wBJdyMB1cPXvbgwRgyQnclgZa0RTYS_rszXRa52Mt77TfzOu0EldEeG62sg-J9AhxPUEywF1CAuF3WTl4dRLCXhJIVpVuUEMpWdLdFKaH7FeUb9Mx3scpFk0XTRbOLvlSkKZsOz-Gi4BsdRiV_EGEydhwlg0tWynEZmd5Yz1bkoaK3xpvKtOU3_UFjOE3SsRs-tfIl1rVVzoWQOI9FzC3eonYcU4ZmgkPdwxz9fSYdYafVT4M7-lEJ80cEtTri0PrH_2q4wlW26f1lioe3p5uDsjQWoS_j_Ct2ipvGU6zO2PWtiivT8RPQudHYmqBXzl-3Yn2slBEMTtklgYt4C_Mv3ROMwA";
        QString xrayKey = "vpn://AAAAtXjadY7NCsJADIRfRXKui1YP0qt3L14EkRK7EQt2d0lS_0rf3awonjyFmW-YyQBNDIptIBao9sNPQgXYBXq2OL0zPqCA96kGSJHV6HK5MFP6YyCt0XsmsQqYz9zKzd3MmDIGyek6cdRoUJsE43gowNMJ-4uu_695kobbpG0MBndmTrbEV4sWcI6iG-zIQE47umOXLuSa2BlNKHKL7PMeiX5lmdH79bIsoBfiT0UOZQnjCw_AXRQ";
        QString wgKey = "vpn://AAAAwXjahY89a8NADIb_StDsHLFDIHjt0C1LhgwlBNWnpgfx3SHp6hDj_15dacnYTS_Po68ZhhQVQyQW6N_mZ4QecIz0CLieAtO1IHto4Fn3M-TEat6u3XetMSnvkfSC3jOJjYN24_audRtjyhil-pfMSZPB4jMsy7kBTx9Ybvryz2ZPMnDIGlI042TktZLVkfjLmhr4TKIHHMnodHV0xzHfyA1pNJZRZEr1alAS_Yvbin6e6LoGihD_DqhSjbB8AyB_ZI8";

        QSignalSpy importFinishedSpy(m_coreController->m_importCoreController, &ImportController::importFinished);
        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);
        
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 0, "Initial servers count should be 0");
        if (m_coreController->m_serversModel) {
            QVERIFY2(m_coreController->m_serversModel->rowCount() == 0, "Initial model row count should be 0");
        }

        auto importResult1 = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        QVERIFY2(importResult1.errorCode == ErrorCode::NoError, "First import should succeed");
        
        m_coreController->m_importCoreController->importConfig(importResult1.config);
        
        QVERIFY2(importFinishedSpy.count() == 1, "importFinished signal should be emitted once");
        QVERIFY2(defaultServerChangedSpy.count() == 0, "defaultServerChanged signal should NOT be emitted (default is already 0)");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 1, "After first import servers count should be 1");
        if (m_coreController->m_serversModel) {
            QVERIFY2(m_coreController->m_serversModel->rowCount() == 1, "After first import model row count should be 1");
        }
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "First server should be default");
        
        QString desc1 = amnezia::test::serverDescription(m_coreController->m_serversRepository,
                                                          m_coreController->m_serversRepository->serverIdAt(0));
        QVERIFY2(desc1 == "AWG Server", "First server description should match");
        
        if (m_coreController->m_serversModel) {
            QString modelDesc1 = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(0, 0), ServersModel::NameRole).toString();
            QVERIFY2(modelDesc1 == "AWG Server", "First server description in model should match");
        }

        auto importResult2 = m_coreController->m_importCoreController->extractConfigFromData(xrayKey);
        QVERIFY2(importResult2.errorCode == ErrorCode::NoError, "Second import should succeed");
        
        m_coreController->m_importCoreController->importConfig(importResult2.config);
        
        QVERIFY2(importFinishedSpy.count() == 2, "importFinished signal should be emitted twice");
        QVERIFY2(defaultServerChangedSpy.count() == 1, "defaultServerChanged signal should be emitted once (0->1, first import doesn't emit)");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 2, "After second import servers count should be 2");
        if (m_coreController->m_serversModel) {
            QVERIFY2(m_coreController->m_serversModel->rowCount() == 2, "After second import model row count should be 2");
        }
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 1, "Second server should be default");
        
        QString desc2 = amnezia::test::serverDescription(m_coreController->m_serversRepository,
                                                          m_coreController->m_serversRepository->serverIdAt(1));
        QVERIFY2(desc2 == "Xray Server", "Second server description should match");
        
        if (m_coreController->m_serversModel) {
            QString modelDesc2 = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(1, 0), ServersModel::NameRole).toString();
            QVERIFY2(modelDesc2 == "Xray Server", "Second server description in model should match");
        }

        auto importResult3 = m_coreController->m_importCoreController->extractConfigFromData(wgKey);
        QVERIFY2(importResult3.errorCode == ErrorCode::NoError, "Third import should succeed");
        
        m_coreController->m_importCoreController->importConfig(importResult3.config);
        
        QVERIFY2(importFinishedSpy.count() == 3, "importFinished signal should be emitted three times");
        QVERIFY2(defaultServerChangedSpy.count() == 2, "defaultServerChanged signal should be emitted twice (0->1, 1->2, first import doesn't emit)");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 3, "After third import servers count should be 3");
        if (m_coreController->m_serversModel) {
            QVERIFY2(m_coreController->m_serversModel->rowCount() == 3, "After third import model row count should be 3");
        }
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 2, "Third server should be default");
        
        QString desc3 = amnezia::test::serverDescription(m_coreController->m_serversRepository,
                                                          m_coreController->m_serversRepository->serverIdAt(2));
        QVERIFY2(desc3 == "WireGuard Server", "Third server description should match");
        
        if (m_coreController->m_serversModel) {
            QString modelDesc3 = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(2, 0), ServersModel::NameRole).toString();
            QVERIFY2(modelDesc3 == "WireGuard Server", "Third server description in model should match");
        }
    }

    void testMultipleImportsRemoval() {
        QString awgKey = "vpn://AAABFHjadZBBT4QwEIX_ipkzS2wBJdyMB1cPXvbgwRgyQnclgZa0RTYS_rszXRa52Mt77TfzOu0EldEeG62sg-J9AhxPUEywF1CAuF3WTl4dRLCXhJIVpVuUEMpWdLdFKaH7FeUb9Mx3scpFk0XTRbOLvlSkKZsOz-Gi4BsdRiV_EGEydhwlg0tWynEZmd5Yz1bkoaK3xpvKtOU3_UFjOE3SsRs-tfIl1rVVzoWQOI9FzC3eonYcU4ZmgkPdwxz9fSYdYafVT4M7-lEJ80cEtTri0PrH_2q4wlW26f1lioe3p5uDsjQWoS_j_Ct2ipvGU6zO2PWtiivT8RPQudHYmqBXzl-3Yn2slBEMTtklgYt4C_Mv3ROMwA";
        QString xrayKey = "vpn://AAAAtXjadY7NCsJADIRfRXKui1YP0qt3L14EkRK7EQt2d0lS_0rf3awonjyFmW-YyQBNDIptIBao9sNPQgXYBXq2OL0zPqCA96kGSJHV6HK5MFP6YyCt0XsmsQqYz9zKzd3MmDIGyek6cdRoUJsE43gowNMJ-4uu_695kobbpG0MBndmTrbEV4sWcI6iG-zIQE47umOXLuSa2BlNKHKL7PMeiX5lmdH79bIsoBfiT0UOZQnjCw_AXRQ";

        QSignalSpy importFinishedSpy(m_coreController->m_importCoreController, &ImportController::importFinished);
        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);
        QSignalSpy serverRemovedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverRemoved);
        
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 0, "Initial servers count should be 0");

        auto importResult1 = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        QVERIFY2(importResult1.errorCode == ErrorCode::NoError, "First import should succeed");
        m_coreController->m_importCoreController->importConfig(importResult1.config);
        
        auto importResult2 = m_coreController->m_importCoreController->extractConfigFromData(xrayKey);
        QVERIFY2(importResult2.errorCode == ErrorCode::NoError, "Second import should succeed");
        m_coreController->m_importCoreController->importConfig(importResult2.config);
        
        QVERIFY2(importFinishedSpy.count() == 2, "importFinished signal should be emitted twice");
        QVERIFY2(defaultServerChangedSpy.count() == 1, "defaultServerChanged signal should be emitted once (0->1, first import doesn't emit)");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 2, "After two imports servers count should be 2");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 1, "Second server should be default");
        
        QString desc0 = amnezia::test::serverDescription(m_coreController->m_serversRepository,
                                                          m_coreController->m_serversRepository->serverIdAt(0));
        QString desc1 = amnezia::test::serverDescription(m_coreController->m_serversRepository,
                                                          m_coreController->m_serversRepository->serverIdAt(1));
        QVERIFY2(desc0 == "AWG Server", "First server description should match");
        QVERIFY2(desc1 == "Xray Server", "Second server description should match");

        defaultServerChangedSpy.clear();
        serverRemovedSpy.clear();

        m_coreController->m_serversController->removeServer(m_coreController->m_serversController->getServerId(0));
        
        QVERIFY2(serverRemovedSpy.count() == 1, "serverRemoved signal should be emitted");
        QVERIFY2(serverRemovedSpy.at(0).at(1).toInt() == 0, "serverRemoved should emit removed index 0");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 1, "After removing first server, servers count should be 1");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "After removing first server, default index should be 0");
        
        QString remainingDesc = amnezia::test::serverDescription(m_coreController->m_serversRepository,
                                                                 m_coreController->m_serversRepository->serverIdAt(0));
        QVERIFY2(remainingDesc == "Xray Server", "Remaining server should be Xray Server");
        
        if (m_coreController->m_serversModel) {
            QVERIFY2(m_coreController->m_serversModel->rowCount() == 1, "After removing first server, model row count should be 1");
            QString modelDesc = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(0, 0), ServersModel::NameRole).toString();
            QVERIFY2(modelDesc == "Xray Server", "Remaining server description in model should match");
        }

        defaultServerChangedSpy.clear();
        serverRemovedSpy.clear();

        m_coreController->m_serversController->removeServer(m_coreController->m_serversController->getServerId(0));
        
        QVERIFY2(serverRemovedSpy.count() == 1, "serverRemoved signal should be emitted");
        QVERIFY2(serverRemovedSpy.at(0).at(1).toInt() == 0, "serverRemoved should emit removed index 0");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 0, "After removing last server, servers count should be 0");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "After removing last server, default index should be 0");
        
        if (m_coreController->m_serversModel) {
            QVERIFY2(m_coreController->m_serversModel->rowCount() == 0, "After removing last server, model row count should be 0");
        }
    }
};

QTEST_MAIN(TestMultipleImports)
#include "testMultipleImports.moc"
