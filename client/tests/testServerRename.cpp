#include <QUuid>
#include <QSignalSpy>
#include <QTest>

#include "core/controllers/coreController.h"
#include "core/models/serverDescription.h"
#include "ui/models/serversModel.h"
#include "utils/testUtils.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

using namespace amnezia;
using namespace amnezia::test;

class TestServerRename : public QObject
{
    Q_OBJECT

private:
    CoreController *m_coreController;
    SecureQSettings *m_settings;

    void importSingleServer()
    {
        const QString awgKey = getValueFromIni("configs/thirdPartyAwgVpnKey");
        const auto importResult = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        QVERIFY2(importResult.errorCode == ErrorCode::NoError, "Import setup should succeed");
        m_coreController->m_importCoreController->importConfig(importResult.config);
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 1, "Should have one server");
    }

private slots:
    void initTestCase()
    {
        QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);

        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);
        m_coreController = new CoreController(vpnConnection, m_settings, nullptr, this);
    }

    void cleanupTestCase()
    {
        m_settings->clearSettings();
        delete m_coreController;
        delete m_settings;
    }

    void init()
    {
        m_settings->clearSettings();
        m_coreController->m_serversRepository->invalidateCache();
        if (m_coreController->m_serversModel) {
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), -1);
        }
    }

    void testRenameServerUpdatesRepositoryAndModel()
    {
        importSingleServer();

        const QString serverId = m_coreController->m_serversController->getServerId(0);
        QSignalSpy serverEditedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverEdited);

        QVERIFY(m_coreController->m_serversController->renameServer(serverId, QStringLiteral("Renamed Server")));

        QVERIFY2(serverEditedSpy.count() == 1, "serverEdited should be emitted");
        const auto renamedDescription = serverDescription(m_coreController->m_serversRepository, serverId);
        QVERIFY2(renamedDescription.has_value(), "Server config should exist");
        QVERIFY2(*renamedDescription == "Renamed Server", "Repository should store renamed description");

        if (m_coreController->m_serversModel) {
            const QString modelName = m_coreController->m_serversModel
                                          ->data(m_coreController->m_serversModel->index(0, 0), ServersModel::NameRole)
                                          .toString();
            QVERIFY2(modelName == "Renamed Server", "Model should reflect renamed server");
        }

        QVERIFY(m_coreController->m_serversController->renameServer(serverId, QStringLiteral("Renamed Again")));

        QVERIFY2(serverEditedSpy.count() == 2, "serverEdited should be emitted again");
        const auto renamedAgainDescription = serverDescription(m_coreController->m_serversRepository, serverId);
        QVERIFY2(renamedAgainDescription.has_value(), "Server config should exist");
        QVERIFY2(*renamedAgainDescription == "Renamed Again", "Repository should store second rename");

        if (m_coreController->m_serversModel) {
            const QString modelName = m_coreController->m_serversModel
                                          ->data(m_coreController->m_serversModel->index(0, 0), ServersModel::NameRole)
                                          .toString();
            QVERIFY2(modelName == "Renamed Again", "Model should reflect second rename");
        }
    }
};

QTEST_MAIN(TestServerRename)
#include "testServerRename.moc"
