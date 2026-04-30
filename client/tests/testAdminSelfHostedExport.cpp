#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <QUuid>
#include <QSignalSpy>

#include "core/controllers/coreController.h"
#include "core/models/serverConfig.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

class TestAdminSelfHostedExport : public QObject
{
    Q_OBJECT

private:
    CoreController* m_coreController;
    SecureQSettings* m_settings;

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
    }

    void testAdminSelfHostedExport() {
        QString vpnKey = "vpn://AAABTXjarZIxT8MwEIX_Cro5jbDjQunKUhhYyoZQZZKjRGpsy3baQtT_zp2bJh3oACLLPfvz3bOe00FpTdS1QR9g_tKB3q1h3sFCwBzEdf9N5ElBBgtJqBiQOkcFoemAbs6RInQ7oNkZemAvrrKvRV9VX6fH-lhSVSwavU9GSdcmXZX0UqSbseJRMqlioDxuSsJZH1mKWTrhvI22tJvVljKoLU-TtB3aN4NxpavKYwhpSD7LRc4t0WsTeMwqNRNsKweHbAyTtnRj8KvWE0pUEut-hNah2TpDM0-Kwu8vKMSd-ttFLrntao_rVvuKWkc9OnIk4n8t915_Ulcqo5FSxa9tYsk2rxlU-K7bTby_lDWfCKWvXTy-5jOGeLVET-9L7MOG-KQbJEBx57jXjdtgXtqG_wUdws5yJhCpa1iefhopM2gD-n4An-ElHL4BvzD6nw";
        
        QSignalSpy importFinishedSpy(m_coreController->m_importCoreController, &ImportController::importFinished);
        QSignalSpy defaultServerChangedSpy(m_coreController->m_serversRepository, &SecureServersRepository::defaultServerChanged);
        
        qDebug() << "IMPORTED KEY:" << vpnKey;
        
        auto importResult = m_coreController->m_importCoreController->extractConfigFromData(vpnKey);
        
        QVERIFY2(importResult.errorCode == ErrorCode::NoError, "Import should succeed");
        QVERIFY2(!importResult.config.isEmpty(), "Config should not be empty");

        QJsonObject importedConfig = importResult.config;

        m_coreController->m_importCoreController->importConfig(importedConfig);
        
        QVERIFY2(importFinishedSpy.count() == 1, "importFinished signal should be emitted");
        QVERIFY2(defaultServerChangedSpy.count() == 0, "defaultServerChanged signal should NOT be emitted (default is already 0)");
        QVERIFY2(m_coreController->m_serversRepository->serversCount() > 0, "Server should be added");

        int serverIndex = m_coreController->m_serversRepository->defaultServerIndex();
        auto exportResult = m_coreController->m_exportController->generateFullAccessConfig(serverIndex);
        
        QVERIFY2(exportResult.errorCode == ErrorCode::NoError, "Export should succeed");
        QVERIFY2(!exportResult.config.isEmpty(), "Exported config should not be empty");

        qDebug() << "EXPORTED KEY:" << exportResult.config;

        QJsonObject exportedConfig = decodeVpnKey(exportResult.config);
        
        auto importResult2 = m_coreController->m_importCoreController->extractConfigFromData(exportResult.config);
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
