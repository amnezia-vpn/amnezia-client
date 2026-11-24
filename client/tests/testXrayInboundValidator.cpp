#include <QTest>

#include "core/models/protocols/xrayInboundValidator.h"
#include "core/models/protocols/xrayPresets.h"

using namespace amnezia;

namespace
{
    const char *realityRaw = R"({
  "port": 443,
  "protocol": "vless",
  "settings": {
    "clients": [{ "id": "@auto", "flow": "xtls-rprx-vision" }],
    "decryption": "none"
  },
  "streamSettings": {
    "network": "raw",
    "security": "reality",
    "realitySettings": {
      "target": "www.google.com:443",
      "serverNames": ["www.google.com"],
      "privateKey": "@auto",
      "shortIds": ["@auto"]
    }
  }
})";

    QString withReplaced(const QString &from, const QString &to)
    {
        QString text = QString::fromUtf8(realityRaw);
        text.replace(from, to);
        return text;
    }
}

class TestXrayInboundValidator : public QObject
{
    Q_OBJECT

private slots:
    void validRealityRaw()
    {
        const XrayInboundValidation v = validateXrayInbound(QString::fromUtf8(realityRaw));
        QVERIFY(v.parsed);
        QVERIFY(v.isValid());
        QVERIFY(v.issues.isEmpty());
    }

    void builtinPresetsAreValid()
    {
        for (const XrayPreset &preset : xrayPresets::builtins(QStringLiteral("www.apple.com"), QStringLiteral("443"))) {
            const XrayInboundValidation v = validateXrayInbound(preset.inbound);
            QVERIFY2(v.isValid(), qPrintable(preset.id + QStringLiteral(": ")
                                             + (v.firstError() ? v.firstError()->message : QString())));
        }
    }

    void missingCommaPointsToPreviousValue()
    {
        const QString text = QStringLiteral("{\n  \"port\": 8443,\n  \"protocol\": \"vless\"\n  \"settings\": { \"clients\": [{ \"id\": \"@auto\" }] }\n}");
        const XrayInboundValidation v = validateXrayInbound(text);
        QVERIFY(!v.parsed);
        QCOMPARE(v.issues.size(), 1);
        QCOMPARE(v.issues.first().line, 3);
        QVERIFY(v.issues.first().message.contains(QStringLiteral("missing comma after \"vless\"")));
    }

    void syntaxErrors_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<int>("line");
        QTest::addColumn<QString>("fragment");

