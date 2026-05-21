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

class TestUiAllowedDnsModelAndController : public QObject
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
        QSignalSpy finishedSpy(m_coreController->m_allowedDnsUiController, &AllowedDnsUiController::finished);
        QSignalSpy errorOccurredSpy(m_coreController->m_allowedDnsUiController, &AllowedDnsUiController::errorOccurred);

        QString ip = "188.40.167.81";

        m_coreController->m_allowedDnsUiController->addDns(ip);
        m_coreController->m_allowedDnsUiController->updateModel();
        QVERIFY2(errorOccurredSpy.count() == 0, "errorOccurred signal should not be emitted");
        QVERIFY2(finishedSpy.count() == 1, "finished signal should be emitted");
        QVERIFY2(m_coreController->m_allowedDnsModel->rowCount() == 1, "AllowedDnsModel should have 1 row");

        QModelIndex allowedDnsModelIndex = m_coreController->m_allowedDnsModel->index(0, 0);
        QVERIFY2(allowedDnsModelIndex.isValid(), "Site model index should be valid");

        auto dnsIp = m_coreController->m_allowedDnsModel->data(allowedDnsModelIndex, AllowedDnsModel::IpRole);
        QString msg = QString("dns ip should be %1, got %2").arg(ip, dnsIp.toString());
        QVERIFY2(dnsIp == ip, msg.toLocal8Bit().constData());

        m_coreController->m_allowedDnsUiController->importDns(getValueFromIni("paths/TEST_DNS_LIST_PATH"), true);
        m_coreController->m_allowedDnsUiController->updateModel();
        QVERIFY2(errorOccurredSpy.count() == 0, "errorOccurred signal should not be emitted");
        QVERIFY2(finishedSpy.count() == 2, "finished signal should be emitted");
        QVERIFY2(m_coreController->m_allowedDnsModel->rowCount() > 1, "AllowedDnsModel should have more than 1 row");

        m_coreController->m_allowedDnsUiController->exportDns(getValueFromIni("paths/TEST_EXPORT_PATH") + "test_dns_export.json");
        QVERIFY2(errorOccurredSpy.count() == 0, "errorOccurred signal should not be emitted");
        QVERIFY2(finishedSpy.count() == 3, "finished signal should be emitted");

        m_coreController->m_allowedDnsUiController->removeDns(0);
        m_coreController->m_allowedDnsUiController->updateModel();
        QVERIFY2(errorOccurredSpy.count() == 0, "errorOccurred signal should not be emitted");
        QVERIFY2(finishedSpy.count() == 4, "finished signal should be emitted");
        QVERIFY2(m_coreController->m_allowedDnsModel->rowCount() == 0, "AllowedDnsModel should have 0 rows");
    }
};

QTEST_MAIN(TestUiAllowedDnsModelAndController)
#include "testUiAllowedDnsModelAndController.moc"
