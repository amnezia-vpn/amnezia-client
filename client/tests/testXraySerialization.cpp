#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QUuid>
#include <QTest>

#include "core/controllers/coreController.h"
#include "core/models/serverDescription.h"
#include "core/utils/serialization/serialization.h"
#include "core/utils/utilities.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

class TestXraySerialization : public QObject
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
    }

    void cleanupTestCase()
    {
        m_settings->clearSettings();
        delete m_coreController;
        delete m_settings;
    }

    void init()
    {
        m_settings->clearSettings();
        if (m_coreController->m_serversModel) {
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), -1);
        }
    }

    void testVless()
    {
        ImportController::ImportResult importResult;

        QString configData = getValueFromIni("configs/thirdPartyVlessImportData");
        m_coreController->m_importCoreController->extractConfigFromData(configData);

        QString config = configData;
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
        
        QString configData = getValueFromIni("configs/thirdPartyVmessNewImportData");
        m_coreController->m_importCoreController->extractConfigFromData(configData);

        QString config = configData;
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

        QString configData = getValueFromIni("configs/thirdPartyVmessImportData");
        m_coreController->m_importCoreController->extractConfigFromData(configData);

        QString config = configData;
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

        QString configData = getValueFromIni("configs/thirdPartyTrojanImportData");
        m_coreController->m_importCoreController->extractConfigFromData(configData);

        QString config = configData;
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

        QString configData = getValueFromIni("configs/thirdPartyShadowsocksImportData");
        m_coreController->m_importCoreController->extractConfigFromData(configData);

        QString config = configData;
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

        QString configData = getValueFromIni("configs/thirdPartyShadowsocksSubscriptionImportData");
        m_coreController->m_importCoreController->extractConfigFromData(configData);

        QString config = configData;
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

QTEST_MAIN(TestXraySerialization)
#include "testXraySerialization.moc"
