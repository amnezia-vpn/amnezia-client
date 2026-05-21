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

class TestUiIpModelAndController : public QObject
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
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), -1);
        }
    }

    void testRolesAndSignals()
    {
        QSignalSpy finishedSpy(m_coreController->m_ipSplitTunnelingUiController, &IpSplitTunnelingUiController::finished);
        QSignalSpy errorOccurredSpy(m_coreController->m_ipSplitTunnelingUiController, &IpSplitTunnelingUiController::errorOccurred);
        QSignalSpy isSplitTunnelingChangedSpy(m_coreController->m_ipSplitTunnelingUiController, &IpSplitTunnelingUiController::isSplitTunnelingEnabledChanged);

        m_coreController->m_ipSplitTunnelingUiController->toggleSplitTunneling(true);
        QVERIFY2(isSplitTunnelingChangedSpy.count() == 1, "isSplitTunnelingChangedSpy signal should be emitted");
        QVERIFY2(m_coreController->m_ipSplitTunnelingUiController->isSplitTunnelingEnabled() == true, "ipSplitTunneling should be enabled");

        m_coreController->m_ipSplitTunnelingUiController->toggleSplitTunneling(false);
        QVERIFY2(isSplitTunnelingChangedSpy.count() == 2, "isSplitTunnelingChangedSpy signal should be emitted 2nd time");
        QVERIFY2(m_coreController->m_ipSplitTunnelingUiController->isSplitTunnelingEnabled() == false, "ipSplitTunneling should be disabled");

        QString site = "2ip.io";

        m_coreController->m_ipSplitTunnelingUiController->addSite(site);
        m_coreController->m_ipSplitTunnelingUiController->addSite("whatismyipaddress.com");
        m_coreController->m_ipSplitTunnelingUiController->updateModel();
        QVERIFY2(finishedSpy.count() == 2, "finished signal should be emitted 2 times");
        QVERIFY2(m_coreController->m_ipSplitTunnelingModel->rowCount() == 2, "IpSplitTunnelingModel should have 2 rows");

        QModelIndex siteModelIndex = m_coreController->m_ipSplitTunnelingModel->index(0, 0);
        QVERIFY2(siteModelIndex.isValid(), "Ip model index should be valid");

        auto siteUrl = m_coreController->m_ipSplitTunnelingModel->data(siteModelIndex, IpSplitTunnelingModel::UrlRole);
        QCOMPARE(siteUrl, normalizeHostname(site));

        auto siteIp = m_coreController->m_ipSplitTunnelingModel->data(siteModelIndex, IpSplitTunnelingModel::IpRole);
        QVERIFY2(siteIp.isNull() == false, "Ip should not be empty");

        m_coreController->m_ipSplitTunnelingUiController->removeSite(0);
        m_coreController->m_ipSplitTunnelingUiController->updateModel();
        QVERIFY2(finishedSpy.count() == 3, "finished signal should be emitted");
        QVERIFY2(m_coreController->m_ipSplitTunnelingModel->rowCount() == 1, "IpSplitTunnelingModel should have 1 row");

        m_coreController->m_ipSplitTunnelingUiController->importSites(getValueFromIni("paths/TEST_SITES_LIST_PATH"), true);
        m_coreController->m_ipSplitTunnelingUiController->updateModel();
        QVERIFY2(errorOccurredSpy.count() == 0, "errorOccurred signal should not be emitted");
        QVERIFY2(finishedSpy.count() == 4, "finished signal should be emitted");
        QVERIFY2(m_coreController->m_ipSplitTunnelingModel->rowCount() > 1, "IpSplitTunnelingModel should have more than 1 row");

        m_coreController->m_ipSplitTunnelingUiController->exportSites(getValueFromIni("paths/TEST_EXPORT_PATH") + "test_ips_export.json");
        QVERIFY2(finishedSpy.count() == 5, "finished signal should be emitted");

        m_coreController->m_ipSplitTunnelingUiController->removeSites();
        m_coreController->m_ipSplitTunnelingUiController->updateModel();
        QVERIFY2(finishedSpy.count() == 6, "finished signal should be emitted");
        QVERIFY2(m_coreController->m_ipSplitTunnelingModel->rowCount() == 0, "IpSplitTunnelingModel should have 0 rows");
    }
};

QTEST_MAIN(TestUiIpModelAndController)
#include "testUiIpModelAndController.moc"
