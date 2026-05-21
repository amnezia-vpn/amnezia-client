#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QSignalSpy>
#include <QTest>

#include "core/controllers/coreController.h"
#include "core/models/serverDescription.h"
#include "ui/models/serversModel.h"
#include "utils/testUtils.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

using namespace amnezia;
using namespace amnezia::test;

class TestDefaultServerChange : public QObject
{
    Q_OBJECT

private:
    CoreController* m_coreController;
    SecureQSettings* m_settings;

private slots:
    void initTestCase() {
        QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);
        
        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);
        m_coreController = new CoreController(vpnConnection, m_settings, nullptr, this);
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
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), -1);
        }
    }

    void testSetDefaultServerIndex() {
        QString awgKey = getValueFromIni("configs/thirdPartyAwgVpnKey");
        QString xrayKey = getValueFromIni("configs/thirdPartyXrayVpnKey");
        QString wgKey = getValueFromIni("configs/thirdPartyWireGuardVpnKey");

        auto importResult1 = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult1.config);
        auto importResult2 = m_coreController->m_importCoreController->extractConfigFromData(xrayKey);
        m_coreController->m_importCoreController->importConfig(importResult2.config);
        auto importResult3 = m_coreController->m_importCoreController->extractConfigFromData(wgKey);
        m_coreController->m_importCoreController->importConfig(importResult3.config);

        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 3, "Should have 3 servers");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 2, "Default should be index 2");

        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);

        m_coreController->m_serversController->setDefaultServer(m_coreController->m_serversController->getServerId(0));
        QVERIFY2(defaultServerChangedSpy.count() == 1, "defaultServerChanged signal should be emitted");
        QVERIFY2(defaultServerChangedSpy.at(0).at(0).toString() == m_coreController->m_serversController->getServerId(0),
                 "defaultServerChanged should emit new default server id");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "Default server index should be 0");

        if (m_coreController->m_serversModel) {
            int modelDefaultIndex = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(0, 0), ServersModel::IsDefaultRole).toBool() ? 0 : -1;
            QVERIFY2(modelDefaultIndex == 0, "Model should reflect default server");
        }

        m_coreController->m_serversController->setDefaultServer(m_coreController->m_serversController->getServerId(2));
        QVERIFY2(defaultServerChangedSpy.count() == 2, "defaultServerChanged signal should be emitted again");
        QVERIFY2(defaultServerChangedSpy.at(1).at(0).toString() == m_coreController->m_serversController->getServerId(2),
                 "defaultServerChanged should emit new default server id");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 2, "Default server index should be 2");
    }

    void testDefaultServerChangeOnRemoveEdgeCases() {
        QString awgKey = getValueFromIni("configs/thirdPartyAwgVpnKey");
        QString xrayKey = getValueFromIni("configs/thirdPartyXrayVpnKey");
        QString wgKey = getValueFromIni("configs/thirdPartyWireGuardVpnKey");

        auto importResult1 = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        m_coreController->m_importCoreController->importConfig(importResult1.config);
        auto importResult2 = m_coreController->m_importCoreController->extractConfigFromData(xrayKey);
        m_coreController->m_importCoreController->importConfig(importResult2.config);
        auto importResult3 = m_coreController->m_importCoreController->extractConfigFromData(wgKey);
        m_coreController->m_importCoreController->importConfig(importResult3.config);

        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 3, "Should have 3 servers");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 2, "Default should be index 2");

        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);
        QSignalSpy serverRemovedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverRemoved);

        m_coreController->m_serversController->removeServer(m_coreController->m_serversController->getServerId(0));
        QVERIFY2(serverRemovedSpy.count() == 1, "serverRemoved signal should be emitted");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 2, "Should have 2 servers");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 1, "Default should be index 1 (was 2, removed 0)");

        const auto description1 = serverDescriptionAt(m_coreController->m_serversRepository, 0);
        const auto description2 = serverDescriptionAt(m_coreController->m_serversRepository, 1);
        QVERIFY2(description1.has_value() && description2.has_value(), "Server configs should exist");
        QVERIFY2(*description1 == "Xray Server", "First remaining server should be Xray");
        QVERIFY2(*description2 == "WireGuard Server", "Second remaining server should be WireGuard");

        defaultServerChangedSpy.clear();
        serverRemovedSpy.clear();

        m_coreController->m_serversController->removeServer(m_coreController->m_serversController->getServerId(0));
        QVERIFY2(serverRemovedSpy.count() == 1, "serverRemoved signal should be emitted");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 1, "Should have 1 server");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "Default should be index 0 (was 1, removed 0)");

        const auto lastDescription = serverDescriptionAt(m_coreController->m_serversRepository, 0);
        QVERIFY2(lastDescription.has_value(), "Server config should exist");
        QVERIFY2(*lastDescription == "WireGuard Server", "Last server should be WireGuard");
    }
};

QTEST_MAIN(TestDefaultServerChange)
#include "testDefaultServerChange.moc"

