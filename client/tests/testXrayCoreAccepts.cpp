#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>

#include <functional>

#include "core/models/protocols/xrayProtocolConfig.h"
#include "ui/models/protocols/xrayConfigModel.h"
#include "utils/xrayContractCases.h"
#include "utils/xrayMatrix.h"

using namespace amnezia;
using namespace xraytest;

namespace
{
    using JsonEdit = std::function<void(QJsonObject &)>;

    struct Probe
    {
        QString name;
        QString ruleId;
        bool server = false;
        QString transport;
        QString security;
        QString flow;
        QString mode;
        JsonEdit edit;
        QString rejectWith;
    };

    JsonEdit onStream(const QString &key, const JsonEdit &fn)
    {
        return [key, fn](QJsonObject &bound) {
            QJsonObject stream = bound.value("streamSettings").toObject();
            if (key.isEmpty()) {
                fn(stream);
            } else {
                QJsonObject sub = stream.value(key).toObject();
                fn(sub);
                stream[key] = sub;
            }
            bound["streamSettings"] = stream;
        };
    }

    JsonEdit setIn(const QString &key, const QString &field, const QJsonValue &value)
    {
        return onStream(key, [field, value](QJsonObject &obj) { obj[field] = value; });
    }

    JsonEdit removeIn(const QString &key, const QString &field)
    {
        return onStream(key, [field](QJsonObject &obj) { obj.remove(field); });
    }

    JsonEdit onSettings(const JsonEdit &fn)
    {
        return [fn](QJsonObject &bound) {
            QJsonObject settings = bound.value("settings").toObject();
            fn(settings);
            bound["settings"] = settings;
        };
    }

    JsonEdit onUser(const JsonEdit &fn)
    {
        return onSettings([fn](QJsonObject &settings) {
            if (settings.contains("vnext")) {
                QJsonArray vnext = settings.value("vnext").toArray();
                QJsonObject entry = vnext.first().toObject();
                QJsonArray users = entry.value("users").toArray();
                QJsonObject user = users.first().toObject();
                fn(user);
                users[0] = user;
                entry["users"] = users;
                vnext[0] = entry;
                settings["vnext"] = vnext;
            } else {
                QJsonArray clients = settings.value("clients").toArray();
                QJsonObject user = clients.first().toObject();
                fn(user);
                clients[0] = user;
                settings["clients"] = clients;
            }
        });
    }

    void editVless(QJsonObject &config, const QString &listKey, const JsonEdit &edit)
    {
        QJsonArray list = config.value(listKey).toArray();
        for (int i = 0; i < list.size(); ++i) {
            QJsonObject bound = list.at(i).toObject();
            if (bound.value("protocol").toString() != QLatin1String("vless"))
                continue;
            edit(bound);
            list[i] = bound;
        }
        config[listKey] = list;
    }

