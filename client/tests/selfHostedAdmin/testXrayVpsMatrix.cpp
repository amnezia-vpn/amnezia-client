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

#include <QUrlQuery>

#include "core/controllers/selfhosted/exportController.h"
#include "core/controllers/selfhosted/installController.h"
#include "core/controllers/selfhosted/usersController.h"
#include "core/controllers/serversController.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/utils/commonStructs.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/errorCodes.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/serialization/serialization.h"
#include "logger.h"
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
        RefusedAndBroken,
        RefusedButWorks,
        NoTraffic,
    };

    using ConfigCheck = std::function<QString(const QJsonObject &client, const QJsonObject &server)>;
    using ClientEdit = std::function<void(QJsonObject &)>;

    struct VpsCase
    {
        QString name;
        std::function<void(XrayConfigModel &)> edit;
        Expect expect = Expect::Connects;
        bool deviceOnly = false;
        ConfigCheck check;
        QString rule;
        QString knownIssue;
        ClientEdit clientEdit;
        QString expectInLog;
    };

    QJsonValue masked(const QJsonValue &value)
    {
        static const QStringList secretKeys = { QStringLiteral("privateKey"), QStringLiteral("shortIds"),
                                                QStringLiteral("shortId"), QStringLiteral("certificates"),
                                                QStringLiteral("id") };
        if (value.isObject()) {
            const QJsonObject in = value.toObject();
            QJsonObject out;
            for (auto it = in.begin(); it != in.end(); ++it)
                out[it.key()] = secretKeys.contains(it.key()) ? QJsonValue(QStringLiteral("***")) : masked(it.value());
            return out;
        }
        if (value.isArray()) {
            QJsonArray out;
            for (const QJsonValue &v : value.toArray())
                out.append(masked(v));
            return out;
        }
        return value;
    }

    QString compactJson(const QJsonValue &value)
    {
        const QJsonValue m = masked(value);
        const QJsonDocument doc = m.isArray() ? QJsonDocument(m.toArray()) : QJsonDocument(m.toObject());
        return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    }

    QString settingsSummary(const XrayConfigModel &m)
    {
        return QStringLiteral("transport=%1 security=%2 flow=%3 mode=%4 fp=%5 alpn=%6 sni=%7")
                .arg(m.transport(), m.security(), m.flow().isEmpty() ? QStringLiteral("-") : m.flow(), m.xhttpMode(),
                     m.fingerprint(), m.alpn(), m.sni());
    }

    void logSaved(const QJsonObject &client, const QJsonObject &server, ErrorCode saveError, qint64 saveMs,
                  const QString &state)
    {
        qInfo().noquote() << "client outbound:" << compactJson(xraytest::vlessEntry(client, QStringLiteral("outbounds")));
        qInfo().noquote() << "server streamSettings:"
                          << compactJson(xraytest::vlessEntry(server, QStringLiteral("inbounds"))
                                                 .value(QStringLiteral("streamSettings")));
        qInfo().noquote() << QStringLiteral("save: error=%1 %2 ms").arg(static_cast<int>(saveError)).arg(saveMs);
        qInfo().noquote() << "server:" << state;
    }

    QJsonObject xhttpOf(const QJsonObject &root, const QString &list)
    {
        for (const QJsonValue &v : root.value(list).toArray()) {
            const QJsonObject stream = v.toObject().value(QStringLiteral("streamSettings")).toObject();
            if (stream.contains(QStringLiteral("xhttpSettings")))
                return stream.value(QStringLiteral("xhttpSettings")).toObject();
        }
        return {};
    }

    void base(XrayConfigModel &m, const QString &transport, const QString &security, const QString &flow = QString(),
              const QString &mode = QStringLiteral("Stream-one"))
    {
        m.setTransport(transport);
        m.setSecurity(security);
        m.setFlow(flow);
        m.setSni(QStringLiteral("www.googletagmanager.com"));
        m.setFingerprint(QStringLiteral("chrome"));
        m.setAlpn(QStringLiteral("h2"));
        if (transport == QLatin1String("xhttp")) {
            m.setXhttpMode(mode);
            m.setXhttpHost(QString());
            m.setXhttpPath(QStringLiteral("/"));
            m.setXhttpScMaxBufferedPosts(QString());
            m.setXPaddingBytesMin(QString());
            m.setXPaddingBytesMax(QString());
            m.setXmuxHKeepAlivePeriod(QString());
            m.setXhttpUplinkDataPlacement(QStringLiteral("Body"));
            m.setXhttpUplinkMethod(QStringLiteral("POST"));
            m.setXPaddingObfsMode(true);
            m.setXmuxEnabled(true);
            m.setXmuxMaxConcurrencyMin(QStringLiteral("0"));
            m.setXmuxMaxConcurrencyMax(QStringLiteral("0"));
            m.setXmuxMaxConnectionsMin(QStringLiteral("0"));
            m.setXmuxMaxConnectionsMax(QStringLiteral("0"));
            namespace px = protocols::xray;
            m.setXhttpSessionPlacement(QString::fromLatin1(px::defaultXhttpSessionPlacement));
            m.setXhttpSeqPlacement(QString::fromLatin1(px::defaultXhttpSeqPlacement));
            m.setXhttpSessionKey(QString());
            m.setXhttpSeqKey(QString());
            m.setXhttpUplinkDataKey(QString());
            m.setXhttpUplinkChunkSize(QString());
            m.setXhttpDisableGrpc(true);
            m.setXhttpScMaxEachPostBytesMin(QString::fromLatin1(px::defaultXhttpScMaxEachPostBytesMin));
            m.setXhttpScMaxEachPostBytesMax(QString::fromLatin1(px::defaultXhttpScMaxEachPostBytesMax));
            m.setXhttpScMinPostsIntervalMsMin(QString::fromLatin1(px::defaultXhttpScMinPostsIntervalMsMin));
            m.setXhttpScMinPostsIntervalMsMax(QString::fromLatin1(px::defaultXhttpScMinPostsIntervalMsMax));
            m.setXhttpScStreamUpServerSecsMin(QString::fromLatin1(px::defaultXhttpScStreamUpServerSecsMin));
            m.setXhttpScStreamUpServerSecsMax(QString::fromLatin1(px::defaultXhttpScStreamUpServerSecsMax));
            m.setXPaddingPlacement(QString::fromLatin1(px::defaultXPaddingPlacement));
            m.setXPaddingMethod(QString::fromLatin1(px::defaultXPaddingMethod));
            m.setXPaddingKey(QString());
            m.setXPaddingHeader(QString());
        }
        if (transport == QLatin1String("mkcp")) {
            m.setMkcpTti(QString());
            m.setMkcpMtu(QString());
            m.setMkcpUplinkCapacity(QString());
            m.setMkcpDownlinkCapacity(QString());
            m.setMkcpCwndMultiplier(QString());
            m.setMkcpMaxSendingWindow(QString());
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

        const QList<QPair<QString, QString>> http11StreamCombos = {
            { "Stream-one", "tls" },  { "Stream-up", "tls" },
        };
        for (const auto &combo : http11StreamCombos) {
            const QString mode = combo.first;
            const QString security = combo.second;
            cases.append({ QStringLiteral("xhttp/%1/%2/http1.1").arg(mode, security),
                           [mode, security](XrayConfigModel &m) {
                               base(m, "xhttp", security, QString(), mode);
                               if (security == QLatin1String("tls"))
                                   m.setAlpn(QStringLiteral("http/1.1"));
                           } });
        }
        for (const QString fp : { QStringLiteral("android"), QStringLiteral("360") }) {
            cases.append({ QStringLiteral("broken/xhttp/packet-up/tls/%1+h2").arg(fp), [fp](XrayConfigModel &m) {
                               base(m, "xhttp", "tls", QString(), "Packet-up");
                               m.setFingerprint(fp);
                           }, Expect::RefusedAndBroken });
        }
        cases.append({ "stricter/raw/reality/360", [](XrayConfigModel &m) {
                           base(m, "raw", "reality");
                           m.setFingerprint(QStringLiteral("360"));
                       }, Expect::RefusedButWorks });
        cases.append({ "broken/raw/tls/vision/android", [](XrayConfigModel &m) {
                           base(m, "raw", "tls", "xtls-rprx-vision");
                           m.setFingerprint(QStringLiteral("android"));
                       }, Expect::RefusedAndBroken });
        cases.append({ "broken/raw/reality/ip sni", [](XrayConfigModel &m) {
                           base(m, "raw", "reality");
                           m.setSni(QStringLiteral("1.2.3.4"));
                           m.setSite(QStringLiteral("www.googletagmanager.com"));
                       }, Expect::RefusedAndBroken });

        cases.append({ "boundary/xhttp/packet-up/tls/android+http1.1", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setFingerprint(QStringLiteral("android"));
                           m.setAlpn(QStringLiteral("http/1.1"));
                       } });
        cases.append({ "boundary/raw/tls/noflow/android", [](XrayConfigModel &m) {
                           base(m, "raw", "tls");
                           m.setFingerprint(QStringLiteral("android"));
                       } });

        cases.append({ "gen/xhttp/reality/obfs off + xPadding 200-400", [](XrayConfigModel &m) {
                           base(m, "xhttp", "reality");
                           m.setXPaddingObfsMode(false);
                           m.setXPaddingBytesMin(QStringLiteral("200"));
                           m.setXPaddingBytesMax(QStringLiteral("400"));
                       }, Expect::Connects, false, [](const QJsonObject &client, const QJsonObject &server) {
                           for (const auto &side : { qMakePair(QStringLiteral("client"), xhttpOf(client, "outbounds")),
                                                     qMakePair(QStringLiteral("server"), xhttpOf(server, "inbounds")) }) {
                               const QString bytes = side.second.value(QStringLiteral("xPaddingBytes")).toString();
                               if (bytes != QLatin1String("200-400"))
                                   return QStringLiteral("%1 xPaddingBytes is \"%2\", expected 200-400").arg(side.first, bytes);
                           }
                           return QString();
                       } });
        cases.append({ "gen/xhttp/packet-up/tls/xPadding 0-0", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXPaddingBytesMin(QStringLiteral("0"));
                           m.setXPaddingBytesMax(QStringLiteral("0"));
                       } });
        cases.append({ "gen/xhttp/packet-up/tls/scMaxBufferedPosts 0", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpScMaxBufferedPosts(QStringLiteral("0"));
                       }, Expect::Connects, false, [](const QJsonObject &client, const QJsonObject &server) {
                           if (xhttpOf(client, "outbounds").contains(QStringLiteral("scMaxBufferedPosts")))
                               return QStringLiteral("client still has scMaxBufferedPosts");
                           if (xhttpOf(server, "inbounds").contains(QStringLiteral("scMaxBufferedPosts")))
                               return QStringLiteral("server still has scMaxBufferedPosts");
                           return QString();
                       } });
        cases.append({ "gen/xhttp/stream-up/tls/scStreamUpServerSecs empty", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Stream-up");
                           m.setXhttpScStreamUpServerSecsMin(QString());
                           m.setXhttpScStreamUpServerSecsMax(QString());
                       }, Expect::Connects, false, [](const QJsonObject &, const QJsonObject &server) {
                           if (xhttpOf(server, "inbounds").contains(QStringLiteral("scStreamUpServerSecs")))
                               return QStringLiteral("server still has scStreamUpServerSecs, expected core default");
                           return QString();
                       } });
        for (const QString alpn : { QStringLiteral("http/1.1"), QStringLiteral("h2") }) {
            const bool expectKey = alpn == QLatin1String("h2");
            cases.append({ QStringLiteral("gen/xhttp/packet-up/tls/%1 + hKeepAlivePeriod 30").arg(alpn),
                           [alpn](XrayConfigModel &m) {
                               base(m, "xhttp", "tls", QString(), "Packet-up");
                               m.setAlpn(alpn);
                               m.setXmuxHKeepAlivePeriod(QStringLiteral("30"));
                           }, Expect::Connects, false, [expectKey](const QJsonObject &client, const QJsonObject &) {
                               const QJsonObject mux = xhttpOf(client, "outbounds").value(QStringLiteral("xmux")).toObject();
                               if (mux.contains(QStringLiteral("hKeepAlivePeriod")) != expectKey)
                                   return QStringLiteral("client xmux.hKeepAlivePeriod present=%1, expected %2")
                                           .arg(mux.contains(QStringLiteral("hKeepAlivePeriod")))
                                           .arg(expectKey);
                               return QString();
                           } });
        }

        cases.append({ "refused/xPadding 0-100", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXPaddingBytesMin(QStringLiteral("0"));
                           m.setXPaddingBytesMax(QStringLiteral("100"));
                       }, Expect::Refused });
        cases.append({ "refused/scMaxEachPostBytes 0-1", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpScMaxEachPostBytesMin(QStringLiteral("0"));
                           m.setXhttpScMaxEachPostBytesMax(QStringLiteral("1"));
                       }, Expect::Refused });
        cases.append({ "refused/scStreamUpServerSecs 0-5", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Stream-up");
                           m.setXhttpScStreamUpServerSecsMin(QStringLiteral("0"));
                           m.setXhttpScStreamUpServerSecsMax(QStringLiteral("5"));
                       }, Expect::Refused });
        cases.append({ "refused/scMaxBufferedPosts 200000", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpScMaxBufferedPosts(QStringLiteral("200000"));
                       }, Expect::Refused });
        cases.append({ "refused/mkcp uplinkCapacity 5000", [](XrayConfigModel &m) {
                           base(m, "mkcp", "tls");
                           m.setMkcpUplinkCapacity(QStringLiteral("5000"));
                       }, Expect::Refused });
        cases.append({ "refused/mkcp cwndMultiplier 6000", [](XrayConfigModel &m) {
                           base(m, "mkcp", "tls");
                           m.setMkcpCwndMultiplier(QStringLiteral("6000"));
                       }, Expect::Refused });
        cases.append({ "refused/mkcp tti 5", [](XrayConfigModel &m) {
                           base(m, "mkcp", "tls");
                           m.setMkcpTti(QStringLiteral("5"));
                       }, Expect::Refused });

        cases.append({ "rule/xhttp/tls/vision dropped", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", "xtls-rprx-vision");
                       }, Expect::Connects, false, {}, "vision-needs-raw" });
        cases.append({ "rule/mkcp/tls/fingerprint android", [](XrayConfigModel &m) {
                           base(m, "mkcp", "tls");
                           m.setFingerprint(QStringLiteral("android"));
                       }, Expect::Connects, false, {}, "tls-fingerprint-kcp-ignored" });
        cases.append({ "rule/xhttp/stream-one/session header", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls");
                           m.setXhttpSessionPlacement(QStringLiteral("Header"));
                           m.setXhttpSessionKey(QStringLiteral("X-Sess"));
                       }, Expect::Connects, false, {}, "use-session-not-stream-one" });
        cases.append({ "rule/xhttp/stream-up/seq header", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Stream-up");
                           m.setXhttpSeqPlacement(QStringLiteral("Header"));
                       }, Expect::Connects, false, {}, "use-seq-packet-up-only" });
        cases.append({ "rule/xhttp/stream-one/scMaxEachPostBytes 500000", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls");
                           m.setXhttpScMaxEachPostBytesMin(QStringLiteral("500000"));
                           m.setXhttpScMaxEachPostBytesMax(QStringLiteral("500000"));
                       }, Expect::Connects, false, {}, "use-max-each-post-packet-up-only" });
        cases.append({ "rule/xhttp/stream-one/scMinPostsIntervalMs 10-20", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls");
                           m.setXhttpScMinPostsIntervalMsMin(QStringLiteral("10"));
                           m.setXhttpScMinPostsIntervalMsMax(QStringLiteral("20"));
                       }, Expect::Connects, false, {}, "use-min-posts-interval-packet-up-only" });
        cases.append({ "rule/xhttp/stream-one/scMaxBufferedPosts 50", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls");
                           m.setXhttpScMaxBufferedPosts(QStringLiteral("50"));
                       }, Expect::Connects, false, {}, "use-max-buffered-posts-packet-up-only" });
        cases.append({ "rule/xhttp/packet-up/scStreamUpServerSecs 5-10", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpScStreamUpServerSecsMin(QStringLiteral("5"));
                           m.setXhttpScStreamUpServerSecsMax(QStringLiteral("10"));
                       }, Expect::Connects, false, {}, "use-stream-up-secs-stream-up-only" });
        cases.append({ "rule/xhttp/packet-up/body + uplinkChunkSize 1024", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpUplinkChunkSize(QStringLiteral("1024"));
                       }, Expect::Connects, false, {}, "use-chunk-size-header-cookie-only" });
        cases.append({ "rule/xhttp/packet-up/body + uplinkDataKey", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpUplinkDataKey(QStringLiteral("x_d"));
                       }, Expect::Connects, false, {}, "use-uplink-data-key-body" });
        cases.append({ "rule/xhttp/obfs off + xPaddingKey", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls");
                           m.setXPaddingObfsMode(false);
                           m.setXPaddingKey(QStringLiteral("x_pad"));
                       }, Expect::Connects, false, {}, "use-padding-obfs-fields" });
        cases.append({ "rule/xhttp/packet-up/gRPC header on", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpDisableGrpc(false);
                       }, Expect::Connects, false, {}, "use-nogrpc-stream-only" });
        cases.append({ "rule/xhttp/packet-up/path placement + sessionKey", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpSessionKey(QStringLiteral("x_s"));
                       }, Expect::Connects, false, {}, "use-session-seq-key-path" });
        cases.append({ "rule/xhttp/xPadding cookie + xPaddingHeader", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls");
                           m.setXPaddingPlacement(QStringLiteral("Cookie"));
                           m.setXPaddingHeader(QStringLiteral("X-Pad"));
                       }, Expect::Connects, false, {}, "use-padding-key-header-placement" });
        cases.append({ "rule/xhttp/packet-up/scMinPostsIntervalMs 0-0", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpScMinPostsIntervalMsMin(QStringLiteral("0"));
                           m.setXhttpScMinPostsIntervalMsMax(QStringLiteral("0"));
                       }, Expect::Connects, false, {}, "xhttp-min-posts-interval-zero" });
        cases.append({ "rule/xhttp/packet-up/header placements + keys", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpSessionPlacement(QStringLiteral("Header"));
                           m.setXhttpSeqPlacement(QStringLiteral("Header"));
                           m.setXhttpUplinkDataPlacement(QStringLiteral("Header"));
                           m.setXhttpSessionKey(QStringLiteral("X-S"));
                           m.setXhttpSeqKey(QStringLiteral("X-Q"));
                           m.setXhttpUplinkDataKey(QStringLiteral("X-D"));
                       }, Expect::Connects, false, {}, "xhttp-placement-enum" });

        cases.append({ "broken/xhttp/packet-up/scMaxEachPostBytes 0-1", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpScMaxEachPostBytesMin(QStringLiteral("0"));
                           m.setXhttpScMaxEachPostBytesMax(QStringLiteral("1"));
                       }, Expect::RefusedAndBroken, false, {}, "xhttp-sc-max-each-post-bytes" });
        cases.append({ "broken/xhttp/packet-up/scMaxEachPostBytes 100-0", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpScMaxEachPostBytesMin(QStringLiteral("100"));
                           m.setXhttpScMaxEachPostBytesMax(QStringLiteral("0"));
                       }, Expect::RefusedAndBroken, false, {}, "xhttp-range-reversed-zero" });

        cases.append({ "stricter/xhttp/stream-up/scStreamUpServerSecs 0-5", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Stream-up");
                           m.setXhttpScStreamUpServerSecsMin(QStringLiteral("0"));
                           m.setXhttpScStreamUpServerSecsMax(QStringLiteral("5"));
                       }, Expect::RefusedButWorks, false, {}, "xhttp-stream-up-server-secs-zero" });
        cases.append({ "stricter/mkcp uplinkCapacity 5000", [](XrayConfigModel &m) {
                           base(m, "mkcp", "tls");
                           m.setMkcpUplinkCapacity(QStringLiteral("5000"));
                       }, Expect::RefusedButWorks, false, {}, "kcp-capacity-overflow" });
        cases.append({ "stricter/mkcp cwndMultiplier 6000", [](XrayConfigModel &m) {
                           base(m, "mkcp", "tls");
                           m.setMkcpCwndMultiplier(QStringLiteral("6000"));
                       }, Expect::RefusedButWorks, false, {}, "kcp-cwnd-overflow" });
        cases.append({ "broken/xhttp/packet-up/sessionKey with space", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpSessionPlacement(QStringLiteral("Header"));
                           m.setXhttpSessionKey(QStringLiteral("a b"));
                       }, Expect::RefusedAndBroken });
        cases.append({ "broken/xhttp/xPaddingHeader with space", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls");
                           m.setXPaddingPlacement(QStringLiteral("Header"));
                           m.setXPaddingHeader(QStringLiteral("a b"));
                       }, Expect::RefusedAndBroken });
        cases.append({ "broken/xhttp/packet-up/uplinkDataKey with colon", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpUplinkDataPlacement(QStringLiteral("Header"));
                           m.setXhttpUplinkDataKey(QStringLiteral("a:b"));
                       }, Expect::RefusedAndBroken });

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
        const QStringList unknownFingerprints = { "hellogolang", "unsafe", "garbage", "hellochrome_100_psk" };
        const QStringList tls12Fingerprints = { "android", "360" };
        for (const auto &realityBase : realityBases) {
            for (const QString &fingerprint : storedFingerprints) {
                const auto apply = realityBase.second;
                const Expect expect = unknownFingerprints.contains(fingerprint) ? Expect::Refused
                        : tls12Fingerprints.contains(fingerprint)              ? Expect::RefusedButWorks
                                                                               : Expect::Connects;
                cases.append({ QStringLiteral("fingerprint/reality/%1/'%2'").arg(realityBase.first, fingerprint),
                               [apply, fingerprint](XrayConfigModel &m) {
                                   apply(m);
                                   m.setFingerprint(fingerprint);
                               },
                               expect });
            }
        }

        for (const QString &fingerprint : { QStringLiteral("hellochrome_120"), QStringLiteral("ios"),
                                            QStringLiteral("android") }) {
            cases.append({ QStringLiteral("old-client/raw-vision/%1").arg(fingerprint),
                           [](XrayConfigModel &m) { base(m, "raw", "reality", "xtls-rprx-vision"); },
                           Expect::NoTraffic, false, {}, {}, {},
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
                               h2WithoutAlpn ? Expect::RefusedAndBroken : Expect::Connects });
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

    struct LinkCase
    {
        QString name;
        std::function<void(XrayConfigModel &)> edit;
        QStringList required;
        QStringList absent;
    };

    QList<LinkCase> linkCases()
    {
        return {
            { "raw/reality/vision", [](XrayConfigModel &m) { base(m, "raw", "reality", "xtls-rprx-vision"); },
              { "security=reality", "flow=xtls-rprx-vision", "fp=chrome", "pbk", "sid" }, { "type", "alpn", "pcs" } },
            { "raw/tls/vision/firefox", [](XrayConfigModel &m) {
                  base(m, "raw", "tls", "xtls-rprx-vision");
                  m.setFingerprint(QStringLiteral("firefox"));
              },
              { "security=tls", "flow=xtls-rprx-vision", "fp=firefox", "alpn=h2", "pcs" }, { "type", "pbk" } },
            { "xhttp/packet-up/tls/host+path", [](XrayConfigModel &m) {
                  base(m, "xhttp", "tls", QString(), "Packet-up");
                  m.setXhttpHost(QStringLiteral("www.googletagmanager.com"));
                  m.setXhttpPath(QStringLiteral("/e2e"));
              },
              { "type=xhttp", "security=tls", "host=www.googletagmanager.com", "path=/e2e", "mode=packet-up", "fp",
                "alpn=h2", "extra", "pcs" },
              { "flow" } },
            { "xhttp/stream-one/reality", [](XrayConfigModel &m) { base(m, "xhttp", "reality"); },
              { "type=xhttp", "security=reality", "mode=stream-one", "pbk", "sid", "extra" }, { "flow", "pcs" } },
        };
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
    QJsonObject storedClientJson() const
    {
        const XrayProtocolConfig xray = storedXray();
        if (!xray.hasClientConfig())
            return {};
        return QJsonDocument::fromJson(sanitizeNativeConfig(xray.clientConfig->nativeConfig).toUtf8()).object();
    }

    QJsonObject serverJson()
    {
        SshSession ssh;
        ErrorCode error = ErrorCode::NoError;
        const QByteArray text = ssh.getTextFileFromContainer(DockerContainer::Xray, m_credentials,
                                                             QStringLiteral("/opt/amnezia/xray/server.json"), error);
        if (error != ErrorCode::NoError)
            qWarning().noquote() << "Could not read server.json, error" << static_cast<int>(error);
        return QJsonDocument::fromJson(text).object();
    }

    QString serverState()
    {
        SshSession ssh;
        QString out;
        auto collect = [&out](const QString &data, libssh::Client &) {
            out += data;
            return ErrorCode::NoError;
        };
        const QString script = QStringLiteral(
                "sudo docker inspect -f 'running={{.State.Running}} restarts={{.RestartCount}}' amnezia-xray 2>&1 || true\n"
                "sudo docker exec amnezia-xray xray run -test -c /opt/amnezia/xray/server.json 2>&1 || true");
        const ErrorCode error = ssh.runScript(m_credentials, script, collect, collect);
        if (error != ErrorCode::NoError)
            out += QStringLiteral(" [ssh error %1]").arg(static_cast<int>(error));
        return out.simplified();
    }

    void revokeExportedClient(const QString &clientId)
    {
        UsersController users(m_coreController->m_serversRepository);
        QJsonArray table;
        QObject::connect(&users, &UsersController::clientsUpdated, &users,
                         [&table](const QJsonArray &clients) { table = clients; });
        const ErrorCode updateError = users.updateClients(m_serverId, DockerContainer::Xray);
        if (updateError != ErrorCode::NoError) {
            qWarning().noquote() << "Could not read clients to revoke" << clientId << "error"
                                 << static_cast<int>(updateError);
            return;
        }
        for (int i = 0; i < table.size(); ++i) {
            if (table.at(i).toObject().value(configKey::clientId).toString() != clientId)
                continue;
            const ErrorCode revokeError = users.revokeClient(m_serverId, i, DockerContainer::Xray);
            if (revokeError != ErrorCode::NoError)
                qWarning().noquote() << "Could not revoke" << clientId << "error" << static_cast<int>(revokeError);
            return;
        }
        qWarning().noquote() << "Exported client" << clientId << "not found in clientsTable, left on the server";
    }

    QJsonObject clientJsonForRun(const ClientEdit &clientEdit, QString &error) const
    {
        QJsonObject native = storedClientJson();
        if (native.isEmpty()) {
            error = QStringLiteral("no client config stored after save");
            return {};
        }
        if (clientEdit)
            clientEdit(native);
        return native;
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

    bool startClient(QJsonObject native, QProcess &client, int &socksPort, QString &error)
    {
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
        const QJsonObject native = clientJsonForRun(clientEdit, error);
        return native.isEmpty() ? QString() : egressIpThroughClient(native, error);
    }

    QString egressIpThroughClient(const QJsonObject &native, QString &error)
    {
        QProcess client;
        int socksPort = 0;
        if (!startClient(native, client, socksPort, error))
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

    static QString maskedAddresses(QString text)
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
        const QJsonObject native = clientJsonForRun(clientEdit, error);
        if (native.isEmpty() || !startClient(native, client, socksPort, error)) {
            result.clientFailed = true;
            result.details = maskedAddresses(error);
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
        result.details = maskedAddresses(errors.join(QStringLiteral(" | ")));
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
                                 "-", maskedAddresses(storedSummary()), "-" });
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
                         QString::number(check.ms), maskedAddresses(storedSummary()), check.details });
    }

