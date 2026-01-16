#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

#include "core/controllers/selfhosted/importController.h"
#include "core/controllers/selfhosted/exportController.h"
#include "core/models/serverConfig.h"
#include "mockRepositories.h"

class TestAdminSelfHostedExport : public QObject
{
    Q_OBJECT

private:
    MockServersRepository* m_serversRepo;
    MockAppSettingsRepository* m_appSettingsRepo;
    ImportController* m_importController;
    ExportController* m_exportController;

    QJsonObject decodeVpnKey(const QString &vpnKey) {
        QString key = vpnKey;
        key.replace("vpn://", "");
        
        QByteArray ba = QByteArray::fromBase64(
            key.toUtf8(), 
            QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals
        );
        
        qDebug() << "Base64 decoded size:" << ba.size();
        
        QJsonDocument testDoc = QJsonDocument::fromJson(ba);
        if (!testDoc.isNull()) {
            qDebug() << "Data is not compressed, using as-is";
            return testDoc.object();
        }
        
        QByteArray baUncompressed = qUncompress(ba);
        if (!baUncompressed.isEmpty()) {
            qDebug() << "Data was compressed, uncompressed size:" << baUncompressed.size();
            ba = baUncompressed;
        } else {
            qDebug() << "qUncompress failed or data is not compressed";
        }
        
        return QJsonDocument::fromJson(ba).object();
    }

    QJsonObject sortContainers(const QJsonObject &config) {
        QJsonObject sorted = config;
        
        if (!config.contains("containers")) {
            return sorted;
        }
        
        QJsonArray containers = config["containers"].toArray();
        QVector<QJsonObject> containerVec;
        
        for (const QJsonValue &val : containers) {
            containerVec.append(val.toObject());
        }
        
        std::sort(containerVec.begin(), containerVec.end(), [](const QJsonObject &a, const QJsonObject &b) {
            return a["container"].toString() < b["container"].toString();
        });
        
        QJsonArray sortedContainers;
        for (const QJsonObject &obj : containerVec) {
            sortedContainers.append(obj);
        }
        
        sorted["containers"] = sortedContainers;
        return sorted;
    }


private slots:
    void initTestCase() {
        m_serversRepo = new MockServersRepository();
        m_appSettingsRepo = new MockAppSettingsRepository();
        m_importController = new ImportController(m_serversRepo, m_appSettingsRepo);
        m_exportController = new ExportController(m_serversRepo, m_appSettingsRepo);
    }

    void cleanupTestCase() {
        delete m_exportController;
        delete m_importController;
        delete m_appSettingsRepo;
        delete m_serversRepo;
    }

    void init() {
        m_serversRepo->clear();
    }

    void testAdminSelfHostedExport() {
        QString vpnKey = "vpn://AAABTXjarZIxT8MwEIX_Cro5jbDjQunKUhhYyoZQZZKjRGpsy3baQtT_zp2bJh3oACLLPfvz3bOe00FpTdS1QR9g_tKB3q1h3sFCwBzEdf9N5ElBBgtJqBiQOkcFoemAbs6RInQ7oNkZemAvrrKvRV9VX6fH-lhSVSwavU9GSdcmXZX0UqSbseJRMqlioDxuSsJZH1mKWTrhvI22tJvVljKoLU-TtB3aN4NxpavKYwhpSD7LRc4t0WsTeMwqNRNsKweHbAyTtnRj8KvWE0pUEut-hNah2TpDM0-Kwu8vKMSd-ttFLrntao_rVvuKWkc9OnIk4n8t915_Ulcqo5FSxa9tYsk2rxlU-K7bTby_lDWfCKWvXTy-5jOGeLVET-9L7MOG-KQbJEBx57jXjdtgXtqG_wUdws5yJhCpa1iefhopM2gD-n4An-ElHL4BvzD6nw";
        
        qDebug() << "IMPORTED KEY:" << vpnKey;
        
        auto importResult = m_importController->extractConfigFromData(vpnKey);
        
        QVERIFY2(importResult.errorCode == ErrorCode::NoError, "Import should succeed");
        QVERIFY2(!importResult.config.isEmpty(), "Config should not be empty");

        QJsonObject importedConfig = importResult.config;

        m_importController->importConfig(importedConfig);
        
        QVERIFY2(m_serversRepo->serversCount() > 0, "Server should be added");

        int serverIndex = m_serversRepo->defaultServerIndex();
        auto exportResult = m_exportController->generateFullAccessConfig(serverIndex);
        
        QVERIFY2(exportResult.errorCode == ErrorCode::NoError, "Export should succeed");
        QVERIFY2(!exportResult.config.isEmpty(), "Exported config should not be empty");

        qDebug() << "EXPORTED KEY:" << exportResult.config;

        QJsonObject exportedConfig = decodeVpnKey(exportResult.config);
        
        auto importResult2 = m_importController->extractConfigFromData(exportResult.config);
        QVERIFY2(importResult2.errorCode == ErrorCode::NoError, "Re-import should succeed");
        
        QJsonObject sortedImported = sortContainers(importedConfig);
        QJsonObject sortedExported = sortContainers(importResult2.config);
        
        QString importedJson = QJsonDocument(sortedImported).toJson(QJsonDocument::Compact);
        QString exportedJson = QJsonDocument(sortedExported).toJson(QJsonDocument::Compact);
        
        qDebug() << "IMPORTED JSON:" << importedJson;
        qDebug() << "EXPORTED JSON:" << exportedJson;
        
        QCOMPARE(exportedJson, importedJson);
    }
};

QTEST_MAIN(TestAdminSelfHostedExport)
#include "testAdminSelfHostedExport.moc"
