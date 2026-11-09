#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

#include "core/models/protocols/xrayProtocolConfig.h"

using namespace amnezia;

namespace
{
    XrayServerConfig fullyConfigured()
    {
        XrayServerConfig c;
        c.port = "444";
        c.transportProto = "tcp";
        c.site = "dest.example.com";
        c.security = "reality";
        c.flow = "xtls-rprx-vision";
        c.sni = "sni.example.com";
        c.alpn = "h2";
        c.fingerprint = "firefox";
        c.transport = "xhttp";

        c.xhttp.mode = "Stream-up";
        c.xhttp.host = "host.example.com";
        c.xhttp.path = "/p";
        c.xhttp.uplinkMethod = "PUT";
        c.xhttp.uplinkChunkSize = "4096";
        c.xhttp.sessionPlacement = "Query";
        c.xhttp.sessionKey = "sid";
        c.xhttp.seqPlacement = "Query";
        c.xhttp.seqKey = "page";
        c.xhttp.uplinkDataPlacement = "Auto";
        c.xhttp.uplinkDataKey = "x_data";
        c.xhttp.scMaxBufferedPosts = "77";
        c.xhttp.scMaxEachPostBytesMin = "1000";
        c.xhttp.scMaxEachPostBytesMax = "2000";
        c.xhttp.scMinPostsIntervalMsMin = "30";
        c.xhttp.scMinPostsIntervalMsMax = "60";
        c.xhttp.scStreamUpServerSecsMin = "5";
        c.xhttp.scStreamUpServerSecsMax = "9";

        c.xhttp.xPadding.bytesMin = "100";
        c.xhttp.xPadding.bytesMax = "1000";
        c.xhttp.xPadding.key = "_dc";
        c.xhttp.xPadding.header = "Referer";
        c.xhttp.xPadding.placement = "Query in header";
        c.xhttp.xPadding.method = "Tokenish";

        c.xhttp.xmux.maxConcurrencyMin = "16";
        c.xhttp.xmux.maxConcurrencyMax = "32";
        c.xhttp.xmux.hKeepAlivePeriod = "45";
        return c;
    }

    XrayServerInboundInputs inboundInputs()
    {
        XrayServerInboundInputs in;
        in.clients = QJsonArray { QJsonObject { { "id", "11111111-2222-3333-4444-555555555555" },
                                                { "flow", "xtls-rprx-vision" } } };
        in.realityPrivateKey = "private-key";
        in.realityShortId = "shortid";
        return in;
    }

    QString compact(const QJsonObject &obj)
    {
        return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }
}

class TestXrayServerInbound : public QObject
{
    Q_OBJECT

private slots:
    void serverInboundCarriesNoDeviceSettings_data();
    void serverInboundCarriesNoDeviceSettings();
    void clientOutboundCarriesDeviceSettings();
    void serverInboundStillCarriesWhatTheServerListensWith();
    void rebuildFromServerLosesOnlyDeviceSettings();
    void realityIsDowngradedOnlyForMkcp_data();
    void realityIsDowngradedOnlyForMkcp();
    void udp443FlowStaysOnTheDevice();
    void mkcpTlsOutboundHasNoFingerprint();
    void xhttpPathGetsLeadingSlash();
};

void TestXrayServerInbound::serverInboundCarriesNoDeviceSettings_data()
{
    QTest::addColumn<QString>("key");

    QTest::newRow("uplinkHTTPMethod") << QStringLiteral("uplinkHTTPMethod");
    QTest::newRow("uplinkChunkSize") << QStringLiteral("uplinkChunkSize");
    QTest::newRow("scMinPostsIntervalMs") << QStringLiteral("scMinPostsIntervalMs");
    QTest::newRow("xmux") << QStringLiteral("xmux");
    QTest::newRow("fingerprint") << QStringLiteral("fingerprint");
}

void TestXrayServerInbound::serverInboundCarriesNoDeviceSettings()
{
    QFETCH(QString, key);

    const QString inbound = compact(fullyConfigured().toServerInboundJson(inboundInputs()));

    QVERIFY2(!inbound.contains(QStringLiteral("\"%1\"").arg(key)),
             qPrintable(QStringLiteral("%1 must not reach server.json. It is a setting of this device: "
                                       "xray's inbound never reads it, so a copy there is a second "
                                       "source of truth. Inbound was: %2")
                                .arg(key, inbound)));
}

void TestXrayServerInbound::clientOutboundCarriesDeviceSettings()
{
    XrayProtocolConfig cfg;
    cfg.serverConfig = fullyConfigured();

    XrayClientOutboundInputs inputs;
    inputs.serverAddress = "203.0.113.10";
    inputs.clientId = "11111111-2222-3333-4444-555555555555";
    inputs.realityPublicKey = "public-key";
    inputs.realityShortId = "shortid";

    const QString outbound = compact(cfg.toClientOutboundJson(inputs));

    QVERIFY2(outbound.contains(QStringLiteral("\"uplinkHTTPMethod\":\"PUT\"")), qPrintable(outbound));
    QVERIFY2(outbound.contains(QStringLiteral("\"uplinkChunkSize\":4096")), qPrintable(outbound));
    QVERIFY2(outbound.contains(QStringLiteral("\"scMinPostsIntervalMs\":\"30-60\"")), qPrintable(outbound));
    QVERIFY2(outbound.contains(QStringLiteral("\"xmux\"")), qPrintable(outbound));
    QVERIFY2(outbound.contains(QStringLiteral("\"fingerprint\":\"firefox\"")), qPrintable(outbound));
}

