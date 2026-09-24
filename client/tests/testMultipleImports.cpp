#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QSignalSpy>
#include <QTest>

#include "utils/testCoreController.h"
#include "core/models/serverDescription.h"
#include "ui/models/serversModel.h"
#include "utils/testUtils.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

using namespace amnezia;
using namespace amnezia::test;

class TestMultipleImports : public QObject
{
    Q_OBJECT

private:
    TestCoreController* m_coreController;
    SecureQSettings* m_settings;

    void serverImportTest(QSignalSpy &importFinishedSpy, QSignalSpy &defaultServerChangedSpy, const QString &key, const int &expected, const QString &expectedDescription) {
        auto importResult = m_coreController->m_importCoreController->extractConfigFromData(key);
        QVERIFY2(importResult.errorCode == ErrorCode::NoError, qPrintable(QString("Import should succeed, expected times: %1").arg(expected)));

        m_coreController->m_importCoreController->importConfig(importResult.config);

        QVERIFY2(importFinishedSpy.count() == expected, qPrintable(QString("importFinished signal should be emitted times: %1").arg(expected)));
        QVERIFY2(defaultServerChangedSpy.count() == expected-1, qPrintable(QString("defaultServerChanged signal should be emitted times: %1").arg(expected-1)));
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == expected, qPrintable(QString("After import servers count should be: %1").arg(expected)));
        if (m_coreController->m_serversModel) {
            QVERIFY2(m_coreController->m_serversModel->rowCount() == expected, qPrintable(QString("After import model row count should be: %1").arg(expected)));
        }
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == expected-1, qPrintable(QString("Default server index should be: %1").arg(expected-1)));

        const auto description = serverDescriptionAt(m_coreController->m_serversRepository, expected-1);
        QVERIFY2(description.has_value(), "Server config should exist");
        if (*description != expectedDescription) qWarning() << "Server description should match";

        if (m_coreController->m_serversModel) {
            QString modelDesc = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(expected-1, 0), ServersModel::NameRole).toString();
            if (modelDesc != expectedDescription) qWarning() << "Server description in model should match";
        }
    }

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

    void testMultipleImports() {
        QString awgKey = getEnvValue("THIRD_PARTY_AWG_VPN_KEY");
        QString xrayKey = getEnvValue("THIRD_PARTY_XRAY_VPN_KEY");
        QString wgKey = getEnvValue("THIRD_PARTY_WIRE_GUARD_VPN_KEY");
        QString ovpnKey = getEnvValue("THIRD_PARTY_OPEN_VPN_KEY");
        QString cloakKey = getEnvValue("THIRD_PARTY_CLOAK_VPN_KEY");
        QString ssKey = getEnvValue("THIRD_PARTY_SS_VPN_KEY");
        QString premKey = getEnvValue("THIRD_PARTY_PREMIUM_VPN_KEY");

        logEnvValueState("THIRD_PARTY_AWG_VPN_KEY");
        logEnvValueState("THIRD_PARTY_XRAY_VPN_KEY");
        logEnvValueState("THIRD_PARTY_WIRE_GUARD_VPN_KEY");
        logEnvValueState("THIRD_PARTY_OPEN_VPN_KEY");
        logEnvValueState("THIRD_PARTY_CLOAK_VPN_KEY");
        logEnvValueState("THIRD_PARTY_SS_VPN_KEY");
        logEnvValueState("THIRD_PARTY_PREMIUM_VPN_KEY");

        if (!isEnvValueConfigured(awgKey) || !isEnvValueConfigured(xrayKey) || !isEnvValueConfigured(wgKey)
            || !isEnvValueConfigured(ovpnKey) || !isEnvValueConfigured(cloakKey) || !isEnvValueConfigured(ssKey)
            || !isEnvValueConfigured(premKey)) {
            QSKIP("Set THIRD_PARTY_AWG_VPN_KEY, THIRD_PARTY_XRAY_VPN_KEY, THIRD_PARTY_WIRE_GUARD_VPN_KEY, "
                  "THIRD_PARTY_OPEN_VPN_KEY, THIRD_PARTY_CLOAK_VPN_KEY, THIRD_PARTY_SS_VPN_KEY, "
                  "THIRD_PARTY_PREMIUM_VPN_KEY");
        }

        QSignalSpy importFinishedSpy(m_coreController->m_importCoreController, &ImportController::importFinished);
        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);
        
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 0, "Initial servers count should be 0");
        if (m_coreController->m_serversModel) {
            QVERIFY2(m_coreController->m_serversModel->rowCount() == 0, "Initial model row count should be 0");
        }

        serverImportTest(importFinishedSpy, defaultServerChangedSpy, awgKey,   1, "AWG Server");
        serverImportTest(importFinishedSpy, defaultServerChangedSpy, xrayKey,  2, "Xray Server");
        serverImportTest(importFinishedSpy, defaultServerChangedSpy, wgKey,    3, "WireGuard Server");
        serverImportTest(importFinishedSpy, defaultServerChangedSpy, ovpnKey,  4, "OpenVPN Server");
        serverImportTest(importFinishedSpy, defaultServerChangedSpy, cloakKey, 5, "Cloak Server");
        serverImportTest(importFinishedSpy, defaultServerChangedSpy, ssKey,    6, "ShadowSocks Server");
        serverImportTest(importFinishedSpy, defaultServerChangedSpy, premKey,  7, "Amnezia Premium");
    }

    void testMultipleImportsRemoval() {
        QString awgKey = getEnvValue("THIRD_PARTY_AWG_VPN_KEY");
        QString xrayKey = getEnvValue("THIRD_PARTY_XRAY_VPN_KEY");

        if (!isEnvValueConfigured(awgKey) || !isEnvValueConfigured(xrayKey)) {
            QSKIP("Set THIRD_PARTY_AWG_VPN_KEY and THIRD_PARTY_XRAY_VPN_KEY");
        }

        QSignalSpy importFinishedSpy(m_coreController->m_importCoreController, &ImportController::importFinished);
        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);
        QSignalSpy serverRemovedSpy(m_coreController->m_serversRepository, &SecureServersRepository::serverRemoved);
        
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 0, "Initial servers count should be 0");

        auto importResult1 = m_coreController->m_importCoreController->extractConfigFromData(awgKey);
        QVERIFY2(importResult1.errorCode == ErrorCode::NoError, "First import should succeed");
        m_coreController->m_importCoreController->importConfig(importResult1.config);
        
        auto importResult2 = m_coreController->m_importCoreController->extractConfigFromData(xrayKey);
        QVERIFY2(importResult2.errorCode == ErrorCode::NoError, "Second import should succeed");
        m_coreController->m_importCoreController->importConfig(importResult2.config);
        
        QVERIFY2(importFinishedSpy.count() == 2, "importFinished signal should be emitted twice");
        QVERIFY2(defaultServerChangedSpy.count() == 1, "defaultServerChanged signal should be emitted once (0->1, first import doesn't emit)");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 2, "After two imports servers count should be 2");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 1, "Second server should be default");
        
        const auto description0 = serverDescriptionAt(m_coreController->m_serversRepository, 0);
        const auto description1 = serverDescriptionAt(m_coreController->m_serversRepository, 1);
        QVERIFY2(description0.has_value() && description1.has_value(), "Server configs should exist");
        if (*description0 != "AWG Server") qWarning() << "First server description should match";
        if (*description1 != "Xray Server") qWarning() << "Second server description should match";

        defaultServerChangedSpy.clear();
        serverRemovedSpy.clear();

        m_coreController->m_serversController->removeServer(m_coreController->m_serversController->getServerId(0));
        
        QVERIFY2(serverRemovedSpy.count() == 1, "serverRemoved signal should be emitted");
        QVERIFY2(serverRemovedSpy.at(0).at(1).toInt() == 0, "serverRemoved should emit removed index 0");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 1, "After removing first server, servers count should be 1");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "After removing first server, default index should be 0");
        
        const auto remainingDescription = serverDescriptionAt(m_coreController->m_serversRepository, 0);
        QVERIFY2(remainingDescription.has_value(), "Server config should exist");
        if (*remainingDescription == "Xray Server") qWarning() << "Remaining server should be Xray Server";
        
        if (m_coreController->m_serversModel) {
            QVERIFY2(m_coreController->m_serversModel->rowCount() == 1, "After removing first server, model row count should be 1");
            QString modelDesc = m_coreController->m_serversModel->data(m_coreController->m_serversModel->index(0, 0), ServersModel::NameRole).toString();
            if (modelDesc != "Xray Server") qWarning() << "Remaining server description in model should match";
        }

        defaultServerChangedSpy.clear();
        serverRemovedSpy.clear();

        m_coreController->m_serversController->removeServer(m_coreController->m_serversController->getServerId(0));
        
        QVERIFY2(serverRemovedSpy.count() == 1, "serverRemoved signal should be emitted");
        QVERIFY2(serverRemovedSpy.at(0).at(1).toInt() == 0, "serverRemoved should emit removed index 0");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() == 0, "After removing last server, servers count should be 0");
        QVERIFY2(m_coreController->m_serversRepository->defaultServerIndex() == 0, "After removing last server, default index should be 0");
        
        if (m_coreController->m_serversModel) {
            QVERIFY2(m_coreController->m_serversModel->rowCount() == 0, "After removing last server, model row count should be 0");
        }
    }

    void testListImports() {
        const QString dnsListPath = QFINDTESTDATA("data/dns_list.json");
        const QString ipListPath = QFINDTESTDATA("data/ip_list.json");

        QVERIFY2(!dnsListPath.isEmpty(), "data/dns_list.json not found");
        QVERIFY2(!ipListPath.isEmpty(), "data/ip_list.json not found");

        QSignalSpy dnsErrorOccurredSpy(m_coreController->m_allowedDnsUiController, &AllowedDnsUiController::errorOccurred);
        QSignalSpy dnsFinishedSpy(m_coreController->m_allowedDnsUiController, &AllowedDnsUiController::finished);

        QSignalSpy ipErrorOccurredSpy(m_coreController->m_ipSplitTunnelingUiController, &IpSplitTunnelingUiController::errorOccurred);
        QSignalSpy ipFinishedSpy(m_coreController->m_ipSplitTunnelingUiController, &IpSplitTunnelingUiController::finished);

        m_coreController->m_allowedDnsUiController->importDns(dnsListPath, true);
        if (dnsErrorOccurredSpy.count() > 0) {
            qWarning() << "(dns) errorOccurred:" << dnsErrorOccurredSpy.at(0).at(0).toString();
        }
        QVERIFY2(dnsErrorOccurredSpy.count() == 0, "(dns) errorOccurred signal should NOT be emitted");
        QVERIFY2(dnsFinishedSpy.count() == 1, "(dns) finished signal should be emitted");

        m_coreController->m_ipSplitTunnelingUiController->importSites(ipListPath, true);
        if (ipErrorOccurredSpy.count() > 0) {
            qWarning() << "(ip) errorOccurred:" << ipErrorOccurredSpy.at(0).at(0).toString();
        }
        QVERIFY2(ipErrorOccurredSpy.count() == 0, "(ip) errorOccurred signal should NOT be emitted");
        QVERIFY2(ipFinishedSpy.count() == 1, "(ip) finished signal should be emitted");
    }

    void testWireGuardDnsImport_data() {
        QTest::addColumn<QString>("dnsLine");
        QTest::addColumn<QString>("expectedDns1");
        QTest::addColumn<QString>("expectedDns2");

        QTest::newRow("single") << "DNS = 192.168.12.1" << "192.168.12.1" << "192.168.12.1";
        QTest::newRow("pair") << "DNS = 1.1.1.1, 8.8.8.8" << "1.1.1.1" << "8.8.8.8";
        QTest::newRow("no spaces") << "DNS=10.0.0.1,10.0.0.2" << "10.0.0.1" << "10.0.0.2";
        QTest::newRow("ipv6 and search domain") << "DNS = 2001:db8::1, 172.16.3.254, lan" << "172.16.3.254" << "172.16.3.254";
        QTest::newRow("none") << "" << "" << "";
    }

    void testWireGuardDnsImport() {
        QFETCH(QString, dnsLine);
        QFETCH(QString, expectedDns1);
        QFETCH(QString, expectedDns2);

        const QString config = QStringLiteral("[Interface]\n"
                                              "Address = 192.168.12.2/32\n"
                                              "PrivateKey = yAnz5TF+lXXJte14tji3zlMNq+hd2rYUIgJBgB3fBmk=\n"
                                              "%1\n"
                                              "MTU = 1280\n"
                                              "\n"
                                              "[Peer]\n"
                                              "PublicKey = xTIBA5rboUvnH4htodjb6e697QjLERt1NAB4mZqp8Dg=\n"
                                              "AllowedIPs = 0.0.0.0/0\n"
                                              "Endpoint = vpn.example.com:51820\n").arg(dnsLine);

        const auto importResult = m_coreController->m_importCoreController->extractConfigFromData(config, "test.conf");
        QCOMPARE(importResult.errorCode, ErrorCode::NoError);
        QCOMPARE(importResult.config.value(configKey::dns1).toString(), expectedDns1);
        QCOMPARE(importResult.config.value(configKey::dns2).toString(), expectedDns2);
    }
};

QTEST_MAIN(TestMultipleImports)
#include "testMultipleImports.moc"
