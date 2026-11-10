#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include "core/models/protocols/xrayProtocolConfig.h"
#include "ui/models/protocols/xrayConfigModel.h"
#include "utils/xrayContractCases.h"
#include "utils/xrayMatrix.h"

using namespace amnezia;
using namespace xraytest;

namespace
{
    XrayProtocolConfig withNative(const XrayServerConfig &srv, const QString &native)
    {
        XrayProtocolConfig cfg;
        cfg.serverConfig = srv;
        XrayClientConfig client;
        client.nativeConfig = native;
        client.localPort = QStringLiteral("10808");
        client.id = clientId();
        cfg.setClientConfig(client);
        return cfg;
    }

    XrayProtocolConfig issued(const XrayServerConfig &srv)
    {
        return withNative(srv, compact(clientJson(srv)));
    }

    XrayServerConfig xhttpWithAppRanges(const QString &mode)
    {
        XrayServerConfig srv = baseConfig("xhttp", "reality", "");
        srv.xhttp.mode = mode;
        srv.xhttp.applyRangeDefaults();
        return srv;
    }

    XrayProtocolConfig stored(const XrayProtocolConfig &saved)
    {
        return issued(XrayServerConfig::fromJson(saved.serverConfig.toJson()));
    }

    bool hasError(const QStringList &errors, const QString &needle)
    {
        for (const QString &e : errors) {
            if (e.contains(needle))
                return true;
        }
        return false;
    }
}

class TestXrayConfigModel : public QObject
{
    Q_OBJECT

private slots:
    void options();
    void isValidSni_data();
    void isValidSni();
    void validationErrors_data();
    void validationErrors();
    void deviceOnlyEditStaysLocal_data();
    void deviceOnlyEditStaysLocal();
    void serverEditTouchesServer_data();
    void serverEditTouchesServer();
    void portChangeRequiresReinstall();
    void switchingToXhttpFillsDefaultsButKeepsParsedMode();
    void switchingToMkcpReplacesReality();
    void realityShowsTheFingerprintItSends();
    void unchangedSaveKeepsClientConfig();
    void serverEditDropsClientConfig();
    void deviceOnlyEditRebuildsClientConfig();
    void deviceOnlyEditWithBrokenNativeDropsClientConfig();
    void unsavedChangesFollowEdits();
    void clearedServerRangeReachesServerAndStaysEmpty();
    void clearedClientRangeStaysEmpty();
    void startingXhttpFillsAppRangeDefaults();
    void xmuxOnWithoutValuesEqualsOff();
    void contractTable();
    void contractAppVerdict_data();
    void contractAppVerdict();
};

void TestXrayConfigModel::options()
{
    QCOMPARE(XrayConfigModel::securityOptions(), QStringList({ "tls", "reality" }));
    QCOMPARE(XrayConfigModel::realityFingerprintOptions(), QStringList({ "chrome", "firefox", "safari" }));
    QCOMPARE(XrayConfigModel::xhttpModeOptions(),
             QStringList({ "Auto", "Packet-up", "Stream-up", "Stream-one" }));
    QCOMPARE(XrayConfigModel::xhttpUplinkDataPlacementOptions(),
             QStringList({ "Body", "Auto", "Header", "Cookie" }));
    QCOMPARE(XrayConfigModel::xPaddingBytesMinDefault(), QStringLiteral("100"));
    QCOMPARE(XrayConfigModel::xPaddingBytesMaxDefault(), QStringLiteral("1000"));
    QCOMPARE(XrayConfigModel::scStreamUpServerSecsMinDefault(), QStringLiteral("20"));
    QCOMPARE(XrayConfigModel::scStreamUpServerSecsMaxDefault(), QStringLiteral("80"));
    QCOMPARE(XrayConfigModel::scMinPostsIntervalMsMinDefault(), QStringLiteral("30"));
    QCOMPARE(XrayConfigModel::scMinPostsIntervalMsMaxDefault(), QStringLiteral("30"));
    QCOMPARE(XrayConfigModel::scMaxEachPostBytesMinDefault(), QStringLiteral("1000000"));
    QCOMPARE(XrayConfigModel::scMaxEachPostBytesMaxDefault(), QStringLiteral("1000000"));
}

