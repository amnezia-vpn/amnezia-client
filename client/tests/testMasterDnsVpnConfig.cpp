// Pure-model tests for MasterDnsVpnProtocolConfig + MasterDnsVpnConfigModel.
// No SSH / privileged service / Qt event loop required — exercises only the
// JSON round-trip and the operator-side defaulting / staleness logic.

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include "core/models/protocols/masterDnsVpnProtocolConfig.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "ui/models/protocols/masterDnsVpnConfigModel.h"

using namespace amnezia;

class TestMasterDnsVpnConfig : public QObject
{
    Q_OBJECT

private slots:
    // ---- MasterDnsVpnServerConfig round-trip ----

    void serverConfigRoundTripPreservesEveryField()
    {
        MasterDnsVpnServerConfig config;
        QJsonArray domains;
        domains.append(QStringLiteral("v.example.com"));
        domains.append(QStringLiteral("tunnel.example.com"));
        config.domains = domains;
        config.port = QStringLiteral("53");
        config.bind = QStringLiteral("0.0.0.0");
        config.encryptionMethod = protocols::masterDnsVpn::encryptionMethodAes256Gcm;
        config.encryptionKey = QStringLiteral("0123456789abcdef0123456789abcdef");
        config.protocolType = QStringLiteral("SOCKS5");
        QJsonArray upstreams;
        upstreams.append(QStringLiteral("1.1.1.1:53"));
        upstreams.append(QStringLiteral("1.0.0.1:53"));
        config.dnsUpstreamServers = upstreams;
        config.useExternalSocks5 = true;
        config.socks5Auth = true;
        config.socks5User = QStringLiteral("upstream-user");
        config.socks5Pass = QStringLiteral("upstream-pass");
        config.forwardIp = QStringLiteral("10.0.0.5");
        config.forwardPort = 1080;
        QJsonObject extra;
        extra.insert(QStringLiteral("MAX_PACKET_SIZE"), 65500);
        config.additionalConfig = extra;
        config.isThirdPartyConfig = true;

        const QJsonObject json = config.toJson();
        const MasterDnsVpnServerConfig parsed = MasterDnsVpnServerConfig::fromJson(json);

        QCOMPARE(parsed.domains, config.domains);
        QCOMPARE(parsed.port, config.port);
        QCOMPARE(parsed.bind, config.bind);
        QCOMPARE(parsed.encryptionMethod, config.encryptionMethod);
        QCOMPARE(parsed.encryptionKey, config.encryptionKey);
        QCOMPARE(parsed.protocolType, config.protocolType);
        QCOMPARE(parsed.dnsUpstreamServers, config.dnsUpstreamServers);
        QCOMPARE(parsed.useExternalSocks5, config.useExternalSocks5);
        QCOMPARE(parsed.socks5Auth, config.socks5Auth);
        QCOMPARE(parsed.socks5User, config.socks5User);
        QCOMPARE(parsed.socks5Pass, config.socks5Pass);
        QCOMPARE(parsed.forwardIp, config.forwardIp);
        QCOMPARE(parsed.forwardPort, config.forwardPort);
        QCOMPARE(parsed.additionalConfig, config.additionalConfig);
        QCOMPARE(parsed.isThirdPartyConfig, config.isThirdPartyConfig);
    }

    void serverConfigToJsonOmitsEmptyOptionalFields()
    {
        MasterDnsVpnServerConfig config;
        // Leave everything except encryptionMethod blank.
        config.encryptionMethod = protocols::masterDnsVpn::encryptionMethodXor;

        const QJsonObject json = config.toJson();
        QVERIFY(!json.contains(configKey::mdvDomains));
        QVERIFY(!json.contains(configKey::port));
        QVERIFY(!json.contains(configKey::mdvForwardIp));
        QVERIFY(!json.contains(configKey::mdvForwardPort));
        QVERIFY(!json.contains(configKey::mdvSocks5User));
        QVERIFY(!json.contains(configKey::isThirdPartyConfig));
        // encryptionMethod is always emitted (it's a primitive int with a
        // meaningful zero value -- "no encryption").
        QVERIFY(json.contains(configKey::mdvEncryptionMethod));
    }

    // ---- MasterDnsVpnClientConfig round-trip ----

