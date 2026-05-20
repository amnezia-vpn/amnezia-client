#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <QUuid>
#include <QSignalSpy>
#include <QTest>

#include "core/controllers/coreController.h"
#include "core/utils/constants/configKeys.h"
#include "vpnConnection.h"
#include "secureQSettings.h"

class TestAdminSelfHostedExport : public QObject
{
    Q_OBJECT

private:
    CoreController* m_coreController;
    SecureQSettings* m_settings;

    QString getValueFromIni(const QString &key)
    {
        QSettings settings("test_vars.ini", QSettings::IniFormat);
        return settings.value(key).toString();
    }

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
        QString vpnKey = getValueFromIni("configs/TEST_CONFIG_ANY");
        
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

        const QString serverId = m_coreController->m_serversRepository->defaultServerId();
        auto exportResult = m_coreController->m_exportController->generateFullAccessConfig(serverId);
        
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