    QList<Probe> probes()
    {
        const QString vision = QStringLiteral("xtls-rprx-vision");
        const QString one = QStringLiteral("Stream-one");
        const QString packet = QStringLiteral("Packet-up");
        const QString rs = QStringLiteral("realitySettings");
        const QString tls = QStringLiteral("tlsSettings");
        const QString xs = QStringLiteral("xhttpSettings");
        const QString kcp = QStringLiteral("kcpSettings");
        const QString longShortId = QStringLiteral("0123456789abcdef0");

        return {
            { "reality on kcp", "reality-network", false, "raw", "reality", "", "", setIn({}, "network", "kcp"),
              "REALITY only supports RAW, XHTTP and gRPC" },
            { "reality without realitySettings", "reality-settings-required", false, "raw", "reality", "", "",
              removeIn({}, rs), "Empty \"realitySettings\"" },
            { "server flow vision-udp443", "server-flow-no-udp443", true, "raw", "reality", vision, "",
              onUser([](QJsonObject &u) { u["flow"] = "xtls-rprx-vision-udp443"; }), "doesn't support" },
            { "client without encryption", "encryption-required-client", false, "raw", "reality", vision, "",
              onUser([](QJsonObject &u) { u.remove("encryption"); }), "\"encryption\":\"none\"" },
            { "server without decryption", "decryption-required-server", true, "raw", "reality", vision, "",
              onSettings([](QJsonObject &s) { s.remove("decryption"); }), "\"decryption\":\"none\"" },
            { "server client with encryption", "encryption-not-in-inbound", true, "raw", "reality", vision, "",
              onUser([](QJsonObject &u) { u["encryption"] = "none"; }), "should not be in inbound" },
            { "tls fingerprint foo", "tls-fingerprint-known", false, "raw", "tls", "", "", setIn(tls, "fingerprint", "foo"),
              "unknown \"fingerprint\"" },
            { "tls allowInsecure", "tls-allow-insecure-removed", false, "raw", "tls", "", "", setIn(tls, "allowInsecure", true),
              "allowInsecure" },
            { "tls pin 4 hex", "tls-pin-length", false, "raw", "tls", "", "", setIn(tls, "pinnedPeerCertSha256", "abcd"),
              "incorrect pinnedPeerCertSha256 length" },
            { "tls pin 64 hex", "tls-pin-length", false, "raw", "tls", "", "",
              setIn(tls, "pinnedPeerCertSha256", QString(64, QLatin1Char('b'))), "" },
            { "reality fingerprint unsafe", "reality-fingerprint-unsafe", false, "raw", "reality", "", "",
              setIn(rs, "fingerprint", "unsafe"), "invalid \"fingerprint\"" },
            { "reality without publicKey", "reality-client-password", false, "raw", "reality", "", "", removeIn(rs, "publicKey"),
              "empty \"password\"" },
            { "reality publicKey abc", "reality-client-password", false, "raw", "reality", "", "", setIn(rs, "publicKey", "abc"),
              "invalid \"password\"" },
            { "reality client serverNames", "reality-client-no-servernames", false, "raw", "reality", "", "",
              setIn(rs, "serverNames", QJsonArray { "www.example.com" }), "non-empty \"serverNames\"" },
            { "reality shortId 17 hex", "reality-shortid", false, "raw", "reality", "", "", setIn(rs, "shortId", longShortId),
              "too long \"shortId\"" },
            { "reality shortId zz", "reality-shortid", false, "raw", "reality", "", "", setIn(rs, "shortId", "zz"),
              "invalid \"shortId\"" },
            { "reality shortIds 17 hex", "reality-shortid", true, "raw", "reality", "", "",
              setIn(rs, "shortIds", QJsonArray { longShortId }), "too long \"shortIds[" },
            { "reality shortIds zz", "reality-shortid", true, "raw", "reality", "", "", setIn(rs, "shortIds", QJsonArray { "zz" }),
              "invalid \"shortIds[" },
            { "reality spiderX abc", "reality-spiderx", false, "raw", "reality", "", "", setIn(rs, "spiderX", "abc"),
              "invalid \"spiderX\"" },
            { "reality spiderX /abc", "reality-spiderx", false, "raw", "reality", "", "", setIn(rs, "spiderX", "/abc"), "" },
            { "reality without privateKey", "reality-server-required", true, "raw", "reality", "", "", removeIn(rs, "privateKey"),
              "empty \"privateKey\"" },
            { "reality empty serverNames", "reality-server-required", true, "raw", "reality", "", "",
              setIn(rs, "serverNames", QJsonArray()), "empty \"serverNames\"" },
            { "reality empty shortIds", "reality-server-required", true, "raw", "reality", "", "", setIn(rs, "shortIds", QJsonArray()),
              "empty \"shortIds\"" },
            { "reality empty target", "reality-server-required", true, "raw", "reality", "", "", setIn(rs, "dest", ""),
              "valid value for \"target\"" },
            { "xhttp mode foo", "xhttp-mode-enum", false, "xhttp", "tls", "", one, setIn(xs, "mode", "foo"), "unsupported mode" },
            { "server xhttp mode foo", "xhttp-mode-enum", true, "xhttp", "tls", "", one, setIn(xs, "mode", "foo"),
              "unsupported mode" },
            { "xhttp headers Host", "xhttp-headers-host", false, "xhttp", "tls", "", one,
              setIn(xs, "headers", QJsonObject { { "Host", "a.example.com" } }), "can't contain \"host\"" },
            { "cookie data in stream-one", "xhttp-uplink-data-placement-packet-up", false, "xhttp", "tls", "", one,
              setIn(xs, "uplinkDataPlacement", "cookie"), "only in packet-up mode" },
            { "cookie data in packet-up", "xhttp-uplink-data-placement-packet-up", false, "xhttp", "tls", "", packet,
              setIn(xs, "uplinkDataPlacement", "cookie"), "" },
            { "GET in stream-one", "xhttp-get-packet-up", false, "xhttp", "tls", "", one, setIn(xs, "uplinkHTTPMethod", "GET"),
              "GET only in packet-up" },
            { "GET in packet-up", "xhttp-get-packet-up", false, "xhttp", "tls", "", packet, setIn(xs, "uplinkHTTPMethod", "GET"),
              "" },
            { "downloadSettings in stream-one", "xhttp-download-settings-stream-one", false, "xhttp", "tls", "", one,
              setIn(xs, "downloadSettings", QJsonObject()), "\"downloadSettings\" in \"stream-one\"" },
            { "xPaddingBytes 0-100", "xhttp-padding-bytes", false, "xhttp", "tls", "", one, setIn(xs, "xPaddingBytes", "0-100"),
              "cannot be disabled" },
            { "xPaddingBytes -1-5", "xhttp-padding-bytes", false, "xhttp", "tls", "", one, setIn(xs, "xPaddingBytes", "-1-5"),
              "cannot be disabled" },
            { "xPaddingBytes 0-0", "xhttp-padding-bytes", false, "xhttp", "tls", "", one, setIn(xs, "xPaddingBytes", "0-0"), "" },
            { "xPaddingBytes 1-1", "xhttp-padding-bytes", false, "xhttp", "tls", "", one, setIn(xs, "xPaddingBytes", "1-1"), "" },
            { "xPaddingPlacement foo", "xhttp-padding-placement", false, "xhttp", "tls", "", one,
              setIn(xs, "xPaddingPlacement", "foo"), "unsupported padding placement" },
            { "xPaddingMethod foo", "xhttp-padding-method", false, "xhttp", "tls", "", one, setIn(xs, "xPaddingMethod", "foo"),
              "unsupported padding method" },
            { "sessionIDPlacement foo", "xhttp-placement-enum", false, "xhttp", "tls", "", packet,
              setIn(xs, "sessionIDPlacement", "foo"), "unsupported session placement" },
            { "seqPlacement foo", "xhttp-placement-enum", false, "xhttp", "tls", "", packet, setIn(xs, "seqPlacement", "foo"),
              "unsupported seq placement" },
            { "sessionIDTable ab length 1", "xhttp-session-table", false, "xhttp", "tls", "", packet,
              onStream(xs, [](QJsonObject &x) {
                  x["sessionIDTable"] = "ab";
                  x["sessionIDLength"] = 1;
              }),
              "too small" },
            { "xmux concurrency and connections", "xmux-exclusive", false, "xhttp", "tls", "", one,
              setIn(xs, "xmux", QJsonObject { { "maxConcurrency", 1 }, { "maxConnections", 1 } }),
              "maxConnections cannot be specified together" },
            { "kcp header", "kcp-header-removed", false, "mkcp", "tls", "", "", setIn(kcp, "header", QJsonObject { { "type", "none" } }),
              "" },
            { "kcp mtu 20", "kcp-mtu", false, "mkcp", "tls", "", "", setIn(kcp, "mtu", 20), "MTU must be at least 21" },
            { "kcp mtu 21", "kcp-mtu", false, "mkcp", "tls", "", "", setIn(kcp, "mtu", 21), "" },
            { "kcp tti 9", "kcp-tti", false, "mkcp", "tls", "", "", setIn(kcp, "tti", 9), "TTI must be between 10 and 1000" },
            { "kcp tti 1000", "kcp-tti", false, "mkcp", "tls", "", "", setIn(kcp, "tti", 1000), "" },
            { "kcp tti 1001", "kcp-tti", false, "mkcp", "tls", "", "", setIn(kcp, "tti", 1001), "TTI must be between 10 and 1000" },
            { "kcp cwndMultiplier 0", "kcp-cwnd", false, "mkcp", "tls", "", "", setIn(kcp, "cwndMultiplier", 0),
              "CwndMultiplier must be at least 1" },
            { "kcp maxSendingWindow 100", "kcp-sending-window", false, "mkcp", "tls", "", "", setIn(kcp, "maxSendingWindow", 100),
              "MaxSendingWindow must be at least" },
            { "kcp uplinkCapacity -1", "kcp-capacity-overflow", false, "mkcp", "tls", "", "", setIn(kcp, "uplinkCapacity", -1),
              "uint32" },
            { "range abc-5", "", false, "xhttp", "tls", "", packet, setIn(xs, "scMaxEachPostBytes", "abc-5"),
              "Invalid integer range" },
            { "range -1-5", "", false, "xhttp", "tls", "", packet, setIn(xs, "scMaxEachPostBytes", "-1-5"), "" },
            { "security foo", "", false, "raw", "tls", "", "", setIn({}, "security", "foo"), "Unknown security" },
        };
    }