    void clientConfigRoundTripPreservesStructuredFields()
    {
        MasterDnsVpnClientConfig config;
        config.listenPort = QStringLiteral("18000");
        config.socks5User = QStringLiteral("alice");
        config.socks5Pass = QStringLiteral("xyzzy");
        QJsonArray resolvers;
        resolvers.append(QStringLiteral("8.8.8.8"));
        resolvers.append(QStringLiteral("1.1.1.1:5353"));
        resolvers.append(QStringLiteral("[2001:4860:4860::8888]:53"));
        config.resolvers = resolvers;
        config.balancingStrategy = 5;
        config.packetDuplication = 3;
        config.setupPacketDuplication = 4;
        config.uploadCompression = 1; // ZSTD
        config.downloadCompression = 0;
        config.id = QStringLiteral("client-uuid-123");

        const QJsonObject json = config.toJson();
        const MasterDnsVpnClientConfig parsed = MasterDnsVpnClientConfig::fromJson(json);

        QCOMPARE(parsed.listenPort, config.listenPort);
        QCOMPARE(parsed.socks5User, config.socks5User);
        QCOMPARE(parsed.socks5Pass, config.socks5Pass);
        QCOMPARE(parsed.resolvers, config.resolvers);
        QCOMPARE(parsed.balancingStrategy, config.balancingStrategy);
        QCOMPARE(parsed.packetDuplication, config.packetDuplication);
        QCOMPARE(parsed.setupPacketDuplication, config.setupPacketDuplication);
        QCOMPARE(parsed.uploadCompression, config.uploadCompression);
        QCOMPARE(parsed.downloadCompression, config.downloadCompression);
        QCOMPARE(parsed.id, config.id);
    }

    void clientConfigDefaultsApplyWhenJsonIsBare()
    {
        const MasterDnsVpnClientConfig parsed =
                MasterDnsVpnClientConfig::fromJson(QJsonObject {});

        // Defaults should mirror the upstream sample so a config that only
        // specifies the inbound bits still produces a working tunnel.
        QCOMPARE(parsed.balancingStrategy, 5);
        QCOMPARE(parsed.packetDuplication, 3);
        QCOMPARE(parsed.setupPacketDuplication, 4);
        QCOMPARE(parsed.uploadCompression, 0);
        QCOMPARE(parsed.downloadCompression, 0);
    }

    // ---- Wrapper behaviour ----

    void protocolConfigWrapsClientConfigUnderLastConfigKey()
    {
        MasterDnsVpnProtocolConfig wrapper;
        wrapper.serverConfig.domains = QJsonArray { QStringLiteral("v.example.com") };
        wrapper.serverConfig.encryptionKey = QStringLiteral("deadbeefcafebabe1234567890abcdef");
        wrapper.serverConfig.encryptionMethod =
                protocols::masterDnsVpn::encryptionMethodAes128Gcm;

        MasterDnsVpnClientConfig client;
        client.listenPort = QStringLiteral("19001");
        client.resolvers = QJsonArray { QStringLiteral("9.9.9.9") };
        wrapper.setClientConfig(client);

        const QJsonObject json = wrapper.toJson();
        QVERIFY(json.contains(configKey::lastConfig));
        const QString lastCfg = json.value(configKey::lastConfig).toString();
        QVERIFY(!lastCfg.isEmpty());
        QJsonDocument lastDoc = QJsonDocument::fromJson(lastCfg.toUtf8());
        QVERIFY(lastDoc.isObject());
        QCOMPARE(lastDoc.object().value(configKey::mdvListenPort).toString(),
                 client.listenPort);
        QCOMPARE(lastDoc.object().value(configKey::mdvResolvers).toArray(),
                 client.resolvers);

        // Round-trip back through fromJson — clientConfig should reappear.
        const MasterDnsVpnProtocolConfig parsed = MasterDnsVpnProtocolConfig::fromJson(json);
        QVERIFY(parsed.hasClientConfig());
        QCOMPARE(parsed.clientConfig->listenPort, client.listenPort);
        QCOMPARE(parsed.clientConfig->resolvers, client.resolvers);
    }

    void clearClientConfigDropsTheClientSlot()
    {
        MasterDnsVpnProtocolConfig wrapper;
        MasterDnsVpnClientConfig client;
        client.listenPort = QStringLiteral("18000");
        wrapper.setClientConfig(client);
        QVERIFY(wrapper.hasClientConfig());
        wrapper.clearClientConfig();
        QVERIFY(!wrapper.hasClientConfig());
    }

    // ---- ConfigModel staleness rules ----

    void configModelDefaultsFillBlankServerFields()
    {
        MasterDnsVpnConfigModel model;
        MasterDnsVpnProtocolConfig blank;
        // Domains intentionally empty -- the model still applies port / bind /
        // encryptionMethod / protocolType defaults for a fresh container.
        model.updateModel(amnezia::DockerContainer::MasterDnsVpn, blank);

        const MasterDnsVpnProtocolConfig out = model.getProtocolConfig();
        QCOMPARE(out.serverConfig.port,
                 QString::fromLatin1(protocols::masterDnsVpn::defaultPort));
        QCOMPARE(out.serverConfig.bind, QStringLiteral("0.0.0.0"));
        QCOMPARE(out.serverConfig.encryptionMethod,
                 protocols::masterDnsVpn::defaultEncryptionMethod);
        QCOMPARE(out.serverConfig.protocolType, QStringLiteral("SOCKS5"));
    }
};

QTEST_MAIN(TestMasterDnsVpnConfig)
#include "testMasterDnsVpnConfig.moc"
