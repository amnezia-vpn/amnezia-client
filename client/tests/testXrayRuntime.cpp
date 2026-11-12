#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSslCertificate>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

#include <cstdio>
#include <functional>

#include "core/models/protocols/xrayProtocolConfig.h"
#include "ui/models/protocols/xrayConfigModel.h"
#include "utils/xrayMatrix.h"
#include "utils/xrayRuntime.h"

using namespace amnezia;
using namespace xraytest;

namespace
{
    using JsonEdit = std::function<void(QJsonObject &server, QJsonObject &client)>;

    struct RuntimeCase
    {
        QString name;
        XrayServerConfig srv;
        JsonEdit edit;
        bool traffic = true;
        QString sinceServer;
    };

    const QString kSite = QStringLiteral("www.googletagmanager.com");
    const QHostAddress kTarget(QStringLiteral("203.0.113.80"));

    void setClientUserFlow(QJsonObject &client, const QString &flow)
    {
        QJsonArray outbounds = client.value("outbounds").toArray();
        for (int i = 0; i < outbounds.size(); ++i) {
            QJsonObject outbound = outbounds.at(i).toObject();
            if (outbound.value("protocol").toString() != QLatin1String("vless"))
                continue;
            QJsonObject settings = outbound.value("settings").toObject();
            QJsonArray vnext = settings.value("vnext").toArray();
            QJsonObject server = vnext.first().toObject();
            QJsonArray users = server.value("users").toArray();
            QJsonObject user = users.first().toObject();
            user["flow"] = flow;
            users[0] = user;
            server["users"] = users;
            vnext[0] = server;
            settings["vnext"] = vnext;
            outbound["settings"] = settings;
            outbounds[i] = outbound;
        }
        client["outbounds"] = outbounds;
    }

    void setServerClientFlow(QJsonObject &server, const QString &flow)
    {
        QJsonArray inbounds = server.value("inbounds").toArray();
        QJsonObject inbound = inbounds.first().toObject();
        QJsonObject settings = inbound.value("settings").toObject();
        QJsonArray clients = settings.value("clients").toArray();
        QJsonObject entry = clients.first().toObject();
        entry["flow"] = flow;
        clients[0] = entry;
        settings["clients"] = clients;
        inbound["settings"] = settings;
        inbounds[0] = inbound;
        server["inbounds"] = inbounds;
    }

    XrayServerConfig runtimeConfig(const QString &transport, const QString &security, const QString &flow,
                                   const QString &mode = QString())
    {
        XrayServerConfig srv = baseConfig(transport, security, flow);
        if (!mode.isEmpty())
            srv.xhttp.mode = mode;
        return srv;
    }

    QList<RuntimeCase> optionCases()
    {
        QList<RuntimeCase> out;
        for (const QString &fp : XrayConfigModel::realityFingerprintOptions()) {
            XrayServerConfig srv = runtimeConfig("raw", "reality", "xtls-rprx-vision");
            srv.fingerprint = fp;
            out.append({ QStringLiteral("option/raw/reality/fingerprint=%1").arg(fp), srv });
        }
        for (const QString &fp : XrayConfigModel::fingerprintOptions()) {
            XrayServerConfig srv = runtimeConfig("raw", "tls", "");
            srv.fingerprint = fp;
            out.append({ QStringLiteral("option/raw/tls/fingerprint=%1").arg(fp), srv });
        }
        for (const QString &alpn : XrayConfigModel::alpnOptions()) {
            XrayServerConfig srv = runtimeConfig("xhttp", "tls", "", "Stream-one");
            srv.alpn = alpn;
            out.append({ QStringLiteral("option/xhttp/tls/alpn=%1").arg(alpn), srv });
        }
        for (const QString &method : XrayConfigModel::xhttpUplinkMethodOptions()) {
            XrayServerConfig srv = runtimeConfig("xhttp", "tls", "", "Packet-up");
            srv.xhttp.uplinkMethod = method;
            out.append({ QStringLiteral("option/xhttp/tls/packet-up/uplinkMethod=%1").arg(method), srv });
        }
        for (const QString &mtu : { QStringLiteral("576"), QStringLiteral("1460") }) {
            XrayServerConfig srv = runtimeConfig("mkcp", "tls", "");
            srv.mkcp.mtu = mtu;
            out.append({ QStringLiteral("option/mkcp/tls/mtu=%1").arg(mtu), srv });
        }
        for (const QString &tti : { QStringLiteral("10"), QStringLiteral("100") }) {
            XrayServerConfig srv = runtimeConfig("mkcp", "tls", "");
            srv.mkcp.tti = tti;
            out.append({ QStringLiteral("option/mkcp/tls/tti=%1").arg(tti), srv });
        }
        return out;
    }

