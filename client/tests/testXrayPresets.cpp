#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include "core/models/protocols/xrayInbound.h"
#include "core/models/protocols/xrayInboundValidator.h"
#include "core/models/protocols/xrayPresets.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;

namespace
{
    const QString site = QStringLiteral("www.apple.com");
    const QString port = QStringLiteral("443");

    xrayInbound::Secrets secrets()
    {
        xrayInbound::Secrets s;
        s.clients = QJsonArray { QJsonObject { { "id", "11111111-1111-1111-1111-111111111111" } },
                                 QJsonObject { { "id", "22222222-2222-2222-2222-222222222222" }, { "flow", "old" } } };
        s.realityPrivateKey = QStringLiteral("private-key");
        s.realityShortId = QStringLiteral("abcd");
        return s;
    }

    XrayPreset builtin(const QString &id)
    {
        for (const XrayPreset &preset : xrayPresets::builtins(site, port)) {
            if (preset.id == id) {
                return preset;
            }
        }
        return {};
    }
}

class TestXrayPresets : public QObject
{
    Q_OBJECT

private slots:
    void builtinsAreComplete()
    {
        const QList<XrayPreset> presets = xrayPresets::builtins(site, port);
        QCOMPARE(presets.size(), 5);
        int recommended = 0;
        for (const XrayPreset &preset : presets) {
            QVERIFY(preset.builtin);
            QVERIFY(!preset.name.isEmpty());
            QVERIFY(!preset.description.isEmpty());
            QVERIFY(validateXrayInbound(preset.inbound).isValid());
            QVERIFY(preset.inbound.contains(QLatin1String(xrayInbound::autoValue)));
            recommended += preset.recommended ? 1 : 0;
        }
        QCOMPARE(recommended, 1);
        QCOMPARE(presets.first().id, QString::fromLatin1(xrayPresets::defaultPresetId));
    }

    void builtinTemplateHasNoSecrets()
    {
        const QJsonObject inbound = xrayInbound::parse(builtin("reality-raw-vision").inbound);
        const QJsonObject reality = inbound.value("streamSettings").toObject().value("realitySettings").toObject();
        QCOMPARE(reality.value("privateKey").toString(), QString::fromLatin1(xrayInbound::autoValue));
        QCOMPARE(reality.value("shortIds").toArray().first().toString(), QString::fromLatin1(xrayInbound::autoValue));
        const QJsonArray clients = inbound.value("settings").toObject().value("clients").toArray();
        QCOMPARE(clients.size(), 1);
        QCOMPARE(clients.first().toObject().value("id").toString(), QString::fromLatin1(xrayInbound::autoValue));
        QCOMPARE(clients.first().toObject().value("flow").toString(), QStringLiteral("xtls-rprx-vision"));
    }

    void toServerJsonFillsEveryAuto_data()
    {
        QTest::addColumn<QString>("id");
        for (const XrayPreset &preset : xrayPresets::builtins(site, port)) {
            QTest::newRow(qPrintable(preset.id)) << preset.id;
        }
    }

    void toServerJsonFillsEveryAuto()
    {
        QFETCH(QString, id);
        const QJsonObject inboundTemplate = xrayInbound::parse(builtin(id).inbound);
        const QJsonObject serverJson = xrayInbound::toServerJson(inboundTemplate, secrets());
        const QString text = QString::fromUtf8(QJsonDocument(serverJson).toJson(QJsonDocument::Compact));
        QVERIFY2(!text.contains(QLatin1String(xrayInbound::autoValue)), qPrintable(text));
        QVERIFY(serverJson.contains("log"));
        QCOMPARE(serverJson.value("outbounds").toArray().first().toObject().value("protocol").toString(), QStringLiteral("freedom"));

        const QJsonObject inbound = xrayInbound::inboundFromServerJson(serverJson);
        const QJsonArray clients = inbound.value("settings").toObject().value("clients").toArray();
        QCOMPARE(clients.size(), 2);
        const QString flow = xrayInbound::templateFlow(inboundTemplate);
        for (const QJsonValue &client : clients) {
            QCOMPARE(client.toObject().value("flow").toString(), flow);
        }
        const QJsonObject stream = inbound.value("streamSettings").toObject();
        if (stream.contains("realitySettings")) {
            QCOMPARE(stream.value("realitySettings").toObject().value("privateKey").toString(), QStringLiteral("private-key"));
            QCOMPARE(stream.value("realitySettings").toObject().value("shortIds").toArray().first().toString(), QStringLiteral("abcd"));
        }
        if (stream.contains("tlsSettings")) {
            const QJsonObject certificate = stream.value("tlsSettings").toObject().value("certificates").toArray().first().toObject();
            QCOMPARE(certificate.value("certificateFile").toString(), QString::fromLatin1(protocols::xray::tlsCertPath));
            QCOMPARE(certificate.value("keyFile").toString(), QString::fromLatin1(protocols::xray::tlsKeyPath));
        }

        const QString roundTrip = xrayInbound::format(xrayInbound::toTemplate(inbound));
        QCOMPARE(xrayInbound::normalized(roundTrip), xrayInbound::normalized(builtin(id).inbound));
    }

