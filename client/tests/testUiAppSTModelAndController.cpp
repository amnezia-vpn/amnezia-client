#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "core/controllers/coreController.h"
#include "core/models/serverConfig.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

class TestUiAppSTModelAndController : public QObject
{
    Q_OBJECT

private:
    CoreController *m_coreController;
    SecureQSettings *m_settings;

    QString getSHAdminConfig()
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        return env.value("TEST_SELF_HOSTED_CONFIG");
    }

    QString getAppPath()
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        return env.value("TEST_APP_PATH");
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
        m_coreController->m_serversController->removeServer(serverIndex);

        qDebug() << "\nSERVER REMOVED\n";

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

    void testRolesAndSignals()
    {
        QSignalSpy finishedSpy(m_coreController->m_appSplitTunnelingUiController, &AppSplitTunnelingUiController::finished);
        QSignalSpy errorOccurredSpy(m_coreController->m_appSplitTunnelingUiController, &AppSplitTunnelingUiController::errorOccurred);
        QSignalSpy isSplitTunnelingChangedSpy(m_coreController->m_appSplitTunnelingUiController, &AppSplitTunnelingUiController::isTunnelingEnabledChanged);

        m_coreController->m_appSplitTunnelingUiController->toggleSplitTunneling(true);
        QVERIFY(isSplitTunnelingChangedSpy.count() == 1, "isSplitTunnelingChangedSpy signal should be emitted");
        QVERIFY(m_coreController->m_appSplitTunnelingUiController->isTunnelingEnabled() == true, "AppSplitTunneling should be enabled");

        m_coreController->m_appSplitTunnelingUiController->toggleSplitTunneling(false);
        QVERIFY(isSplitTunnelingChangedSpy.count() == 2, "isSplitTunnelingChangedSpy signal should be emitted 2nd time");
        QVERIFY(m_coreController->m_appSplitTunnelingUiController->isTunnelingEnabled() == false, "AppSplitTunneling should be disabled");

        QString app = getAppPath();

        m_coreController->m_appSplitTunnelingUiController->addApp(app);
        m_coreController->m_appSplitTunnelingUiController->updateModel();
        QVERIFY(finishedSpy.count() == 1, "finished signal should be emitted");
        QVERIFY(m_coreController->m_appSplitTunnelingModel->rowCount() == 1, "AppSplitTunnelingModel should have 1 row");

        QModelIndex appSTModelIndex = m_coreController->m_appSplitTunnelingModel->index(0, 0);
        QVERIFY2(appSTModelIndex.isValid(), "Site model index should be valid");

        auto appPath = m_coreController->m_appSplitTunnelingModel->data(appSTModelIndex, AppSplitTunnelingModel::AppPathRole);
        QVERIFY(app.contains(appPath.toString()) == true, QString("app path should be %1, got %2").arg(app, appPath));

        auto pkgAppName = m_coreController->m_appSplitTunnelingModel->data(appSTModelIndex, AppSplitTunnelingModel::PackageAppNameRole);
        QVERIFY(pkgAppName == true, "app name should be set");

        auto pkgAppIcon = m_coreController->m_appSplitTunnelingModel->data(appSTModelIndex, AppSplitTunnelingModel::PackageAppIconRole);
        QVERIFY(pkgAppIcon == true, "app image should be set");

        m_coreController->m_appSplitTunnelingUiController->addApp(app);
        m_coreController->m_appSplitTunnelingUiController->updateModel();
        QVERIFY(errorOccurredSpy.count() == 1, "errorOccurred signal should be emitted");
        QVERIFY(m_coreController->m_appSplitTunnelingModel->rowCount() == 1, "AppSplitTunnelingModel should have 3 rows (same app should not be added)");

        m_coreController->m_appSplitTunnelingUiController->removeApp(0);
        m_coreController->m_appSplitTunnelingUiController->updateModel();
        QVERIFY(finishedSpy.count() == 2, "finished signal should be emitted");
        QVERIFY(m_coreController->m_appSplitTunnelingModel->rowCount() == 0, "AppSplitTunnelingModel should have 0 rows");
    }
};

QTEST_MAIN(TestUiAppSTModelAndController)
#include "testUiAppSTModelAndController.moc"