    QString lastLine(const QString &output)
    {
        const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        return lines.isEmpty() ? QString() : lines.last().trimmed().left(240);
    }
    struct CoreCase
    {
        QString name;
        XrayServerConfig srv;
        bool allowedByUi = true;
    };

    QList<CoreCase> optionSweep()
    {
        QList<CoreCase> out;
        for (const QString &security : { QStringLiteral("tls"), QStringLiteral("reality") }) {
            const QStringList fingerprints = security == QLatin1String("reality")
                    ? XrayConfigModel::realityFingerprintOptions()
                    : XrayConfigModel::fingerprintOptions();
            for (const QString &fp : fingerprints) {
                XrayServerConfig srv = baseConfig("raw", security, "xtls-rprx-vision");
                srv.fingerprint = fp;
                out.append({ QStringLiteral("option/raw/%1/fingerprint=%2").arg(security, fp), srv });
            }
        }
        for (const QString &alpn : XrayConfigModel::alpnOptions()) {
            XrayServerConfig srv = baseConfig("xhttp", "tls", "");
            srv.alpn = alpn;
            out.append({ QStringLiteral("option/xhttp/tls/alpn=%1").arg(alpn), srv });
        }
        for (const QString &method : XrayConfigModel::xhttpUplinkMethodOptions()) {
            XrayServerConfig srv = baseConfig("xhttp", "tls", "");
            srv.xhttp.mode = QStringLiteral("Packet-up");
            srv.xhttp.uplinkMethod = method;
            out.append({ QStringLiteral("option/xhttp/packet-up/uplinkMethod=%1").arg(method), srv });
        }
        for (const QString &pl : XrayConfigModel::xhttpSessionPlacementOptions()) {
            XrayServerConfig srv = baseConfig("xhttp", "tls", "");
            srv.xhttp.mode = QStringLiteral("Packet-up");
            srv.xhttp.sessionPlacement = pl;
            srv.xhttp.seqPlacement = pl;
            out.append({ QStringLiteral("option/xhttp/packet-up/session+seqPlacement=%1").arg(pl), srv });
        }
        for (const QString &pl : XrayConfigModel::xPaddingPlacementOptions()) {
            for (const QString &method : XrayConfigModel::xPaddingMethodOptions()) {
                XrayServerConfig srv = baseConfig("xhttp", "reality", "");
                srv.xhttp.xPadding.obfsMode = true;
                srv.xhttp.xPadding.placement = pl;
                srv.xhttp.xPadding.method = method;
                srv.xhttp.xPadding.bytesMin = QStringLiteral("100");
                srv.xhttp.xPadding.bytesMax = QStringLiteral("1000");
                out.append({ QStringLiteral("option/xhttp/xPadding/%1/%2").arg(pl, method), srv });
            }
        }
        for (const QString &mode : XrayConfigModel::xhttpModeOptions()) {
            for (const QString &pl : XrayConfigModel::xhttpUplinkDataPlacementOptions()) {
                XrayServerConfig srv = baseConfig("xhttp", "tls", "");
                srv.xhttp.mode = mode;
                srv.xhttp.uplinkDataPlacement = pl;
                out.append({ QStringLiteral("option/xhttp/tls/mode=%1/dataPlacement=%2").arg(mode, pl), srv });
            }
        }
        return out;
    }

