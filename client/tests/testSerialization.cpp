#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QUuid>
#include <QTest>

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

    QString getValueFromIni(const QString &key)
    {
        QSettings settings("test_vars.ini", QSettings::IniFormat);
        return settings.value(key).toString();
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

    void testVless()
    {
        int serverIndex = m_coreController->m_serversRepository->defaultServerIndex();

        QString clientName = "Test Client (vless (de)serialization)";

        ExportController::ExportResult exportResult = m_coreController->m_exportController->generateXrayConfig(serverIndex, clientName);

        ImportController::ImportResult importResult;

        QString config = exportResult.config;
        QString prefix;
        QString errormsg;
        ConfigTypes configType = ConfigTypes::Invalid;

        if (config.startsWith("vless://")) {
            configType = ConfigTypes::Xray;
            importResult.config = extractXrayConfig(
                Utils::JsonToString(serialization::vless::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact),
                configType, prefix);
            QVERIFY2(!importResult.config.empty(), "Config shouldn't be empty");
        } else {
            QSKIP("Config not starts with vless://");
        }

        QCOMPARE(importResult.config, config);
    }

    void testVmessNew()
    {
        QString clientName = "Test Client (vmess_new deserialization)";

        ImportController::ImportResult importResult;
        
        m_coreController->m_importController->extractConfigFromData(getValueFromIni("configs/TEST_CONFIG_VMESS_NEW"));

        QString config = m_coreController->m_importController->getConfig();
        QString prefix;
        QString errormsg;
        ConfigTypes configType = ConfigTypes::Invalid;

        if (config.startsWith("vmess://") && config.contains("@")) {
            configType = ConfigTypes::Xray;
            importResult.config = extractXrayConfig(
                Utils::JsonToString(serialization::vmess_new::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact),
                configType, prefix);
            QVERIFY2(!importResult.config.empty(), "Config shouldn't be empty");
        } else {
            QSKIP("Config not starts with vmess:// or not contain @");
        }

        QCOMPARE(importResult.config, config);
    }

    void testVmess()
    {
        QString clientName = "Test Client (vmess deserialization)";

        ImportController::ImportResult importResult;

        m_coreController->m_importController->extractConfigFromData(getValueFromIni("configs/TEST_CONFIG_VMESS"));

        QString config = m_coreController->m_importController->getConfig();
        QString prefix;
        QString errormsg;
        ConfigTypes configType = ConfigTypes::Invalid;

        if (config.startsWith("vmess://")) {
            configType = ConfigTypes::Xray;
            importResult.config = extractXrayConfig(
                Utils::JsonToString(serialization::vmess::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact),
                configType, prefix);
            QVERIFY2(!importResult.config.empty(), "Config shouldn't be empty");
        } else {
            QSKIP("Config not starts with vmess://");
        }

        QCOMPARE(importResult.config, config);
    }

    void testTrojan()
    {
        QString clientName = "Test Client (trojan deserialization)";

        ImportController::ImportResult importResult;

        m_coreController->m_importController->extractConfigFromData(getValueFromIni("configs/TEST_CONFIG_TROJAN"));

        QString config = m_coreController->m_importController->getConfig();
        QString prefix;
        QString errormsg;
        ConfigTypes configType = ConfigTypes::Invalid;

        if (config.startsWith("trojan://")) {
            configType = ConfigTypes::Xray;
            importResult.config = extractXrayConfig(
                Utils::JsonToString(serialization::trojan::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact),
                configType, prefix);
            QVERIFY2(!importResult.config.empty(), "Config shouldn't be empty");
        } else {
            QSKIP("Config not starts with trojan://");
        }

        QCOMPARE(importResult.config, config);
    }

    void testSS()
    {
        QString clientName = "Test Client (ss deserialization)";

        ImportController::ImportResult importResult;

        m_coreController->m_importController->extractConfigFromData(getValueFromIni("configs/TEST_CONFIG_SS"));

        QString config = m_coreController->m_importController->getConfig();
        QString prefix;
        QString errormsg;
        ConfigTypes configType = ConfigTypes::Invalid;

        if (config.startsWith("ss://") && !config.contains("plugin=")) {
            configType = ConfigTypes::ShadowSocks;
            importResult.config = extractXrayConfig(
                Utils::JsonToString(serialization::ss::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact),
                configType, prefix);
            QVERIFY2(!importResult.config.empty(), "Config shouldn't be empty");
        } else {
            QSKIP("Config not starts with ss:// or contain plugin=");
        }

        QCOMPARE(importResult.config, config);
    }

    void testSSd()
    {
        QString clientName = "Test Client (ssd deserialization)";

        ImportController::ImportResult importResult;

        m_coreController->m_importController->extractConfigFromData(getValueFromIni("configs/TEST_CONFIG_SSD"));

        QString config = m_coreController->m_importController->getConfig();
        QString prefix;
        QString errormsg;
        ConfigTypes configType = ConfigTypes::Invalid;

        if (config.startsWith("ssd://")) {
            QStringList tmp;
            QList<std::pair<QString, QJsonObject>> servers = serialization::ssd::Deserialize(config, &prefix, &tmp);
            configType = ConfigTypes::ShadowSocks;
            // Took only first config from list
            if (!servers.isEmpty()) {
                importResult.config = extractXrayConfig(servers.first().first, configType);
            }
            if (!importResult.config.empty()) {
                importResult.configType = configType;
            }
            QVERIFY2(!importResult.config.empty(), "Config shouldn't be empty");
        } else {
            QSKIP("Config not starts with ssd://");
        }

        QCOMPARE(importResult.config, config);
    }
};

QTEST_MAIN(TestSerialization)
#include "testSerialization.moc"
