#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

#include "core/models/protocols/xrayProtocolConfig.h"
#include "utils/xrayMatrix.h"

using namespace amnezia;
using namespace xraytest;

namespace
{
    const QList<MatrixCase> &cases()
    {
        static const QList<MatrixCase> all = matrix();
        return all;
    }

    QString str(const QJsonObject &obj, const char *key)
    {
        return obj.value(QString::fromLatin1(key)).toString();
    }

    QString serverClientFlow(const QJsonObject &inbound)
    {
        const QJsonArray clients = inbound.value("settings").toObject().value("clients").toArray();
        return clients.isEmpty() ? QString() : str(clients.first().toObject(), "flow");
    }

    QString outboundUserFlow(const QJsonObject &outbound)
    {
        const QJsonArray vnext = outbound.value("settings").toObject().value("vnext").toArray();
        if (vnext.isEmpty())
            return {};
        const QJsonArray users = vnext.first().toObject().value("users").toArray();
        return users.isEmpty() ? QString() : str(users.first().toObject(), "flow");
    }
}

class TestXrayGeneratorMatrix : public QObject
{
    Q_OBJECT

private slots:
    void invariants_data();
    void invariants();
    void deviceOnlyChangesKeepServerView_data();
    void deviceOnlyChangesKeepServerView();
    void serverChangesChangeServerView_data();
    void serverChangesChangeServerView();
};

void TestXrayGeneratorMatrix::invariants_data()
{
    QTest::addColumn<int>("index");
    for (int i = 0; i < cases().size(); ++i)
        QTest::newRow(qPrintable(cases().at(i).name)) << i;
}

