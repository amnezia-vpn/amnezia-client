#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QSignalSpy>
#include <QTest>

#include "core/controllers/coreController.h"
#include "core/models/serverConfig.h"
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

    void testServerEditTriggersHandlers() {
        QString awgKey = getValueFromIni("configs/TEST_CONFIG_AWG");

        QSignalSpy importFinishedSpy(m_coreController->m_importCoreController, &ImportController::importFinished);
        auto importResult = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult.config);
        QVERIFY2(importFinishedSpy.count() == 1, "Import should succeed");

        QSignalSpy serverEditedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverEdited);
        QSignalSpy gatewayStacksExpandedSpy(m_coreController->m_serversController, &ServersController::gatewayStacksExpanded);

        ServerConfig serverConfig = m_coreController->m_serversController->getServerConfig(0);
        serverConfig.visit([](auto& arg) {
            arg.description = "Edited AWG Server";
        });

        m_coreController->m_serversController->editServer(0, serverConfig);

        QVERIFY2(serverEditedSpy.count() == 1, "serverEdited signal should be emitted");
        QVERIFY2(serverEditedSpy.at(0).at(0).toInt() == 0, "serverEdited should emit index 0");

        ServerConfig editedServer = m_coreController->m_serversRepository->server(0);
        QString editedDesc = editedServer.description();
        QVERIFY2(editedDesc == "Edited AWG Server", "Server description should be updated");

        if (m_coreController->m_serversModel) {
            QString modelDesc = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(0, 0), ServersModel::NameRole).toString();
            QVERIFY2(modelDesc == "Edited AWG Server", "Server description in model should be updated");
        }
    }

    void testServerEditPreservesDefault() {
        QString awgKey = getValueFromIni("configs/TEST_CONFIG_AWG");
        QString xrayKey = getValueFromIni("configs/TEST_CONFIG_XRAY");

        auto importResult1 = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult1.config);
        auto importResult2 = m_coreController->m_importCoreController->extractConfigFromData(xrayKey);
        m_coreController->m_importCoreController->importConfig(importResult2.config);

        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 1, "Default server should be index 1");

        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);

        ServerConfig defaultServerConfig = m_coreController->m_serversController->getServerConfig(1);
        defaultServerConfig.visit([](auto& arg) {
            arg.description = "Edited Default Server";
        });
        m_coreController->m_serversController->editServer(1, defaultServerConfig);

        QVERIFY2(defaultServerChangedSpy.count() == 0, "defaultServerChanged should NOT be emitted when editing default server");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 1, "Default server index should remain 1");

        ServerConfig nonDefaultServerConfig = m_coreController->m_serversController->getServerConfig(0);
        nonDefaultServerConfig.visit([](auto& arg) {
            arg.description = "Edited Non-Default Server";
        });
        m_coreController->m_serversController->editServer(0, nonDefaultServerConfig);

        QVERIFY2(defaultServerChangedSpy.count() == 0, "defaultServerChanged should NOT be emitted when editing non-default server");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 1, "Default server index should remain 1");
    }
};

QTEST_MAIN(TestServerEdit)
#include "testServerEdit.moc"