    void applyTemplateDerivesTypedSettings()
    {
        XrayServerConfig srv;
        srv.site = site;
        QVERIFY(xrayInbound::applyTemplate(builtin("tls-xhttp-padding").inbound, srv));
        QCOMPARE(srv.port, QStringLiteral("443"));
        QCOMPARE(srv.transport, QStringLiteral("xhttp"));
        QCOMPARE(srv.security, QStringLiteral("tls"));
        QVERIFY(srv.flow.isEmpty());
        QVERIFY(!srv.inboundTemplate.isEmpty());

        QString edited = builtin("reality-raw-vision").inbound;
        edited.replace(QStringLiteral("\"port\": 443"), QStringLiteral("\"port\": 8443"));
        QVERIFY(xrayInbound::applyTemplate(edited, srv));
        QCOMPARE(srv.port, QStringLiteral("8443"));
        QCOMPARE(srv.security, QStringLiteral("reality"));
        QCOMPARE(srv.site, site);

        QVERIFY(!xrayInbound::applyTemplate(QStringLiteral("{ not json"), srv));
        QCOMPARE(srv.port, QStringLiteral("8443"));
    }

    void matchBuiltin()
    {
        QCOMPARE(xrayPresets::matchBuiltin(builtin("tls-mkcp").inbound, site, port), QStringLiteral("tls-mkcp"));
        QString edited = builtin("tls-mkcp").inbound;
        edited.replace(QStringLiteral("\"port\": 443"), QStringLiteral("\"port\": 8443"));
        QVERIFY(xrayPresets::matchBuiltin(edited, site, port).isEmpty());
        QCOMPARE(xrayPresets::matchBuiltin(edited, site, QStringLiteral("8443")), QStringLiteral("tls-mkcp"));
    }

    void legacyServerRecordGetsTemplate()
    {
        XrayServerConfig legacy;
        legacy.port = QStringLiteral("444");
        legacy.transport = QStringLiteral("raw");
        legacy.security = QStringLiteral("reality");
        legacy.flow = QStringLiteral("xtls-rprx-vision");
        legacy.site = site;
        legacy.sni = site;
        legacy.alpn = QStringLiteral("h2");
        const QJsonObject stored = legacy.toJson();
        QVERIFY(!stored.contains("xray_inbound"));

        const XrayServerConfig restored = XrayProtocolConfig::fromJson(stored).serverConfig;
        QVERIFY(restored.inboundTemplate.isEmpty());

        const QString inbound = xrayInbound::templateFromServerConfig(restored);
        QVERIFY(validateXrayInbound(inbound).isValid());
        QCOMPARE(xrayPresets::matchBuiltin(inbound, site, restored.port), QString::fromLatin1(xrayPresets::defaultPresetId));
        QVERIFY(xrayPresets::matchBuiltin(inbound, site, port).isEmpty());

        XrayServerConfig withTemplate = restored;
        withTemplate.inboundTemplate = inbound;
        withTemplate.presetId = QStringLiteral("tls-mkcp");
        const XrayServerConfig again = XrayProtocolConfig::fromJson(withTemplate.toJson()).serverConfig;
        QCOMPARE(again.inboundTemplate, inbound);
        QCOMPARE(again.presetId, QStringLiteral("tls-mkcp"));
    }

