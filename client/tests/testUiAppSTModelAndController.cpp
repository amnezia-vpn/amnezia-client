#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QUuid>
#include <QTest>

#include "core/controllers/coreController.h"
#include "core/models/serverDescription.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

class TestUiAppSTModelAndController : public QObject
{
    Q_OBJECT

private:
    CoreController *m_coreController;
    SecureQSettings *m_settings;

    QString getValueFromIni(const QString &key)
    {
        QSettings settings("test_vars.ini", QSettings::IniFormat);
        return settings.value(key).toString();
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
        if (m_coreController->m_serversModel) {
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), -1);
        }
    }

    void testRolesAndSignals()
    {
        QSignalSpy finishedSpy(m_coreController->m_appSplitTunnelingUiController, &AppSplitTunnelingUiController::finished);
        QSignalSpy errorOccurredSpy(m_coreController->m_appSplitTunnelingUiController, &AppSplitTunnelingUiController::errorOccurred);
        QSignalSpy isSplitTunnelingChangedSpy(m_coreController->m_appSplitTunnelingUiController, &AppSplitTunnelingUiController::isSplitTunnelingEnabledChanged);

        m_coreController->m_appSplitTunnelingUiController->toggleSplitTunneling(true);
        QVERIFY2(isSplitTunnelingChangedSpy.count() == 1, "isSplitTunnelingChangedSpy signal should be emitted");
        QVERIFY2(m_coreController->m_appSplitTunnelingUiController->isSplitTunnelingEnabled() == true, "AppSplitTunneling should be enabled");

        m_coreController->m_appSplitTunnelingUiController->toggleSplitTunneling(false);
        QVERIFY2(isSplitTunnelingChangedSpy.count() == 2, "isSplitTunnelingChangedSpy signal should be emitted 2nd time");
        QVERIFY2(m_coreController->m_appSplitTunnelingUiController->isSplitTunnelingEnabled() == false, "AppSplitTunneling should be disabled");

        QString app = getValueFromIni("paths/TEST_APP_PATH");

        m_coreController->m_appSplitTunnelingUiController->addApp(app);
        m_coreController->m_appSplitTunnelingUiController->updateModel();
        QVERIFY2(finishedSpy.count() == 1, "finished signal should be emitted");
        QVERIFY2(m_coreController->m_appSplitTunnelingModel->rowCount() == 1, "AppSplitTunnelingModel should have 1 row");

        QModelIndex appSTModelIndex = m_coreController->m_appSplitTunnelingModel->index(0, 0);
        QVERIFY2(appSTModelIndex.isValid(), "Site model index should be valid");

        auto appPath = m_coreController->m_appSplitTunnelingModel->data(appSTModelIndex, AppSplitTunnelingModel::AppPathRole);
        QString msg = QString("app path should be %1, got %2").arg(app, appPath.toString());
        QVERIFY2(app.contains(appPath.toString()) == true, msg.toLocal8Bit().constData());

        auto pkgAppName = m_coreController->m_appSplitTunnelingModel->data(appSTModelIndex, AppSplitTunnelingModel::PackageAppNameRole);
        QVERIFY2(pkgAppName == true, "app name should be set");

        auto pkgAppIcon = m_coreController->m_appSplitTunnelingModel->data(appSTModelIndex, AppSplitTunnelingModel::PackageAppIconRole);
        QVERIFY2(pkgAppIcon == true, "app image should be set");

        m_coreController->m_appSplitTunnelingUiController->addApp(app);
        m_coreController->m_appSplitTunnelingUiController->updateModel();
        QVERIFY2(errorOccurredSpy.count() == 1, "errorOccurred signal should be emitted");
        QVERIFY2(m_coreController->m_appSplitTunnelingModel->rowCount() == 1, "AppSplitTunnelingModel should have 3 rows (same app should not be added)");

        m_coreController->m_appSplitTunnelingUiController->removeApp(0);
        m_coreController->m_appSplitTunnelingUiController->updateModel();
        QVERIFY2(finishedSpy.count() == 2, "finished signal should be emitted");
        QVERIFY2(m_coreController->m_appSplitTunnelingModel->rowCount() == 0, "AppSplitTunnelingModel should have 0 rows");
    }
};

QTEST_MAIN(TestUiAppSTModelAndController)
#include "testUiAppSTModelAndController.moc"