    QList<RuntimeCase> ruleCases()
    {
        const QString vision = QStringLiteral("xtls-rprx-vision");
        QList<RuntimeCase> out;

        out.append({ "rule/vision-needs-raw", runtimeConfig("xhttp", "reality", "", "Stream-one"),
                     [vision](QJsonObject &server, QJsonObject &client) {
                         setServerClientFlow(server, vision);
                         setClientUserFlow(client, vision);
                     },
                     false });
        out.append({ "rule/vision-needs-tls13", runtimeConfig("raw", "tls", vision),
                     [](QJsonObject &server, QJsonObject &) {
                         editSection(server, "inbounds", "tlsSettings",
                                     [](QJsonObject &tls) { tls["maxVersion"] = QStringLiteral("1.2"); });
                     },
                     false });
        out.append({ "rule/flow-server-vision-client-empty", runtimeConfig("raw", "reality", vision),
                     [](QJsonObject &, QJsonObject &client) { setClientUserFlow(client, QString()); }, false });
        out.append({ "rule/flow-client-vision-server-empty", runtimeConfig("raw", "reality", ""),
                     [vision](QJsonObject &, QJsonObject &client) { setClientUserFlow(client, vision); }, false });
        out.append({ "rule/vision-mux-tcp", runtimeConfig("raw", "reality", vision),
                     [](QJsonObject &, QJsonObject &client) {
                         QJsonArray outbounds = client.value("outbounds").toArray();
                         QJsonObject outbound = outbounds.first().toObject();
                         outbound["mux"] = QJsonObject { { "enabled", true } };
                         outbounds[0] = outbound;
                         client["outbounds"] = outbounds;
                     },
                     false });
        out.append({ "rule/reality-client-hello-mlkem", runtimeConfig("raw", "reality", ""),
                     [](QJsonObject &, QJsonObject &client) {
                         editSection(client, "outbounds", "realitySettings",
                                     [](QJsonObject &r) { r["fingerprint"] = QStringLiteral("hellochrome_120"); });
                     },
                     false, "26.10.17" });
        out.append({ "rule/reality-fingerprint-tls12", runtimeConfig("raw", "reality", ""),
                     [](QJsonObject &, QJsonObject &client) {
                         editSection(client, "outbounds", "realitySettings",
                                     [](QJsonObject &r) { r["fingerprint"] = QStringLiteral("android"); });
                     },
                     false });
        out.append({ "rule/vision-fingerprint-tls12", runtimeConfig("raw", "tls", vision),
                     [](QJsonObject &, QJsonObject &client) {
                         editSection(client, "outbounds", "tlsSettings",
                                     [](QJsonObject &tls) { tls["fingerprint"] = QStringLiteral("android"); });
                     },
                     false });
        out.append({ "rule/reality-sni-ip", runtimeConfig("raw", "reality", ""),
                     [](QJsonObject &, QJsonObject &client) {
                         editSection(client, "outbounds", "realitySettings",
                                     [](QJsonObject &r) { r["serverName"] = QStringLiteral("1.1.1.1"); });
                     },
                     false });

        const QList<QStringList> modeClashes = {
            { "rule/xhttp-server-mode-stream-one", "tls", "Packet-up", "stream-one" },
            { "rule/xhttp-server-mode-stream-up", "tls", "Packet-up", "stream-up" },
            { "rule/xhttp-server-mode-packet-up", "tls", "Stream-one", "packet-up" },
            { "rule/xhttp-auto-vs-server-stream-one", "tls", "Auto", "stream-one" },
            { "rule/xhttp-auto-vs-server-packet-up", "reality", "Auto", "packet-up" },
        };
        for (const QStringList &clash : modeClashes) {
            const QString serverMode = clash.at(3);
            out.append({ clash.at(0), runtimeConfig("xhttp", clash.at(1), "", clash.at(2)),
                         [serverMode](QJsonObject &server, QJsonObject &) {
                             editSection(server, "inbounds", "xhttpSettings",
                                         [serverMode](QJsonObject &x) { x["mode"] = serverMode; });
                         },
                         false });
        }

        XrayServerConfig noH2 = runtimeConfig("xhttp", "tls", "", "Stream-one");
        noH2.alpn = QStringLiteral("h2");
        out.append({ "rule/xhttp-fingerprint-no-h2", noH2,
                     [](QJsonObject &, QJsonObject &client) {
                         editSection(client, "outbounds", "tlsSettings",
                                     [](QJsonObject &tls) { tls["fingerprint"] = QStringLiteral("android"); });
                     },
                     false });

        const QList<QStringList> brokenPostBytes = {
            { "rule/xhttp-sc-max-each-post-bytes", "0-100" },
            { "rule/xhttp-range-reversed-zero", "1000000-0" },
        };
        for (const QStringList &broken : brokenPostBytes) {
            const QString range = broken.at(1);
            out.append({ broken.at(0), runtimeConfig("xhttp", "tls", "", "Packet-up"),
                         [range](QJsonObject &, QJsonObject &client) {
                             editSection(client, "outbounds", "xhttpSettings",
                                         [range](QJsonObject &x) { x["scMaxEachPostBytes"] = range; });
                         },
                         false });
        }

        out.append({ "rule/xhttp-auto-resolve-reality", runtimeConfig("xhttp", "reality", "", "Auto") });
        out.append({ "rule/xhttp-auto-resolve-other", runtimeConfig("xhttp", "tls", "", "Auto") });

        XrayServerConfig unlimited = runtimeConfig("xhttp", "tls", "", "Stream-one");
        unlimited.xhttp.xmux.enabled = true;
        unlimited.xhttp.xmux.cMaxReuseTimesMin = QStringLiteral("0");
        unlimited.xhttp.xmux.cMaxReuseTimesMax = QStringLiteral("0");
        out.append({ "rule/xmux-zero-unlimited", unlimited });

        XrayServerConfig capacity = runtimeConfig("mkcp", "tls", "");
        capacity.mkcp.uplinkCapacity = QStringLiteral("5000");
        capacity.mkcp.downlinkCapacity = QStringLiteral("5000");
        out.append({ "rule/kcp-capacity-overflow", capacity });

        return out;
    }

