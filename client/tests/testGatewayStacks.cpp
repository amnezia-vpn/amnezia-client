#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QSignalSpy>

#include "core/controllers/coreController.h"
#include "core/models/serverConfig.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

using namespace amnezia;

class TestGatewayStacks : public QObject
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
            m_coreController->m_serversModel->updateModel(QVector<ServerConfig>(), -1, false);
        }
    }

    void testGatewayStacksRecomputeOnServerOperations() {
        QString awgKey = "vpn://AAABFHjadZBBT4QwEIX_ipkzS2wBJdyMB1cPXvbgwRgyQnclgZa0RTYS_rszXRa52Mt77TfzOu0EldEeG62sg-J9AhxPUEywF1CAuF3WTl4dRLCXhJIVpVuUEMpWdLdFKaH7FeUb9Mx3scpFk0XTRbOLvlSkKZsOz-Gi4BsdRiV_EGEydhwlg0tWynEZmd5Yz1bkoaK3xpvKtOU3_UFjOE3SsRs-tfIl1rVVzoWQOI9FzC3eonYcU4ZmgkPdwxz9fSYdYafVT4M7-lEJ80cEtTri0PrH_2q4wlW26f1lioe3p5uDsjQWoS_j_Ct2ipvGU6zO2PWtiivT8RPQudHYmqBXzl-3Yn2slBEMTtklgYt4C_Mv3ROMwA";

        QSignalSpy gatewayStacksExpandedSpy(m_coreController->m_serversController, &ServersController::gatewayStacksExpanded);
        QSignalSpy serverAddedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverAdded);
        QSignalSpy serverEditedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverEdited);
        QSignalSpy serverRemovedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverRemoved);

        auto importResult = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult.config);

        QVERIFY2(serverAddedSpy.count() == 1, "serverAdded signal should be emitted");
        QVERIFY2(m_coreController->m_serversController->gatewayStacks().isEmpty(), "Gateway stacks should be empty for self-hosted servers");

        ServerConfig serverConfig = m_coreController->m_serversController->getServerConfig(0);
        serverConfig.visit([](auto& arg) {
            arg.description = "Edited Server";
        });
        m_coreController->m_serversController->editServer(0, serverConfig);

        QVERIFY2(serverEditedSpy.count() == 1, "serverEdited signal should be emitted");

        m_coreController->m_serversController->removeServer(0);

        QVERIFY2(serverRemovedSpy.count() == 1, "serverRemoved signal should be emitted");
        QVERIFY2(m_coreController->m_serversController->gatewayStacks().isEmpty(), "Gateway stacks should remain empty");
    }
};

QTEST_MAIN(TestGatewayStacks)
#include "testGatewayStacks.moc"

