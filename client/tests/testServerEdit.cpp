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

class TestServerEdit : public QObject
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
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), -1);
        }
    }

    void testServerEditTriggersHandlers() {
        QString awgKey = "vpn://AAABFHjadZBBT4QwEIX_ipkzS2wBJdyMB1cPXvbgwRgyQnclgZa0RTYS_rszXRa52Mt77TfzOu0EldEeG62sg-J9AhxPUEywF1CAuF3WTl4dRLCXhJIVpVuUEMpWdLdFKaH7FeUb9Mx3scpFk0XTRbOLvlSkKZsOz-Gi4BsdRiV_EGEydhwlg0tWynEZmd5Yz1bkoaK3xpvKtOU3_UFjOE3SsRs-tfIl1rVVzoWQOI9FzC3eonYcU4ZmgkPdwxz9fSYdYafVT4M7-lEJ80cEtTri0PrH_2q4wlW26f1lioe3p5uDsjQWoS_j_Ct2ipvGU6zO2PWtiivT8RPQudHYmqBXzl-3Yn2slBEMTtklgYt4C_Mv3ROMwA";

        QSignalSpy importFinishedSpy(m_coreController->m_importCoreController, &ImportController::importFinished);
        auto importResult = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult.config);
        QVERIFY2(importFinishedSpy.count() == 1, "Import should succeed");

        QSignalSpy serverEditedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverEdited);

        amnezia::test::setServerDescription(m_coreController->m_serversRepository,
                                            m_coreController->m_serversController->getServerId(0),
                                            QStringLiteral("Edited AWG Server"));

        QVERIFY2(serverEditedSpy.count() == 1, "serverEdited signal should be emitted");
        QVERIFY2(serverEditedSpy.at(0).at(0).toString() == m_coreController->m_serversRepository->serverIdAt(0),
                 "serverEdited should emit edited server id");

        const QString editedDesc = amnezia::test::serverDescription(m_coreController->m_serversRepository,
                                                                     m_coreController->m_serversRepository->serverIdAt(0));
        QVERIFY2(editedDesc == "Edited AWG Server", "Server description should be updated");

        if (m_coreController->m_serversModel) {
            QString modelDesc = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(0, 0), ServersModel::NameRole).toString();
            QVERIFY2(modelDesc == "Edited AWG Server", "Server description in model should be updated");
        }
    }

    void testServerEditPreservesDefault() {
        QString awgKey = "vpn://AAABFHjadZBBT4QwEIX_ipkzS2wBJdyMB1cPXvbgwRgyQnclgZa0RTYS_rszXRa52Mt77TfzOu0EldEeG62sg-J9AhxPUEywF1CAuF3WTl4dRLCXhJIVpVuUEMpWdLdFKaH7FeUb9Mx3scpFk0XTRbOLvlSkKZsOz-Gi4BsdRiV_EGEydhwlg0tWynEZmd5Yz1bkoaK3xpvKtOU3_UFjOE3SsRs-tfIl1rVVzoWQOI9FzC3eonYcU4ZmgkPdwxz9fSYdYafVT4M7-lEJ80cEtTri0PrH_2q4wlW26f1lioe3p5uDsjQWoS_j_Ct2ipvGU6zO2PWtiivT8RPQudHYmqBXzl-3Yn2slBEMTtklgYt4C_Mv3ROMwA";
        QString xrayKey = "vpn://AAAAtXjadY7NCsJADIRfRXKui1YP0qt3L14EkRK7EQt2d0lS_0rf3awonjyFmW-YyQBNDIptIBao9sNPQgXYBXq2OL0zPqCA96kGSJHV6HK5MFP6YyCt0XsmsQqYz9zKzd3MmDIGyek6cdRoUJsE43gowNMJ-4uu_695kobbpG0MBndmTrbEV4sWcI6iG-zIQE47umOXLuSa2BlNKHKL7PMeiX5lmdH79bIsoBfiT0UOZQnjCw_AXRQ";

        auto importResult1 = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult1.config);
        auto importResult2 = m_coreController->m_importCoreController->extractConfigFromData(xrayKey);
        m_coreController->m_importCoreController->importConfig(importResult2.config);

        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 1, "Default server should be index 1");

        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);

        amnezia::test::setServerDescription(m_coreController->m_serversRepository,
                                            m_coreController->m_serversController->getServerId(1),
                                            QStringLiteral("Edited Default Server"));

        QVERIFY2(defaultServerChangedSpy.count() == 0, "defaultServerChanged should NOT be emitted when editing default server");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 1, "Default server index should remain 1");

        amnezia::test::setServerDescription(m_coreController->m_serversRepository,
                                            m_coreController->m_serversController->getServerId(0),
                                            QStringLiteral("Edited Non-Default Server"));

        QVERIFY2(defaultServerChangedSpy.count() == 0, "defaultServerChanged should NOT be emitted when editing non-default server");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 1, "Default server index should remain 1");
    }
};

QTEST_MAIN(TestServerEdit)
#include "testServerEdit.moc"

