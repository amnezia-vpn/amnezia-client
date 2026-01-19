#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QUuid>
#include <QSignalSpy>

#include "core/controllers/selfhosted/importController.h"
#include "core/controllers/serversController.h"
#include "core/models/serverConfig.h"
#include "core/repositories/qServersRepository.h"
#include "core/repositories/qAppSettingsRepository.h"
#include "ui/models/serversModel.h"
#include "secure_qsettings.h"

class TestMultipleImports : public QObject
{
    Q_OBJECT

private:
    SecureQSettings* m_settings;
    QServersRepository* m_qServersRepo;
    QAppSettingsRepository* m_qAppSettingsRepo;
    ServersController* m_serversController;
    ServersModel* m_serversModel;
    ImportController* m_importController;

private slots:
    void initTestCase() {
        QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);
        m_qServersRepo = new QServersRepository(m_settings);
        m_qAppSettingsRepo = new QAppSettingsRepository(m_settings);
        m_serversController = new ServersController(m_qServersRepo->repository(), m_qAppSettingsRepo->repository());
        m_serversModel = new ServersModel();
        m_importController = new ImportController(m_qServersRepo->repository(), m_qAppSettingsRepo->repository());
        
        // Emulate CoreSignalHandlers::initServersModelUpdateHandler
        connect(m_qServersRepo, &QServersRepository::serverAdded, this, [this]() {
            m_serversModel->updateModel(m_serversController->getServersArray(), 
                                       m_serversController->getDefaultServerIndex(), false);
        });
        connect(m_qServersRepo, &QServersRepository::defaultServerChanged, this, [this]() {
            m_serversModel->updateModel(m_serversController->getServersArray(), 
                                       m_serversController->getDefaultServerIndex(), false);
        });
        
