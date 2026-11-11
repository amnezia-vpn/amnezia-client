#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkProxy>
#include <QProcess>
#include <QScopeGuard>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>

#include "core/models/protocols/xrayProtocolConfig.h"
#include "ui/models/protocols/xrayConfigModel.h"
#include "utils/xrayMatrix.h"

using namespace amnezia;
using namespace xraytest;

namespace
{
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
            XrayServerConfig srv = baseConfig("xhttp", "none", "");
            srv.xhttp.mode = QStringLiteral("Packet-up");
            srv.xhttp.uplinkMethod = method;
            out.append({ QStringLiteral("option/xhttp/packet-up/uplinkMethod=%1").arg(method), srv });
        }
        for (const QString &pl : XrayConfigModel::xhttpSessionPlacementOptions()) {
            XrayServerConfig srv = baseConfig("xhttp", "none", "");
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

    quint16 freePort()
    {
        QTcpServer probe;
        probe.listen(QHostAddress::LocalHost);
        return probe.serverPort();
    }

    bool waitForPort(quint16 port)
    {
        for (int attempt = 0; attempt < 50; ++attempt) {
            QTcpSocket probe;
            probe.connectToHost(QHostAddress::LocalHost, port);
            if (probe.waitForConnected(100))
                return true;
            QTest::qWait(100);
        }
        return false;
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
    void realityHandshake_data();
    void realityHandshake();

private:
    struct Run
    {
        int exitCode = -1;
        QString output;
    };

    QString writeConfig(const QJsonObject &config, const QString &fileName);
    Run xrayTest(const QJsonObject &config, const QString &fileName);
    bool startXray(QProcess &process, const QJsonObject &config, const QString &fileName, quint16 port);
    void useLocalCertificate(QJsonObject &serverConfig) const;

    QString m_xray;
    QTemporaryDir m_dir;
    QString m_certFile;
    QString m_keyFile;
};

void TestXrayCoreAccepts::initTestCase()
{
    m_xray = qEnvironmentVariable("XRAY_BIN");
    if (m_xray.isEmpty() || !QFileInfo(m_xray).isExecutable())
        QSKIP("Set XRAY_BIN to an xray v26.7.28 binary to run the core checks");
    QVERIFY(m_dir.isValid());

    QProcess version;
    version.start(m_xray, { QStringLiteral("version") });
    QVERIFY(version.waitForFinished(30000));
    const QString firstLine = QString::fromUtf8(version.readAllStandardOutput()).section(QLatin1Char('\n'), 0, 0);
    qInfo().noquote() << "xray under test:" << firstLine;
    if (!firstLine.contains(QLatin1String("26.7.28")))
        qWarning().noquote() << "XRAY_BIN is not v26.7.28, results may differ from the VPS";

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

QString TestXrayCoreAccepts::writeConfig(const QJsonObject &config, const QString &fileName)
{
    const QString path = m_dir.filePath(fileName + QStringLiteral(".json"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return {};
    file.write(QJsonDocument(config).toJson());
    return path;
}

TestXrayCoreAccepts::Run TestXrayCoreAccepts::xrayTest(const QJsonObject &config, const QString &fileName)
{
    const QString path = writeConfig(config, fileName);
    if (path.isEmpty())
        return { -1, QStringLiteral("cannot write ") + fileName };

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

bool TestXrayCoreAccepts::startXray(QProcess &process, const QJsonObject &config, const QString &fileName, quint16 port)
{
    const QString path = writeConfig(config, fileName);
    if (path.isEmpty())
        return false;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(m_xray, { QStringLiteral("run"), QStringLiteral("-c"), path });
    return process.waitForStarted() && waitForPort(port);
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
    if (!c.allowedByUi)
        QSKIP("Blocked by validationErrors()");

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

void TestXrayCoreAccepts::realityHandshake_data()
{
    QTest::addColumn<QString>("fingerprint");
    QTest::newRow("default") << QString::fromLatin1(protocols::xray::defaultFingerprint);
    QTest::newRow("saved legacy default") << QString::fromLatin1(protocols::xray::legacyDefaultFingerprint);
    for (const QString &fp : XrayConfigModel::realityFingerprintOptions())
        QTest::newRow(qPrintable(QStringLiteral("option ") + fp)) << fp;
}

void TestXrayCoreAccepts::realityHandshake()
{
    QFETCH(QString, fingerprint);

    QProcess keygen;
    keygen.start(m_xray, { QStringLiteral("x25519") });
    QVERIFY(keygen.waitForFinished(30000));
    const QStringList keys = QString::fromUtf8(keygen.readAllStandardOutput()).split(QLatin1Char('\n'));
    QVERIFY(keys.size() >= 2);
    const QString privateKey = keys.at(0).section(QStringLiteral(": "), 1).trimmed();
    const QString publicKey = keys.at(1).section(QStringLiteral(": "), 1).trimmed();

    QTcpServer origin;
    QVERIFY(origin.listen(QHostAddress::LocalHost));
    connect(&origin, &QTcpServer::newConnection, &origin, [&origin] {
        QTcpSocket *peer = origin.nextPendingConnection();
        connect(peer, &QTcpSocket::readyRead, peer, [peer] { peer->write("pong:" + peer->readAll()); });
    });

    const quint16 decoyPort = freePort();
    XrayServerConfig decoySrv = baseConfig("raw", "tls", "");
    decoySrv.port = QString::number(decoyPort);
    QJsonObject decoy = serverJson(decoySrv);
    useLocalCertificate(decoy);

    XrayServerConfig srv = baseConfig("raw", "reality", "xtls-rprx-vision");
    srv.port = QString::number(freePort());
    srv.fingerprint = fingerprint;

    XrayServerInboundInputs serverInputs = inboundInputs(srv);
    serverInputs.realityPrivateKey = privateKey;
    QJsonObject server = srv.toServerInboundJson(serverInputs);
    QJsonArray inbounds = server.value("inbounds").toArray();
    QJsonObject inbound = inbounds.first().toObject();
    QJsonObject stream = streamOf(inbound);
    QJsonObject reality = stream.value("realitySettings").toObject();
    reality["dest"] = QStringLiteral("127.0.0.1:%1").arg(decoyPort);
    stream["realitySettings"] = reality;
    inbound["streamSettings"] = stream;
    inbounds[0] = inbound;
    server["inbounds"] = inbounds;
    const QJsonObject allowLoopback { { "finalRules", QJsonArray { QJsonObject { { "action", "allow" } } } } };
    server["outbounds"] = QJsonArray { QJsonObject { { "protocol", "freedom" }, { "settings", allowLoopback } } };

    XrayClientOutboundInputs clientInputs = outboundInputs();
    clientInputs.serverAddress = QStringLiteral("127.0.0.1");
    clientInputs.realityPublicKey = publicKey;
    XrayProtocolConfig cfg;
    cfg.serverConfig = srv;
    QJsonObject client =
            QJsonDocument::fromJson(sanitizeNativeConfig(compact(cfg.toClientOutboundJson(clientInputs))).toUtf8()).object();
    const quint16 socksPort = freePort();
    QJsonArray clientInbounds = client.value("inbounds").toArray();
    QJsonObject socks = clientInbounds.first().toObject();
    socks["port"] = socksPort;
    clientInbounds[0] = socks;
    client["inbounds"] = clientInbounds;

    QProcess decoyProcess;
    QProcess serverProcess;
    QProcess clientProcess;
    const auto stopProcesses = qScopeGuard([&] {
        for (QProcess *process : { &clientProcess, &serverProcess, &decoyProcess }) {
            process->kill();
            process->waitForFinished();
        }
    });
    QVERIFY2(startXray(decoyProcess, decoy, QStringLiteral("handshake_decoy"), decoyPort),
             qPrintable(decoyProcess.readAll()));
    QVERIFY2(startXray(serverProcess, server, QStringLiteral("handshake_server"), srv.port.toUShort()),
             qPrintable(serverProcess.readAll()));
    QVERIFY2(startXray(clientProcess, client, QStringLiteral("handshake_client"), socksPort),
             qPrintable(clientProcess.readAll()));

    QTcpSocket socket;
    socket.setProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, QStringLiteral("127.0.0.1"), socksPort));
    QByteArray reply;
    connect(&socket, &QTcpSocket::connected, &socket, [&socket] { socket.write("ping"); });
    connect(&socket, &QTcpSocket::readyRead, &socket, [&socket, &reply] { reply += socket.readAll(); });
    socket.connectToHost(QStringLiteral("127.0.0.1"), origin.serverPort());

    QTRY_VERIFY2_WITH_TIMEOUT(
            reply == "pong:ping",
            qPrintable(QStringLiteral("fingerprint %1 was not proxied\n").arg(fingerprint) + compact(client)), 10000);
}

QTEST_GUILESS_MAIN(TestXrayCoreAccepts)

#include "testXrayCoreAccepts.moc"
