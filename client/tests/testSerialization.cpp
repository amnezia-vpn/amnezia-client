#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "core/controllers/coreController.h"
#include "core/models/serverConfig.h"
#include "core/utils/serialization/serialization.h"
#include "core/utils/utilities.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

class TestSerialization : public QObject
{
    Q_OBJECT

private:
    CoreController *m_coreController;
    SecureQSettings *m_settings;

    QString getSHAdminConfig()
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        return env.value("TEST_SELF_HOSTED_CONFIG");
    }

    QJsonObject extractXrayConfig(const QString &data, ConfigTypes configType, const QString &description = "") const
    {
        QJsonParseError parserErr;
        QJsonDocument jsonConf = QJsonDocument::fromJson(data.toLocal8Bit(), &parserErr);

        QJsonObject xrayVpnConfig;
        xrayVpnConfig[configKey::config] = jsonConf.toJson().constData();
        QJsonObject lastConfig;
        lastConfig[configKey::lastConfig] = jsonConf.toJson().constData();
        lastConfig[configKey::isThirdPartyConfig] = true;

        QJsonObject containers;
        if (configType == ConfigTypes::ShadowSocks) {
            containers.insert(configKey::ssxray, QJsonValue(lastConfig));
            containers.insert(configKey::container, QJsonValue(configKey::amneziaSsxray));
        } else {
            containers.insert(configKey::container, QJsonValue(configKey::amneziaXray));
            containers.insert(configKey::xray, QJsonValue(lastConfig));
        }

        QJsonArray arr;
        arr.push_back(containers);

        QString hostName;

        const static QRegularExpression hostNameRegExp("\"address\":\\s*\"([^\"]+)");
        QRegularExpressionMatch hostNameMatch = hostNameRegExp.match(data);
        if (hostNameMatch.hasMatch()) {
            hostName = hostNameMatch.captured(1);
        }

        QJsonObject config;
        config[configKey::containers] = arr;
        config[configKey::defaultContainer] =
                (configType == ConfigTypes::ShadowSocks) ? configKey::amneziaSsxray : configKey::amneziaXray;
        if (description.isEmpty()) {
            config[configKey::description] = m_coreController->m_appSettingsRepository->nextAvailableServerName();
        } else {
            config[configKey::description] = description;
        }
        config[configKey::hostName] = hostName;

        return config;
    }

private slots:
    void initTestCase()
    {
        QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);

        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);

        m_coreController = new CoreController(vpnConnection, m_settings, nullptr, this);

        QString vpnKey = getSHAdminConfig();
        QJsonObject importedConfig = m_coreController->m_importCoreController->extractConfigFromData(vpnKey).config;

        m_coreController->m_importCoreController->importConfig(importedConfig);

        qDebug() << "SELF-HOSTED ADMIN SERVER IMPORTED\n";
    }

    void cleanupTestCase()
    {
        int serverIndex = m_coreController->m_serversRepository->defaultServerIndex();
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

    void testVless()
    {
        int serverIndex = m_coreController->m_serversRepository->defaultServerIndex();

        QString clientName = "Serialization Test Client";

        auto exportResult = m_coreController->m_exportController->generateXrayConfig(serverIndex, clientName);

        ImportController::ImportResult importResult;

        QString config = exportResult.config;
        QString prefix;
        QString errormsg;
        ConfigTypes configType = ConfigTypes::Invalid;

        if (config.startsWith("vless://")) {
            configType = ConfigTypes::Xray;
            importResult.config = extractXrayConfig(Utils::JsonToString(serialization::vless::Deserialize(config, &prefix, &errormsg),
                                                    QJsonDocument::JsonFormat::Compact), configType, prefix);
            QVERIFY(!importResult.config.empty(), "Config shouldn't be empty");
        } else {
            QSKIP("Config not starts with vless://");
        }

        QCOMPARE(importResult.config, exportResult.config);
    }

    void testVmessNew()
    {
        QSKIP("test not completed");
    }

    void testVmess()
    {
        QSKIP("test not completed");
    }

    void testTrojan()
    {
        QSKIP("test not completed");
    }

    void testSS()
    {
        QSKIP("test not completed");
    }

    void testSSd()
    {
        QSKIP("test not completed");
    }
};

QTEST_MAIN(TestSerialization)
#include "testSerialization.moc"
