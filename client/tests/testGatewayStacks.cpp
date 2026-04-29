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

class TestGatewayStacks : public QObject
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
        if (m_coreController->m_serversModel) {
            m_coreController->m_serversModel->updateModel(QVector<ServerConfig>(), -1, false);
        }
    }

    void testGatewayStacksRecomputeOnServerOperations() {
        QString awgKey = getValueFromIni("configs/TEST_CONFIG_AWG");

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