void TestXrayConfigModel::isValidSni_data()
{
    QTest::addColumn<QString>("sni");
    QTest::addColumn<bool>("valid");
    QTest::newRow("empty") << "" << true;
    QTest::newRow("domain") << "www.example.com" << true;
    QTest::newRow("ipv4") << "203.0.113.10" << true;
    QTest::newRow("star") << "*" << false;
    QTest::newRow("wildcard") << "*.asd.com" << false;
    QTest::newRow("star inside") << "a*b.example.com" << false;
    QTest::newRow("no tld") << "example" << false;
    QTest::newRow("digits") << "12345" << false;
}

void TestXrayConfigModel::isValidSni()
{
    QFETCH(QString, sni);
    QFETCH(bool, valid);
    QCOMPARE(XrayConfigModel::isValidSni(sni), valid);
}

void TestXrayConfigModel::validationErrors_data()
{
    QTest::addColumn<QString>("transport");
    QTest::addColumn<QString>("security");
    QTest::addColumn<QString>("edit");
    QTest::addColumn<QString>("expected");

    QTest::newRow("valid raw reality") << "raw" << "reality" << "" << "";
    QTest::newRow("port 0") << "raw" << "reality" << "port=0" << "Port must be";
    QTest::newRow("port 70000") << "raw" << "reality" << "port=70000" << "Port must be";
    QTest::newRow("tls sni *") << "raw" << "tls" << "sni=*" << "SNI";
    QTest::newRow("reality wildcard sni") << "xhttp" << "reality" << "sni=*.asd.com" << "SNI";
    QTest::newRow("none is rejected") << "raw" << "none" << "" << "None is not supported";
    QTest::newRow("mkcp + reality is rejected") << "mkcp" << "reality" << "" << "mKCP does not work with REALITY";
    QTest::newRow("native none is accepted") << "raw" << "none" << "native" << "";
    QTest::newRow("packet-up + reality") << "xhttp" << "reality" << "mode=Packet-up" << "";
    QTest::newRow("packet-up + tls") << "xhttp" << "tls" << "mode=Packet-up" << "";
    QTest::newRow("auto + reality") << "xhttp" << "reality" << "mode=Auto" << "";
    QTest::newRow("xmux both") << "xhttp" << "tls" << "xmux=both" << "XMux";
    QTest::newRow("xmux both but off") << "xhttp" << "tls" << "xmux=both-off" << "";
    QTest::newRow("path without slash") << "xhttp" << "tls" << "path=text" << "";
    QTest::newRow("bad host") << "xhttp" << "tls" << "host=bad host" << "Host";
}

void TestXrayConfigModel::validationErrors()
{
    QFETCH(QString, transport);
    QFETCH(QString, security);
    QFETCH(QString, edit);
    QFETCH(QString, expected);

    XrayServerConfig srv = baseConfig(transport, security, "");
    srv.isThirdPartyConfig = edit == QLatin1String("native");
    XrayConfigModel model;
    model.updateModel(DockerContainer::Xray, issued(srv));

    const QString value = edit.section(QLatin1Char('='), 1);
    if (edit.startsWith("port="))
        model.setPort(value);
    else if (edit.startsWith("sni="))
        model.setSni(value);
    else if (edit.startsWith("mode="))
        model.setXhttpMode(value);
    else if (edit.startsWith("path="))
        model.setXhttpPath(value);
    else if (edit.startsWith("host="))
        model.setXhttpHost(value);
    else if (edit.startsWith("xmux=")) {
        model.setXmuxMaxConcurrencyMin("1");
        model.setXmuxMaxConcurrencyMax("1");
        model.setXmuxMaxConnectionsMin("1");
        model.setXmuxMaxConnectionsMax("1");
        model.setXmuxEnabled(value == QLatin1String("both"));
    }

    const QStringList errors = model.validationErrors();
    if (expected.isEmpty())
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join("; ")));
    else
        QVERIFY2(hasError(errors, expected), qPrintable(errors.join("; ")));
}

