#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QUuid>
#include <QTest>

#include "utils/testCoreController.h"
#include "core/configurators/xrayConfigurator.h"
#include "core/installers/xrayInstaller.h"
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

    void testReadServerConfigOverridesCachedSettings()
    {
        const QJsonObject realitySettings {
            { "dest", "www.googletagmanager.com:443" },
            { "fingerprint", "firefox" },
            { "privateKey", "private" },
            { "serverNames", QJsonArray { "www.googletagmanager.com" } },
            { "shortIds", QJsonArray { "abcd" } }
        };
        const QJsonObject inbound {
            { "port", 8443 },
            { "protocol", "vless" },
            { "settings", QJsonObject { { "clients", QJsonArray { QJsonObject { { "id", "c1" }, { "flow", "xtls-rprx-vision" } } } },
                                        { "decryption", "none" } } },
            { "streamSettings", QJsonObject { { "network", "tcp" }, { "security", "reality" }, { "realitySettings", realitySettings } } }
        };
        const QJsonObject serverConfig { { "inbounds", QJsonArray { inbound } } };

        XrayServerConfig srv;
        srv.port = "443";
        srv.transport = "raw";
        srv.security = "";
        srv.flow = "";
        srv.site = "www.example.com";

        QCOMPARE(XrayInstaller::readServerConfig(serverConfig, srv), ErrorCode::NoError);
        QCOMPARE(srv.port, QString("8443"));
        QCOMPARE(srv.transport, QString("raw"));
        QCOMPARE(srv.security, QString("reality"));
        QCOMPARE(srv.flow, QString("xtls-rprx-vision"));
        QCOMPARE(srv.sni, QString("www.googletagmanager.com"));
        QCOMPARE(srv.site, QString("www.googletagmanager.com"));
        QCOMPARE(srv.fingerprint, QString("firefox"));
    }

    void testMergeChangedSettingsFollowsServer()
    {
        XrayServerConfig remote;
        remote.security = "reality";
        remote.flow = "xtls-rprx-vision";
        remote.transport = "raw";
        remote.sni = "www.googletagmanager.com";
        remote.site = "www.googletagmanager.com";
        remote.fingerprint = "chrome";

        XrayServerConfig oldSrv;
        oldSrv.security = "";
        oldSrv.transport = "raw";
        oldSrv.sni = "www.example.com";
        oldSrv.site = "www.example.com";
        oldSrv.fingerprint = "chrome";

        XrayServerConfig newSrv = oldSrv;
        newSrv.fingerprint = "firefox";

        XrayServerConfig target = XrayConfigurator::mergeChangedSettings(remote, oldSrv, newSrv);
        QCOMPARE(target.fingerprint, QString("firefox"));
        QCOMPARE(target.security, QString("reality"));
        QCOMPARE(target.flow, QString("xtls-rprx-vision"));
        QCOMPARE(target.sni, QString("www.googletagmanager.com"));

        newSrv = oldSrv;
        newSrv.sni = "www.microsoft.com";
        target = XrayConfigurator::mergeChangedSettings(remote, oldSrv, newSrv);
        QCOMPARE(target.sni, QString("www.microsoft.com"));
        QCOMPARE(target.site, QString("www.microsoft.com"));
    }

    void testPatchServerConfigKeepsIdentity()
    {
        const QJsonObject reality {
            { "dest", "www.googletagmanager.com:443" },
            { "fingerprint", "chrome" },
            { "privateKey", "private-key" },
            { "serverNames", QJsonArray { "www.googletagmanager.com", "extra.example.com" } },
            { "shortIds", QJsonArray { "aaaa", "bbbb" } }
        };
        const QJsonArray clients {
            QJsonObject { { "id", "admin" }, { "flow", "xtls-rprx-vision" } },
            QJsonObject { { "id", "friend" } }
        };
        const QJsonObject outbound { { "protocol", "freedom" }, { "settings", QJsonObject { { "finalRules", QJsonArray {} } } } };
        const QJsonObject serverConfig {
            { "log", QJsonObject { { "loglevel", "error" } } },
            { "routing", QJsonObject { { "rules", QJsonArray {} } } },
            { "inbounds", QJsonArray { QJsonObject {
                { "port", 443 },
                { "protocol", "vless" },
                { "settings", QJsonObject { { "clients", clients }, { "decryption", "none" } } },
                { "streamSettings", QJsonObject { { "network", "tcp" }, { "security", "reality" }, { "realitySettings", reality },
                                                  { "sockopt", QJsonObject { { "tcpFastOpen", true } } } } }
            } } },
            { "outbounds", QJsonArray { outbound } }
        };

        XrayServerConfig current;
        QCOMPARE(XrayInstaller::readServerConfig(serverConfig, current), ErrorCode::NoError);

        XrayConfigurator configurator(nullptr);

        // Fingerprint only: nothing but the fingerprint changes on the server
        XrayServerConfig target = current;
        target.fingerprint = "firefox";
        QJsonObject patched = configurator.patchServerConfig(serverConfig, current, target, "unused", "unused");
        QJsonObject inbound = patched["inbounds"].toArray()[0].toObject();
        QJsonObject stream = inbound["streamSettings"].toObject();
        QJsonObject patchedReality = stream["realitySettings"].toObject();
        QCOMPARE(patchedReality["fingerprint"].toString(), QString("firefox"));
        QCOMPARE(patchedReality["privateKey"].toString(), QString("private-key"));
        QCOMPARE(patchedReality["shortIds"].toArray(), reality["shortIds"].toArray());
        QCOMPARE(patchedReality["serverNames"].toArray(), reality["serverNames"].toArray());
        QCOMPARE(patchedReality["dest"].toString(), QString("www.googletagmanager.com:443"));
        QCOMPARE(inbound["settings"].toObject()["clients"].toArray(), clients);
        QCOMPARE(stream["sockopt"].toObject(), (QJsonObject { { "tcpFastOpen", true } }));
        QCOMPARE(patched["routing"].toObject(), serverConfig["routing"].toObject());
        QCOMPARE(patched["outbounds"].toArray(), QJsonArray { outbound });

        // SNI change: the new name leads, extra names stay, the destination follows
        target = current;
        target.sni = "www.microsoft.com";
        target.site = "www.microsoft.com";
        patched = configurator.patchServerConfig(serverConfig, current, target, "unused", "unused");
        patchedReality = patched["inbounds"].toArray()[0].toObject()["streamSettings"].toObject()["realitySettings"].toObject();
        QCOMPARE(patchedReality["serverNames"].toArray(), (QJsonArray { "www.microsoft.com", "extra.example.com" }));
        QCOMPARE(patchedReality["dest"].toString(), QString("www.microsoft.com:443"));
        QCOMPARE(patchedReality["privateKey"].toString(), QString("private-key"));

        // Switching to XHTTP drops Vision from every client but keeps the clients and Reality keys
        target = current;
        target.transport = "xhttp";
        patched = configurator.patchServerConfig(serverConfig, current, target, "unused", "unused");
        inbound = patched["inbounds"].toArray()[0].toObject();
        stream = inbound["streamSettings"].toObject();
        QCOMPARE(stream["network"].toString(), QString("xhttp"));
        QVERIFY(stream.contains("xhttpSettings"));
        QCOMPARE(stream["realitySettings"].toObject()["privateKey"].toString(), QString("private-key"));
        const QJsonArray patchedClients = inbound["settings"].toObject()["clients"].toArray();
        QCOMPARE(patchedClients.size(), 2);
        QCOMPARE(patchedClients[0].toObject(), (QJsonObject { { "id", "admin" } }));
        QCOMPARE(patchedClients[1].toObject(), (QJsonObject { { "id", "friend" } }));
    }
};

QTEST_MAIN(TestXraySerialization)
#include "testXraySerialization.moc"
