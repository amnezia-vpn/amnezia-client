#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QProcess>
#include <QRegularExpression>
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
#include "core/utils/containers/containerUtils.h"
#include "core/utils/errorCodes.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/selfhosted/sshSession.h"
#include "secureQSettings.h"
#include "ui/models/protocols/xrayConfigModel.h"
#include "utils/testCoreController.h"
#include "utils/testUtils.h"
#include "utils/xrayExploreCases.h"
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
        NoTraffic,
    };

    using ClientEdit = std::function<void(QJsonObject &)>;

    struct VpsCase
    {
        QString name;
        std::function<void(XrayConfigModel &)> edit;
        Expect expect = Expect::Connects;
        bool deviceOnly = false;
        ClientEdit clientEdit;
        QString expectInLog;
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

        const QList<QPair<QString, std::function<void(XrayConfigModel &)>>> realityBases = {
            { "raw-vision", [](XrayConfigModel &m) { base(m, "raw", "reality", "xtls-rprx-vision"); } },
            { "xhttp-stream-one", [](XrayConfigModel &m) { base(m, "xhttp", "reality"); } },
        };
        QStringList storedFingerprints = XrayConfigModel::fingerprintOptions()
                + XrayConfigModel::realityFingerprintOptions() + xraytest::legacyRealityFingerprints();
        storedFingerprints.removeDuplicates();
        for (const auto &realityBase : realityBases) {
            for (const QString &fingerprint : storedFingerprints) {
                const auto apply = realityBase.second;
                cases.append({ QStringLiteral("fingerprint/reality/%1/'%2'").arg(realityBase.first, fingerprint),
                               [apply, fingerprint](XrayConfigModel &m) {
                                   apply(m);
                                   m.setFingerprint(fingerprint);
                               } });
            }
        }

        for (const QString &fingerprint : { QStringLiteral("hellochrome_120"), QStringLiteral("ios"),
                                            QStringLiteral("android") }) {
            cases.append({ QStringLiteral("old-client/raw-vision/%1").arg(fingerprint),
                           [](XrayConfigModel &m) { base(m, "raw", "reality", "xtls-rprx-vision"); },
                           Expect::NoTraffic, false,
                           [fingerprint](QJsonObject &client) { xraytest::setClientRealityFingerprint(client, fingerprint); },
                           fingerprint == QLatin1String("android") ? QString()
                                                                   : QStringLiteral("received real certificate") });
        }

        const QList<QPair<QString, std::function<void(XrayConfigModel &)>>> tlsBases = {
            { "raw-noflow", [](XrayConfigModel &m) { base(m, "raw", "tls"); } },
            { "xhttp-stream-one", [](XrayConfigModel &m) { base(m, "xhttp", "tls"); } },
        };
        for (const auto &tlsBase : tlsBases) {
            for (const QString &fingerprint : XrayConfigModel::fingerprintOptions()) {
                const auto apply = tlsBase.second;
                const bool h2WithoutAlpn = tlsBase.first.startsWith(QLatin1String("xhttp"))
                        && (fingerprint == QLatin1String("android") || fingerprint == QLatin1String("360"));
                cases.append({ QStringLiteral("fingerprint/tls/%1/%2").arg(tlsBase.first, fingerprint),
                               [apply, fingerprint](XrayConfigModel &m) {
                                   apply(m);
                                   m.setFingerprint(fingerprint);
                               },
                               h2WithoutAlpn ? Expect::Explore : Expect::Connects });
            }
        }

        cases.append({ "refused/security none", [](XrayConfigModel &m) { base(m, "raw", "none"); }, Expect::Refused });

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
    QFile m_report;
    QHash<QString, QString> m_exploreSeen;
    bool m_explorePrepared = false;
    QString m_exploreSkip;

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

    ErrorCode save(const XrayProtocolConfig &protocolConfig)
    {
        const ContainerConfig oldConfig = storedContainer();
        ContainerConfig newConfig = oldConfig;
        newConfig.protocolConfig = protocolConfig;
        return m_coreController->m_installController->updateServerConfig(m_serverId, DockerContainer::Xray, oldConfig,
                                                                          newConfig, true);
    }

    ErrorCode save(XrayConfigModel &model)
    {
        return save(model.getProtocolConfig());
    }

    QString runOnServer(const QString &command)
    {
        SshSession ssh;
        QString out;
        const ErrorCode error = ssh.runScript(m_credentials, command, [&out](const QString &data, libssh::Client &) {
            out += data;
            return ErrorCode::NoError;
        });
        return error == ErrorCode::NoError ? out.trimmed() : QString();
    }

    QString xrayContainerId()
    {
        return runOnServer(QStringLiteral("sudo docker inspect -f '{{.Id}}' %1")
                                   .arg(ContainerUtils::containerToString(DockerContainer::Xray)));
    }

    bool startClient(QProcess &client, int &socksPort, const ClientEdit &clientEdit, QString &error)
    {
        const XrayProtocolConfig xray = storedXray();
        if (!xray.hasClientConfig()) {
            error = QStringLiteral("no client config stored after save");
            return false;
        }
        QJsonObject native =
                QJsonDocument::fromJson(sanitizeNativeConfig(xray.clientConfig->nativeConfig).toUtf8()).object();
        if (clientEdit)
            clientEdit(native);
        socksPort = xrayruntime::freeLocalPort();
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
            return false;
        }
        file.write(QJsonDocument(native).toJson());
        file.close();

        client.setProcessChannelMode(QProcess::MergedChannels);
        client.start(m_xray, { QStringLiteral("run"), QStringLiteral("-c"), path });
        if (!client.waitForStarted(10000)) {
            error = QStringLiteral("xray client did not start");
            return false;
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
            return false;
        }
        return true;
    }

    static bool fetchThroughTunnel(int socksPort, const QUrl &url, int timeoutMs, int &status, QByteArray &body,
                                   QString &error)
    {
        QNetworkAccessManager manager;
        manager.setProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, QStringLiteral("127.0.0.1"), socksPort));
        QNetworkRequest request(url);
        request.setTransferTimeout(timeoutMs);
        QNetworkReply *reply = manager.get(request);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(timeoutMs + 5000, &loop, &QEventLoop::quit);
        loop.exec();
        status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool ok = reply->isFinished() && reply->error() == QNetworkReply::NoError;
        if (ok)
            body = reply->readAll();
        else
            error = QStringLiteral("request through the tunnel failed: ") + reply->errorString();
        reply->deleteLater();
        return ok;
    }

    QString egressIpThroughTunnel(QString &error, const ClientEdit &clientEdit = {})
    {
        QProcess client;
        int socksPort = 0;
        if (!startClient(client, socksPort, clientEdit, error))
            return {};

        QString ip;
        for (int attempt = 0; attempt < 3 && ip.isEmpty(); ++attempt) {
            int status = 0;
            QByteArray body;
            if (fetchThroughTunnel(socksPort, QUrl(QStringLiteral("https://api.ipify.org")), 20000, status, body, error))
                ip = QString::fromUtf8(body).trimmed();
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

    struct TunnelCheck
    {
        QString egress = QStringLiteral("-");
        QString gstatic = QStringLiteral("-");
        QString http = QStringLiteral("-");
        QString details;
        qint64 ms = 0;
        bool clientFailed = false;
    };

    static QString masked(QString text)
    {
        static const QRegularExpression address(QStringLiteral(R"(\d{1,3}(?:\.\d{1,3}){3}(?::\d+)?)"));
        return text.replace(address, QStringLiteral("<ip>"));
    }

    static bool exploreEnabled()
    {
        return qEnvironmentVariableIsSet("VPS_EXPLORE");
    }

    static const QList<xrayexplore::ExploreCase> &exploreCases()
    {
        static const QList<xrayexplore::ExploreCase> all = [] {
            const QString groups = qEnvironmentVariable("VPS_EXPLORE_GROUPS");
            return xrayexplore::cases(groups.isEmpty() ? xrayexplore::groupNames()
                                                       : groups.split(QLatin1Char(','), Qt::SkipEmptyParts));
        }();
        return all;
    }

    XrayServerConfig cleanConfig() const
    {
        XrayServerConfig clean = xraytest::baseConfig(QStringLiteral("raw"), QStringLiteral("reality"),
                                                      QStringLiteral("xtls-rprx-vision"));
        clean.port = m_originalSettings.port;
        clean.sni = QStringLiteral("www.googletagmanager.com");
        clean.site = clean.sni;
        return clean;
    }

    void writeReportRow(const QStringList &cells)
    {
        QStringList flat;
        for (QString cell : cells)
            flat << cell.replace(QLatin1Char('\t'), QLatin1Char(' ')).replace(QLatin1Char('\n'), QLatin1Char(' '));
        m_report.write((flat.join(QLatin1Char('\t')) + QLatin1Char('\n')).toUtf8());
        m_report.flush();
    }

    void prepareExplore()
    {
        if (m_explorePrepared)
            return;
        m_explorePrepared = true;

        const QString expected = QString::fromLatin1(protocols::xray::expectedServerXrayRelease).mid(1);
        const QString serverVersion =
                runOnServer(QStringLiteral("sudo docker exec %1 xray version")
                                    .arg(ContainerUtils::containerToString(DockerContainer::Xray)))
                        .section(QLatin1Char('\n'), 0, 0);
        if (!serverVersion.contains(QStringLiteral("Xray ") + expected + QLatin1Char(' '))) {
            m_exploreSkip = QStringLiteral("server xray is '%1', explore runs only against %2").arg(serverVersion, expected);
            return;
        }

        const QString path = qEnvironmentVariable("VPS_EXPLORE_REPORT", QStringLiteral("xray-explore-report.tsv"));
        const bool fresh = !QFileInfo::exists(path);
        m_report.setFileName(path);
        if (!m_report.open(QIODevice::Append)) {
            m_exploreSkip = QStringLiteral("cannot open the report ") + path;
            return;
        }
        if (fresh) {
            writeReportRow({ "group", "case", "verdict", "deploy", "save error", "save ms", "egress", "gstatic 204",
                             "http 204", "check ms", "stored", "details" });
        }
        writeReportRow({ "#run", QDateTime::currentDateTime().toString(Qt::ISODate), serverVersion,
                         QStringLiteral("client ") + xrayruntime::version(m_xray) });
        qInfo().noquote() << "Explore report:" << QFileInfo(path).absoluteFilePath();
    }

    TunnelCheck checkInternet(const xrayexplore::ClientEdit &clientEdit)
    {
        TunnelCheck result;
        QElapsedTimer timer;
        timer.start();
        QProcess client;
        int socksPort = 0;
        QString error;
        if (!startClient(client, socksPort, clientEdit, error)) {
            result.clientFailed = true;
            result.details = masked(error);
            result.ms = timer.elapsed();
            return result;
        }

        QStringList errors;
        auto fetch = [&](const QString &url, int &status, QByteArray &body) {
            QString why;
            const bool ok = fetchThroughTunnel(socksPort, QUrl(url), 15000, status, body, why);
            if (!ok && !errors.contains(why))
                errors << why;
            return ok;
        };

        int status = 0;
        QByteArray body;
        if (fetch(QStringLiteral("https://api.ipify.org"), status, body)) {
            const QString ip = QString::fromUtf8(body).trimmed();
            result.egress = m_expectedIps.isEmpty() || m_expectedIps.contains(ip) ? QStringLiteral("vps")
                                                                                  : QStringLiteral("other");
        } else {
            result.egress = QStringLiteral("fail");
        }
        status = 0;
        result.gstatic = fetch(QStringLiteral("https://www.gstatic.com/generate_204"), status, body)
                ? QString::number(status)
                : QStringLiteral("fail");
        status = 0;
        result.http = fetch(QStringLiteral("http://www.gstatic.com/generate_204"), status, body)
                ? QString::number(status)
                : QStringLiteral("fail");

        const bool alive = client.state() == QProcess::Running;
        client.kill();
        client.waitForFinished(3000);
        const QString output = QString::fromUtf8(client.readAll());
        if (!alive) {
            for (const QString &line : output.split(QLatin1Char('\n'))) {
                if (line.startsWith(QLatin1String("panic:"))) {
                    errors.prepend(QStringLiteral("client died: ") + line.trimmed());
                    break;
                }
            }
        }
        const QString clientLog = xrayruntime::collapsedLog(output);
        if (!clientLog.isEmpty())
            errors << clientLog;
        result.details = masked(errors.join(QStringLiteral(" | ")));
        result.ms = timer.elapsed();
        return result;
    }

    QString storedSummary() const
    {
        const XrayServerConfig s = storedXray().serverConfig;
        return QStringLiteral("port=%1 transport=%2 security=%3 flow=%4 fingerprint=%5 sni=%6 alpn=%7 mode=%8 "
                              "tti=%9 mtu=%10 msw=%11")
                .arg(s.port, s.transport, s.security, s.flow, s.fingerprint, s.sni, s.alpn, s.xhttp.mode, s.mkcp.tti)
                .arg(s.mkcp.mtu, s.mkcp.maxSendingWindow);
    }

    void runExploreCase(const QString &group, const QString &name, const xrayexplore::Edit &edit,
                        const xrayexplore::ClientEdit &clientEdit, bool fromClean, bool dedupe)
    {
        XrayConfigModel model;
        model.updateModel(DockerContainer::Xray, storedXray());
        if (fromClean)
            model.applyServerConfig(cleanConfig());
        if (edit)
            edit(model);

        const QStringList refused = model.validationErrors();
        if (!refused.isEmpty()) {
            writeReportRow({ group, name, "refused by app", "-", "-", "-", "-", "-", "-", "-", "-",
                             refused.join(QStringLiteral("; ")) });
            return;
        }

        const bool unsaved = model.hasUnsavedChanges();
        const QString deploy = !unsaved                                       ? QStringLiteral("nothing to save")
                : model.pendingChangeRequiresReinstall(QString())             ? QStringLiteral("reinstall")
                : model.pendingChangeTouchesServer(QString())                 ? QStringLiteral("server")
                                                                              : QStringLiteral("client only");
        const XrayProtocolConfig pending = model.getProtocolConfig();

        if (dedupe) {
            QJsonObject client = xraytest::clientJson(pending.serverConfig);
            if (clientEdit)
                clientEdit(client);
            const QString key = xraytest::compact(xraytest::serverJson(pending.serverConfig)) + QLatin1Char('|')
                    + xraytest::compact(client);
            const auto seen = m_exploreSeen.constFind(key);
            if (seen != m_exploreSeen.constEnd()) {
                writeReportRow({ group, name, QStringLiteral("duplicate of ") + seen.value(), deploy, "-", "-", "-", "-",
                                 "-", "-", "-", "-" });
                return;
            }
            m_exploreSeen.insert(key, name);
        }

        QString saveError = QStringLiteral("-");
        QString saveMs = QStringLiteral("-");
        if (unsaved) {
            QElapsedTimer timer;
            timer.start();
            const ErrorCode error = save(pending);
            saveMs = QString::number(timer.elapsed());
            saveError = QString::number(static_cast<int>(error));
            if (error != ErrorCode::NoError) {
                writeReportRow({ group, name, QStringLiteral("server rejected"), deploy, saveError, saveMs, "-", "-", "-",
                                 "-", masked(storedSummary()), "-" });
                return;
            }
        }

        const TunnelCheck check = checkInternet(clientEdit);
        const bool works = check.egress == QLatin1String("vps") && check.gstatic == QLatin1String("204")
                && check.http == QLatin1String("204");
        const bool partial = check.egress == QLatin1String("vps") || check.gstatic == QLatin1String("204")
                || check.http == QLatin1String("204");
        const QString verdict = check.clientFailed ? QStringLiteral("client failed")
                : works                            ? QStringLiteral("works")
                : partial                          ? QStringLiteral("partial")
                                                   : QStringLiteral("no internet");
        writeReportRow({ group, name, verdict, deploy, saveError, saveMs, check.egress, check.gstatic, check.http,
                         QString::number(check.ms), masked(storedSummary()), check.details });
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
        const QString ip = egressIpThroughTunnel(why, c.clientEdit);
        if (c.expect == Expect::NoTraffic) {
            m_lastReason = QStringLiteral("expected no traffic, but it flowed");
            QVERIFY2(ip.isEmpty(), qPrintable(m_lastReason));
            m_lastReason = QStringLiteral("expected '%1' in the client log, got: %2").arg(c.expectInLog, why);
            QVERIFY2(c.expectInLog.isEmpty() || why.contains(c.expectInLog), qPrintable(m_lastReason));
            return;
        }
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

    void mkcpReplacesReality()
    {
        XrayConfigModel model;
        model.updateModel(DockerContainer::Xray, storedXray());
        base(model, "raw", "reality", "xtls-rprx-vision");
        QCOMPARE(save(model), ErrorCode::NoError);

        model.updateModel(DockerContainer::Xray, storedXray());
        model.setFlow(QString());
        model.setTransport(QStringLiteral("mkcp"));
        QCOMPARE(model.security(), QStringLiteral("tls"));
        QVERIFY2(model.validationErrors().isEmpty(), qPrintable(model.validationErrors().join("; ")));
        QCOMPARE(save(model), ErrorCode::NoError);
        QCOMPARE(storedXray().serverConfig.security, QStringLiteral("tls"));

        QString why;
        const bool tunnelUp = !egressIpThroughTunnel(why).isEmpty();
        m_lastReason = tunnelUp ? QString() : why;
        QVERIFY2(tunnelUp, qPrintable(why));
    }

    void serverSettingChangeKeepsContainer()
    {
        XrayConfigModel model;
        model.updateModel(DockerContainer::Xray, storedXray());
        base(model, "raw", "reality", "xtls-rprx-vision");
        QCOMPARE(save(model), ErrorCode::NoError);
        const QString before = xrayContainerId();
        QVERIFY2(!before.isEmpty(), "cannot read the xray container id over ssh");

        model.updateModel(DockerContainer::Xray, storedXray());
        base(model, "xhttp", "reality");
        QVERIFY(model.pendingChangeTouchesServer(QString()));
        QVERIFY(!model.pendingChangeRequiresReinstall(QString()));
        QCOMPARE(save(model), ErrorCode::NoError);
        m_lastReason = QStringLiteral("a server setting change without a port change recreated the container");
        QVERIFY2(xrayContainerId() == before, qPrintable(m_lastReason));

        QString why;
        const bool tunnelUp = !egressIpThroughTunnel(why).isEmpty();
        m_lastReason = tunnelUp ? QString() : why;
        QVERIFY2(tunnelUp, qPrintable(why));
    }

    void explore_data()
    {
        QTest::addColumn<int>("index");
        if (!exploreEnabled()) {
            QTest::newRow("disabled") << -1;
            return;
        }
        const auto &all = exploreCases();
        for (int i = 0; i < all.size(); ++i)
            QTest::newRow(qPrintable(all.at(i).name)) << i;
    }

    void explore()
    {
        QFETCH(int, index);
        if (index < 0)
            QSKIP("Set VPS_EXPLORE=1 to run the explore cases");
        prepareExplore();
        if (!m_exploreSkip.isEmpty())
            QSKIP(qPrintable(m_exploreSkip));
        const xrayexplore::ExploreCase &c = exploreCases().at(index);
        runExploreCase(c.group, c.name, c.edit, c.clientEdit, true, true);
    }

    void exploreTransitions()
    {
        if (!exploreEnabled())
            QSKIP("Set VPS_EXPLORE=1 to run the explore transitions");
        prepareExplore();
        if (!m_exploreSkip.isEmpty())
            QSKIP(qPrintable(m_exploreSkip));

        using xrayexplore::Edit;
        using xrayexplore::apply;
        const QString originalPort = m_originalSettings.port;
        const QString otherPort = originalPort == QLatin1String("2053") ? QStringLiteral("2054") : QStringLiteral("2053");
        const xrayexplore::Carrier realityVision = xrayexplore::raw(QStringLiteral("reality"), QStringLiteral("xtls-rprx-vision"));
        const xrayexplore::Carrier tlsVision = xrayexplore::raw(QStringLiteral("tls"), QStringLiteral("xtls-rprx-vision"));
        const xrayexplore::Carrier tlsRaw = xrayexplore::raw(QStringLiteral("tls"), QString());
        const xrayexplore::Carrier kcp = xrayexplore::mkcp();
        auto text = [](void (XrayConfigModel::*setter)(const QString &), const QString &value) -> Edit {
            return [setter, value](XrayConfigModel &m) { (m.*setter)(value); };
        };
        auto start = [](const xrayexplore::Carrier &carrier, const Edit &more = {}) -> Edit {
            return [carrier, more](XrayConfigModel &m) {
                apply(m, carrier);
                if (more)
                    more(m);
            };
        };
        auto both = [](const Edit &first, const Edit &second) -> Edit {
            return [first, second](XrayConfigModel &m) {
                first(m);
                second(m);
            };
        };

        struct Step
        {
            QString name;
            Edit edit;
        };
        const QList<QPair<QString, QList<Step>>> chains = {
            { "reality-mkcp-raw",
              { { "raw/reality/vision", start(realityVision) },
                { "transport=mkcp", text(&XrayConfigModel::setTransport, "mkcp") },
                { "transport=raw", text(&XrayConfigModel::setTransport, "raw") } } },
            { "mkcp-values-survive",
              { { "mkcp tti=20 mtu=1200 msw=1200",
                  start(kcp, both(both(text(&XrayConfigModel::setMkcpTti, "20"), text(&XrayConfigModel::setMkcpMtu, "1200")),
                                  text(&XrayConfigModel::setMkcpMaxSendingWindow, "1200"))) },
                { "transport=raw", text(&XrayConfigModel::setTransport, "raw") },
                { "transport=mkcp", text(&XrayConfigModel::setTransport, "mkcp") } } },
            { "mkcp-edit-on-raw",
              { { "raw/tls", start(tlsRaw) }, { "tti=20 while raw", text(&XrayConfigModel::setMkcpTti, "20") } } },
            { "xhttp-mkcp-xhttp",
              { { "xhttp/tls/Packet-up path=/keep",
                  start(xrayexplore::xhttp(QStringLiteral("tls"), QStringLiteral("Packet-up")),
                        text(&XrayConfigModel::setXhttpPath, "/keep")) },
                { "transport=mkcp", text(&XrayConfigModel::setTransport, "mkcp") },
                { "transport=xhttp", text(&XrayConfigModel::setTransport, "xhttp") } } },
            { "mkcp-reality-then-raw",
              { { "mkcp/tls", start(kcp) },
                { "security=reality", text(&XrayConfigModel::setSecurity, "reality") },
                { "security=reality transport=raw",
                  both(text(&XrayConfigModel::setSecurity, "reality"), text(&XrayConfigModel::setTransport, "raw")) } } },
            { "tls-reality-tls",
              { { "raw/tls/vision", start(tlsVision) },
                { "security=reality", text(&XrayConfigModel::setSecurity, "reality") },
                { "security=tls", text(&XrayConfigModel::setSecurity, "tls") } } },
            { "port-and-mkcp",
              { { "mkcp/tls", start(kcp) },
                { "port=" + otherPort + " tti=30",
                  both(text(&XrayConfigModel::setPort, otherPort), text(&XrayConfigModel::setMkcpTti, "30")) },
                { "port=" + originalPort, text(&XrayConfigModel::setPort, originalPort) } } },
            { "mtu-raise-with-msw",
              { { "mkcp mtu=1350 msw=1400",
                  start(kcp, both(text(&XrayConfigModel::setMkcpMtu, "1350"),
                                  text(&XrayConfigModel::setMkcpMaxSendingWindow, "1400"))) },
                { "mtu=1460", text(&XrayConfigModel::setMkcpMtu, "1460") } } },
            { "equivalent-edits",
              { { "mkcp tti=50", start(kcp, text(&XrayConfigModel::setMkcpTti, "50")) },
                { "tti=''", text(&XrayConfigModel::setMkcpTti, "") },
                { "tti=5", text(&XrayConfigModel::setMkcpTti, "5") },
                { "tti=05", text(&XrayConfigModel::setMkcpTti, "05") } } },
            { "client-only-edits",
              { { "raw/reality/vision fingerprint=chrome", start(realityVision, text(&XrayConfigModel::setFingerprint, "chrome")) },
                { "fingerprint=firefox", text(&XrayConfigModel::setFingerprint, "firefox") },
                { "flow=xtls-rprx-vision-udp443", text(&XrayConfigModel::setFlow, "xtls-rprx-vision-udp443") } } },
        };

        for (const auto &chain : chains) {
            for (int i = 0; i < chain.second.size(); ++i) {
                const Step &step = chain.second.at(i);
                runExploreCase(QStringLiteral("transition"),
                               QStringLiteral("transition/%1 #%2 %3").arg(chain.first).arg(i + 1).arg(step.name), step.edit,
                               {}, i == 0, false);
            }
        }
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
