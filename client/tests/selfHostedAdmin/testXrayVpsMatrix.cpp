#include <QApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QProcess>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QUuid>

#include <cstdio>
#include <functional>
#include <libssh/libssh.h>

#include "core/controllers/selfhosted/installController.h"
#include "core/controllers/serversController.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/utils/commonStructs.h"
#include "core/utils/containerEnum.h"
#include "core/utils/errorCodes.h"
#include "core/utils/protocolEnum.h"
#include "secureQSettings.h"
#include "ui/models/protocols/xrayConfigModel.h"
#include "utils/testCoreController.h"
#include "utils/testUtils.h"
#include "utils/xrayMatrix.h"
#include "utils/xrayRuntime.h"
#include "vpnConnection.h"

using namespace amnezia;
using namespace amnezia::test;

namespace
{
    enum class Expect {
        Connects,
        Refused,
        Explore,
    };

    struct VpsCase
    {
        QString name;
        std::function<void(XrayConfigModel &)> edit;
        Expect expect = Expect::Connects;
        bool deviceOnly = false;
    };

    void base(XrayConfigModel &m, const QString &transport, const QString &security, const QString &flow = QString(),
              const QString &mode = QStringLiteral("Stream-one"))
    {
        m.setTransport(transport);
        m.setSecurity(security);
        m.setFlow(flow);
        m.setSni(QStringLiteral("www.googletagmanager.com"));
        if (transport == QLatin1String("xhttp")) {
            m.setXhttpMode(mode);
            m.setXhttpPath(QStringLiteral("/"));
            m.setXhttpUplinkDataPlacement(QStringLiteral("Body"));
            m.setXhttpUplinkMethod(QStringLiteral("POST"));
            m.setXPaddingObfsMode(true);
            m.setXmuxEnabled(true);
            m.setXmuxMaxConcurrencyMin(QStringLiteral("0"));
            m.setXmuxMaxConcurrencyMax(QStringLiteral("0"));
            m.setXmuxMaxConnectionsMin(QStringLiteral("0"));
            m.setXmuxMaxConnectionsMax(QStringLiteral("0"));
        }
    }

    QList<VpsCase> vpsCases()
    {
        QList<VpsCase> cases;

        const QList<QPair<QString, QString>> xhttpCombos = {
            { "Auto", "tls" },       { "Auto", "reality" },
            { "Packet-up", "tls" },
            { "Stream-up", "tls" },  { "Stream-up", "reality" },
            { "Stream-one", "tls" }, { "Stream-one", "reality" },
        };
        for (const auto &combo : xhttpCombos) {
            const QString mode = combo.first;
            const QString security = combo.second;
            cases.append({ QStringLiteral("xhttp/%1/%2").arg(mode, security),
                           [mode, security](XrayConfigModel &m) { base(m, "xhttp", security, QString(), mode); } });
        }

        const QList<QPair<QString, QString>> rawCombos = {
            { "tls", "" },     { "tls", "xtls-rprx-vision" },     { "tls", "xtls-rprx-vision-udp443" },
            { "reality", "" }, { "reality", "xtls-rprx-vision" }, { "reality", "xtls-rprx-vision-udp443" },
        };
        for (const auto &combo : rawCombos) {
            const QString security = combo.first;
            const QString flow = combo.second;
            cases.append({ QStringLiteral("raw/%1/%2").arg(security, flow.isEmpty() ? QStringLiteral("noflow") : flow),
                           [security, flow](XrayConfigModel &m) { base(m, "raw", security, flow); } });
        }

        cases.append({ "mkcp/tls", [](XrayConfigModel &m) { base(m, "mkcp", "tls"); } });

        cases.append({ "xhttp/packet-up/tls/cookie+GET", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpUplinkDataPlacement(QStringLiteral("Cookie"));
                           m.setXhttpUplinkMethod(QStringLiteral("GET"));
                       } });
        cases.append({ "xhttp/reality/xPadding obfs off", [](XrayConfigModel &m) {
                           base(m, "xhttp", "reality");
                           m.setXPaddingObfsMode(false);
                       } });
        cases.append({ "xhttp/reality/xPadding 200-400", [](XrayConfigModel &m) {
                           base(m, "xhttp", "reality");
                           m.setXPaddingBytesMin(QStringLiteral("200"));
                           m.setXPaddingBytesMax(QStringLiteral("400"));
                       } });
        cases.append({ "xhttp/reality/path without slash", [](XrayConfigModel &m) {
                           base(m, "xhttp", "reality");
                           m.setXhttpPath(QStringLiteral("amnezia"));
                       } });

