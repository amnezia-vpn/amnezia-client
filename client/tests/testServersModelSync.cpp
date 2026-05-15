#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QSignalSpy>

#include "core/controllers/coreController.h"
#include "core/models/serverDescription.h"
#include "tests/testServerRepositoryHelpers.h"
#include "ui/models/serversModel.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

using namespace amnezia;

class TestServersModelSync : public QObject
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

    void testServersModelSyncOnOperations() {
        QString awgKey = "vpn://AAABFHjadZBBT4QwEIX_ipkzS2wBJdyMB1cPXvbgwRgyQnclgZa0RTYS_rszXRa52Mt77TfzOu0EldEeG62sg-J9AhxPUEywF1CAuF3WTl4dRLCXhJIVpVuUEMpWdLdFKaH7FeUb9Mx3scpFk0XTRbOLvlSkKZsOz-Gi4BsdRiV_EGEydhwlg0tWynEZmd5Yz1bkoaK3xpvKtOU3_UFjOE3SsRs-tfIl1rVVzoWQOI9FzC3eonYcU4ZmgkPdwxz9fSYdYafVT4M7-lEJ80cEtTri0PrH_2q4wlW26f1lioe3p5uDsjQWoS_j_Ct2ipvGU6zO2PWtiivT8RPQudHYmqBXzl-3Yn2slBEMTtklgYt4C_Mv3ROMwA";

        if (!m_coreController->m_serversModel) {
            QSKIP("ServersModel not available");
        }

        QVERIFY2(m_coreController->m_serversModel->rowCount() == 0, "Initial model row count should be 0");

        auto importResult = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult.config);

        QVERIFY2(m_coreController->m_serversModel->rowCount() == 1, "Model should have 1 row after import");
        QString modelDesc1 = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(0, 0), ServersModel::NameRole).toString();
        QVERIFY2(modelDesc1 == "AWG Server", "Model should have correct server name");

        amnezia::test::setServerDescription(m_coreController->m_serversRepository,
                                            m_coreController->m_serversController->getServerId(0),
                                            QStringLiteral("Edited AWG Server"));

        QString modelDesc2 = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(0, 0), ServersModel::NameRole).toString();
        QVERIFY2(modelDesc2 == "Edited AWG Server", "Model should be updated after edit");

        m_coreController->m_serversController->removeServer(m_coreController->m_serversController->getServerId(0));
        QVERIFY2(m_coreController->m_serversModel->rowCount() == 0, "Model should have 0 rows after removal");
    }

    void testServersModelDefaultIndexSync() {
        QString awgKey = "vpn://AAABFHjadZBBT4QwEIX_ipkzS2wBJdyMB1cPXvbgwRgyQnclgZa0RTYS_rszXRa52Mt77TfzOu0EldEeG62sg-J9AhxPUEywF1CAuF3WTl4dRLCXhJIVpVuUEMpWdLdFKaH7FeUb9Mx3scpFk0XTRbOLvlSkKZsOz-Gi4BsdRiV_EGEydhwlg0tWynEZmd5Yz1bkoaK3xpvKtOU3_UFjOE3SsRs-tfIl1rVVzoWQOI9FzC3eonYcU4ZmgkPdwxz9fSYdYafVT4M7-lEJ80cEtTri0PrH_2q4wlW26f1lioe3p5uDsjQWoS_j_Ct2ipvGU6zO2PWtiivT8RPQudHYmqBXzl-3Yn2slBEMTtklgYt4C_Mv3ROMwA";
        QString xrayKey = "vpn://AAAAtXjadY7NCsJADIRfRXKui1YP0qt3L14EkRK7EQt2d0lS_0rf3awonjyFmW-YyQBNDIptIBao9sNPQgXYBXq2OL0zPqCA96kGSJHV6HK5MFP6YyCt0XsmsQqYz9zKzd3MmDIGyek6cdRoUJsE43gowNMJ-4uu_695kobbpG0MBndmTrbEV4sWcI6iG-zIQE47umOXLuSa2BlNKHKL7PMeiX5lmdH79bIsoBfiT0UOZQnjCw_AXRQ";
        QString wgKey = "vpn://AAAAwXjahY89a8NADIb_StDsHLFDIHjt0C1LhgwlBNWnpgfx3SHp6hDj_15dacnYTS_Po68ZhhQVQyQW6N_mZ4QecIz0CLieAtO1IHto4Fn3M-TEat6u3XetMSnvkfSC3jOJjYN24_audRtjyhil-pfMSZPB4jMsy7kBTx9Ybvryz2ZPMnDIGlI042TktZLVkfjLmhr4TKIHHMnodHV0xzHfyA1pNJZRZEr1alAS_Yvbin6e6LoGihD_DqhSjbB8AyB_ZI8";

        if (!m_coreController->m_serversModel) {
            QSKIP("ServersModel not available");
        }

        auto importResult1 = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult1.config);
        auto importResult2 = m_coreController->m_importCoreController->extractConfigFromData(xrayKey);
        m_coreController->m_importCoreController->importConfig(importResult2.config);
        auto importResult3 = m_coreController->m_importCoreController->extractConfigFromData(wgKey);
        m_coreController->m_importCoreController->importConfig(importResult3.config);

        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 2, "Default should be index 2");
        QVERIFY2(m_coreController->m_serversModel->rowCount() == 3, "Model should have 3 rows");

        bool isDefault0 = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(0, 0), ServersModel::IsDefaultRole).toBool();
        bool isDefault1 = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(1, 0), ServersModel::IsDefaultRole).toBool();
        bool isDefault2 = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(2, 0), ServersModel::IsDefaultRole).toBool();

        QVERIFY2(!isDefault0, "Server 0 should not be default");
        QVERIFY2(!isDefault1, "Server 1 should not be default");
        QVERIFY2(isDefault2, "Server 2 should be default");

        m_coreController->m_serversController->setDefaultServer(m_coreController->m_serversController->getServerId(0));

        isDefault0 = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(0, 0), ServersModel::IsDefaultRole).toBool();
        isDefault2 = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(2, 0), ServersModel::IsDefaultRole).toBool();

        QVERIFY2(isDefault0, "Server 0 should be default after change");
        QVERIFY2(!isDefault2, "Server 2 should not be default after change");
    }
};

QTEST_MAIN(TestServersModelSync)
#include "testServersModelSync.moc"