private slots:
    void initTestCase()
    {
        // Called after QTest installed its message handler, so the app logger takes over and writes every qInfo to file.
        if (Logger::init(false))
            qInfo().noquote() << "log file:" << Logger::userLogsFilePath();
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
        Logger::deInit();
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
        qInfo().noquote() << "=====" << c.name << "=====";
        if (!c.rule.isEmpty())
            qInfo().noquote() << "rule:" << c.rule;
        qInfo().noquote() << "settings:" << settingsSummary(model);
        qInfo().noquote() << "validation:" << (errors.isEmpty() ? QStringLiteral("ok") : errors.join(QStringLiteral("; ")));

        if (c.expect == Expect::Refused) {
            m_lastReason = QStringLiteral("validation must refuse this before anything reaches the server");
            QVERIFY2(!errors.isEmpty(), qPrintable(m_lastReason));
            return;
        }
        if (c.expect == Expect::RefusedAndBroken || c.expect == Expect::RefusedButWorks) {
            if (!c.knownIssue.isEmpty())
                QEXPECT_FAIL("", qPrintable(c.knownIssue), Continue);
            QVERIFY2(!errors.isEmpty(), "validation must refuse this combination");
        }
        if (c.expect == Expect::Connects || c.expect == Expect::NoTraffic) {
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
        const QJsonObject client = storedClientJson();
        const QJsonObject server = serverJson();
        const QString state = serverState();
        logSaved(client, server, saveError, saveMs, state);

        QString why;
        const QString ip = saveError == ErrorCode::NoError ? egressIpThroughTunnel(why, c.clientEdit) : QString();
        qInfo().noquote() << "traffic:" << (ip.isEmpty() ? QStringLiteral("no traffic: ") + why : QStringLiteral("egress ") + ip);

        if (c.expect == Expect::RefusedAndBroken) {
            m_lastReason = QStringLiteral("validation refuses a working combination: traffic left through ") + ip;
            QVERIFY2(ip.isEmpty(), qPrintable(m_lastReason));
            return;
        }

        m_lastReason = QStringLiteral("save failed, error %1; server: %2").arg(static_cast<int>(saveError)).arg(state);
        QVERIFY2(saveError == ErrorCode::NoError, qPrintable(m_lastReason));

        if (c.expect == Expect::NoTraffic) {
            m_lastReason = QStringLiteral("expected no traffic, but it flowed");
            QVERIFY2(ip.isEmpty(), qPrintable(m_lastReason));
            m_lastReason = QStringLiteral("expected '%1' in the client log, got: %2").arg(c.expectInLog, why);
            QVERIFY2(c.expectInLog.isEmpty() || why.contains(c.expectInLog), qPrintable(m_lastReason));
            return;
        }

        m_lastReason = why + QStringLiteral(" | server: ") + state;
        QVERIFY2(!ip.isEmpty(), qPrintable(m_lastReason));
        m_lastReason = QStringLiteral("traffic left through an unexpected address");
        QVERIFY2(m_expectedIps.isEmpty() || m_expectedIps.contains(ip),
                 qPrintable(QStringLiteral("traffic left through %1, expected %2")
                                    .arg(ip, m_expectedIps.join(QStringLiteral(", ")))));

        if (c.check) {
            m_lastReason = c.check(client, server);
            QVERIFY2(m_lastReason.isEmpty(), qPrintable(m_lastReason));
        }
    }

    void shareLinkRoundTrip_data()
    {
        QTest::addColumn<int>("index");
        for (int i = 0; i < linkCases().size(); ++i)
            QTest::newRow(qPrintable(linkCases().at(i).name)) << i;
    }

    void shareLinkRoundTrip()
    {
        QFETCH(int, index);
        const LinkCase c = linkCases().at(index);

        XrayConfigModel model;
        model.updateModel(DockerContainer::Xray, storedXray());
        c.edit(model);
        const QStringList errors = model.validationErrors();
        qInfo().noquote() << "=====" << QStringLiteral("link/") + c.name << "=====";
        qInfo().noquote() << "settings:" << settingsSummary(model);
        qInfo().noquote() << "validation:" << (errors.isEmpty() ? QStringLiteral("ok") : errors.join(QStringLiteral("; ")));
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join("; ")));

        QElapsedTimer timer;
        timer.start();
        const ErrorCode saveError = save(model);
        const qint64 saveMs = timer.elapsed();
        const QString state = serverState();
        logSaved(storedClientJson(), serverJson(), saveError, saveMs, state);
        QCOMPARE(saveError, ErrorCode::NoError);

        const QString clientName = QStringLiteral("e2e-link-%1").arg(QUuid::createUuid().toString(QUuid::Id128).left(8));
        const ExportController::ExportResult exported =
                m_coreController->m_exportController->generateXrayConfig(m_serverId, clientName);
        QCOMPARE(exported.errorCode, ErrorCode::NoError);
        const QString link = exported.nativeConfigString;
        const QUrl url(link);
        const QString shownLink = url.userName().isEmpty() ? link : QString(link).replace(url.userName(), QStringLiteral("***"));
        qInfo().noquote() << "link:" << shownLink;
        QVERIFY2(link.startsWith(QStringLiteral("vless://")), qPrintable(shownLink));

        const QUrlQuery query(url);
        for (const QString &item : c.required) {
            const QString key = item.section(QLatin1Char('='), 0, 0);
            QVERIFY2(query.hasQueryItem(key), qPrintable(QStringLiteral("link has no %1: %2").arg(key, shownLink)));
            if (item.contains(QLatin1Char('=')))
                QCOMPARE(query.queryItemValue(key, QUrl::FullyDecoded), item.section(QLatin1Char('='), 1));
        }
        for (const QString &key : c.absent)
            QVERIFY2(!query.hasQueryItem(key), qPrintable(QStringLiteral("link must not have %1: %2").arg(key, shownLink)));

        QString alias;
        QString importError;
        const QJsonObject imported = serialization::vless::Deserialize(link, &alias, &importError);
        qInfo().noquote() << "imported outbound:"
                          << (imported.isEmpty()
                                      ? QStringLiteral("import failed: ") + importError
                                      : compactJson(xraytest::vlessEntry(imported, QStringLiteral("outbounds"))));
        QVERIFY2(!imported.isEmpty(), qPrintable(QStringLiteral("import failed: ") + importError));

        QString why;
        const QString ip = egressIpThroughClient(imported, why);
        qInfo().noquote() << "traffic:" << (ip.isEmpty() ? QStringLiteral("no traffic: ") + why : QStringLiteral("egress ") + ip);
        revokeExportedClient(url.userName());
        if (ip.isEmpty())
            QFAIL(qPrintable(why + QStringLiteral("\nserver: ") + serverState()));
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
        model.applyServerConfig(cleanConfig());
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
