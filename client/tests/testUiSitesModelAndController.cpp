#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QUuid>

#ifdef Q_OS_WIN
    #include <QTest>
#else
    #include <QtTest/qtest.h>
#endif

#include "core/controllers/coreController.h"
#include "core/models/serverConfig.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

class TestUiSitesModelAndController : public QObject
{
    Q_OBJECT

private:
    CoreController *m_coreController;
    SecureQSettings *m_settings;

    QString getPath()
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        return env.value("TEST_PATH");
    }

    QString getExportPath()
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        return env.value("TEST_EXPORT_PATH");
    }

    QString normalizeHostname(const QString &hostname) const
    {
        QString normalized = hostname;
        normalized.replace("https://", "");
        normalized.replace("http://", "");
        normalized.replace("ftp://", "");
        normalized = normalized.split("/", Qt::SkipEmptyParts).first();
        return normalized;
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
            m_coreController->m_serversModel->updateModel(QVector<ServerConfig>(), -1, false);
        }
    }

    void testRolesAndSignals()
    {
        QSignalSpy finishedSpy(m_coreController->m_sitesUiController, &SitesUiController::finished);
        QSignalSpy errorOccurredSpy(m_coreController->m_sitesUiController, &SitesUiController::errorOccurred);
        QSignalSpy isSplitTunnelingChangedSpy(m_coreController->m_sitesUiController, &SitesUiController::isTunnelingEnabledChanged);

        m_coreController->m_sitesUiController->toggleSplitTunneling(true);
        QVERIFY2(isSplitTunnelingChangedSpy.count() == 1, "isSplitTunnelingChangedSpy signal should be emitted");
        QVERIFY2(m_coreController->m_sitesUiController->isTunnelingEnabled() == true, "SiteSplitTunneling should be enabled");

        m_coreController->m_sitesUiController->toggleSplitTunneling(false);
        QVERIFY2(isSplitTunnelingChangedSpy.count() == 2, "isSplitTunnelingChangedSpy signal should be emitted 2nd time");
        QVERIFY2(m_coreController->m_sitesUiController->isTunnelingEnabled() == false, "SiteSplitTunneling should be disabled");

        QString site = "2ip.io";

        m_coreController->m_sitesUiController->addSite(site);
        m_coreController->m_sitesUiController->addSite("whatismyipaddress.com");
        m_coreController->m_sitesUiController->updateModel();
        QVERIFY2(finishedSpy.count() == 2, "finished signal should be emitted 2 times");
        QVERIFY2(m_coreController->m_sitesModel->rowCount() == 2, "SitesModel should have 2 rows");

        QModelIndex siteModelIndex = m_coreController->m_sitesModel->index(0, 0);
        QVERIFY2(siteModelIndex.isValid(), "Site model index should be valid");

        auto siteUrl = m_coreController->m_sitesModel->data(siteModelIndex, SitesModel::UrlRole);
        QCOMPARE(siteUrl, normalizeHostname(site));

        auto siteIp = m_coreController->m_sitesModel->data(siteModelIndex, SitesModel::IpRole);
        QVERIFY2(siteIp.isNull() == false, "site ip should not be empty");

        m_coreController->m_sitesUiController->removeSite(0);
        m_coreController->m_sitesUiController->updateModel();
        QVERIFY2(finishedSpy.count() == 3, "finished signal should be emitted");
        QVERIFY2(m_coreController->m_sitesModel->rowCount() == 1, "SitesModel should have 1 row");

        m_coreController->m_sitesUiController->importSites(getPath(), true);
        m_coreController->m_sitesUiController->updateModel();
        QVERIFY2(errorOccurredSpy.count() == 0, "errorOccurred signal should not be emitted");
        QVERIFY2(finishedSpy.count() == 4, "finished signal should be emitted");
        QVERIFY2(m_coreController->m_sitesModel->rowCount() > 1, "SitesModel should have more than 1 row");

        m_coreController->m_sitesUiController->exportSites(getExportPath() + "test_sites_export.json");
        QVERIFY2(finishedSpy.count() == 5, "finished signal should be emitted");

        m_coreController->m_sitesUiController->removeSites();
        m_coreController->m_sitesUiController->updateModel();
        QVERIFY2(finishedSpy.count() == 6, "finished signal should be emitted");
        QVERIFY2(m_coreController->m_sitesModel->rowCount() == 0, "SitesModel should have 0 rows");
    }
};

QTEST_MAIN(TestUiSitesModelAndController)
#include "testUiSitesModelAndController.moc"