    void builtinsUseServerPort()
    {
        for (const XrayPreset &preset : xrayPresets::builtins(site, QStringLiteral("8443"))) {
            QCOMPARE(xrayInbound::templatePort(xrayInbound::parse(preset.inbound)), 8443);
        }
        for (const XrayPreset &preset : xrayPresets::builtins(site, QString())) {
            QCOMPARE(xrayInbound::templatePort(xrayInbound::parse(preset.inbound)), 443);
        }
    }

    void duplicateIdsAreReplaced()
    {
        XrayServerConfig snapshot;
        snapshot.port = QStringLiteral("3443");
        snapshot.transport = QStringLiteral("mkcp");
        snapshot.site = site;
        const QJsonObject entry { { "id", "same-id" }, { "displayName", "mKCP" }, { "serverConfig", snapshot.toJson() } };
        const QList<XrayPreset> migrated =
                xrayPresets::customFromLegacySnapshots(QJsonDocument(QJsonArray { entry, entry }).toJson(QJsonDocument::Compact));
        QCOMPARE(migrated.size(), 2);
        QCOMPARE(migrated.first().id, QStringLiteral("same-id"));
        QVERIFY(migrated.last().id != QStringLiteral("same-id"));
        QVERIFY(!migrated.last().id.isEmpty());

        QList<XrayPreset> stored = xrayPresets::customFromStorage(
                xrayPresets::customToStorage({ migrated.first(), migrated.first() }));
        QCOMPARE(stored.size(), 2);
        QVERIFY(xrayPresets::ensureUniqueIds(stored));
        QVERIFY(stored.first().id != stored.last().id);
        QVERIFY(!xrayPresets::ensureUniqueIds(stored));
    }

    void legacySnapshotsMigrate()
    {
        XrayServerConfig snapshot;
        snapshot.port = QStringLiteral("8443");
        snapshot.transport = QStringLiteral("xhttp");
        snapshot.security = QStringLiteral("tls");
        snapshot.site = site;
        const QJsonArray array { QJsonObject { { "id", "snap-1" },
                                               { "displayName", "Office" },
                                               { "createdAt", "2026-11-18T14:05:00" },
                                               { "serverConfig", snapshot.toJson() } } };
        const QList<XrayPreset> migrated =
                xrayPresets::customFromLegacySnapshots(QJsonDocument(array).toJson(QJsonDocument::Compact));
        QCOMPARE(migrated.size(), 1);
        QCOMPARE(migrated.first().id, QStringLiteral("snap-1"));
        QCOMPARE(migrated.first().name, QStringLiteral("Office"));
        QVERIFY(migrated.first().createdAt.isValid());
        QVERIFY(validateXrayInbound(migrated.first().inbound).isValid());
        QCOMPARE(xrayInbound::templatePort(xrayInbound::parse(migrated.first().inbound)), 8443);
    }

    void customStorageRoundTrip()
    {
        const XrayPreset preset = xrayPresets::makeCustom(QStringLiteral("  REALITY · port 8443 "), builtin("reality-xhttp").inbound);
        QCOMPARE(preset.name, QStringLiteral("REALITY · port 8443"));
        const QList<XrayPreset> loaded = xrayPresets::customFromStorage(xrayPresets::customToStorage({ preset }));
        QCOMPARE(loaded.size(), 1);
        QCOMPARE(loaded.first().id, preset.id);
        QCOMPARE(loaded.first().name, preset.name);
        QCOMPARE(loaded.first().inbound, preset.inbound);
        QCOMPARE(loaded.first().createdAt.toString(Qt::ISODate), preset.createdAt.toString(Qt::ISODate));
    }

    void equalTemplatesMeanEqualServerSettings()
    {
        XrayServerConfig a;
        a.site = site;
        QVERIFY(xrayInbound::applyTemplate(builtin("reality-raw-vision").inbound, a));
        XrayServerConfig b = a;
        b.inboundTemplate = xrayInbound::normalized(a.inboundTemplate);
        QVERIFY(a.hasEqualServerSettings(b));
        QVERIFY(xrayInbound::applyTemplate(builtin("reality-xhttp").inbound, b));
        QVERIFY(!a.hasEqualServerSettings(b));
    }
};

QTEST_GUILESS_MAIN(TestXrayPresets)

#include "testXrayPresets.moc"
