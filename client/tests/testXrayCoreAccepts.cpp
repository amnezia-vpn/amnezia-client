#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
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
            for (const QString &fp : XrayConfigModel::fingerprintOptions()) {
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

private:
    struct Run
    {
        int exitCode = -1;
        QString output;
    };

    Run xrayTest(const QJsonObject &config, const QString &fileName);
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

QTEST_GUILESS_MAIN(TestXrayCoreAccepts)

#include "testXrayCoreAccepts.moc"