    QList<RuntimeCase> fingerprintCases()
    {
        const QList<QPair<QString, XrayServerConfig>> realityTransports = {
            { "raw-vision", runtimeConfig("raw", "reality", "xtls-rprx-vision") },
            { "xhttp-stream-one", runtimeConfig("xhttp", "reality", "", "Stream-one") },
            { "xhttp-packet-up", runtimeConfig("xhttp", "reality", "", "Packet-up") },
        };
        QList<RuntimeCase> out;
        for (const RealityFingerprintCase &fp : realityFingerprintCases()) {
            if (fp.hello == RealityHello::Unreliable)
                continue;
            const QString name = fp.name;
            const QString since =
                    fp.hello == RealityHello::RealCertificate ? QStringLiteral("26.10.17") : QString();
            for (const auto &transport : realityTransports) {
                out.append({ QStringLiteral("fingerprint/core/%1/%2").arg(transport.first, name), transport.second,
                             [name](QJsonObject &, QJsonObject &client) { setClientRealityFingerprint(client, name); },
                             fp.hello == RealityHello::Connects, since });
            }
        }

        for (const QString &stored : legacyRealityFingerprints()) {
            for (const auto &transport : realityTransports) {
                XrayServerConfig srv = transport.second;
                srv.fingerprint = stored;
                out.append({ QStringLiteral("fingerprint/app/%1/'%2'").arg(transport.first, stored), srv });
            }
        }

        XrayServerConfig xhttpTls = runtimeConfig("xhttp", "tls", "", "Stream-one");
        xhttpTls.alpn = QStringLiteral("h2");
        const QList<QPair<QString, XrayServerConfig>> tlsTransports = {
            { "raw-vision", runtimeConfig("raw", "tls", "xtls-rprx-vision") },
            { "xhttp-stream-one-h2", xhttpTls },
        };
        for (const QString &fp : XrayConfigModel::fingerprintOptions()) {
            const bool tls12Only = fp == QLatin1String("android") || fp == QLatin1String("360");
            for (const auto &transport : tlsTransports) {
                XrayServerConfig srv = transport.second;
                srv.fingerprint = fp;
                out.append({ QStringLiteral("fingerprint/tls/%1/%2").arg(transport.first, fp), srv, {}, !tls12Only });
            }
        }
        return out;
    }