void TestXrayConfigModel::deviceOnlyEditStaysLocal_data()
{
    QTest::addColumn<QString>("edit");
    QTest::newRow("fingerprint") << "fingerprint";
    QTest::newRow("uplinkHTTPMethod") << "method";
    QTest::newRow("uplinkChunkSize") << "chunk";
    QTest::newRow("scMinPostsIntervalMs") << "minPosts";
    QTest::newRow("xmux") << "xmux";
    QTest::newRow("vision -> vision-udp443") << "udp443";
}

void TestXrayConfigModel::deviceOnlyEditStaysLocal()
{
    QFETCH(QString, edit);

    XrayConfigModel model;
    const bool flowCase = edit == QLatin1String("udp443");
    model.updateModel(DockerContainer::Xray,
                      issued(baseConfig(flowCase ? "raw" : "xhttp", "reality", flowCase ? "xtls-rprx-vision" : "")));

    if (edit == QLatin1String("fingerprint"))
        model.setFingerprint("firefox");
    else if (edit == QLatin1String("method"))
        model.setXhttpUplinkMethod("PATCH");
    else if (edit == QLatin1String("chunk"))
        model.setXhttpUplinkChunkSize("4096");
    else if (edit == QLatin1String("minPosts"))
        model.setXhttpScMinPostsIntervalMsMin("5");
    else if (edit == QLatin1String("xmux"))
        model.setXmuxMaxConcurrencyMax("64");
    else
        model.setFlow("xtls-rprx-vision-udp443");

    QVERIFY(model.hasUnsavedChanges());
    QVERIFY(!model.pendingChangeTouchesServer(QString()));
    QVERIFY(!model.pendingChangeRequiresReinstall(QString()));
    QCOMPARE(model.saveDescription(QString()),
             QStringLiteral("The server will not be changed now. The new settings apply the next time you connect."));
}

void TestXrayConfigModel::serverEditTouchesServer_data()
{
    QTest::addColumn<QString>("edit");
    QTest::newRow("sni") << "sni";
    QTest::newRow("path") << "path";
    QTest::newRow("mode") << "mode";
    QTest::newRow("transport") << "transport";
    QTest::newRow("security") << "security";
    QTest::newRow("xPaddingObfsMode") << "obfs";
}

void TestXrayConfigModel::serverEditTouchesServer()
{
    QFETCH(QString, edit);

    XrayConfigModel model;
    model.updateModel(DockerContainer::Xray, issued(baseConfig("xhttp", "reality", "")));

    if (edit == QLatin1String("sni"))
        model.setSni("www.microsoft.com");
    else if (edit == QLatin1String("path"))
        model.setXhttpPath("/other");
    else if (edit == QLatin1String("mode"))
        model.setXhttpMode("Stream-up");
    else if (edit == QLatin1String("transport"))
        model.setTransport("raw");
    else if (edit == QLatin1String("security"))
        model.setSecurity("tls");
    else
        model.setXPaddingObfsMode(!model.xPaddingObfsMode());

    QVERIFY(model.pendingChangeTouchesServer(QString()));
    QVERIFY(!model.pendingChangeRequiresReinstall(QString()));
}

void TestXrayConfigModel::portChangeRequiresReinstall()
{
    XrayConfigModel model;
    model.updateModel(DockerContainer::Xray, issued(baseConfig("raw", "reality", "")));
    QVERIFY(model.pendingChangeRequiresReinstall(QStringLiteral("8443")));
    QVERIFY(model.pendingChangeTouchesServer(QStringLiteral("8443")));
}

