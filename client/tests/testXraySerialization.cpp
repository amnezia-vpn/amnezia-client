#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QUuid>
#include <QTest>

#include "utils/testCoreController.h"
#include "core/models/serverDescription.h"
#include "core/utils/serialization/serialization.h"
#include "core/utils/utilities.h"
#include "secureQSettings.h"
#include "utils/testUtils.h"
#include "vpnConnection.h"

using namespace amnezia;
using namespace amnezia::test;

class TestXraySerialization : public QObject
{
    Q_OBJECT

private:
    TestCoreController *m_coreController;
    SecureQSettings *m_settings;

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
            config[configKey::description] = m_coreController->m_serversRepository->nextAvailableServerName();
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

        m_coreController = new TestCoreController(vpnConnection, m_settings, nullptr, this);
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
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), QString{});
        }
    }

    void testVless()
    {
        const QString configData = getEnvValue("THIRD_PARTY_VLESS_IMPORT_DATA");
        if (!isEnvValueConfigured(configData)) {
            QSKIP("THIRD_PARTY_VLESS_IMPORT_DATA is not configured");
        }

        ImportController::ImportResult importResult;
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

    void testVlessLinkRoundTrip()
    {
        const QJsonObject extra {
            { "xPaddingObfsMode", true },
            { "xPaddingKey", "x_pad" },
            { "xPaddingPlacement", "cookie" },
            { "xPaddingBytes", "200-400" },
            { "xmux", QJsonObject { { "maxConnections", 4 } } },
        };
        const QString pin = QStringLiteral("0a1b2c3d4e5f60718293a4b5c6d7e8f90a1b2c3d4e5f60718293a4b5c6d7e8f9");

        serialization::VlessServerObject server;
        server.address = QStringLiteral("203.0.113.10");
        server.port = 443;
        server.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        server.flow.clear();
        server.network = QStringLiteral("xhttp");
        server.security = QStringLiteral("tls");
        server.serverName = QStringLiteral("www.example.com");
        server.fingerprint = QStringLiteral("chrome");
        server.alpn = QStringLiteral("h2,http/1.1");
        server.xhttpHost = QStringLiteral("www.example.com");
        server.xhttpPath = QStringLiteral("/p");
        server.xhttpMode = QStringLiteral("packet-up");
        server.xhttpExtra = QString::fromUtf8(QJsonDocument(extra).toJson(QJsonDocument::Compact));
        server.tlsPin = pin;

        const QString link = serialization::vless::Serialize(server, QStringLiteral("rt"));
        QVERIFY2(link.startsWith(QStringLiteral("vless://")), qPrintable(link));

        QString alias;
        QString error;
        const QJsonObject root = serialization::vless::Deserialize(link, &alias, &error);
        QVERIFY2(!root.isEmpty(), qPrintable(error + QStringLiteral(" ") + link));

        const QJsonObject stream =
                root.value("outbounds").toArray().first().toObject().value("streamSettings").toObject();
        QCOMPARE(stream.value("network").toString(), QStringLiteral("xhttp"));
        QCOMPARE(stream.value("security").toString(), QStringLiteral("tls"));

        const QJsonObject xhttp = stream.value("xhttpSettings").toObject();
        QCOMPARE(xhttp.value("host").toString(), server.xhttpHost);
        QCOMPARE(xhttp.value("path").toString(), server.xhttpPath);
        QCOMPARE(xhttp.value("mode").toString(), server.xhttpMode);
        QCOMPARE(xhttp.value("extra").toObject(), extra);

        const QJsonObject tls = stream.value("tlsSettings").toObject();
        QCOMPARE(tls.value("pinnedPeerCertSha256").toString(), pin);
        QCOMPARE(tls.value("serverName").toString(), server.serverName);
        QCOMPARE(tls.value("fingerprint").toString(), server.fingerprint);
        QCOMPARE(tls.value("alpn").toArray(), QJsonArray({ "h2", "http/1.1" }));
    }

    void testVmessNew()
    {
        const QString configData = getEnvValue("THIRD_PARTY_VMESS_NEW_IMPORT_DATA");
        if (!isEnvValueConfigured(configData)) {
            QSKIP("THIRD_PARTY_VMESS_NEW_IMPORT_DATA is not configured");
        }

        QString clientName = "Test Client (vmess_new deserialization)";

        ImportController::ImportResult importResult;
        
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
        const QString configData = getEnvValue("THIRD_PARTY_VMESS_IMPORT_DATA");
        if (!isEnvValueConfigured(configData)) {
            QSKIP("THIRD_PARTY_VMESS_IMPORT_DATA is not configured");
        }

        QString clientName = "Test Client (vmess deserialization)";

        ImportController::ImportResult importResult;

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
        const QString configData = getEnvValue("THIRD_PARTY_TROJAN_IMPORT_DATA");
        if (!isEnvValueConfigured(configData)) {
            QSKIP("THIRD_PARTY_TROJAN_IMPORT_DATA is not configured");
        }

        QString clientName = "Test Client (trojan deserialization)";

        ImportController::ImportResult importResult;

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
        const QString configData = getEnvValue("THIRD_PARTY_SHADOWSOCKS_IMPORT_DATA");
        if (!isEnvValueConfigured(configData)) {
            QSKIP("THIRD_PARTY_SHADOWSOCKS_IMPORT_DATA is not configured");
        }

        QString clientName = "Test Client (ss deserialization)";

        ImportController::ImportResult importResult;

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
        const QString configData = getEnvValue("THIRD_PARTY_SHADOWSOCKS_SUBSCRIPTION_IMPORT_DATA");
        if (!isEnvValueConfigured(configData)) {
            QSKIP("THIRD_PARTY_SHADOWSOCKS_SUBSCRIPTION_IMPORT_DATA is not configured");
        }

        QString clientName = "Test Client (ssd deserialization)";

        ImportController::ImportResult importResult;

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
