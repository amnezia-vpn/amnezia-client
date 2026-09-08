#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QSignalSpy>
#include <QTest>

#include "utils/testCoreController.h"
#include "core/models/serverDescription.h"
#include "utils/testUtils.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

using namespace amnezia;
using namespace amnezia::test;

class TestSignalOrder : public QObject
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
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), QString{});
        }
    }

    void testSignalOrderOnImport() {
        QString awgKey = getEnvValue("THIRD_PARTY_AWG_VPN_KEY");

        if (!isEnvValueConfigured(awgKey)) {
            QSKIP("Set THIRD_PARTY_AWG_VPN_KEY");
        }

        QSignalSpy importFinishedSpy(m_coreController->m_importCoreController, &ImportController::importFinished);
        QSignalSpy serverAddedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverAdded);
        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);

        auto importResult = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult.config);

        QVERIFY2(importFinishedSpy.count() == 1, "importFinished signal should be emitted");
        QVERIFY2(serverAddedSpy.count() == 1, "serverAdded signal should be emitted");
        QVERIFY2(defaultServerChangedSpy.count() == 0, "defaultServerChanged signal should NOT be emitted (default is already 0)");

        QVERIFY2(serverAddedSpy.at(0).count() > 0, "serverAdded should have arguments");
    }

    void testSignalOrderOnRemoveDefault() {
        QString awgKey = getEnvValue("THIRD_PARTY_AWG_VPN_KEY");
        QString xrayKey = getEnvValue("THIRD_PARTY_XRAY_VPN_KEY");

        if (!isEnvValueConfigured(awgKey) || !isEnvValueConfigured(xrayKey)) {
            QSKIP("Set THIRD_PARTY_AWG_VPN_KEY and THIRD_PARTY_XRAY_VPN_KEY");
        }

        auto importResult1 = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult1.config);
        auto importResult2 = m_coreController->m_importCoreController->extractConfigFromData(xrayKey);
        m_coreController->m_importCoreController->importConfig(importResult2.config);

        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 1, "Default should be index 1");

        QSignalSpy serverRemovedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverRemoved);
        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);

        m_coreController->m_serversController->removeServer(m_coreController->m_serversController->getServerId(1));

        QVERIFY2(serverRemovedSpy.count() == 1, "serverRemoved signal should be emitted");
        QVERIFY2(defaultServerChangedSpy.count() == 1, "defaultServerChanged signal should be emitted when removing default server");
        QVERIFY2(defaultServerChangedSpy.at(0).at(0).toString() == m_coreController->m_serversRepository->defaultServerId(),
                 "defaultServerChanged should emit new default server id");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "Default server index should be 0");
    }
};

QTEST_MAIN(TestSignalOrder)
#include "testSignalOrder.moc"