        QTest::newRow("trailing comma") << QStringLiteral("{\n  \"port\": 443,\n}") << 2 << QStringLiteral("comma");
        QTest::newRow("duplicate key") << QStringLiteral("{\n  \"port\": 443,\n  \"port\": 444\n}") << 3 << QStringLiteral("duplicate");
        QTest::newRow("single quotes") << QStringLiteral("{\n  'port': 443\n}") << 2 << QStringLiteral("double quotes");
        QTest::newRow("unclosed string") << QStringLiteral("{\n  \"port\n}") << 2 << QStringLiteral("closing quote");
        QTest::newRow("unclosed object") << QStringLiteral("{\n  \"port\": 443") << 2 << QStringLiteral("closing");
        QTest::newRow("trailing text") << QStringLiteral("{}\nx") << 2 << QStringLiteral("after the end");
    }

    void syntaxErrors()
    {
        QFETCH(QString, text);
        QFETCH(int, line);
        QFETCH(QString, fragment);
        const XrayInboundValidation v = validateXrayInbound(text);
        QVERIFY(!v.parsed);
        QCOMPARE(v.issues.first().line, line);
        QVERIFY2(v.issues.first().message.contains(fragment), qPrintable(v.issues.first().message));
    }

    void ruleErrors_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<int>("line");
        QTest::addColumn<QString>("fragment");

        QTest::newRow("vision on xhttp") << withReplaced(QStringLiteral("\"raw\""), QStringLiteral("\"xhttp\"")) << 9
                                         << QStringLiteral("xtls-rprx-vision works only");
        QTest::newRow("reality on mkcp") << withReplaced(QStringLiteral("\"raw\""), QStringLiteral("\"kcp\"")) << 10
                                         << QStringLiteral("Reality works only");
        QTest::newRow("port out of range") << withReplaced(QStringLiteral("443,"), QStringLiteral("70000,")) << 2
                                           << QStringLiteral("port");
        QTest::newRow("not vless") << withReplaced(QStringLiteral("\"vless\""), QStringLiteral("\"vmess\"")) << 3
                                   << QStringLiteral("vless");
        QTest::newRow("no decryption") << withReplaced(QStringLiteral("\"decryption\": \"none\""), QStringLiteral("\"level\": 0")) << 4
                                       << QStringLiteral("decryption");
        QTest::newRow("removed transport") << withReplaced(QStringLiteral("\"raw\""), QStringLiteral("\"h2\"")) << 9
                                           << QStringLiteral("removed");
        QTest::newRow("no private key") << withReplaced(QStringLiteral("\"privateKey\": \"@auto\","), QString()) << 11
                                        << QStringLiteral("privateKey");
        QTest::newRow("short id not hex") << withReplaced(QStringLiteral("[\"@auto\"]"), QStringLiteral("[\"zz\"]")) << 15
                                          << QStringLiteral("shortId");
        QTest::newRow("unknown flow") << withReplaced(QStringLiteral("xtls-rprx-vision"), QStringLiteral("xtls-rprx-direct")) << 5
                                      << QStringLiteral("unknown flow");
        QTest::newRow("client id not auto") << withReplaced(QStringLiteral("\"id\": \"@auto\""), QStringLiteral("\"id\": \"@auto1\"")) << 5
                                            << QStringLiteral("clients id");
        QTest::newRow("client id missing") << withReplaced(QStringLiteral("\"id\": \"@auto\", "), QString()) << 5
                                           << QStringLiteral("clients id");
        QTest::newRow("client flow on the server") << withReplaced(QStringLiteral("\"xtls-rprx-vision\""), QStringLiteral("\"xtls-rprx-vision-udp443\"")) << 5
                                                   << QStringLiteral("client flow");
        QTest::newRow("unsupported network") << withReplaced(QStringLiteral("\"raw\""), QStringLiteral("\"ws\"")) << 9
                                             << QStringLiteral("not supported");
        QTest::newRow("no security") << withReplaced(QStringLiteral("\"security\": \"reality\""), QStringLiteral("\"security\": \"none\"")) << 10
                                     << QStringLiteral("security none");
        QTest::newRow("reality ip server name") << withReplaced(QStringLiteral("[\"www.google.com\"]"), QStringLiteral("[\"1.2.3.4\"]")) << 13
                                                << QStringLiteral("not an IP address");
        QTest::newRow("reality wildcard server name") << withReplaced(QStringLiteral("[\"www.google.com\"]"), QStringLiteral("[\"*.example.com\"]")) << 13
                                                      << QStringLiteral("valid domain name");
    }

    void ruleErrors()
    {
        QFETCH(QString, text);
        QFETCH(int, line);
        QFETCH(QString, fragment);
        const XrayInboundValidation v = validateXrayInbound(text);
        QVERIFY(v.parsed);
        QVERIFY(!v.isValid());
        const XrayInboundIssue *match = nullptr;
        for (const XrayInboundIssue &issue : v.issues) {
            if (issue.level == XrayInboundIssue::Level::Error && issue.message.contains(fragment)) {
                match = &issue;
                break;
            }
        }
        QVERIFY2(match, qPrintable(v.firstError()->message));
        QCOMPARE(match->line, line);
    }

    void xhttpRules()
    {
        const QString base = QStringLiteral(R"({
  "port": 443,
  "protocol": "vless",
  "settings": { "clients": [{ "id": "@auto" }], "decryption": "none" },
  "streamSettings": {
    "network": "xhttp",
    "security": "tls",
    "tlsSettings": { "serverName": "a.com", "certificates": [{ "certificateFile": "@auto", "keyFile": "@auto" }] },
    "xhttpSettings": %1
  }
})");
        QVERIFY(validateXrayInbound(base.arg(QStringLiteral(R"({ "mode": "auto", "xPaddingBytes": "0-0" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "mode": "Auto" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "headers": { "Host": "x" } })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "xPaddingBytes": "0-100" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "uplinkDataPlacement": "header" })"))).isValid());
        QVERIFY(validateXrayInbound(base.arg(QStringLiteral(R"({ "mode": "packet-up", "uplinkDataPlacement": "header" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "xmux": { "maxConnections": 1, "maxConcurrency": 2 } })"))).isValid());
        QVERIFY(validateXrayInbound(base.arg(QStringLiteral(R"({ "xmux": { "maxConnections": 0, "maxConcurrency": "16-32" } })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "xmux": { "maxConcurrency": "a-b" } })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "host": "*.example.com" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "host": "::1" })"))).isValid());
        QVERIFY(validateXrayInbound(base.arg(QStringLiteral(R"({ "host": "cdn.example.com" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "sessionIDPlacement": "none" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "seqKey": "bad key" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "uplinkHTTPMethod": "GET" })"))).isValid());
        QVERIFY(validateXrayInbound(base.arg(QStringLiteral(R"({ "mode": "packet-up", "uplinkHTTPMethod": "GET" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "uplinkDataPlacement": "query" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "xPaddingObfsMode": true, "xPaddingMethod": "random" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "xPaddingObfsMode": false, "xPaddingMethod": "random" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "xPaddingBytes": "100-0" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "xPaddingBytes": "1-2147483648" })"))).isValid());
        QVERIFY(validateXrayInbound(base.arg(QStringLiteral(R"({ "xPaddingBytes": "400-200" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "mode": "packet-up", "uplinkDataPlacement": "Body" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "scMaxBufferedPosts": "5" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "xmux": { "hKeepAlivePeriod": "30" } })"))).isValid());
        QVERIFY(validateXrayInbound(base.arg(QStringLiteral(R"({ "mode": "packet-up", "sessionIDPlacement": "header", "sessionIDKey": "" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "mode": "packet-up", "scMaxEachPostBytes": "0-1000" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "mode": "packet-up", "scMaxEachPostBytes": 500 })"))).isValid());
        QVERIFY(validateXrayInbound(base.arg(QStringLiteral(R"({ "mode": "packet-up", "scMaxEachPostBytes": "0-0" })"))).isValid());
        QVERIFY(validateXrayInbound(base.arg(QStringLiteral(R"({ "mode": "stream-one", "scMaxEachPostBytes": 500 })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "scMaxBufferedPosts": 100001 })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "mode": "stream-up", "scStreamUpServerSecs": "0-80" })"))).isValid());
        QVERIFY(validateXrayInbound(base.arg(QStringLiteral(R"({ "mode": "stream-up", "scStreamUpServerSecs": "20-80" })"))).isValid());
    }

    void tlsRules()
    {
        const QString base = QStringLiteral(R"({
  "port": 443,
  "protocol": "vless",
  "settings": { "clients": [{ "id": "@auto" }], "decryption": "none" },
  "streamSettings": {
    "network": "raw",
    "security": "tls",
    "tlsSettings": %1
  }
})");
        QVERIFY(validateXrayInbound(base.arg(QStringLiteral(R"({ "serverName": "1.2.3.4", "alpn": ["h2", "http/1.1"] })"))).isValid());
        QVERIFY(validateXrayInbound(base.arg(QStringLiteral(R"({ "serverName": "2001:db8::1" })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "alpn": ["h3"] })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "serverName": "*.example.com" })"))).isValid());
    }

    void kcpRules()
    {
        const QString base = QStringLiteral(R"({
  "port": 443,
  "protocol": "vless",
  "settings": { "clients": [{ "id": "@auto" }], "decryption": "none" },
  "streamSettings": {
    "network": "kcp",
    "security": "tls",
    "kcpSettings": %1
  }
})");
        QVERIFY(validateXrayInbound(base.arg(QStringLiteral(R"({ "mtu": 1350, "tti": 50, "uplinkCapacity": 5, "downlinkCapacity": 20, "cwndMultiplier": 1 })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "mtu": 500 })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "tti": 200 })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "uplinkCapacity": 5000 })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "cwndMultiplier": 0 })"))).isValid());
        QVERIFY(!validateXrayInbound(base.arg(QStringLiteral(R"({ "mtu": 1400, "maxSendingWindow": 1000 })"))).isValid());
        QVERIFY(validateXrayInbound(base.arg(QStringLiteral(R"({ "mtu": 1400, "maxSendingWindow": 1400 })"))).isValid());
    }

    void realityXhttpAnyModeIsValid()
    {
        QString text = withReplaced(QStringLiteral("\"raw\""), QStringLiteral("\"xhttp\""));
        text.replace(QStringLiteral(", \"flow\": \"xtls-rprx-vision\""), QString());
        text.replace(QStringLiteral("\"security\": \"reality\","), QStringLiteral("\"security\": \"reality\",\n    \"xhttpSettings\": { \"mode\": \"auto\" },"));
        const XrayInboundValidation v = validateXrayInbound(text);
        QVERIFY2(v.isValid(), qPrintable(v.firstError() ? v.firstError()->message : QString()));
        QVERIFY(v.issues.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestXrayInboundValidator)

#include "testXrayInboundValidator.moc"
