#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QUuid>
#include <QProcessEnvironment>
#include <QTest>

#include "core/controllers/coreController.h"
#include "core/models/serverConfig.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

class TestMultipleExports : public QObject
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

        QString vpnKey = getValueFromIni("configs/TEST_SELF_HOSTED_CONFIG");
        QJsonObject importedConfig = m_coreController->m_importCoreController->extractConfigFromData(vpnKey).config;

        m_coreController->m_importCoreController->importConfig(importedConfig);

        qDebug() << "SELF-HOSTED ADMIN SERVER IMPORTED\n";
    }

    void cleanupTestCase()
    {
        int serverIndex = m_coreController->m_serversRepository->defaultServerIndex();
        
        for (int containerIndex = 1; containerIndex < 7; ++containerIndex)
            m_coreController->m_installUiController->clearCachedProfile(serverIndex, containerIndex);

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

    void testMultipleExports()
    {
        int serverIndex = m_coreController->m_serversRepository->defaultServerIndex();

        QString clientName = "MultipleExports Test Client";

        for (int containerIndex = 1; containerIndex < 7; ++containerIndex) {

            QString containerName;

            switch (containerIndex) {
            case 1: containerName = "AwgLegacy"; break;
            case 2: containerName = "Awg2"; break;
            case 3: containerName = "WireGuard"; break;
            case 4: containerName = "OpenVPN"; break;
            case 5: continue; break; // skipping IPsec
            case 6: containerName = "XRay"; break;
            }

            if (!m_coreController->m_containersModel->data(containerIndex, ContainersModel::Roles::IsInstalledRole).toBool()) {
                qDebug() << QStringLiteral("%1: Not installed").arg(containerName).toUtf8().constData();
                continue;
            }

            auto exportResult = m_coreController->m_exportController->generateConnectionConfig(serverIndex, containerIndex, clientName);

            QVERIFY2(exportResult.errorCode == ErrorCode::NoError,
                     QStringLiteral("\n%1: Export should succeed").arg(containerName).toUtf8().constData());
            QVERIFY2(!exportResult.config.isEmpty(),
                     QStringLiteral("%1: Exported config should not be empty\n").arg(containerName).toUtf8().constData());
        }
    }

    void testMultipleExportsNative()
    {
        int serverIndex = m_coreController->m_serversRepository->defaultServerIndex();

        QString clientName = "MultipleExports Test Client";

        auto exportResultAwg = m_coreController->m_exportController->generateAwgConfig(serverIndex, clientName);
        auto exportResultWg = m_coreController->m_exportController->generateWireGuardConfig(serverIndex, clientName);
        auto exportResultOvpn = m_coreController->m_exportController->generateOpenVpnConfig(serverIndex, clientName);
        auto exportResultXray = m_coreController->m_exportController->generateXrayConfig(serverIndex, clientName);

        QVERIFY2(exportResultAwg.errorCode == ErrorCode::NoError, "\nAwg (native): Export should succeed");
        QVERIFY2(exportResultWg.errorCode == ErrorCode::NoError, "\nWg (native): Export should succeed");
        QVERIFY2(exportResultOvpn.errorCode == ErrorCode::NoError, "\nOvpn (native): Export should succeed");
        QVERIFY2(exportResultXray.errorCode == ErrorCode::NoError, "\nXray (native): Export should succeed");

        QVERIFY2(!exportResultAwg.config.isEmpty(), "Awg (native): Exported config should not be empty\n");
        QVERIFY2(!exportResultWg.config.isEmpty(), "Wg (native): Exported config should not be empty\n");
        QVERIFY2(!exportResultOvpn.config.isEmpty(), "Ovpn (native): Exported config should not be empty\n");
        QVERIFY2(!exportResultXray.config.isEmpty(), "Xray (native): Exported config should not be empty\n");
    }

};

QTEST_MAIN(TestMultipleExports)
#include "testMultipleExports.moc"
