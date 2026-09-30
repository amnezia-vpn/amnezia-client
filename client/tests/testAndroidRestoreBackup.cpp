#include <QTest>
#include <QDebug>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

#include "utils/testCoreController.h"
#include "utils/testUtils.h"
#include "core/controllers/settingsController.h"
#include "core/repositories/secureServersRepository.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

using namespace amnezia;
using namespace amnezia::test;

class TestAndroidRestoreBackup : public QObject
{
    Q_OBJECT

private:
    TestCoreController *m_coreController;
    SecureQSettings    *m_settings;

    void importServer(const QString &key)
    {
        auto result = m_coreController->m_importCoreController->extractConfigFromData(key);
        QVERIFY2(result.errorCode == ErrorCode::NoError, "Server import must succeed");
        m_coreController->m_importCoreController->importConfig(result.config);
    }

private slots:
    void initTestCase()
    {
        const QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);
        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);
        m_coreController = new TestCoreController(vpnConnection, m_settings, nullptr, this);
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
    }

    // -----------------------------------------------------------------------
    // Android safety: killSwitch and autoConnect must be DISABLED after restore
    // even if the backup was created with them enabled.
    // (SettingsController::restoreAppConfigFromData unconditionally resets
    //  these flags on Android / iOS – see Q_OS_ANDROID block in the impl.)
    // -----------------------------------------------------------------------
    void testRestoreDisablesDangerousSettingsOnAndroid()
    {
        const QString key = getEnvValue("THIRD_PARTY_AWG_VPN_KEY");
        if (!isEnvValueConfigured(key)) {
            QSKIP("THIRD_PARTY_AWG_VPN_KEY not set – skipping");
        }

        importServer(key);

        // Enable dangerous settings before creating the backup
        m_coreController->m_appSettingsRepository->setKillSwitchEnabled(true);
        m_coreController->m_appSettingsRepository->setAutoConnect(true);

        QVERIFY(m_coreController->m_appSettingsRepository->isKillSwitchEnabled());
        QVERIFY(m_coreController->m_appSettingsRepository->isAutoConnect());

        const QByteArray backup = m_coreController->m_settingsController->backupAppConfig();
        QVERIFY(!backup.isEmpty());

        const QJsonObject backupJson = QJsonDocument::fromJson(backup).object();
        qInfo() << "Backup killSwitchEnabled :" << backupJson.value("Conf/killSwitchEnabled").toBool();
        qInfo() << "Backup autoConnect       :" << backupJson.value("Conf/autoConnect").toBool();

        m_settings->clearSettings();
        m_coreController->m_serversRepository->invalidateCache();

        const ErrorCode err = m_coreController->m_settingsController->restoreAppConfigFromData(backup);
        QVERIFY2(err == ErrorCode::NoError, "Restore must succeed");

        const bool killSwitch = m_coreController->m_appSettingsRepository->isKillSwitchEnabled();
        const bool autoConnect = m_coreController->m_appSettingsRepository->isAutoConnect();

        qInfo() << "killSwitchEnabled after restore:" << killSwitch;
        qInfo() << "autoConnect after restore      :" << autoConnect;

        QVERIFY2(!killSwitch,
                 "killSwitch must be DISABLED after restore on Android (safety reset)");
        QVERIFY2(!autoConnect,
                 "autoConnect must be DISABLED after restore on Android (safety reset)");
    }

    // -----------------------------------------------------------------------
    // Cross-platform backup: restoring a backup from another platform must
    // succeed and must clear the app split-tunneling apps list on Android.
    // -----------------------------------------------------------------------
    void testRestoreFromOtherPlatformClearsAppSplitTunnelingList()
    {
        const QString key = getEnvValue("THIRD_PARTY_AWG_VPN_KEY");
        if (!isEnvValueConfigured(key)) {
            QSKIP("THIRD_PARTY_AWG_VPN_KEY not set – skipping");
        }

        importServer(key);

        // Build a synthetic backup that claims to come from Windows
        QByteArray baseBackup = m_coreController->m_settingsController->backupAppConfig();
        QJsonObject obj = QJsonDocument::fromJson(baseBackup).object();
        obj["AppPlatform"] = "Windows";
        const QByteArray crossPlatformBackup = QJsonDocument(obj).toJson();

        QSignalSpy clearAppsListSpy(m_coreController->m_settingsController,
                                    &SettingsController::appSplitTunnelingClearAppsList);

        m_settings->clearSettings();
        m_coreController->m_serversRepository->invalidateCache();

        const ErrorCode err =
            m_coreController->m_settingsController->restoreAppConfigFromData(crossPlatformBackup);
        QVERIFY2(err == ErrorCode::NoError, "Cross-platform restore must succeed");

        qInfo() << "appSplitTunnelingClearAppsList emitted:" << clearAppsListSpy.count() << "time(s)";
        QVERIFY2(clearAppsListSpy.count() == 1,
                 "appSplitTunnelingClearAppsList must be emitted when restoring from different platform");
    }

    // -----------------------------------------------------------------------
    // Invalid data must return an error, not crash
    // -----------------------------------------------------------------------
    void testRestoreInvalidDataReturnsError()
    {
        const QByteArray garbage = QByteArray("not a valid backup {{{{");
        const ErrorCode err = m_coreController->m_settingsController->restoreAppConfigFromData(garbage);

        qInfo() << "ErrorCode for invalid backup:" << static_cast<int>(err);
        QVERIFY2(err != ErrorCode::NoError,
                 "Restoring invalid data must return an error code");
    }
};

QTEST_MAIN(TestAndroidRestoreBackup)
#include "testAndroidRestoreBackup.moc"