        cases.append({ "device-only/xhttp/xmux concurrency 16-32", [](XrayConfigModel &m) {
                           m.setXmuxEnabled(true);
                           m.setXmuxMaxConcurrencyMin(QStringLiteral("16"));
                           m.setXmuxMaxConcurrencyMax(QStringLiteral("32"));
                       }, Expect::Connects, true });
        cases.append({ "device-only/xhttp/fingerprint firefox + PUT", [](XrayConfigModel &m) {
                           m.setFingerprint(QStringLiteral("firefox"));
                           m.setXhttpUplinkMethod(QStringLiteral("PUT"));
                       }, Expect::Connects, true });

        cases.append({ "refused/xmux concurrency and connections", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls");
                           m.setXmuxMaxConcurrencyMax(QStringLiteral("1"));
                           m.setXmuxMaxConnectionsMax(QStringLiteral("1"));
                       }, Expect::Refused });
        cases.append({ "refused/wildcard sni", [](XrayConfigModel &m) {
                           base(m, "raw", "reality");
                           m.setSni(QStringLiteral("*.example.com"));
                       }, Expect::Refused });

        cases.append({ "xhttp/packet-up/reality",
                       [](XrayConfigModel &m) { base(m, "xhttp", "reality", QString(), "Packet-up"); } });

        for (const QString &fingerprint : { QStringLiteral("chrome"), QStringLiteral("hellochrome_120") }) {
            cases.append(
                    { QStringLiteral("raw/reality/fingerprint %1").arg(fingerprint), [fingerprint](XrayConfigModel &m) {
                         base(m, "raw", "reality");
                         m.setFingerprint(fingerprint);
                     } });
        }

        if (!qEnvironmentVariableIsSet("VPS_MATRIX_FULL"))
            return cases;

        for (const xraytest::MatrixCase &full : xraytest::matrix()) {
            if (!full.allowedByUi)
                continue;
            const XrayServerConfig srv = full.srv;
            cases.append({ QStringLiteral("full/") + full.name, [srv](XrayConfigModel &m) {
                              XrayServerConfig edited = srv;
                              edited.port = m.port();
                              edited.sni = QStringLiteral("www.googletagmanager.com");
                              edited.site = m.site();
                              m.applyServerConfig(edited);
                          } });
        }
        return cases;
    }

    const QList<VpsCase> &cases()
    {
        static const QList<VpsCase> all = vpsCases();
        return all;
    }
}

class TestXrayVpsMatrix : public QObject
{
    Q_OBJECT

private:
    TestCoreController *m_coreController = nullptr;
    SecureQSettings *m_settings = nullptr;
    ServerCredentials m_credentials;
    QString m_serverId;
    QString m_xray;
    QStringList m_expectedIps;
    XrayServerConfig m_originalSettings;
    QTemporaryDir m_dir;
    bool m_ready = false;
    QString m_lastReason;
    QStringList m_failures;
    int m_casesRun = 0;

    ContainerConfig storedContainer() const
    {
        return m_coreController->m_serversController->getContainerConfig(m_serverId, DockerContainer::Xray);
    }

    XrayProtocolConfig storedXray() const
    {
        const ContainerConfig container = storedContainer();
        const auto *xray = container.getXrayProtocolConfig();
        return xray ? *xray : XrayProtocolConfig {};
    }

    ErrorCode save(XrayConfigModel &model)
    {
        const ContainerConfig oldConfig = storedContainer();
        ContainerConfig newConfig = oldConfig;
        newConfig.protocolConfig = model.getProtocolConfig();
        return m_coreController->m_installController->updateServerConfig(m_serverId, DockerContainer::Xray, oldConfig,
                                                                          newConfig, true);
    }