void TestXrayGeneratorMatrix::invariants()
{
    QFETCH(int, index);
    const MatrixCase &c = cases().at(index);
    const XrayServerConfig &srv = c.srv;

    const QJsonObject server = serverJson(srv);
    const QJsonObject client = clientJson(srv);
    const QJsonObject inbound = firstInbound(server);
    const QJsonObject outbound = vlessOutbound(client);
    const QJsonObject sIn = streamOf(inbound);
    const QJsonObject sOut = streamOf(outbound);
    const QString serverText = compact(server);
    const QString clientText = compact(client);

    QVERIFY2(!outbound.isEmpty(), qPrintable(clientText));

    const bool raw = srv.transport == QLatin1String("raw");
    const bool xhttp = srv.transport == QLatin1String("xhttp");
    const bool mkcp = srv.transport == QLatin1String("mkcp");
    const QString network = xhttp ? QStringLiteral("xhttp") : mkcp ? QStringLiteral("kcp") : QStringLiteral("tcp");
    QCOMPARE(str(sIn, "network"), network);
    QCOMPARE(str(sOut, "network"), network);
    QCOMPARE(sIn.contains("xhttpSettings"), xhttp);
    QCOMPARE(sOut.contains("xhttpSettings"), xhttp);
    QCOMPARE(sIn.contains("kcpSettings"), mkcp);
    QCOMPARE(sOut.contains("kcpSettings"), mkcp);

    const QString security = (mkcp && srv.security == QLatin1String("reality")) ? QStringLiteral("none") : srv.security;
    QCOMPARE(str(sIn, "security"), security);
    QCOMPARE(str(sOut, "security"), security);

    for (const char *key : { "uplinkHTTPMethod", "uplinkChunkSize", "scMinPostsIntervalMs", "xmux", "fingerprint" })
        QVERIFY2(!containsKeyDeep(server, QString::fromLatin1(key)), qPrintable(QString::fromLatin1(key) + ": " + serverText));
    QVERIFY2(!containsKeyDeep(server, "allowInsecure") && !containsKeyDeep(client, "allowInsecure"), qPrintable(clientText));

    const bool visionAllowed = raw && (security == QLatin1String("tls") || security == QLatin1String("reality"));
    QVERIFY2(!serverText.contains("udp443"), qPrintable(serverText));
    if (!visionAllowed || srv.flow.isEmpty()) {
        QCOMPARE(serverClientFlow(inbound), QString());
        QCOMPARE(outboundUserFlow(outbound), QString());
    } else {
        QCOMPARE(serverClientFlow(inbound), QStringLiteral("xtls-rprx-vision"));
        QCOMPARE(outboundUserFlow(outbound), srv.flow);
    }

    if (security == QLatin1String("reality")) {
        const QJsonObject rIn = sIn.value("realitySettings").toObject();
        const QJsonObject rOut = sOut.value("realitySettings").toObject();
        QCOMPARE(str(rIn, "dest"), srv.site + QStringLiteral(":443"));
        QCOMPARE(rIn.value("serverNames").toArray(), QJsonArray { srv.sni });
        QCOMPARE(str(rIn, "privateKey"), key32('\x01'));
        QCOMPARE(rIn.value("shortIds").toArray(), QJsonArray { shortId() });
        QCOMPARE(str(rOut, "serverName"), srv.sni);
        QCOMPARE(str(rOut, "publicKey"), key32('\x02'));
        QCOMPARE(str(rOut, "shortId"), shortId());
        QCOMPARE(str(rOut, "fingerprint"), srv.fingerprint);
        QVERIFY(!sIn.contains("tlsSettings") && !sOut.contains("tlsSettings"));
    } else if (security == QLatin1String("tls")) {
        const QJsonObject tIn = sIn.value("tlsSettings").toObject();
        const QJsonObject tOut = sOut.value("tlsSettings").toObject();
        QVERIFY2(!tIn.value("certificates").toArray().isEmpty(), qPrintable(serverText));
        QCOMPARE(str(tOut, "serverName"), srv.sni);
        QCOMPARE(str(tOut, "pinnedPeerCertSha256"), tlsPin());
        QCOMPARE(tOut.contains("fingerprint"), !mkcp);
        QVERIFY(!sIn.contains("realitySettings") && !sOut.contains("realitySettings"));
    } else {
        QVERIFY2(!sIn.contains("tlsSettings") && !sIn.contains("realitySettings"), qPrintable(serverText));
        QVERIFY2(!sOut.contains("tlsSettings") && !sOut.contains("realitySettings"), qPrintable(clientText));
    }

    if (!xhttp)
        return;

    const QJsonObject xIn = sIn.value("xhttpSettings").toObject();
    const QJsonObject xOut = sOut.value("xhttpSettings").toObject();
    const QString mode = xrayEffective::xhttpMode(srv.xhttp.mode);
    QCOMPARE(str(xIn, "mode"), mode);
    QCOMPARE(str(xOut, "mode"), mode);
    QVERIFY2(str(xIn, "path").startsWith('/') && str(xOut, "path").startsWith('/'), qPrintable(clientText));
    QCOMPARE(str(xIn, "path"), str(xOut, "path"));

    const bool packetUp = mode == QLatin1String("packet-up");
    for (const QJsonObject &x : { xIn, xOut }) {
        const QString dataPlacement = str(x, "uplinkDataPlacement");
        if (dataPlacement == QLatin1String("header") || dataPlacement == QLatin1String("cookie"))
            QVERIFY2(packetUp, qPrintable(compact(x)));
    }
    if (str(xOut, "uplinkHTTPMethod") == QLatin1String("GET"))
        QVERIFY2(packetUp, qPrintable(clientText));

    for (const QJsonObject &x : { xIn, xOut }) {
        QVERIFY(x.contains("xPaddingObfsMode"));
        QCOMPARE(x.value("xPaddingObfsMode").toBool(), srv.xhttp.xPadding.obfsMode);
        if (!srv.xhttp.xPadding.obfsMode) {
            for (const char *key : { "xPaddingBytes", "xPaddingKey", "xPaddingHeader", "xPaddingPlacement", "xPaddingMethod" })
                QVERIFY2(!x.contains(QString::fromLatin1(key)), qPrintable(compact(x)));
        }
    }

    const XrayXmuxConfig &xmux = srv.xhttp.xmux;
    QCOMPARE(xOut.contains("xmux"), xmux.enabled && (xmux.maxConcurrencyMax != "0" || xmux.maxConnectionsMax != "0"));
    if (xOut.contains("xmux")) {
        const QJsonObject mux = xOut.value("xmux").toObject();
        QCOMPARE(mux.contains("maxConcurrency"), xmux.maxConcurrencyMax != "0");
        QCOMPARE(mux.contains("maxConnections"), xmux.maxConnectionsMax != "0");
    }
}

