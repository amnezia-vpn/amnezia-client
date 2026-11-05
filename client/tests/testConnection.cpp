#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QUuid>
#include <QSignalSpy>
#include <QTest>

#include "utils/testCoreController.h"
#include "utils/testUtils.h"
#include "vpnConnection.h"
#include "secureQSettings.h"
#include "core/utils/utilities.h"
#include "version.h"
#include "ui/utils/notificationHandler.h"

using namespace amnezia;
using namespace amnezia::test;

class TestConnection : public QObject
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
        m_coreController->m_serversRepository->invalidateCache();
        if (m_coreController->m_serversModel) {
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), "");
        }
        m_coreController->m_connectionUiController->onConnectionStateChanged(Vpn::ConnectionState::Disconnected);
    }

    void testTrayConnectRejectsUnpreparedServer()
    {
        SelfHostedUserServerConfig config { };
        config.hostName = QStringLiteral("vpn.example.invalid");
        config.defaultContainer = DockerContainer::None;
        const QString serverId = m_coreController->m_serversRepository->addServer(
                { }, config.toJson(), serverConfigUtils::ConfigType::SelfHostedUser);
        QVERIFY(m_coreController->m_serversRepository->selfHostedUserConfig(serverId).has_value());
        m_coreController->m_serversRepository->setDefaultServer(serverId);

        auto *connection = m_coreController->findChild<ConnectionController *>();
        auto *notification = m_coreController->findChild<NotificationHandler *>();
        QVERIFY(connection);
        QVERIFY(notification);
        QSignalSpy openSpy(connection, &ConnectionController::openConnectionRequested);
        QSignalSpy noContainersSpy(m_coreController->m_connectionUiController,
                                   &ConnectionUiController::noInstalledContainers);
        QSignalSpy errorSpy(m_coreController->m_connectionUiController, &ConnectionUiController::connectionErrorOccurred);
        const bool serviceReady = connection->isServiceReady();

        emit notification->connectRequested();

        QCOMPARE(openSpy.count(), 0);
        if (serviceReady) {
            QCOMPARE(noContainersSpy.count(), 1);
            QCOMPARE(errorSpy.count(), 0);
        } else {
            QCOMPARE(errorSpy.count(), 1);
            QCOMPARE(qvariant_cast<ErrorCode>(errorSpy.first().first()), ErrorCode::AmneziaServiceNotRunning);
        }
    }

    void testTrayConnectDoesNotBypassPreparationInProgress()
    {
        auto *notification = m_coreController->findChild<NotificationHandler *>();
        QVERIFY(notification);
        auto *connectionUi = m_coreController->m_connectionUiController;
        connectionUi->onConnectionStateChanged(Vpn::ConnectionState::Preparing);
        QSignalSpy preparingSpy(connectionUi, &ConnectionUiController::preparingConfig);

        emit notification->connectRequested();

        QCOMPARE(preparingSpy.count(), 1);
        QVERIFY(connectionUi->isConnectionInProgress());
    }

    void testConnect() {
        QString awgKey = getEnvValue("THIRD_PARTY_AWG_VPN_KEY");
        QString xrayKey = getEnvValue("THIRD_PARTY_XRAY_VPN_KEY");
        QString wgKey = getEnvValue("THIRD_PARTY_WIRE_GUARD_VPN_KEY");
        QString ovpnKey = getEnvValue("THIRD_PARTY_OPEN_VPN_KEY");
        QString ikevKey = getEnvValue("THIRD_PARTY_IKEV_VPN_KEY");

        logEnvValueState("THIRD_PARTY_AWG_VPN_KEY");
        logEnvValueState("THIRD_PARTY_XRAY_VPN_KEY");
        logEnvValueState("THIRD_PARTY_WIRE_GUARD_VPN_KEY");
        logEnvValueState("THIRD_PARTY_OPEN_VPN_KEY");
        logEnvValueState("THIRD_PARTY_IKEV_VPN_KEY");

        // toggleConnection() silently refuses to emit prepareConfig when the
        // AmneziaVPN service process is not running (isConnectionSupported returns
        // AmneziaServiceNotRunning), so this test needs an installed client.
        if (!Utils::processIsRunning(Utils::executable(SERVICE_NAME, false), true)) {
            QSKIP("AmneziaVPN service is not running - connection flow cannot be tested");
        }

        QSignalSpy prepareConfigSpy(m_coreController->m_connectionUiController, &ConnectionUiController::prepareConfig);

        auto importResult0 = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        if (importResult0.errorCode == ErrorCode::NoError) {
            m_coreController->m_importCoreController->importConfig(importResult0.config);

            m_coreController->m_connectionUiController->toggleConnection();
            QVERIFY2(prepareConfigSpy.count() == 1, "prepareConfig signal should be emitted once");

            m_coreController->m_connectionUiController->toggleConnection();
            QVERIFY2(m_coreController->m_connectionUiController->isConnected() == false, "Connection should NOT be active");
        } else {
            qWarning() << "Error on AWG key import";
        }

        auto importResult1 = m_coreController->m_importCoreController->extractConfigFromData(xrayKey);
        if (importResult1.errorCode == ErrorCode::NoError) {
            m_coreController->m_importCoreController->importConfig(importResult1.config);

            m_coreController->m_connectionUiController->toggleConnection();
            QVERIFY2(prepareConfigSpy.count() == 3, "prepareConfig signal should be emitted three times");

            m_coreController->m_connectionUiController->toggleConnection();
            QVERIFY2(m_coreController->m_connectionUiController->isConnected() == false, "Connection should NOT be active");
        } else {
            qWarning() << "Error on XRay key import";
        }

        auto importResult2 = m_coreController->m_importCoreController->extractConfigFromData(wgKey);
        if (importResult2.errorCode == ErrorCode::NoError) {
            m_coreController->m_importCoreController->importConfig(importResult2.config);

            m_coreController->m_connectionUiController->toggleConnection();
            QVERIFY2(prepareConfigSpy.count() == 5, "prepareConfig signal should be emitted five times");

            m_coreController->m_connectionUiController->toggleConnection();
            QVERIFY2(m_coreController->m_connectionUiController->isConnected() == false, "Connection should NOT be active");
        } else {
            qWarning() << "Error on WG key import";
        }

        auto importResult3 = m_coreController->m_importCoreController->extractConfigFromData(ovpnKey);
        if (importResult3.errorCode == ErrorCode::NoError) {
            m_coreController->m_importCoreController->importConfig(importResult3.config);

            m_coreController->m_connectionUiController->toggleConnection();
            QVERIFY2(prepareConfigSpy.count() == 7, "prepareConfig signal should be emitted seven times");

            m_coreController->m_connectionUiController->toggleConnection();
            QVERIFY2(m_coreController->m_connectionUiController->isConnected() == false, "Connection should NOT be active");
        } else {
            qWarning() << "Error on OpenVPN key import";
        }

        auto importResult4 = m_coreController->m_importCoreController->extractConfigFromData(ikevKey);
        if (importResult4.errorCode == ErrorCode::NoError) {
            m_coreController->m_importCoreController->importConfig(importResult4.config);

            m_coreController->m_connectionUiController->toggleConnection();
            QVERIFY2(prepareConfigSpy.count() == 9, "prepareConfig signal should be emitted nine times");

            m_coreController->m_connectionUiController->toggleConnection();
            QVERIFY2(m_coreController->m_connectionUiController->isConnected() == false, "Connection should NOT be active");
        }
        else {
            qWarning() << "Error on IKEV key import";
        }
    }
};

QTEST_MAIN(TestConnection)
#include "testConnection.moc"
