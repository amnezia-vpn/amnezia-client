#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QSignalSpy>

#include "core/controllers/coreController.h"
#include "core/models/serverConfig.h"
#include "ui/models/serversModel.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

using namespace amnezia;

class TestDefaultServerChange : public QObject
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
            m_coreController->m_serversModel->updateModel(QVector<ServerConfig>(), -1, false);
        }
    }

    void testSetDefaultServerIndex() {
        QString awgKey = "vpn://AAABFHjadZBBT4QwEIX_ipkzS2wBJdyMB1cPXvbgwRgyQnclgZa0RTYS_rszXRa52Mt77TfzOu0EldEeG62sg-J9AhxPUEywF1CAuF3WTl4dRLCXhJIVpVuUEMpWdLdFKaH7FeUb9Mx3scpFk0XTRbOLvlSkKZsOz-Gi4BsdRiV_EGEydhwlg0tWynEZmd5Yz1bkoaK3xpvKtOU3_UFjOE3SsRs-tfIl1rVVzoWQOI9FzC3eonYcU4ZmgkPdwxz9fSYdYafVT4M7-lEJ80cEtTri0PrH_2q4wlW26f1lioe3p5uDsjQWoS_j_Ct2ipvGU6zO2PWtiivT8RPQudHYmqBXzl-3Yn2slBEMTtklgYt4C_Mv3ROMwA";
        QString xrayKey = "vpn://AAAAtXjadY7NCsJADIRfRXKui1YP0qt3L14EkRK7EQt2d0lS_0rf3awonjyFmW-YyQBNDIptIBao9sNPQgXYBXq2OL0zPqCA96kGSJHV6HK5MFP6YyCt0XsmsQqYz9zKzd3MmDIGyek6cdRoUJsE43gowNMJ-4uu_695kobbpG0MBndmTrbEV4sWcI6iG-zIQE47umOXLuSa2BlNKHKL7PMeiX5lmdH79bIsoBfiT0UOZQnjCw_AXRQ";
        QString wgKey = "vpn://AAAAwXjahY89a8NADIb_StDsHLFDIHjt0C1LhgwlBNWnpgfx3SHp6hDj_15dacnYTS_Po68ZhhQVQyQW6N_mZ4QecIz0CLieAtO1IHto4Fn3M-TEat6u3XetMSnvkfSC3jOJjYN24_audRtjyhil-pfMSZPB4jMsy7kBTx9Ybvryz2ZPMnDIGlI042TktZLVkfjLmhr4TKIHHMnodHV0xzHfyA1pNJZRZEr1alAS_Yvbin6e6LoGihD_DqhSjbB8AyB_ZI8";

        auto importResult1 = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult1.config);
        auto importResult2 = m_coreController->m_importCoreController->extractConfigFromData(xrayKey);
        m_coreController->m_importCoreController->importConfig(importResult2.config);
        auto importResult3 = m_coreController->m_importCoreController->extractConfigFromData(wgKey);
        m_coreController->m_importCoreController->importConfig(importResult3.config);

        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 3, "Should have 3 servers");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 2, "Default should be index 2");

        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);

        m_coreController->m_serversController->setDefaultServerIndex(0);
        QVERIFY2(defaultServerChangedSpy.count() == 1, "defaultServerChanged signal should be emitted");
        QVERIFY2(defaultServerChangedSpy.at(0).at(0).toInt() == 0, "defaultServerChanged should emit index 0");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "Default server index should be 0");

        if (m_coreController->m_serversModel) {
            int modelDefaultIndex = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(0, 0), ServersModel::IsDefaultRole).toBool() ? 0 : -1;
            QVERIFY2(modelDefaultIndex == 0, "Model should reflect default server");
        }

        m_coreController->m_serversController->setDefaultServerIndex(2);
        QVERIFY2(defaultServerChangedSpy.count() == 2, "defaultServerChanged signal should be emitted again");
        QVERIFY2(defaultServerChangedSpy.at(1).at(0).toInt() == 2, "defaultServerChanged should emit index 2");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 2, "Default server index should be 2");
    }

    void testDefaultServerChangeOnRemoveEdgeCases() {
        QString awgKey = "vpn://AAABFHjadZBBT4QwEIX_ipkzS2wBJdyMB1cPXvbgwRgyQnclgZa0RTYS_rszXRa52Mt77TfzOu0EldEeG62sg-J9AhxPUEywF1CAuF3WTl4dRLCXhJIVpVuUEMpWdLdFKaH7FeUb9Mx3scpFk0XTRbOLvlSkKZsOz-Gi4BsdRiV_EGEydhwlg0tWynEZmd5Yz1bkoaK3xpvKtOU3_UFjOE3SsRs-tfIl1rVVzoWQOI9FzC3eonYcU4ZmgkPdwxz9fSYdYafVT4M7-lEJ80cEtTri0PrH_2q4wlW26f1lioe3p5uDsjQWoS_j_Ct2ipvGU6zO2PWtiivT8RPQudHYmqBXzl-3Yn2slBEMTtklgYt4C_Mv3ROMwA";
        QString xrayKey = "vpn://AAAAtXjadY7NCsJADIRfRXKui1YP0qt3L14EkRK7EQt2d0lS_0rf3awonjyFmW-YyQBNDIptIBao9sNPQgXYBXq2OL0zPqCA96kGSJHV6HK5MFP6YyCt0XsmsQqYz9zKzd3MmDIGyek6cdRoUJsE43gowNMJ-4uu_695kobbpG0MBndmTrbEV4sWcI6iG-zIQE47umOXLuSa2BlNKHKL7PMeiX5lmdH79bIsoBfiT0UOZQnjCw_AXRQ";
        QString wgKey = "vpn://AAAAwXjahY89a8NADIb_StDsHLFDIHjt0C1LhgwlBNWnpgfx3SHp6hDj_15dacnYTS_Po68ZhhQVQyQW6N_mZ4QecIz0CLieAtO1IHto4Fn3M-TEat6u3XetMSnvkfSC3jOJjYN24_audRtjyhil-pfMSZPB4jMsy7kBTx9Ybvryz2ZPMnDIGlI042TktZLVkfjLmhr4TKIHHMnodHV0xzHfyA1pNJZRZEr1alAS_Yvbin6e6LoGihD_DqhSjbB8AyB_ZI8";

        auto importResult1 = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult1.config);
        auto importResult2 = m_coreController->m_importCoreController->extractConfigFromData(xrayKey);
        m_coreController->m_importCoreController->importConfig(importResult2.config);
        auto importResult3 = m_coreController->m_importCoreController->extractConfigFromData(wgKey);
        m_coreController->m_importCoreController->importConfig(importResult3.config);

        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 3, "Should have 3 servers");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 2, "Default should be index 2");

        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);
        QSignalSpy serverRemovedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverRemoved);

        m_coreController->m_serversController->removeServer(0);
        QVERIFY2(serverRemovedSpy.count() == 1, "serverRemoved signal should be emitted");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 2, "Should have 2 servers");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 1, "Default should be index 1 (was 2, removed 0)");

        ServerConfig remainingServer1 = m_coreController->m_serversRepository->server(0);
        ServerConfig remainingServer2 = m_coreController->m_serversRepository->server(1);
        QString desc1 = remainingServer1.description();
        QString desc2 = remainingServer2.description();
        QVERIFY2(desc1 == "Xray Server", "First remaining server should be Xray");
        QVERIFY2(desc2 == "WireGuard Server", "Second remaining server should be WireGuard");

        defaultServerChangedSpy.clear();
        serverRemovedSpy.clear();

        m_coreController->m_serversController->removeServer(0);
        QVERIFY2(serverRemovedSpy.count() == 1, "serverRemoved signal should be emitted");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 1, "Should have 1 server");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "Default should be index 0 (was 1, removed 0)");

        ServerConfig lastServer = m_coreController->m_serversRepository->server(0);
        QString lastDesc = lastServer.description();
        QVERIFY2(lastDesc == "WireGuard Server", "Last server should be WireGuard");
    }
};

QTEST_MAIN(TestDefaultServerChange)
#include "testDefaultServerChange.moc"

