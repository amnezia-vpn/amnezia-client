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
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QUuid>

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
#include "core/utils/errorCodes.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/serialization/serialization.h"
#include "secureQSettings.h"
#include "ui/models/protocols/xrayConfigModel.h"
#include "utils/testCoreController.h"
#include "utils/testUtils.h"
#include "logger.h"
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
        Explore,
    };

    using ConfigCheck = std::function<QString(const QJsonObject &client, const QJsonObject &server)>;

    struct VpsCase
    {
        QString name;
        std::function<void(XrayConfigModel &)> edit;
        Expect expect = Expect::Connects;
        bool deviceOnly = false;
        ConfigCheck check;
        QString rule;
        QString knownIssue;
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

    QJsonObject vlessEntry(const QJsonObject &root, const QString &list)
    {
        for (const QJsonValue &v : root.value(list).toArray()) {
            if (v.toObject().value(QStringLiteral("protocol")).toString() == QLatin1String("vless"))
                return v.toObject();
        }
        return {};
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
        qInfo().noquote() << "client outbound:" << compactJson(vlessEntry(client, QStringLiteral("outbounds")));
        qInfo().noquote() << "server streamSettings:"
                          << compactJson(vlessEntry(server, QStringLiteral("inbounds")).value(QStringLiteral("streamSettings")));
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
            { "Auto", "none" },       { "Auto", "tls" },       { "Auto", "reality" },
            { "Packet-up", "none" },  { "Packet-up", "tls" },
            { "Stream-up", "tls" },   { "Stream-up", "reality" },
            { "Stream-one", "tls" },  { "Stream-one", "reality" },
        };
        for (const auto &combo : xhttpCombos) {
            const QString mode = combo.first;
            const QString security = combo.second;
            cases.append({ QStringLiteral("xhttp/%1/%2").arg(mode, security),
                           [mode, security](XrayConfigModel &m) { base(m, "xhttp", security, QString(), mode); } });
        }

        const QList<QPair<QString, QString>> rawCombos = {
            { "none", "" },
            { "tls", "" },     { "tls", "xtls-rprx-vision" },     { "tls", "xtls-rprx-vision-udp443" },
            { "reality", "" }, { "reality", "xtls-rprx-vision" }, { "reality", "xtls-rprx-vision-udp443" },
        };
        for (const auto &combo : rawCombos) {
            const QString security = combo.first;
            const QString flow = combo.second;
            cases.append({ QStringLiteral("raw/%1/%2").arg(security, flow.isEmpty() ? QStringLiteral("noflow") : flow),
                           [security, flow](XrayConfigModel &m) { base(m, "raw", security, flow); } });
        }

        cases.append({ "mkcp/none", [](XrayConfigModel &m) { base(m, "mkcp", "none"); } });
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
            { "Stream-one", "none" }, { "Stream-up", "none" },
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
        cases.append({ "broken/raw/reality/vision/android", [](XrayConfigModel &m) {
                           base(m, "raw", "reality", "xtls-rprx-vision");
                           m.setFingerprint(QStringLiteral("android"));
                       }, Expect::RefusedAndBroken });
        cases.append({ "broken/raw/reality/360", [](XrayConfigModel &m) {
                           base(m, "raw", "reality");
                           m.setFingerprint(QStringLiteral("360"));
                       }, Expect::RefusedAndBroken });
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
                           base(m, "mkcp", "none");
                           m.setMkcpUplinkCapacity(QStringLiteral("5000"));
                       }, Expect::Refused });
        cases.append({ "refused/mkcp cwndMultiplier 6000", [](XrayConfigModel &m) {
                           base(m, "mkcp", "none");
                           m.setMkcpCwndMultiplier(QStringLiteral("6000"));
                       }, Expect::Refused });
        cases.append({ "refused/mkcp tti 5", [](XrayConfigModel &m) {
                           base(m, "mkcp", "none");
                           m.setMkcpTti(QStringLiteral("5"));
                       }, Expect::Refused });

        // Runtime rules: xray -test passes all of these, only traffic shows the effect.
        cases.append({ "rule/xhttp/tls/vision dropped", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", "xtls-rprx-vision");
                       }, Expect::Connects, false, {}, "vision-needs-raw" });
        cases.append({ "rule/raw/none/vision dropped", [](XrayConfigModel &m) {
                           base(m, "raw", "none", "xtls-rprx-vision");
                       }, Expect::Connects, false, {}, "vision-needs-security" });
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
        cases.append({ "rule/xhttp/none/hKeepAlivePeriod 30", [](XrayConfigModel &m) {
                           base(m, "xhttp", "none");
                           m.setXmuxHKeepAlivePeriod(QStringLiteral("30"));
                       }, Expect::Connects, false, {}, "use-keepalive-h1-none" });
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
                           base(m, "mkcp", "none");
                           m.setMkcpUplinkCapacity(QStringLiteral("5000"));
                       }, Expect::RefusedButWorks, false, {}, "kcp-capacity-overflow" });
        cases.append({ "stricter/mkcp cwndMultiplier 6000", [](XrayConfigModel &m) {
                           base(m, "mkcp", "none");
                           m.setMkcpCwndMultiplier(QStringLiteral("6000"));
                       }, Expect::RefusedButWorks, false, {}, "kcp-cwnd-overflow" });
        const QString keyIssue = QStringLiteral("validationErrors does not check XHTTP key characters; QML allows only [A-Za-z0-9_-]");
        cases.append({ "broken/xhttp/packet-up/sessionKey with space", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpSessionPlacement(QStringLiteral("Header"));
                           m.setXhttpSessionKey(QStringLiteral("a b"));
                       }, Expect::RefusedAndBroken, false, {}, {}, keyIssue });
        cases.append({ "broken/xhttp/xPaddingHeader with space", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls");
                           m.setXPaddingPlacement(QStringLiteral("Header"));
                           m.setXPaddingHeader(QStringLiteral("a b"));
                       }, Expect::RefusedAndBroken, false, {}, {}, keyIssue });
        cases.append({ "broken/xhttp/packet-up/uplinkDataKey with colon", [](XrayConfigModel &m) {
                           base(m, "xhttp", "tls", QString(), "Packet-up");
                           m.setXhttpUplinkDataPlacement(QStringLiteral("Header"));
                           m.setXhttpUplinkDataKey(QStringLiteral("a:b"));
                       }, Expect::RefusedAndBroken, false, {}, {}, keyIssue });

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
            { "xhttp/auto/none", [](XrayConfigModel &m) { base(m, "xhttp", "none", QString(), "Auto"); },
              { "type=xhttp", "mode=auto", "extra" }, { "flow", "security", "pcs" } },
        };
    }

    const QList<VpsCase> &cases()
    {
        static const QList<VpsCase> all = vpsCases();
        return all;
    }

    int freeLocalPort()
    {
        QTcpServer server;
        server.listen(QHostAddress::LocalHost, 0);
        const int port = server.serverPort();
        server.close();
        return port;
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

    QString egressIpThroughTunnel(QString &error)
    {
        const QJsonObject native = storedClientJson();
        if (native.isEmpty()) {
            error = QStringLiteral("no client config stored after save");
            return {};
        }
        return egressIpThroughClient(native, error);
    }

    QString egressIpThroughClient(QJsonObject native, QString &error)
    {
        const int socksPort = freeLocalPort();
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
            error = QStringLiteral("local socks never came up: ") + QString::fromUtf8(client.readAll());
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
        if (ip.isEmpty())
            error += QStringLiteral("\nxray client output: ") + QString::fromUtf8(client.readAll());
        return ip;
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
        }
        if (m_coreController) {
            ssh_finalize();
            m_settings->clearSettings();
            delete m_coreController;
            delete m_settings;
        }
        Logger::deInit();
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
            QVERIFY2(!errors.isEmpty(), "validation must refuse this before anything reaches the server");
            return;
        }
        if (c.expect == Expect::RefusedAndBroken || c.expect == Expect::RefusedButWorks) {
            if (!c.knownIssue.isEmpty())
                QEXPECT_FAIL("", qPrintable(c.knownIssue), Continue);
            QVERIFY2(!errors.isEmpty(), "validation must refuse this combination");
        }
        if (c.expect == Expect::Connects)
            QVERIFY2(errors.isEmpty(), qPrintable(errors.join("; ")));
        if (c.deviceOnly)
            QVERIFY2(!model.pendingChangeTouchesServer(QString()), "a device-only edit must not count as a server change");

        QElapsedTimer timer;
        timer.start();
        const ErrorCode saveError = save(model);
        const qint64 saveMs = timer.elapsed();
        const QJsonObject client = storedClientJson();
        const QJsonObject server = serverJson();
        const QString state = serverState();
        logSaved(client, server, saveError, saveMs, state);

        QString why;
        const QString ip = saveError == ErrorCode::NoError ? egressIpThroughTunnel(why) : QString();
        qInfo().noquote() << "traffic:" << (ip.isEmpty() ? QStringLiteral("no traffic: ") + why : QStringLiteral("egress ") + ip);

        if (c.expect == Expect::RefusedAndBroken) {
            if (!ip.isEmpty())
                QFAIL(qPrintable(QStringLiteral("validation refuses a working combination: traffic left through ") + ip));
            return;
        }
        if (c.expect == Expect::Explore)
            QSKIP(qPrintable(QStringLiteral("EXPLORE %1: %2")
                                     .arg(c.name, ip.isEmpty() ? QStringLiteral("no traffic") : QStringLiteral("traffic flows"))));

        if (saveError != ErrorCode::NoError)
            QFAIL(qPrintable(QStringLiteral("save failed, error %1; server: %2").arg(static_cast<int>(saveError)).arg(state)));
        if (ip.isEmpty())
            QFAIL(qPrintable(why + QStringLiteral("\nserver: ") + state));
        QVERIFY2(m_expectedIps.isEmpty() || m_expectedIps.contains(ip),
                 qPrintable(QStringLiteral("traffic left through %1, expected %2")
                                    .arg(ip, m_expectedIps.join(QStringLiteral(", ")))));

        if (c.check) {
            const QString problem = c.check(client, server);
            QVERIFY2(problem.isEmpty(), qPrintable(problem));
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
                          << (imported.isEmpty() ? QStringLiteral("import failed: ") + importError
                                                 : compactJson(vlessEntry(imported, QStringLiteral("outbounds"))));
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
        QVERIFY2(!egressIpThroughTunnel(why).isEmpty(), qPrintable(why));

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
        QVERIFY2(!egressIpThroughTunnel(why).isEmpty(), qPrintable(why));
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
