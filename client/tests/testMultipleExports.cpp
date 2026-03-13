#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>
#include <QProcessEnvironment>

#include "core/controllers/coreController.h"
#include "core/models/serverConfig.h"
#include "ui/models/containerProps.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

class TestMultipleExports : public QObject
{
    Q_OBJECT

private:
    CoreController *m_coreController;
    SecureQSettings *m_settings;

    QString getSHAdminConfig()
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        return env.value("TEST_SELF_HOSTED_CONFIG_EMPTY");
    }

    void ExportWithContainer(DockerContainer container)
    {

        ContainerProps props;

        int serverIndex = m_coreController->m_serversRepository->defaultServerIndex();
        auto port = m_coreController->m_installUiController->defaultPort(props.defaultProtocol(container));
        auto transportProto = m_coreController->m_installUiController->defaultTransportProto(props.defaultProtocol(container));

        m_coreController->m_installUiController->install(
                container, port,
                static_cast<TransportProto>(transportProto),
                serverIndex);

        qDebug() << "CONTAINER INSTALLED\n";

        qDebug() << m_coreController->m_serversUiController->getProcessedContainerIndex();

        QString clientName = "MultipleExports Test Client";

        auto exportResult = [&]() {
            switch (container) {
            case DockerContainer::Awg:
            case DockerContainer::Awg2:
                return m_coreController->m_exportController->generateAwgConfig(serverIndex, clientName);
            case DockerContainer::WireGuard:
                return m_coreController->m_exportController->generateWireGuardConfig(serverIndex, clientName);
            case DockerContainer::OpenVpn:
                return m_coreController->m_exportController->generateOpenVpnConfig(serverIndex, clientName);
            case DockerContainer::Xray:
                return m_coreController->m_exportController->generateXrayConfig(serverIndex, clientName);
            }
        }();

        QVERIFY2(exportResult.errorCode == ErrorCode::NoError, "\nExport should succeed");
        QVERIFY2(!exportResult.config.isEmpty(), "Exported config should not be empty\n");

        /*
        QString fileName = "";
        QString configFileName = "amnezia_config";
        QString configExtension = ".vpn";

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
            fileName = configFileName + configExtension;
#else
        fileName = m_coreController->m_systemController->getFileName(
                "Save AmneziaVPN config", "Config files (*" + configExtension + ")",
                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + configFileName, true,
                configExtension);
#endif
        if (fileName != "")
            m_coreController->m_exportUiController->exportConfig(fileName);
        */
        
        qDebug() << "\nEXPORTED CONFIG:\n" << exportResult.config << "\n";

        QString exportedConfig = exportResult.config;

        auto reimportResult = m_coreController->m_importCoreController->extractConfigFromData(exportedConfig);
        QVERIFY2(reimportResult.errorCode == ErrorCode::NoError, "Re-import should succeed");

        QString reimportedJson = QJsonDocument(reimportResult.config).toJson(QJsonDocument::Compact);

        qDebug() << "\nEXPORTED JSON:\n" << exportedConfig << "\n";
        qDebug() << "REIMPORTED JSON:\n" << reimportedJson << "\n";

        QCOMPARE(reimportedJson, exportedConfig);

        // TODO: remove only client for test
        // m_coreController->m_exportController->revokeConfig(clientIndex, serverIndex, processedContainerIndex);

        m_coreController->m_installController->clearCachedProfile(serverIndex, container);
    }

private slots:
    void initTestCase()
    {
        QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);

        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);

        m_coreController = new CoreController(vpnConnection, m_settings, nullptr, this);

        QString vpnKey = getSHAdminConfig();
        QJsonObject importedConfig = m_coreController->m_importCoreController->extractConfigFromData(vpnKey).config;

        m_coreController->m_importCoreController->importConfig(importedConfig);

        qDebug() << "SELF-HOSTED ADMIN SERVER IMPORTED\n";
    }

    void cleanupTestCase()
    {
        int serverIndex = m_coreController->m_serversRepository->defaultServerIndex();
        m_coreController->m_installController->removeAllContainers(serverIndex);
        m_coreController->m_serversController->removeServer(serverIndex);

        qDebug() << "SERVER CLEARED AND REMOVED\n";

        m_settings->clearSettings();
        delete m_coreController;
        delete m_settings;
    }

    void init()
    {
        m_settings->clearSettings();
        if (m_coreController->m_serversModel) {
            m_coreController->m_serversModel->updateModel(QVector<ServerConfig>(), -1, false);
        }
    }

    void testExportAwgConfig()
    {
        ExportWithContainer(DockerContainer::Awg2);
    }

    void testExportWireGuardConfig()
    {
        ExportWithContainer(DockerContainer::WireGuard);
    }

    void testExportOpenVpnConfig()
    {
        ExportWithContainer(DockerContainer::OpenVpn);
    }

    void testExportXrayConfig()
    {
        ExportWithContainer(DockerContainer::Xray);
    }

};

QTEST_MAIN(TestMultipleExports)
#include "testMultipleExports.moc"