    const QList<CoreCase> &cases()
    {
        static const QList<CoreCase> all = [] {
            QList<CoreCase> out;
            for (const MatrixCase &m : matrix())
                out.append({ m.name, m.srv, m.allowedByUi });
            out.append(optionSweep());
            return out;
        }();
        return all;
    }

    QString fileNameFor(const QString &name)
    {
        QString out = name;
        for (QChar &ch : out) {
            if (!ch.isLetterOrNumber() && ch != QLatin1Char('-'))
                ch = QLatin1Char('_');
        }
        return out;
    }
}

class TestXrayCoreAccepts : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void generatedConfigs_data();
    void generatedConfigs();
    void coreRejectsWhatWeRelyOn_data();
    void coreRejectsWhatWeRelyOn();
    void contract_data();
    void contract();
    void coreBoundaries_data();
    void coreBoundaries();

private:
    struct Run
    {
        int exitCode = -1;
        QString output;
    };

    Run xrayTest(const QJsonObject &config, const QString &fileName);
    void useLocalCertificate(QJsonObject &serverConfig) const;

    QString m_xray;
    bool m_coreBypassesNoTlsRule = false;
    QTemporaryDir m_dir;
    QString m_certFile;
    QString m_keyFile;
};

void TestXrayCoreAccepts::initTestCase()
{
    m_xray = qEnvironmentVariable("XRAY_BIN");
    if (m_xray.isEmpty() || !QFileInfo(m_xray).isExecutable())
        QSKIP("Set XRAY_BIN to an xray v26.10.31 or v26.7.28 binary to run the core checks");
    QVERIFY(m_dir.isValid());

    QProcess version;
    version.start(m_xray, { QStringLiteral("version") });
    QVERIFY(version.waitForFinished(30000));
    const QString firstLine = QString::fromUtf8(version.readAllStandardOutput()).section(QLatin1Char('\n'), 0, 0);
    qInfo().noquote() << "xray under test:" << firstLine;
    m_coreBypassesNoTlsRule = firstLine.contains(QLatin1String("26.7.28"));
    if (!m_coreBypassesNoTlsRule && !firstLine.contains(QLatin1String("26.10.31")))
        qWarning().noquote() << "XRAY_BIN is neither v26.10.31 nor v26.7.28, results may differ from the VPS";

    const QString prefix = m_dir.filePath(QStringLiteral("tls"));
    QProcess cert;
    cert.start(m_xray, { QStringLiteral("tls"), QStringLiteral("cert"), QStringLiteral("-domain"),
                         QStringLiteral("amnezia-xray"), QStringLiteral("-file"), prefix });
    QVERIFY(cert.waitForFinished(30000));
    m_certFile = prefix + QStringLiteral(".crt");
    m_keyFile = prefix + QStringLiteral(".key");
    QVERIFY2(QFile::exists(m_certFile) && QFile::exists(m_keyFile),
             qPrintable(QString::fromUtf8(cert.readAllStandardError())));
}