void TestXrayConfigModel::switchingToXhttpFillsDefaultsButKeepsParsedMode()
{
    XrayConfigModel fresh;
    XrayServerConfig raw = baseConfig("raw", "reality", "");
    raw.xhttp = XrayXhttpConfig {};
    fresh.updateModel(DockerContainer::Xray, issued(raw));
    fresh.setTransport("xhttp");
    QCOMPARE(fresh.xhttpMode(), QStringLiteral("Stream-one"));
    QVERIFY(!fresh.xhttpSessionPlacement().isEmpty());
    QVERIFY(!fresh.xhttpUplinkMethod().isEmpty());

    XrayConfigModel parsed;
    XrayServerConfig rawWithPacketUp = baseConfig("raw", "tls", "");
    rawWithPacketUp.xhttp.mode = QStringLiteral("Packet-up");
    parsed.updateModel(DockerContainer::Xray, issued(rawWithPacketUp));
    parsed.setTransport("xhttp");
    QCOMPARE(parsed.xhttpMode(), QStringLiteral("Packet-up"));
}

void TestXrayConfigModel::switchingToMkcpReplacesReality()
{
    XrayConfigModel reality;
    reality.updateModel(DockerContainer::Xray, issued(baseConfig("raw", "reality", "")));
    reality.setTransport("mkcp");
    QCOMPARE(reality.security(), QStringLiteral("tls"));
    QVERIFY2(reality.validationErrors().isEmpty(), qPrintable(reality.validationErrors().join("; ")));

    XrayConfigModel tls;
    tls.updateModel(DockerContainer::Xray, issued(baseConfig("raw", "tls", "")));
    tls.setTransport("mkcp");
    QCOMPARE(tls.security(), QStringLiteral("tls"));
}

void TestXrayConfigModel::realityShowsTheFingerprintItSends()
{
    XrayServerConfig selfHosted = baseConfig("raw", "reality", "");
    selfHosted.fingerprint = QStringLiteral("hellochrome_120");
    XrayConfigModel model;
    model.updateModel(DockerContainer::Xray, issued(selfHosted));
    QCOMPARE(model.realityFingerprint(), QStringLiteral("chrome"));
    model.setFingerprint(QStringLiteral("firefox"));
    QCOMPARE(model.realityFingerprint(), QStringLiteral("firefox"));

    XrayServerConfig native = selfHosted;
    native.isThirdPartyConfig = true;
    XrayConfigModel nativeModel;
    nativeModel.updateModel(DockerContainer::Xray, issued(native));
    QCOMPARE(nativeModel.realityFingerprint(), QStringLiteral("hellochrome_120"));
}

void TestXrayConfigModel::unchangedSaveKeepsClientConfig()
{
    XrayConfigModel model;
    const XrayProtocolConfig stored = issued(baseConfig("raw", "reality", "xtls-rprx-vision"));
    model.updateModel(DockerContainer::Xray, stored);

    const XrayProtocolConfig saved = model.getProtocolConfig();
    QVERIFY(saved.hasClientConfig());
    QCOMPARE(saved.clientConfig->nativeConfig, stored.clientConfig->nativeConfig);
}

void TestXrayConfigModel::serverEditDropsClientConfig()
{
    XrayConfigModel model;
    model.updateModel(DockerContainer::Xray, issued(baseConfig("raw", "reality", "")));
    model.setSni("www.microsoft.com");

    QVERIFY(!model.getProtocolConfig().hasClientConfig());
}

