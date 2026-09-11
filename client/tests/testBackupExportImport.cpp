#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QUuid>
#include <QSignalSpy>
#include <QTest>

#include "utils/testCoreController.h"
#include "utils/testUtils.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

using namespace amnezia::test;

class TestBackupExportImport : public QObject
{
    Q_OBJECT

private:
    TestCoreController* m_coreController;
    SecureQSettings* m_settings;


private slots:
    void initTestCase() {
        QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);

        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);

        m_coreController = new TestCoreController(vpnConnection, m_settings, nullptr, this);
    }

    void cleanupTestCase() {
        m_settings->clearSettings();
        delete m_coreController;
        delete m_settings;
    }

    void init() {
        m_settings->clearSettings();
    }

    void testBackupExportImport() {
        QString backup = getEnvValue("BACKUP_PATH");

        logEnvValueState("BACKUP_PATH");
        qInfo() << "BACKUP_PATH file exists:" << QFile::exists(backup)
                << "size:" << QFileInfo(backup).size();

        if (!isEnvValueConfigured(backup)) {
            QSKIP("Set BACKUP_PATH");
        }

        QSignalSpy errorOccurredSpy(m_coreController->m_settingsUiController, &SettingsUiController::errorOccurred);

        m_coreController->m_settingsUiController->restoreAppConfig(backup);
        QVERIFY2(errorOccurredSpy.count() == 0, "errorOccurred signal should NOT be emitted (default is already 0)");

        QByteArray backupData = m_coreController->m_settingsController->backupAppConfig();

        QFile file(backup);
        QVERIFY2(file.open(QIODevice::ReadOnly), "Backup file should open for reading");
        const QByteArray fileData = file.readAll();

        QJsonObject importedObj = QJsonDocument::fromJson(fileData).object();
        QJsonObject exportedObj = QJsonDocument::fromJson(backupData).object();
        QVERIFY2(!importedObj.isEmpty(), "Backup file should contain valid JSON");
        QVERIFY2(!exportedObj.isEmpty(), "Exported backup should contain valid JSON");

        // backupAppConfig() injects live platform state on every export
        // (AppPlatform, autoStart, killSwitch...), and restoreAppConfigFromData()
        // normalizes split tunneling / startMinimized / ExceptApps to the current
        // platform, so these keys legitimately differ from a backup made on
        // another platform/machine.
        const QStringList volatileKeys = { "AppPlatform", "Conf/autoStart", "Conf/killSwitchEnabled",
                                           "Conf/strictKillSwitchEnabled", "Conf/useAmneziaDns",
                                           "Conf/appsSplitTunnelingEnabled", "Conf/sitesSplitTunnelingEnabled",
                                           "Conf/startMinimized", "Conf/ExceptApps" };
        for (const QString &key : volatileKeys) {
            importedObj.remove(key);
            exportedObj.remove(key);
        }

        if (exportedObj != importedObj) {
            for (auto it = importedObj.constBegin(); it != importedObj.constEnd(); ++it) {
                if (!exportedObj.contains(it.key())) {
                    qWarning() << "Key missing in exported backup:" << it.key();
                } else if (exportedObj.value(it.key()) != it.value()) {
                    qWarning() << "Key differs:" << it.key();
                }
            }
            for (auto it = exportedObj.constBegin(); it != exportedObj.constEnd(); ++it) {
                if (!importedObj.contains(it.key())) {
                    qWarning() << "Extra key in exported backup:" << it.key();
                }
            }
        }
        QVERIFY2(exportedObj == importedObj,
                 "Imported and exported backup config should be equal (ignoring platform-specific keys)");
    }
};

QTEST_MAIN(TestBackupExportImport)
#include "testBackupExportImport.moc"