void TestXrayGeneratorMatrix::deviceOnlyChangesKeepServerView_data()
{
    QTest::addColumn<QString>("field");
    QTest::newRow("fingerprint") << "fingerprint";
    QTest::newRow("uplinkMethod") << "uplinkMethod";
    QTest::newRow("uplinkChunkSize") << "uplinkChunkSize";
    QTest::newRow("scMinPostsIntervalMs") << "scMinPostsIntervalMs";
    QTest::newRow("xmux") << "xmux";
    QTest::newRow("vision -> vision-udp443") << "udp443";
}

void TestXrayGeneratorMatrix::deviceOnlyChangesKeepServerView()
{
    QFETCH(QString, field);

    XrayServerConfig before = baseConfig(field == "udp443" ? "raw" : "xhttp", "reality",
                                         field == "udp443" ? "xtls-rprx-vision" : "");
    XrayServerConfig after = before;
    if (field == "fingerprint")
        after.fingerprint = QStringLiteral("firefox");
    else if (field == "uplinkMethod")
        after.xhttp.uplinkMethod = QStringLiteral("PATCH");
    else if (field == "uplinkChunkSize")
        after.xhttp.uplinkChunkSize = QStringLiteral("4096");
    else if (field == "scMinPostsIntervalMs")
        after.xhttp.scMinPostsIntervalMsMin = QStringLiteral("5");
    else if (field == "xmux")
        after.xhttp.xmux.maxConcurrencyMax = QStringLiteral("64");
    else if (field == "udp443")
        after.flow = QStringLiteral("xtls-rprx-vision-udp443");

    QVERIFY2(before.toJson() != after.toJson(), "the edit must change the stored settings");
    QVERIFY2(before.hasEqualServerSettings(after),
             qPrintable(QStringLiteral("a device-only edit must not count as a server change: ")
                        + compact(before.serverView()) + QStringLiteral(" -> ") + compact(after.serverView())));
    QCOMPARE(compact(serverJson(before)), compact(serverJson(after)));
}

void TestXrayGeneratorMatrix::serverChangesChangeServerView_data()
{
    QTest::addColumn<QString>("field");
    QTest::newRow("port") << "port";
    QTest::newRow("transport") << "transport";
    QTest::newRow("security") << "security";
    QTest::newRow("sni") << "sni";
    QTest::newRow("path") << "path";
    QTest::newRow("mode") << "mode";
    QTest::newRow("host") << "host";
    QTest::newRow("xPaddingObfsMode") << "obfs";
}

void TestXrayGeneratorMatrix::serverChangesChangeServerView()
{
    QFETCH(QString, field);

    const XrayServerConfig before = baseConfig("xhttp", "reality", "");
    XrayServerConfig after = before;
    if (field == "port")
        after.port = QStringLiteral("8443");
    else if (field == "transport")
        after.transport = QStringLiteral("raw");
    else if (field == "security")
        after.security = QStringLiteral("tls");
    else if (field == "sni") {
        after.sni = QStringLiteral("www.microsoft.com");
        after.site = after.sni;
    } else if (field == "path")
        after.xhttp.path = QStringLiteral("/other");
    else if (field == "mode")
        after.xhttp.mode = QStringLiteral("Stream-up");
    else if (field == "host")
        after.xhttp.host = QStringLiteral("cdn.example.com");
    else if (field == "obfs")
        after.xhttp.xPadding.obfsMode = !before.xhttp.xPadding.obfsMode;

    QVERIFY2(!before.hasEqualServerSettings(after), qPrintable(field + " must count as a server change"));
}

QTEST_APPLESS_MAIN(TestXrayGeneratorMatrix)

#include "testXrayGeneratorMatrix.moc"