void TestXrayConfigModel::deviceOnlyEditRebuildsClientConfig()
{
    XrayConfigModel model;
    model.updateModel(DockerContainer::Xray, issued(baseConfig("xhttp", "reality", "")));
    model.setFingerprint("firefox");
    model.setXhttpUplinkMethod("PATCH");

    const XrayProtocolConfig saved = model.getProtocolConfig();
    QVERIFY(saved.hasClientConfig());
    const QJsonObject native = QJsonDocument::fromJson(saved.clientConfig->nativeConfig.toUtf8()).object();
    const QJsonObject stream = streamOf(vlessOutbound(native));
    QCOMPARE(stream.value("realitySettings").toObject().value("fingerprint").toString(), QStringLiteral("firefox"));
    QCOMPARE(stream.value("xhttpSettings").toObject().value("uplinkHTTPMethod").toString(), QStringLiteral("PATCH"));
    QCOMPARE(stream.value("realitySettings").toObject().value("publicKey").toString(), key32('\x02'));
    QCOMPARE(saved.clientConfig->id, clientId());
}

void TestXrayConfigModel::deviceOnlyEditWithBrokenNativeDropsClientConfig()
{
    XrayConfigModel model;
    const QString broken = QStringLiteral(R"({"outbounds":[{"protocol":"vless","settings":{}}]})");
    model.updateModel(DockerContainer::Xray, withNative(baseConfig("xhttp", "reality", ""), broken));
    model.setFingerprint("firefox");

    QVERIFY2(!model.getProtocolConfig().hasClientConfig(),
             "a native config that cannot be rebuilt must be dropped so the server issues a fresh one");
}

void TestXrayConfigModel::unsavedChangesFollowEdits()
{
    XrayConfigModel model;
    model.updateModel(DockerContainer::Xray, issued(baseConfig("raw", "reality", "")));
    QVERIFY(!model.hasUnsavedChanges());

    const QString original = model.fingerprint();
    model.setFingerprint("firefox");
    QVERIFY(model.hasUnsavedChanges());
    model.setFingerprint(original);
    QVERIFY(!model.hasUnsavedChanges());
}

void TestXrayConfigModel::clearedServerRangeReachesServerAndStaysEmpty()
{
    XrayConfigModel model;
    model.updateModel(DockerContainer::Xray, issued(xhttpWithAppRanges("Stream-up")));
    QCOMPARE(model.xhttpScStreamUpServerSecsMin(), QStringLiteral("1"));
    QCOMPARE(model.xhttpScStreamUpServerSecsMax(), QStringLiteral("100"));

    model.setXhttpScStreamUpServerSecsMin(QString());
    model.setXhttpScStreamUpServerSecsMax(QString());
    QVERIFY(model.validationErrors().isEmpty());
    QVERIFY(model.pendingChangeTouchesServer(QString()));

    const XrayProtocolConfig saved = model.getProtocolConfig();
    const QString server = compact(serverJson(saved.serverConfig));
    QVERIFY2(!server.contains(QStringLiteral("scStreamUpServerSecs")), qPrintable(server));

    XrayConfigModel reopened;
    reopened.updateModel(DockerContainer::Xray, stored(saved));
    QVERIFY(reopened.xhttpScStreamUpServerSecsMin().isEmpty());
    QVERIFY(reopened.xhttpScStreamUpServerSecsMax().isEmpty());
    QVERIFY(!reopened.hasUnsavedChanges());
}

void TestXrayConfigModel::clearedClientRangeStaysEmpty()
{
    XrayConfigModel model;
    model.updateModel(DockerContainer::Xray, issued(xhttpWithAppRanges("Packet-up")));
    QCOMPARE(model.xhttpScMinPostsIntervalMsMin(), QStringLiteral("100"));

    model.setXhttpScMinPostsIntervalMsMin(QString());
    model.setXhttpScMinPostsIntervalMsMax(QString());
    QVERIFY(model.validationErrors().isEmpty());
    QVERIFY(!model.pendingChangeTouchesServer(QString()));

    const XrayProtocolConfig saved = model.getProtocolConfig();
    const QString client = compact(clientJson(saved.serverConfig));
    QVERIFY2(!client.contains(QStringLiteral("scMinPostsIntervalMs")), qPrintable(client));

    XrayConfigModel reopened;
    reopened.updateModel(DockerContainer::Xray, stored(saved));
    QVERIFY(reopened.xhttpScMinPostsIntervalMsMin().isEmpty());
    QVERIFY(reopened.xhttpScMinPostsIntervalMsMax().isEmpty());
}