    const QList<RuntimeCase> &cases()
    {
        static const QList<RuntimeCase> all = [] {
            QList<RuntimeCase> out;
            for (const MatrixCase &m : matrix()) {
                if (m.allowedByUi)
                    out.append({ m.name, m.srv });
            }
            out << optionCases() << ruleCases() << fingerprintCases();
            return out;
        }();
        return all;
    }
}

class TestXrayRuntime : public QObject
{
    Q_OBJECT

private:
    QString m_serverBin;
    QString m_clientBin;
    QString m_serverVersion;
    QTemporaryDir m_dir;
    QString m_certFile;
    QString m_keyFile;
    QString m_certPin;
    QString m_realityPrivateKey;
    QString m_realityPublicKey;
    QTcpServer m_http;
    QString m_lastReason;
    QStringList m_failures;
    int m_casesRun = 0;
    bool m_ready = false;

    QString writeConfig(const QJsonObject &config, const QString &name)
    {
        const QString path = m_dir.filePath(name + QStringLiteral(".json"));
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            file.write(QJsonDocument(config).toJson());
        return path;
    }

    void answerHttp()
    {
        while (QTcpSocket *socket = m_http.nextPendingConnection()) {
            connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                const QByteArray request = socket->peek(socket->bytesAvailable());
                if (!request.contains("\r\n\r\n"))
                    return;
                const QByteArray path = request.split(' ').value(1);
                const QByteArray body = path.mid(1);
                socket->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(body.size())
                              + "\r\nConnection: close\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    }

    QJsonObject serverConfig(const XrayServerConfig &srv) const
    {
        XrayServerInboundInputs in;
        in.clients = QJsonArray { XrayServerConfig::makeClientEntry(clientId(), xrayEffective::serverFlow(srv)) };
        in.realityPrivateKey = m_realityPrivateKey;
        in.realityShortId = shortId();
        QJsonObject server = srv.toServerInboundJson(in);

        QJsonArray inbounds = server.value("inbounds").toArray();
        QJsonObject inbound = inbounds.first().toObject();
        inbound["listen"] = QStringLiteral("127.0.0.1");
        inbounds[0] = inbound;
        server["inbounds"] = inbounds;
        editSection(server, "inbounds", "tlsSettings", [this](QJsonObject &tls) {
            tls["certificates"] =
                    QJsonArray { QJsonObject { { "certificateFile", m_certFile }, { "keyFile", m_keyFile } } };
        });
        server["log"] = QJsonObject { { "loglevel", "warning" } };
        server["outbounds"] = QJsonArray { QJsonObject {
                { "protocol", "freedom" },
                { "settings",
                  QJsonObject { { "redirect", QStringLiteral("127.0.0.1:%1").arg(m_http.serverPort()) },
                                { "finalRules",
                                  QJsonArray { QJsonObject { { "action", "allow" },
                                                             { "ip", QJsonArray { "127.0.0.1/32" } } } } } } } } };
        return server;
    }

    QJsonObject clientConfig(const XrayServerConfig &srv, int socksPort) const
    {
        XrayClientOutboundInputs in;
        in.serverAddress = QStringLiteral("127.0.0.1");
        in.clientId = clientId();
        in.realityPublicKey = m_realityPublicKey;
        in.realityShortId = shortId();
        in.tlsPinnedPeerCertSha256 = m_certPin;
        XrayProtocolConfig cfg;
        cfg.serverConfig = srv;
        QJsonObject client = cfg.toClientOutboundJson(in);

        QJsonArray inbounds = client.value("inbounds").toArray();
        QJsonObject socks = inbounds.first().toObject();
        socks["listen"] = QStringLiteral("127.0.0.1");
        socks["port"] = socksPort;
        inbounds[0] = socks;
        client["inbounds"] = inbounds;
        client["log"] = QJsonObject { { "loglevel", "warning" } };
        return client;
    }

    bool runCase(const RuntimeCase &c, QString &why)
    {
        XrayServerConfig srv = c.srv;
        srv.port = QString::number(xrayruntime::freeLocalPort());
        srv.sni = kSite;
        srv.site = kSite;
        QJsonObject server = serverConfig(srv);
        const int socksPort = xrayruntime::freeLocalPort();
        QJsonObject client = clientConfig(srv, socksPort);
        if (c.edit)
            c.edit(server, client);

        const QString id = QUuid::createUuid().toString(QUuid::Id128);
        xrayruntime::XrayRun serverRun;
        xrayruntime::XrayRun clientRun;
        auto logs = [&] {
            const QString serverLog = xrayruntime::collapsedLog(serverRun.stop());
            const QString clientLog = xrayruntime::collapsedLog(clientRun.stop());
            QStringList parts;
            if (!serverLog.isEmpty())
                parts << QStringLiteral("server: ") + serverLog;
            if (!clientLog.isEmpty())
                parts << QStringLiteral("client: ") + clientLog;
            return parts.join(QStringLiteral(" | "));
        };

        if (!serverRun.start(m_serverBin, writeConfig(server, id + QStringLiteral("_server")))) {
            why = QStringLiteral("server did not start | ") + logs();
            return false;
        }
        if (!clientRun.start(m_clientBin, writeConfig(client, id + QStringLiteral("_client")))) {
            why = QStringLiteral("client did not start | ") + logs();
            return false;
        }

        const QByteArray path = "/" + id.toLatin1();
        QString error;
        const int attempts = c.traffic ? 2 : 1;
        bool flowed = false;
        for (int attempt = 0; attempt < attempts && !flowed; ++attempt) {
            error.clear();
            xrayruntime::httpGetThroughSocks(socksPort, kTarget, 80, path, 8000, error);
            flowed = error.isEmpty();
        }
        const QString runLog = logs();
        why = flowed ? QStringLiteral("traffic flowed") : error;
        if (!runLog.isEmpty())
            why += QStringLiteral(" | ") + runLog;
        return flowed;
    }

private slots:
    void initTestCase()
    {
        const QString bin = qEnvironmentVariable("XRAY_BIN");
        m_serverBin = qEnvironmentVariable("XRAY_SERVER_BIN", bin);
        m_clientBin = qEnvironmentVariable("XRAY_CLIENT_BIN", bin);
        if (!QFileInfo(m_serverBin).isExecutable() || !QFileInfo(m_clientBin).isExecutable())
            QSKIP("Set XRAY_BIN, or XRAY_SERVER_BIN and XRAY_CLIENT_BIN, to xray binaries");
        QVERIFY(m_dir.isValid());

        m_serverVersion = xrayruntime::version(m_serverBin);
        qInfo().noquote() << "server xray" << m_serverVersion << "client xray" << xrayruntime::version(m_clientBin);

        const QString prefix = m_dir.filePath(QStringLiteral("tls"));
        QProcess cert;
        cert.start(m_serverBin, { "tls", "cert", "-domain", "amnezia-xray", "-file", prefix });
        QVERIFY(cert.waitForFinished(30000));
        m_certFile = prefix + QStringLiteral(".crt");
        m_keyFile = prefix + QStringLiteral(".key");
        const QList<QSslCertificate> certs = QSslCertificate::fromPath(m_certFile);
        QVERIFY2(!certs.isEmpty(), "xray tls cert did not produce a certificate");
        m_certPin = QString::fromLatin1(certs.first().digest(QCryptographicHash::Sha256).toHex());

        QProcess keys;
        keys.start(m_serverBin, { "x25519" });
        QVERIFY(keys.waitForFinished(30000));
        for (const QString &line : QString::fromUtf8(keys.readAllStandardOutput()).split(QLatin1Char('\n'))) {
            const QString value = line.section(QLatin1Char(':'), 1).trimmed();
            if (line.startsWith(QLatin1String("PrivateKey")))
                m_realityPrivateKey = value;
            else if (line.contains(QLatin1String("PublicKey")))
                m_realityPublicKey = value;
        }
        QVERIFY2(!m_realityPrivateKey.isEmpty() && !m_realityPublicKey.isEmpty(), "xray x25519 output not recognised");

        connect(&m_http, &QTcpServer::newConnection, this, &TestXrayRuntime::answerHttp);
        QVERIFY(m_http.listen(QHostAddress::LocalHost, 0));
        m_ready = true;
    }

    void cleanup()
    {
        ++m_casesRun;
        if (QTest::currentTestFailed())
            m_failures << QStringLiteral("%1: %2").arg(QString::fromLatin1(QTest::currentDataTag()), m_lastReason);
        m_lastReason.clear();
    }

    void cleanupTestCase()
    {
        if (m_ready)
            std::fprintf(stderr, "%s\n",
                         qPrintable(xrayruntime::failureSummary(
                                 QStringLiteral("xray runtime (server %1, client %2)")
                                         .arg(m_serverVersion, xrayruntime::version(m_clientBin)),
                                 m_failures, m_casesRun)));
    }

    void connects_data()
    {
        QTest::addColumn<int>("index");
        for (int i = 0; i < cases().size(); ++i)
            QTest::newRow(qPrintable(cases().at(i).name)) << i;
    }

    void connects()
    {
        QFETCH(int, index);
        const RuntimeCase &c = cases().at(index);
        const bool ruleApplies = c.sinceServer.isEmpty() || xrayruntime::versionAtLeast(m_serverVersion, c.sinceServer);
        const bool expectTraffic = ruleApplies ? c.traffic : !c.traffic;

        QString why;
        const bool flowed = runCase(c, why);
        m_lastReason = expectTraffic ? why : QStringLiteral("expected no traffic, but it flowed");
        QVERIFY2(flowed == expectTraffic, qPrintable(m_lastReason));
    }

    void appMapsRealityFingerprint_data()
    {
        QTest::addColumn<QString>("stored");
        QTest::addColumn<QString>("sent");
        for (const QString &stored : legacyRealityFingerprints())
            QTest::newRow(qPrintable(QStringLiteral("'%1'").arg(stored))) << stored << QStringLiteral("chrome");
        for (const RealityFingerprintCase &fp : realityFingerprintCases()) {
            if (fp.hello == RealityHello::Connects)
                QTest::newRow(qPrintable(fp.name)) << fp.name << fp.name;
        }
    }

    void appMapsRealityFingerprint()
    {
        QFETCH(QString, stored);
        QFETCH(QString, sent);
        XrayServerConfig srv = runtimeConfig("raw", "reality", "xtls-rprx-vision");
        srv.fingerprint = stored;
        const QJsonObject reality = streamOf(vlessOutbound(clientJson(srv))).value("realitySettings").toObject();
        QCOMPARE(reality.value("fingerprint").toString(), sent);
    }

    void realityRandomFingerprint()
    {
        const RuntimeCase random { QStringLiteral("random"), runtimeConfig("raw", "reality", "xtls-rprx-vision"),
                                   [](QJsonObject &, QJsonObject &client) {
                                       setClientRealityFingerprint(client, QStringLiteral("random"));
                                   } };
        const int runs = 10;
        int flowed = 0;
        for (int i = 0; i < runs; ++i) {
            QString why;
            flowed += runCase(random, why) ? 1 : 0;
        }
        qInfo().noquote() << QStringLiteral("REALITY fingerprint random: traffic in %1 of %2 runs on server %3")
                                     .arg(flowed)
                                     .arg(runs)
                                     .arg(m_serverVersion);
    }
};

QTEST_GUILESS_MAIN(TestXrayRuntime)

#include "testXrayRuntime.moc"