    QString egressIpThroughTunnel(QString &error)
    {
        const XrayProtocolConfig xray = storedXray();
        if (!xray.hasClientConfig()) {
            error = QStringLiteral("no client config stored after save");
            return {};
        }
        QJsonObject native =
                QJsonDocument::fromJson(sanitizeNativeConfig(xray.clientConfig->nativeConfig).toUtf8()).object();
        const int socksPort = xrayruntime::freeLocalPort();
        QJsonArray inbounds = native.value("inbounds").toArray();
        QJsonObject socks = inbounds.first().toObject();
        socks["listen"] = QStringLiteral("127.0.0.1");
        socks["port"] = socksPort;
        inbounds[0] = socks;
        native["inbounds"] = inbounds;
        native["log"] = QJsonObject { { "loglevel", "warning" } };

        const QString path = m_dir.filePath(QStringLiteral("client-%1.json").arg(QUuid::createUuid().toString(QUuid::Id128)));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            error = QStringLiteral("cannot write ") + path;
            return {};
        }
        file.write(QJsonDocument(native).toJson());
        file.close();

        QProcess client;
        client.setProcessChannelMode(QProcess::MergedChannels);
        client.start(m_xray, { QStringLiteral("run"), QStringLiteral("-c"), path });
        if (!client.waitForStarted(10000)) {
            error = QStringLiteral("xray client did not start");
            return {};
        }

        QElapsedTimer listening;
        listening.start();
        bool up = false;
        while (!up && listening.elapsed() < 10000 && client.state() == QProcess::Running) {
            QTcpSocket probe;
            probe.connectToHost(QHostAddress::LocalHost, socksPort);
            up = probe.waitForConnected(300);
            if (!up)
                QTest::qWait(200);
        }
        if (!up) {
            client.kill();
            client.waitForFinished(3000);
            error = QStringLiteral("local socks never came up: ")
                    + xrayruntime::collapsedLog(QString::fromUtf8(client.readAll()));
            return {};
        }

        QString ip;
        for (int attempt = 0; attempt < 3 && ip.isEmpty(); ++attempt) {
            QNetworkAccessManager manager;
            manager.setProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, QStringLiteral("127.0.0.1"), socksPort));
            QNetworkRequest request(QUrl(QStringLiteral("https://api.ipify.org")));
            request.setTransferTimeout(20000);
            QNetworkReply *reply = manager.get(request);
            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            QTimer::singleShot(25000, &loop, &QEventLoop::quit);
            loop.exec();
            if (reply->isFinished() && reply->error() == QNetworkReply::NoError)
                ip = QString::fromUtf8(reply->readAll()).trimmed();
            else
                error = QStringLiteral("request through the tunnel failed: ") + reply->errorString();
            reply->deleteLater();
        }

        client.kill();
        client.waitForFinished(3000);
        if (ip.isEmpty()) {
            const QString clientLog = xrayruntime::collapsedLog(QString::fromUtf8(client.readAll()));
            if (!clientLog.isEmpty())
                error += QStringLiteral(" | ") + clientLog;
        }
        return ip;
    }