void TestXrayConfigModel::startingXhttpFillsAppRangeDefaults()
{
    XrayServerConfig raw = baseConfig("raw", "reality", "");
    raw.xhttp = XrayXhttpConfig {};

    XrayConfigModel model;
    model.updateModel(DockerContainer::Xray, issued(raw));
    model.setTransport("xhttp");
    QCOMPARE(model.xhttpScStreamUpServerSecsMin(), QStringLiteral("1"));
    QCOMPARE(model.xhttpScStreamUpServerSecsMax(), QStringLiteral("100"));
    QCOMPARE(model.xhttpScMinPostsIntervalMsMin(), QStringLiteral("100"));
    QCOMPARE(model.xhttpScMinPostsIntervalMsMax(), QStringLiteral("800"));
    QCOMPARE(model.xhttpScMaxEachPostBytesMin(), QStringLiteral("1000000"));
    QCOMPARE(model.xhttpScMaxEachPostBytesMax(), QStringLiteral("1000000"));

    model.setXhttpScStreamUpServerSecsMin(QString());
    model.setXhttpScStreamUpServerSecsMax(QString());
    model.resetToDefaults();
    QCOMPARE(model.xhttpScStreamUpServerSecsMin(), QStringLiteral("1"));
    QCOMPARE(model.xhttpScStreamUpServerSecsMax(), QStringLiteral("100"));
}

void TestXrayConfigModel::xmuxOnWithoutValuesEqualsOff()
{
    XrayServerConfig on = baseConfig("xhttp", "reality", "");
    on.xhttp.xmux = XrayXmuxConfig::fromJson(QJsonObject {});
    on.xhttp.xmux.enabled = true;
    XrayServerConfig off = on;
    off.xhttp.xmux.enabled = false;

    const QString onClient = compact(clientJson(on));
    QVERIFY2(!onClient.contains(QStringLiteral("\"xmux\"")), qPrintable(onClient));
    QCOMPARE(onClient, compact(clientJson(off)));
}

void TestXrayConfigModel::contractTable()
{
    QStringList names;
    for (const ContractCase &c : contractCases()) {
        QVERIFY2(!names.contains(c.name), qPrintable(QStringLiteral("duplicate row ") + c.name));
        names << c.name;
        if (c.app == AppVerdict::Blocked && c.core == CoreVerdict::Accept)
            QVERIFY2(!c.stricterWhy.isEmpty(), qPrintable(c.name + QStringLiteral(": app is stricter than the core, say why")));
        if (c.app == AppVerdict::Ok)
            QVERIFY2(c.core != CoreVerdict::Reject,
                     qPrintable(c.name + QStringLiteral(": a row may not expect the app to pass what the core rejects")));
    }
    qInfo().noquote() << "contract rows:" << names.size();
}

void TestXrayConfigModel::contractAppVerdict_data()
{
    QTest::addColumn<int>("index");
    for (int i = 0; i < contractCases().size(); ++i)
        QTest::newRow(qPrintable(contractCases().at(i).name)) << i;
}

void TestXrayConfigModel::contractAppVerdict()
{
    QFETCH(int, index);
    const ContractCase &c = contractCases().at(index);

    const QStringList errors = contractErrors(c);
    const AppVerdict actual = errors.isEmpty() ? AppVerdict::Ok : AppVerdict::Blocked;
    if (!c.knownIssue.isEmpty())
        QEXPECT_FAIL("", qPrintable(c.knownIssue), Abort);
    QVERIFY2(actual == c.app, qPrintable(QStringLiteral("app %1, expected %2; errors: %3")
                                                 .arg(verdictName(actual), verdictName(c.app), errors.join(QStringLiteral("; ")))));
}

QTEST_GUILESS_MAIN(TestXrayConfigModel)

#include "testXrayConfigModel.moc"