void TestXrayServerInbound::serverInboundStillCarriesWhatTheServerListensWith()
{
    const QString inbound = compact(fullyConfigured().toServerInboundJson(inboundInputs()));

    QVERIFY2(inbound.contains(QStringLiteral("\"port\":444")), qPrintable(inbound));
    QVERIFY2(inbound.contains(QStringLiteral("\"network\":\"xhttp\"")), qPrintable(inbound));
    QVERIFY2(inbound.contains(QStringLiteral("\"security\":\"reality\"")), qPrintable(inbound));
    QVERIFY2(inbound.contains(QStringLiteral("\"host\":\"host.example.com\"")), qPrintable(inbound));
    QVERIFY2(inbound.contains(QStringLiteral("\"path\":\"/p\"")), qPrintable(inbound));
}

void TestXrayServerInbound::rebuildFromServerLosesOnlyDeviceSettings()
{
    const XrayServerConfig before = fullyConfigured();
    const QJsonObject inbound = before.toServerInboundJson(inboundInputs());

    XrayServerConfig rebuilt;
    QCOMPARE(XrayServerConfig::fromServerInboundJson(inbound, rebuilt), XrayServerJsonStatus::Ok);

    QCOMPARE(rebuilt.port, before.port);
    QCOMPARE(rebuilt.transport, before.transport);
    QCOMPARE(rebuilt.security, before.security);
    QCOMPARE(rebuilt.sni, before.sni);
    QCOMPARE(rebuilt.site, before.site);
    QCOMPARE(rebuilt.xhttp.mode, before.xhttp.mode);
    QCOMPARE(rebuilt.xhttp.host, before.xhttp.host);
    QCOMPARE(rebuilt.xhttp.path, before.xhttp.path);
    QCOMPARE(rebuilt.xhttp.xPadding.placement, before.xhttp.xPadding.placement);
    QCOMPARE(rebuilt.xhttp.xPadding.method, before.xhttp.xPadding.method);

    QVERIFY(rebuilt.xhttp.uplinkMethod.isEmpty());
    QVERIFY(rebuilt.xhttp.uplinkChunkSize.isEmpty());
    QVERIFY(rebuilt.xhttp.scMinPostsIntervalMsMin.isEmpty());
    QCOMPARE(rebuilt.xhttp.xmux.maxConcurrencyMin, QStringLiteral("0"));
}

void TestXrayServerInbound::realityIsDowngradedOnlyForMkcp_data()
{
    QTest::addColumn<QString>("transport");
    QTest::addColumn<QString>("security");
    QTest::addColumn<QString>("effective");

    QTest::newRow("mkcp + reality") << "mkcp" << "reality" << "none";
    QTest::newRow("mkcp + tls") << "mkcp" << "tls" << "tls";
    QTest::newRow("mkcp + none") << "mkcp" << "none" << "none";
    QTest::newRow("raw + reality") << "raw" << "reality" << "reality";
    QTest::newRow("xhttp + reality") << "xhttp" << "reality" << "reality";
}

void TestXrayServerInbound::realityIsDowngradedOnlyForMkcp()
{
    QFETCH(QString, transport);
    QFETCH(QString, security);
    QFETCH(QString, effective);

    XrayServerConfig c;
    c.transport = transport;
    c.security = security;

    QCOMPARE(xrayEffective::security(c), effective);
    QCOMPARE(c.security, security);
}

void TestXrayServerInbound::udp443FlowStaysOnTheDevice()
{
    XrayServerConfig c = fullyConfigured();
    c.transport = "raw";
    c.flow = "xtls-rprx-vision-udp443";

    QCOMPARE(xrayEffective::serverFlow(c), QStringLiteral("xtls-rprx-vision"));
    QCOMPARE(xrayEffective::clientFlow(c), QStringLiteral("xtls-rprx-vision-udp443"));

    XrayProtocolConfig cfg;
    cfg.serverConfig = c;
    XrayClientOutboundInputs inputs;
    inputs.serverAddress = "203.0.113.10";
    inputs.clientId = "11111111-2222-3333-4444-555555555555";
    inputs.realityPublicKey = "public-key";
    inputs.realityShortId = "shortid";
    const QString outbound = compact(cfg.toClientOutboundJson(inputs));
    QVERIFY2(outbound.contains(QStringLiteral("\"flow\":\"xtls-rprx-vision-udp443\"")), qPrintable(outbound));

    XrayServerConfig seeded = c;
    QCOMPARE(XrayServerConfig::fromServerInboundJson(c.toServerInboundJson(inboundInputs()), seeded),
             XrayServerJsonStatus::Ok);
    QCOMPARE(seeded.flow, QStringLiteral("xtls-rprx-vision-udp443"));
}

void TestXrayServerInbound::mkcpTlsOutboundHasNoFingerprint()
{
    XrayProtocolConfig cfg;
    cfg.serverConfig = fullyConfigured();
    cfg.serverConfig.transport = "mkcp";
    cfg.serverConfig.security = "tls";

    XrayClientOutboundInputs inputs;
    inputs.serverAddress = "203.0.113.10";
    inputs.clientId = "11111111-2222-3333-4444-555555555555";
    const QString outbound = compact(cfg.toClientOutboundJson(inputs));

    QVERIFY2(outbound.contains(QStringLiteral("\"tlsSettings\"")), qPrintable(outbound));
    QVERIFY2(!outbound.contains(QStringLiteral("\"fingerprint\"")), qPrintable(outbound));
}

void TestXrayServerInbound::xhttpPathGetsLeadingSlash()
{
    XrayServerConfig c = fullyConfigured();
    c.xhttp.path = "text";

    const QString inbound = compact(c.toServerInboundJson(inboundInputs()));
    QVERIFY2(inbound.contains(QStringLiteral("\"path\":\"/text\"")), qPrintable(inbound));
}

QTEST_APPLESS_MAIN(TestXrayServerInbound)

#include "testXrayServerInbound.moc"