private slots:
    void initTestCase()
    {
        m_credentials.hostName = getEnvValue("SELF_HOSTED_SERVER_HOST_NAME");
        m_credentials.userName = getEnvValue("SELF_HOSTED_SERVER_USER_NAME");
        m_credentials.secretData = getEnvValue("SELF_HOSTED_SERVER_PASSWORD");
        m_credentials.port = getEnvValue("SELF_HOSTED_SERVER_SSH_PORT").toInt();
        if (m_credentials.port <= 0)
            m_credentials.port = 22;
        m_xray = getEnvValue("XRAY_BIN");
        for (const char *key : { "SELF_HOSTED_SERVER_HOST_NAME", "SELF_HOSTED_SERVER_USER_NAME",
                                 "SELF_HOSTED_SERVER_PASSWORD", "SELF_HOSTED_SERVER_SSH_PORT", "XRAY_BIN" })
            logEnvValueState(QString::fromLatin1(key));

        if (m_credentials.hostName.isEmpty() || m_credentials.userName.isEmpty() || m_credentials.secretData.isEmpty())
            QSKIP("Set SELF_HOSTED_SERVER_HOST_NAME, SELF_HOSTED_SERVER_USER_NAME, SELF_HOSTED_SERVER_PASSWORD");
        if (m_xray.isEmpty() || !QFileInfo(m_xray).isExecutable())
            QSKIP("Set XRAY_BIN to the xray binary used as the client");
        QVERIFY(m_dir.isValid());

        const QString expected = getEnvValue("XRAY_E2E_EXPECT_IP");
        if (!expected.isEmpty()) {
            m_expectedIps << expected;
        } else {
            for (const QHostAddress &a : QHostInfo::fromName(m_credentials.hostName).addresses())
                m_expectedIps << a.toString();
        }

        const QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);
        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);
        m_coreController = new TestCoreController(vpnConnection, m_settings, nullptr, this);
        ssh_init();

        int port = getEnvValue("XRAY_E2E_PORT").toInt();
        if (port <= 0)
            port = 8443;
        bool wasInstalled = false;
        const ErrorCode installError = m_coreController->m_installController->installServer(
                m_credentials, DockerContainer::Xray, port, TransportProto::Tcp, wasInstalled);
        QVERIFY2(installError == ErrorCode::NoError,
                 qPrintable(QStringLiteral("installServer(Xray) failed, error %1").arg(static_cast<int>(installError))));
        m_serverId = m_coreController->m_serversRepository->serverIdAt(
                m_coreController->m_serversRepository->serversCount() - 1);
        qInfo().noquote() << "Xray on the VPS:" << (wasInstalled ? "installed by this run" : "already there, reused");

        m_originalSettings = storedXray().serverConfig;
        m_ready = true;
    }

    void cleanupTestCase()
    {
        if (m_ready) {
            XrayConfigModel model;
            model.updateModel(DockerContainer::Xray, storedXray());
            model.applyServerConfig(m_originalSettings);
            const ErrorCode restoreError = save(model);
            if (restoreError != ErrorCode::NoError)
                qWarning().noquote() << "Could not restore the original Xray settings, error"
                                     << static_cast<int>(restoreError);
            else
                qInfo().noquote() << "Original Xray settings restored";

            std::fprintf(stderr, "%s\n",
                         qPrintable(xrayruntime::failureSummary(QStringLiteral("VPS matrix"), m_failures, m_casesRun)));
        }
        if (m_coreController) {
            ssh_finalize();
            m_settings->clearSettings();
            delete m_coreController;
            delete m_settings;
        }
    }

    void cleanup()
    {
        ++m_casesRun;
        if (QTest::currentTestFailed()) {
            const QString name = QStringLiteral("%1(%2)").arg(QString::fromLatin1(QTest::currentTestFunction()),
                                                              QString::fromLatin1(QTest::currentDataTag()));
            m_failures << name + QStringLiteral(": ")
                            + (m_lastReason.isEmpty() ? QStringLiteral("details in the FAIL line above") : m_lastReason);
        }
        m_lastReason.clear();
    }

    void matrix_data()
    {
        QTest::addColumn<int>("index");
        for (int i = 0; i < cases().size(); ++i)
            QTest::newRow(qPrintable(cases().at(i).name)) << i;
    }

    void matrix()
    {
        QFETCH(int, index);
        const VpsCase &c = cases().at(index);

        XrayConfigModel model;
        model.updateModel(DockerContainer::Xray, storedXray());
        c.edit(model);

        const QStringList errors = model.validationErrors();
        if (c.expect == Expect::Refused) {
            m_lastReason = QStringLiteral("validation must refuse this before anything reaches the server");
            QVERIFY2(!errors.isEmpty(), qPrintable(m_lastReason));
            return;
        }
        if (c.expect == Expect::Connects) {
            m_lastReason = QStringLiteral("validation refused it: ") + errors.join("; ");
            QVERIFY2(errors.isEmpty(), qPrintable(m_lastReason));
        }
        if (c.deviceOnly) {
            m_lastReason = QStringLiteral("a device-only edit must not count as a server change");
            QVERIFY2(!model.pendingChangeTouchesServer(QString()), qPrintable(m_lastReason));
        }

        QElapsedTimer timer;
        timer.start();
        const ErrorCode saveError = save(model);
        const qint64 saveMs = timer.elapsed();

        if (c.expect == Expect::Explore) {
            QString why;
            const QString ip = saveError == ErrorCode::NoError ? egressIpThroughTunnel(why) : QString();
            const QString result = QStringLiteral("EXPLORE %1: save error %2 %3")
                                           .arg(c.name)
                                           .arg(static_cast<int>(saveError))
                                           .arg(ip.isEmpty() ? QStringLiteral("-> NO TRAFFIC: ") + why
                                                             : QStringLiteral("-> traffic flows, egress ") + ip);
            QSKIP(qPrintable(result));
        }

        m_lastReason = QStringLiteral("save failed, error %1").arg(static_cast<int>(saveError));
        QVERIFY2(saveError == ErrorCode::NoError, qPrintable(m_lastReason));
        qInfo().noquote() << c.name << "saved in" << saveMs << "ms";

        QString why;
        const QString ip = egressIpThroughTunnel(why);
        m_lastReason = why;
        QVERIFY2(!ip.isEmpty(), qPrintable(why));
        m_lastReason = QStringLiteral("traffic left through an unexpected address");
        QVERIFY2(m_expectedIps.isEmpty() || m_expectedIps.contains(ip),
                 qPrintable(QStringLiteral("traffic left through %1, expected %2")
                                    .arg(ip, m_expectedIps.join(QStringLiteral(", ")))));
    }

    void portChangeRecreatesContainer()
    {
        XrayConfigModel model;
        model.updateModel(DockerContainer::Xray, storedXray());
        base(model, "raw", "reality", "xtls-rprx-vision");
        QCOMPARE(save(model), ErrorCode::NoError);

        const QString originalPort = storedXray().serverConfig.port;
        const QString newPort = QString::number(originalPort.toInt() == 8443 ? 8444 : 8443);
        model.updateModel(DockerContainer::Xray, storedXray());
        QVERIFY(model.pendingChangeRequiresReinstall(newPort));
        model.setPort(newPort);
        QCOMPARE(save(model), ErrorCode::NoError);
        QCOMPARE(storedXray().serverConfig.port, newPort);

        QString why;
        const bool tunnelUp = !egressIpThroughTunnel(why).isEmpty();
        m_lastReason = tunnelUp ? QString() : why;
        QVERIFY2(tunnelUp, qPrintable(why));

        model.updateModel(DockerContainer::Xray, storedXray());
        model.setPort(originalPort);
        QCOMPARE(save(model), ErrorCode::NoError);
    }

    void rescanKeepsDeviceSettings()
    {
        XrayConfigModel model;
        model.updateModel(DockerContainer::Xray, storedXray());
        base(model, "raw", "reality", "xtls-rprx-vision-udp443");
        model.setFingerprint(QStringLiteral("firefox"));
        QCOMPARE(save(model), ErrorCode::NoError);

        QCOMPARE(m_coreController->m_installController->scanServerForInstalledContainers(m_serverId), ErrorCode::NoError);

        const XrayServerConfig after = storedXray().serverConfig;
        QCOMPARE(after.flow, QStringLiteral("xtls-rprx-vision-udp443"));
        QCOMPARE(after.fingerprint, QStringLiteral("firefox"));

        QString why;
        const bool tunnelUp = !egressIpThroughTunnel(why).isEmpty();
        m_lastReason = tunnelUp ? QString() : why;
        QVERIFY2(tunnelUp, qPrintable(why));
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestXrayVpsMatrix test;
    QStringList args = app.arguments();
    if (!args.contains(QStringLiteral("-maxwarnings")))
        args << QStringLiteral("-maxwarnings") << QStringLiteral("0");
    return QTest::qExec(&test, args);
}

#include "testXrayVpsMatrix.moc"