TestXrayCoreAccepts::Run TestXrayCoreAccepts::xrayTest(const QJsonObject &config, const QString &fileName)
{
    const QString path = m_dir.filePath(fileName + QStringLiteral(".json"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return { -1, QStringLiteral("cannot write ") + path };
    file.write(QJsonDocument(config).toJson());
    file.close();

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(m_xray, { QStringLiteral("-test"), QStringLiteral("-format"), QStringLiteral("json"),
                            QStringLiteral("-config"), path });
    if (!process.waitForFinished(30000))
        return { -1, QStringLiteral("xray -test timed out") };
    Run run;
    run.exitCode = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
    run.output = QString::fromUtf8(process.readAll()).trimmed();
    return run;
}

void TestXrayCoreAccepts::useLocalCertificate(QJsonObject &serverConfig) const
{
    QJsonArray inbounds = serverConfig.value("inbounds").toArray();
    QJsonObject inbound = inbounds.first().toObject();
    QJsonObject stream = inbound.value("streamSettings").toObject();
    QJsonObject tls = stream.value("tlsSettings").toObject();
    if (tls.isEmpty())
        return;
    tls["certificates"] = QJsonArray { QJsonObject { { "certificateFile", m_certFile }, { "keyFile", m_keyFile } } };
    stream["tlsSettings"] = tls;
    inbound["streamSettings"] = stream;
    inbounds[0] = inbound;
    serverConfig["inbounds"] = inbounds;
}

void TestXrayCoreAccepts::generatedConfigs_data()
{
    QTest::addColumn<int>("index");
    for (int i = 0; i < cases().size(); ++i)
        QTest::newRow(qPrintable(cases().at(i).name)) << i;
}

void TestXrayCoreAccepts::generatedConfigs()
{
    QFETCH(int, index);
    const CoreCase &c = cases().at(index);

    QJsonObject server = serverJson(c.srv);
    useLocalCertificate(server);
    const QJsonObject client = clientJson(c.srv);
    const QString base = fileNameFor(c.name);

    const Run serverRun = xrayTest(server, base + QStringLiteral("_server"));
    const Run clientRun = xrayTest(client, base + QStringLiteral("_client"));

    if (c.name.endsWith(QLatin1String("xmux-concurrency-and-connections"))) {
        QVERIFY2(clientRun.exitCode != 0, qPrintable(clientRun.output));
        QVERIFY2(clientRun.output.contains(QLatin1String("maxConnections cannot be specified together")),
                 qPrintable(clientRun.output));
        return;
    }
    if (!c.allowedByUi) {
        QVERIFY2(serverRun.exitCode != 0 || clientRun.exitCode != 0,
                 qPrintable(QStringLiteral("validationErrors() blocks a config the core accepts\n") + compact(client)));
        return;
    }

    QVERIFY2(serverRun.exitCode == 0, qPrintable(QStringLiteral("server.json rejected: ") + serverRun.output
                                                 + QStringLiteral("\n") + compact(server)));
    QVERIFY2(clientRun.exitCode == 0, qPrintable(QStringLiteral("client config rejected: ") + clientRun.output
                                                 + QStringLiteral("\n") + compact(client)));
}

void TestXrayCoreAccepts::coreRejectsWhatWeRelyOn_data()
{
    QTest::addColumn<QString>("what");
    QTest::addColumn<QString>("expected");
    QTest::newRow("inbound flow with -udp443") << "udp443" << "doesn't support";
    QTest::newRow("cookie uplinkDataPlacement outside packet-up") << "cookie" << "only in packet-up";
    QTest::newRow("GET outside packet-up") << "get" << "GET only in packet-up";
    QTest::newRow("allowInsecure") << "insecure" << "allowInsecure";
    QTest::newRow("vless without TLS to a public address") << "none" << "vless without TLS";
}

void TestXrayCoreAccepts::coreRejectsWhatWeRelyOn()
{
    QFETCH(QString, what);
    QFETCH(QString, expected);

    QJsonObject config;
    if (what == QLatin1String("udp443")) {
        XrayServerConfig srv = baseConfig("raw", "reality", "xtls-rprx-vision");
        XrayServerInboundInputs in = inboundInputs(srv);
        in.clients = QJsonArray { XrayServerConfig::makeClientEntry(clientId(), "xtls-rprx-vision-udp443") };
        config = srv.toServerInboundJson(in);
    } else if (what == QLatin1String("none")) {
        if (m_coreBypassesNoTlsRule)
            QSKIP("xray v26.7.28 lets VLESS outbounds in the vnext style skip this check");
        config = clientJson(baseConfig("raw", "none", ""));
    } else {
        XrayServerConfig srv = baseConfig("xhttp", "tls", "");
        srv.xhttp.mode = QStringLiteral("Stream-one");
        config = clientJson(srv);
        QJsonArray outbounds = config.value("outbounds").toArray();
        for (int i = 0; i < outbounds.size(); ++i) {
            QJsonObject ob = outbounds.at(i).toObject();
            if (ob.value("protocol").toString() != QLatin1String("vless"))
                continue;
            QJsonObject stream = ob.value("streamSettings").toObject();
            if (what == QLatin1String("insecure")) {
                QJsonObject tls = stream.value("tlsSettings").toObject();
                tls["allowInsecure"] = true;
                stream["tlsSettings"] = tls;
            } else {
                QJsonObject x = stream.value("xhttpSettings").toObject();
                if (what == QLatin1String("cookie"))
                    x["uplinkDataPlacement"] = QStringLiteral("cookie");
                else
                    x["uplinkHTTPMethod"] = QStringLiteral("GET");
                stream["xhttpSettings"] = x;
            }
            ob["streamSettings"] = stream;
            outbounds[i] = ob;
        }
        config["outbounds"] = outbounds;
    }

    const Run run = xrayTest(config, QStringLiteral("negative_") + what);
    QVERIFY2(run.exitCode != 0, qPrintable(QStringLiteral("xray accepted it: ") + compact(config)));
    QVERIFY2(run.output.contains(expected), qPrintable(run.output));
}

void TestXrayCoreAccepts::contract_data()
{
    QTest::addColumn<int>("index");
    for (int i = 0; i < contractCases().size(); ++i)
        QTest::newRow(qPrintable(contractCases().at(i).name)) << i;
}

void TestXrayCoreAccepts::contract()
{
    QFETCH(int, index);
    const ContractCase &c = contractCases().at(index);

    XrayServerConfig srv;
    const QStringList errors = contractErrors(c, &srv);
    const AppVerdict app = errors.isEmpty() ? AppVerdict::Ok : AppVerdict::Blocked;

    QJsonObject server = serverJson(srv);
    useLocalCertificate(server);
    const QJsonObject client = clientJson(srv);
    const QString base = QStringLiteral("contract_%1").arg(index);
    const Run serverRun = xrayTest(server, base + QStringLiteral("_server"));
    const Run clientRun = xrayTest(client, base + QStringLiteral("_client"));
    const CoreVerdict core = serverRun.exitCode == 0 && clientRun.exitCode == 0 ? CoreVerdict::Accept : CoreVerdict::Reject;

    QString coreOutput;
    if (serverRun.exitCode != 0)
        coreOutput += QStringLiteral("server: ") + serverRun.output + QLatin1Char('\n');
    if (clientRun.exitCode != 0)
        coreOutput += QStringLiteral("client: ") + clientRun.output + QLatin1Char('\n');

    qInfo().noquote() << QStringLiteral("contract app=%1 core=%2 rule=%3 input=%4 | %5%6")
                                 .arg(verdictName(app), verdictName(core), c.ruleId.isEmpty() ? QStringLiteral("-") : c.ruleId,
                                      c.viaUi ? QStringLiteral("ui") : QStringLiteral("import"), c.name,
                                      core == CoreVerdict::Reject ? QStringLiteral(" | ") + lastLine(coreOutput) : QString());

    const QString configs = QStringLiteral("\nclient: ") + compact(client) + QStringLiteral("\nserver: ") + compact(server);
    if (c.core == CoreVerdict::Explore) {
        if (c.knownIssue.isEmpty())
            QVERIFY2(!(app == AppVerdict::Ok && core == CoreVerdict::Reject),
                     qPrintable(QStringLiteral("the app passes a config the core rejects\n") + coreOutput + configs));
        QSKIP(qPrintable(QStringLiteral("EXPLORE app=%1 core=%2").arg(verdictName(app), verdictName(core))));
    }

    QVERIFY2(core == c.core, qPrintable(QStringLiteral("core %1, expected %2\n%3%4")
                                                .arg(verdictName(core), verdictName(c.core), coreOutput, configs)));
    if (c.core == CoreVerdict::Reject && !c.coreMessage.isEmpty())
        QVERIFY2(coreOutput.contains(c.coreMessage), qPrintable(coreOutput));
    if (!c.knownIssue.isEmpty())
        QEXPECT_FAIL("", qPrintable(c.knownIssue), Abort);
    QVERIFY2(app == c.app, qPrintable(QStringLiteral("app %1, expected %2; errors: %3")
                                              .arg(verdictName(app), verdictName(c.app), errors.join(QStringLiteral("; ")))));
}

void TestXrayCoreAccepts::coreBoundaries_data()
{
    QTest::addColumn<int>("index");
    const QList<Probe> all = probes();
    for (int i = 0; i < all.size(); ++i)
        QTest::newRow(qPrintable((all.at(i).server ? QStringLiteral("server/") : QStringLiteral("client/")) + all.at(i).name)) << i;
}

void TestXrayCoreAccepts::coreBoundaries()
{
    QFETCH(int, index);
    const Probe p = probes().at(index);

    XrayServerConfig srv = baseConfig(p.transport, p.security, p.flow);
    if (!p.mode.isEmpty())
        srv.xhttp.mode = p.mode;
    QJsonObject config = p.server ? serverJson(srv) : clientJson(srv);
    if (p.server)
        useLocalCertificate(config);
    editVless(config, p.server ? QStringLiteral("inbounds") : QStringLiteral("outbounds"), p.edit);

    const Run run = xrayTest(config, QStringLiteral("probe_%1").arg(index));
    qInfo().noquote() << QStringLiteral("probe core=%1 rule=%2 | %3%4")
                                 .arg(run.exitCode == 0 ? QStringLiteral("accept") : QStringLiteral("reject"),
                                      p.ruleId.isEmpty() ? QStringLiteral("-") : p.ruleId, p.name,
                                      run.exitCode == 0 ? QString() : QStringLiteral(" | ") + lastLine(run.output));

    if (p.rejectWith.isEmpty()) {
        QVERIFY2(run.exitCode == 0, qPrintable(run.output + QStringLiteral("\n") + compact(config)));
        return;
    }
    QVERIFY2(run.exitCode != 0, qPrintable(QStringLiteral("xray accepted it: ") + compact(config)));
    QVERIFY2(run.output.contains(p.rejectWith), qPrintable(run.output));
}

QTEST_GUILESS_MAIN(TestXrayCoreAccepts)

#include "testXrayCoreAccepts.moc"
