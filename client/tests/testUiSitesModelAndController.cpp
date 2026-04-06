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

class TestUiSitesModelAndController : public QObject
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

        QString vpnKey = getSHAdminConfig();
        QJsonObject importedConfig = m_coreController->m_importCoreController->extractConfigFromData(vpnKey).config;

        m_coreController->m_importCoreController->importConfig(importedConfig);

        qDebug() << "SELF-HOSTED ADMIN SERVER IMPORTED\n";
    }

    void cleanupTestCase()
    {
        int serverIndex = m_coreController->m_serversRepository->defaultServerIndex();
        m_coreController->m_serversController->removeServer(serverIndex);

        qDebug() << "SERVER REMOVED\n";

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

    void testRoles()
    {
        QSignalSpy finishedSpy(m_coreController->m_sitesUiController, &SitesUiController::finished);

        m_coreController->m_sitesUiController->addSite("2ip.io");
        m_coreController->m_sitesUiController->addSite("whatismyipaddress.com");
        m_coreController->m_sitesUiController->updateModel();
        QVERIFY(finishedSpy.count() == 2, "finished signal should be emitted 2 times");
        QVERIFY(m_coreController->m_sitesModel->rowCount() == 2, "SitesModel should have 2 rows");

        QModelIndex siteModelIndex = m_coreController->m_sitesModel->index(0, 0);
        QVERIFY2(siteModelIndex.isValid(), "Site model index should be valid");

        auto siteUrl = m_coreController->m_sitesModel->data(siteModelIndex, SitesModel::UrlRole);
        QVERIFY(siteUrl == normalizeHostname("2ip.io"), QString("site url should be %1, got %2").arg(normalizeHostname("2ip.io"), siteUrl));

        // auto siteIp = m_coreController->m_sitesModel->data(siteModelIndex, SitesModel::IpRole);
        // QVERIFY(siteIp != "", "site ip should not be empty");

        m_coreController->m_sitesUiController->removeSite(0);
        m_coreController->m_sitesUiController->updateModel();
        QVERIFY(finishedSpy.count() == 3, "finished signal should be emitted");
        QVERIFY(m_coreController->m_sitesModel->rowCount() == 1, "SitesModel should have 1 row");

        m_coreController->m_sitesUiController->removeSites();
        m_coreController->m_sitesUiController->updateModel();
        QVERIFY(finishedSpy.count() == 4, "finished signal should be emitted");
        QVERIFY(m_coreController->m_sitesModel->rowCount() == 0, "SitesModel should have 0 rows");
    }
};

QTEST_MAIN(TestUiSitesModelAndController)
#include "testUiSitesModelAndController.moc"
