#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "utils/testCoreController.h"
#include "core/models/api/apiV2ServerConfig.h"
#include "core/models/selfhosted/nativeServerConfig.h"
#include "core/models/selfhosted/selfHostedAdminServerConfig.h"
#include "core/models/selfhosted/selfHostedUserServerConfig.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/serverConfigUtils.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

namespace
{
    QJsonObject adminConfigJson(const QString &hostName, int formatVersion = -1)
    {
        QJsonObject json;
        json[configKey::hostName] = hostName;
        json[configKey::userName] = QStringLiteral("root");
        json[configKey::password] = QStringLiteral("secret");
        json[configKey::port] = 22;
        json[configKey::description] = QStringLiteral("Server ") + hostName;
        if (formatVersion >= 0) {
            json[configKey::formatVersion] = formatVersion;
        }
        return json;
    }

    QByteArray serversListBytes(const QList<QJsonObject> &configs)
    {
        QJsonArray array;
        for (const QJsonObject &config : configs) {
            array.append(config);
        }
        return QJsonDocument(array).toJson();
    }
} // namespace

class TestConfigFormatVersion : public QObject
{
    Q_OBJECT

private:
    TestCoreController *m_coreController = nullptr;
    SecureQSettings *m_settings = nullptr;

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

    void testVersionCheckHelpers()
    {
        QCOMPARE(serverConfigUtils::configFormatVersion(QJsonObject()), 0);
        QVERIFY(serverConfigUtils::isConfigFormatVersionSupported(QJsonObject()));
        QVERIFY(serverConfigUtils::isConfigFormatVersionSupported(
                adminConfigJson("1.1.1.1", serverConfigUtils::currentConfigFormatVersion)));
        QVERIFY(!serverConfigUtils::isConfigFormatVersionSupported(
                adminConfigJson("1.1.1.1", serverConfigUtils::currentConfigFormatVersion + 1)));
    }

    void testToJsonWritesCurrentFormatVersion()
    {
        SelfHostedAdminServerConfig admin;
        admin.hostName = "1.1.1.1";
        QCOMPARE(admin.toJson().value(configKey::formatVersion).toInt(-1), serverConfigUtils::currentConfigFormatVersion);

        SelfHostedUserServerConfig user;
        user.hostName = "1.1.1.1";
        QCOMPARE(user.toJson().value(configKey::formatVersion).toInt(-1), serverConfigUtils::currentConfigFormatVersion);

        NativeServerConfig native;
        native.hostName = "1.1.1.1";
        QCOMPARE(native.toJson().value(configKey::formatVersion).toInt(-1), serverConfigUtils::currentConfigFormatVersion);

        ApiV2ServerConfig api;
        api.hostName = "1.1.1.1";
        QCOMPARE(api.toJson().value(configKey::formatVersion).toInt(-1), serverConfigUtils::currentConfigFormatVersion);
    }

    void testImportAcceptsMissingAndCurrentVersion()
    {
        auto *importController = m_coreController->m_importCoreController;
        QSignalSpy importFinishedSpy(importController, &ImportController::importFinished);

        const QString withoutVersion = QJsonDocument(adminConfigJson("1.1.1.1")).toJson();
        auto result = importController->extractConfigFromData(withoutVersion);
        QCOMPARE(result.errorCode, ErrorCode::NoError);
        importController->importConfig(result.config);

        const QString currentVersion =
                QJsonDocument(adminConfigJson("2.2.2.2", serverConfigUtils::currentConfigFormatVersion)).toJson();
        result = importController->extractConfigFromData(currentVersion);
        QCOMPARE(result.errorCode, ErrorCode::NoError);
        importController->importConfig(result.config);

        QCOMPARE(importFinishedSpy.count(), 2);
        QCOMPARE(m_coreController->m_serversRepository->serversCount(), 2);

        const auto stored = m_coreController->m_serversRepository->selfHostedAdminConfig(
                m_coreController->m_serversRepository->serverIdAt(0));
        QVERIFY(stored.has_value());
        QCOMPARE(stored->toJson().value(configKey::formatVersion).toInt(-1), serverConfigUtils::currentConfigFormatVersion);
    }