        // Emulate CoreSignalHandlers::initImportControllerHandler
        connect(m_importController, &ImportController::importFinished, this, [this]() {
            int newServerIndex = m_serversController->getServersCount() - 1;
            m_qServersRepo->setDefaultServer(newServerIndex);
        });
    }

    void cleanupTestCase() {
        delete m_importController;
        delete m_serversModel;
        delete m_serversController;
        delete m_qAppSettingsRepo;
        delete m_qServersRepo;
        m_settings->clearSettings();
        delete m_settings;
    }

    void init() {
        m_settings->clearSettings();
        m_serversModel->updateModel(QJsonArray(), -1, false);
    }

    void testMultipleImports() {
        QString awgKey = "vpn://AAABFHjadZBBT4QwEIX_ipkzS2wBJdyMB1cPXvbgwRgyQnclgZa0RTYS_rszXRa52Mt77TfzOu0EldEeG62sg-J9AhxPUEywF1CAuF3WTl4dRLCXhJIVpVuUEMpWdLdFKaH7FeUb9Mx3scpFk0XTRbOLvlSkKZsOz-Gi4BsdRiV_EGEydhwlg0tWynEZmd5Yz1bkoaK3xpvKtOU3_UFjOE3SsRs-tfIl1rVVzoWQOI9FzC3eonYcU4ZmgkPdwxz9fSYdYafVT4M7-lEJ80cEtTri0PrH_2q4wlW26f1lioe3p5uDsjQWoS_j_Ct2ipvGU6zO2PWtiivT8RPQudHYmqBXzl-3Yn2slBEMTtklgYt4C_Mv3ROMwA";
        QString xrayKey = "vpn://AAAAtXjadY7NCsJADIRfRXKui1YP0qt3L14EkRK7EQt2d0lS_0rf3awonjyFmW-YyQBNDIptIBao9sNPQgXYBXq2OL0zPqCA96kGSJHV6HK5MFP6YyCt0XsmsQqYz9zKzd3MmDIGyek6cdRoUJsE43gowNMJ-4uu_695kobbpG0MBndmTrbEV4sWcI6iG-zIQE47umOXLuSa2BlNKHKL7PMeiX5lmdH79bIsoBfiT0UOZQnjCw_AXRQ";
        QString wgKey = "vpn://AAAAwXjahY89a8NADIb_StDsHLFDIHjt0C1LhgwlBNWnpgfx3SHp6hDj_15dacnYTS_Po68ZhhQVQyQW6N_mZ4QecIz0CLieAtO1IHto4Fn3M-TEat6u3XetMSnvkfSC3jOJjYN24_audRtjyhil-pfMSZPB4jMsy7kBTx9Ybvryz2ZPMnDIGlI042TktZLVkfjLmhr4TKIHHMnodHV0xzHfyA1pNJZRZEr1alAS_Yvbin6e6LoGihD_DqhSjbB8AyB_ZI8";

        QSignalSpy importFinishedSpy(m_importController, &ImportController::importFinished);
        QSignalSpy defaultServerChangedSpy(m_qServersRepo, &QServersRepository::defaultServerChanged);
        
        QVERIFY2(m_qServersRepo->serversCount() == 0, "Initial servers count should be 0");
        QVERIFY2(m_serversModel->rowCount() == 0, "Initial model row count should be 0");

        auto importResult1 = m_importController->extractConfigFromData(awgKey);
        QVERIFY2(importResult1.errorCode == ErrorCode::NoError, "First import should succeed");
        
        m_importController->importConfig(importResult1.config);
        
        QVERIFY2(importFinishedSpy.count() == 1, "importFinished signal should be emitted once");
        QVERIFY2(defaultServerChangedSpy.count() == 1, "defaultServerChanged signal should be emitted by CoreSignalHandlers handler");
        QVERIFY2(m_qServersRepo->serversCount() == 1, "After first import servers count should be 1");
        QVERIFY2(m_serversModel->rowCount() == 1, "After first import model row count should be 1");
        QVERIFY2(m_qServersRepo->defaultServerIndex() == 0, "First server should be default");
        
        ServerConfig server1 = m_qServersRepo->server(0);
        QString desc1 = ServerConfigUtils::description(server1);
        QVERIFY2(desc1 == "AWG Server", "First server description should match");
        
        QString modelDesc1 = m_serversModel->data(m_serversModel->index(0, 0), ServersModel::NameRole).toString();
        QVERIFY2(modelDesc1 == "AWG Server", "First server description in model should match");

        auto importResult2 = m_importController->extractConfigFromData(xrayKey);
        QVERIFY2(importResult2.errorCode == ErrorCode::NoError, "Second import should succeed");
        
        m_importController->importConfig(importResult2.config);
        
        QVERIFY2(importFinishedSpy.count() == 2, "importFinished signal should be emitted twice");
        QVERIFY2(defaultServerChangedSpy.count() == 2, "defaultServerChanged signal should be emitted twice");
        QVERIFY2(m_qServersRepo->serversCount() == 2, "After second import servers count should be 2");
        QVERIFY2(m_serversModel->rowCount() == 2, "After second import model row count should be 2");
        QVERIFY2(m_qServersRepo->defaultServerIndex() == 1, "Second server should be default");
        
        ServerConfig server2 = m_qServersRepo->server(1);
        QString desc2 = ServerConfigUtils::description(server2);
        QVERIFY2(desc2 == "Xray Server", "Second server description should match");
        
        QString modelDesc2 = m_serversModel->data(m_serversModel->index(1, 0), ServersModel::NameRole).toString();
        QVERIFY2(modelDesc2 == "Xray Server", "Second server description in model should match");

        auto importResult3 = m_importController->extractConfigFromData(wgKey);
        QVERIFY2(importResult3.errorCode == ErrorCode::NoError, "Third import should succeed");
        
        m_importController->importConfig(importResult3.config);
        
        QVERIFY2(importFinishedSpy.count() == 3, "importFinished signal should be emitted three times");
        QVERIFY2(defaultServerChangedSpy.count() == 3, "defaultServerChanged signal should be emitted three times");
        QVERIFY2(m_qServersRepo->serversCount() == 3, "After third import servers count should be 3");
        QVERIFY2(m_serversModel->rowCount() == 3, "After third import model row count should be 3");
        QVERIFY2(m_qServersRepo->defaultServerIndex() == 2, "Third server should be default");
        
        ServerConfig server3 = m_qServersRepo->server(2);
        QString desc3 = ServerConfigUtils::description(server3);
        QVERIFY2(desc3 == "WireGuard Server", "Third server description should match");
        
        QString modelDesc3 = m_serversModel->data(m_serversModel->index(2, 0), ServersModel::NameRole).toString();
        QVERIFY2(modelDesc3 == "WireGuard Server", "Third server description in model should match");
    }
};

QTEST_MAIN(TestMultipleImports)
#include "testMultipleImports.moc"

