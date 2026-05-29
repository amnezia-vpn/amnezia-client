#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QSignalSpy>

#include "core/controllers/coreController.h"
#include "core/models/serverDescription.h"
#include "tests/testServerRepositoryHelpers.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

using namespace amnezia;

class TestComplexOperations : public QObject
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
        if (m_coreController->m_serversModel) {
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), -1);
        }
    }

    void testComplexOperationSequence() {
        QString awgKey = "vpn://AAABFHjadZBBT4QwEIX_ipkzS2wBJdyMB1cPXvbgwRgyQnclgZa0RTYS_rszXRa52Mt77TfzOu0EldEeG62sg-J9AhxPUEywF1CAuF3WTl4dRLCXhJIVpVuUEMpWdLdFKaH7FeUb9Mx3scpFk0XTRbOLvlSkKZsOz-Gi4BsdRiV_EGEydhwlg0tWynEZmd5Yz1bkoaK3xpvKtOU3_UFjOE3SsRs-tfIl1rVVzoWQOI9FzC3eonYcU4ZmgkPdwxz9fSYdYafVT4M7-lEJ80cEtTri0PrH_2q4wlW26f1lioe3p5uDsjQWoS_j_Ct2ipvGU6zO2PWtiivT8RPQudHYmqBXzl-3Yn2slBEMTtklgYt4C_Mv3ROMwA";
        QString xrayKey = "vpn://AAAAtXjadY7NCsJADIRfRXKui1YP0qt3L14EkRK7EQt2d0lS_0rf3awonjyFmW-YyQBNDIptIBao9sNPQgXYBXq2OL0zPqCA96kGSJHV6HK5MFP6YyCt0XsmsQqYz9zKzd3MmDIGyek6cdRoUJsE43gowNMJ-4uu_695kobbpG0MBndmTrbEV4sWcI6iG-zIQE47umOXLuSa2BlNKHKL7PMeiX5lmdH79bIsoBfiT0UOZQnjCw_AXRQ";
        QString wgKey = "vpn://AAAAwXjahY89a8NADIb_StDsHLFDIHjt0C1LhgwlBNWnpgfx3SHp6hDj_15dacnYTS_Po68ZhhQVQyQW6N_mZ4QecIz0CLieAtO1IHto4Fn3M-TEat6u3XetMSnvkfSC3jOJjYN24_audRtjyhil-pfMSZPB4jMsy7kBTx9Ybvryz2ZPMnDIGlI042TktZLVkfjLmhr4TKIHHMnodHV0xzHfyA1pNJZRZEr1alAS_Yvbin6e6LoGihD_DqhSjbB8AyB_ZI8";

        QSignalSpy importFinishedSpy(m_coreController->m_importCoreController, &ImportController::importFinished);
        QSignalSpy serverAddedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverAdded);
        QSignalSpy serverEditedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverEdited);
        QSignalSpy serverRemovedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverRemoved);
        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);

        auto importResult1 = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult1.config);
        auto importResult2 = m_coreController->m_importCoreController->extractConfigFromData(xrayKey);
        m_coreController->m_importCoreController->importConfig(importResult2.config);
        auto importResult3 = m_coreController->m_importCoreController->extractConfigFromData(wgKey);
        m_coreController->m_importCoreController->importConfig(importResult3.config);

        QVERIFY2(importFinishedSpy.count() == 3, "importFinished should be emitted 3 times");
        QVERIFY2(serverAddedSpy.count() == 3, "serverAdded should be emitted 3 times");
        QVERIFY2(defaultServerChangedSpy.count() == 2, "defaultServerChanged should be emitted 2 times (0->1, 1->2, first import doesn't emit as default is already 0)");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 3, "Should have 3 servers");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 2, "Default should be index 2");

        amnezia::test::setServerDescription(m_coreController->m_serversRepository,
                                            m_coreController->m_serversController->getServerId(0),
                                            QStringLiteral("Edited First Server"));

        QVERIFY2(serverEditedSpy.count() == 1, "serverEdited should be emitted");
        QString editedDesc0 = amnezia::test::serverDescription(m_coreController->m_serversRepository,
                                                               m_coreController->m_serversRepository->serverIdAt(0));
        QVERIFY2(editedDesc0 == "Edited First Server", "First server should be edited");

        m_coreController->m_serversController->removeServer(m_coreController->m_serversController->getServerId(1));

        QVERIFY2(serverRemovedSpy.count() == 1, "serverRemoved should be emitted");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 2, "Should have 2 servers");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 1, "Default should be index 1 (was 2, removed 1)");

        m_coreController->m_serversController->setDefaultServer(m_coreController->m_serversController->getServerId(0));

        QVERIFY2(defaultServerChangedSpy.count() == 4, "defaultServerChanged should be emitted again");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "Default should be index 0");

        amnezia::test::setServerDescription(m_coreController->m_serversRepository,
                                            m_coreController->m_serversController->getServerId(0),
                                            QStringLiteral("Final Edited Server"));

        QVERIFY2(serverEditedSpy.count() == 2, "serverEdited should be emitted again");
        QString finalDesc0 = amnezia::test::serverDescription(m_coreController->m_serversRepository,
                                                              m_coreController->m_serversRepository->serverIdAt(0));
        QVERIFY2(finalDesc0 == "Final Edited Server", "First server should be edited again");

        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 2, "Final servers count should be 2");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "Final default index should be 0");

        if (m_coreController->m_serversModel) {
            QVERIFY2(m_coreController->m_serversModel->rowCount() == 2, "Model should have 2 rows");
            QString modelDesc0 = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(0, 0), ServersModel::NameRole).toString();
            QVERIFY2(modelDesc0 == "Final Edited Server", "Model should reflect final edited name");
        }
    }
};

QTEST_MAIN(TestComplexOperations)
#include "testComplexOperations.moc"