    void testImportRejectsNewerVersion()
    {
        auto *importController = m_coreController->m_importCoreController;
        QSignalSpy importFinishedSpy(importController, &ImportController::importFinished);
        QSignalSpy importErrorSpy(importController, &ImportController::importErrorOccurred);

        const QJsonObject newer = adminConfigJson("3.3.3.3", serverConfigUtils::currentConfigFormatVersion + 1);
        const QByteArray newerBytes = QJsonDocument(newer).toJson();

        auto result = importController->extractConfigFromData(QString::fromUtf8(newerBytes));
        QCOMPARE(result.errorCode, ErrorCode::ConfigFormatVersionNotSupportedError);
        QVERIFY(result.config.isEmpty());

        const QString vpnUrl = "vpn://"
                + QString::fromUtf8(qCompress(newerBytes, 8).toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
        result = importController->extractConfigFromData(vpnUrl);
        QCOMPARE(result.errorCode, ErrorCode::ConfigFormatVersionNotSupportedError);
        QVERIFY(result.config.isEmpty());

        result = importController->extractConfigFromQr(newerBytes);
        QCOMPARE(result.errorCode, ErrorCode::ConfigFormatVersionNotSupportedError);
        QVERIFY(result.config.isEmpty());

        importController->importConfig(newer);
        QCOMPARE(importErrorSpy.count(), 1);
        QCOMPARE(importErrorSpy.first().at(0).value<ErrorCode>(), ErrorCode::ConfigFormatVersionNotSupportedError);
        QCOMPARE(importFinishedSpy.count(), 0);
        QCOMPARE(m_coreController->m_serversRepository->serversCount(), 0);
    }

    void testRepositorySkipsNewerVersionOnLoad()
    {
        m_settings->setValue("Servers/serversList",
                             serversListBytes({ adminConfigJson("1.1.1.1"),
                                                adminConfigJson("2.2.2.2", serverConfigUtils::currentConfigFormatVersion + 1),
                                                adminConfigJson("3.3.3.3", serverConfigUtils::currentConfigFormatVersion) }));
        m_coreController->m_serversRepository->invalidateCache();

        QCOMPARE(m_coreController->m_serversRepository->serversCount(), 2);
        QCOMPARE(m_coreController->m_serversRepository->unsupportedFormatConfigsCount(), 1);

        const auto rejected = m_coreController->m_serversRepository->addServer(
                QString(), adminConfigJson("4.4.4.4", serverConfigUtils::currentConfigFormatVersion + 1),
                serverConfigUtils::ConfigType::SelfHostedAdmin);
        QVERIFY(!rejected.isEmpty());
        QCOMPARE(m_coreController->m_serversRepository->serversCount(), 2);
    }

    void testRestoreBackupReportsSkippedConfigs()
    {
        QJsonObject backup;
        backup["Servers/serversList"] = QString::fromUtf8(serversListBytes(
                { adminConfigJson("1.1.1.1"),
                  adminConfigJson("2.2.2.2", serverConfigUtils::currentConfigFormatVersion + 1) }));

        QSignalSpy errorSpy(m_coreController->m_settingsUiController, &SettingsUiController::errorOccurred);
        QSignalSpy restoredSpy(m_coreController->m_settingsUiController, &SettingsUiController::restoreBackupFinished);

        m_coreController->m_settingsUiController->restoreAppConfigFromData(QJsonDocument(backup).toJson());

        QCOMPARE(restoredSpy.count(), 1);
        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).value<ErrorCode>(), ErrorCode::RestoreBackupUnsupportedConfigsSkipped);
        QCOMPARE(m_coreController->m_serversRepository->serversCount(), 1);

        errorSpy.clear();
        restoredSpy.clear();
        QJsonObject cleanBackup;
        cleanBackup["Servers/serversList"] = QString::fromUtf8(serversListBytes({ adminConfigJson("5.5.5.5") }));
        m_coreController->m_settingsUiController->restoreAppConfigFromData(QJsonDocument(cleanBackup).toJson());
        QCOMPARE(restoredSpy.count(), 1);
        QCOMPARE(errorSpy.count(), 0);
    }
};

QTEST_MAIN(TestConfigFormatVersion)
#include "testConfigFormatVersion.moc"
