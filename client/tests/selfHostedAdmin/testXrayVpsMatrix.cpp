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
#include <QUrlQuery>
#include <QUuid>

#include <cstdio>
#include <functional>
#include <libssh/libssh.h>

#include "core/controllers/selfhosted/exportController.h"
#include "core/controllers/selfhosted/installController.h"
#include "core/controllers/selfhosted/usersController.h"
#include "core/controllers/serversController.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/xrayInbound.h"
#include "core/models/protocols/xrayInboundValidator.h"
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
#include "ui/models/protocols/xrayPresetsModel.h"
#include "ui/models/protocols/xraySettingsModel.h"
#include "utils/testCoreController.h"
#include "utils/testUtils.h"
#include "utils/xrayContractCases.h"
#include "utils/xrayExploreCases.h"
#include "utils/xrayMatrix.h"
#include "utils/xrayRuntime.h"
#include "vpnConnection.h"

using namespace amnezia;
using namespace amnezia::test;
using xrayexplore::Carrier;
using xraytest::InboundEdit;

namespace
{
    enum class Expect {
        Connects,
        Refused,
        RefusedAndBroken,
        RefusedButWorks,
        NoTraffic,
    };

    const QString vpsSite = QStringLiteral("www.googletagmanager.com");

    using ConfigCheck = std::function<QString(const QJsonObject &client, const QJsonObject &server)>;
    using ClientEdit = xrayexplore::ClientEdit;
    using InboundSource = std::function<QJsonObject(const QString &port)>;

    InboundSource fromCarrier(const Carrier &carrier)
    {
        return [carrier](const QString &port) { return xrayexplore::carrierInbound(carrier, port, vpsSite); };
    }

    struct VpsCase
    {
        QString name;
        InboundSource source;
        InboundEdit edit;
        Expect expect = Expect::Connects;
        QString fingerprint;
        ConfigCheck check;
        QString rule;
        ClientEdit clientEdit;
        QString expectInLog;

        VpsCase &expecting(Expect value)
        {
            expect = value;
            return *this;
        }
        VpsCase &storedFingerprint(const QString &value)
        {
            fingerprint = value;
            return *this;
        }
        VpsCase &checking(const ConfigCheck &value)
        {
            check = value;
            return *this;
        }
        VpsCase &forRule(const QString &value)
        {
            rule = value;
            return *this;
        }
        VpsCase &withClient(const ClientEdit &edit, const QString &log)
        {
            clientEdit = edit;
            expectInLog = log;
            return *this;
        }
    };

    VpsCase vps(const QString &name, const Carrier &carrier, const InboundEdit &edit = {})
    {
        VpsCase c;
        c.name = name;
        c.source = fromCarrier(carrier);
        c.edit = edit;
        return c;
    }

    Carrier rawC(const QString &security, const QString &flow = QString())
    {
        return xrayexplore::raw(security, flow);
    }

    Carrier xhttpC(const QString &security, const QString &mode = QStringLiteral("Stream-one"))
    {
        return xrayexplore::xhttp(security, mode);
    }

    InboundEdit xh(const QString &key, const QJsonValue &value)
    {
        return xraytest::xhttpValue(key, value);
    }

    InboundEdit kcp(const QString &key, const QJsonValue &value)
    {
        return xraytest::kcpValue(key, value);
    }

    InboundEdit alpn(const QString &value)
    {
        return xraytest::tlsValue(QStringLiteral("alpn"), xrayexplore::alpnValue(value));
    }

    InboundEdit all(const QList<InboundEdit> &edits)
    {
        return [edits](QJsonObject &inbound) {
            for (const InboundEdit &edit : edits)
                edit(inbound);
        };
    }

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

    QString settingsSummary(const XrayServerConfig &s)
    {
        return QStringLiteral("transport=%1 security=%2 flow=%3 mode=%4 fp=%5 alpn=%6 sni=%7")
                .arg(s.transport, s.security, s.flow.isEmpty() ? QStringLiteral("-") : s.flow, s.xhttp.mode,
                     s.fingerprint, s.alpn, s.sni);
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

    bool templateIsValid(const QJsonObject &inbound)
    {
        for (const XrayInboundIssue &issue : validateXrayInbound(xrayInbound::format(inbound)).issues) {
            if (issue.level == XrayInboundIssue::Level::Error)
                return false;
        }
        return true;
    }

    QList<VpsCase> vpsCases()
    {
        QList<VpsCase> cases;
        const QString tls = QStringLiteral("tls");
        const QString reality = QStringLiteral("reality");
        const QString vision = QStringLiteral("xtls-rprx-vision");
        const QString packetUp = QStringLiteral("Packet-up");
        const QString streamUp = QStringLiteral("Stream-up");

        const QList<QPair<QString, QString>> xhttpCombos = {
            { "Auto", "tls" },          { "Auto", "reality" },   { "Packet-up", "tls" },      { "Stream-up", "tls" },
            { "Stream-up", "reality" }, { "Stream-one", "tls" }, { "Stream-one", "reality" },
        };
        for (const auto &combo : xhttpCombos)
            cases << vps(QStringLiteral("xhttp/%1/%2").arg(combo.first, combo.second), xhttpC(combo.second, combo.first));

        for (const QString &security : { tls, reality }) {
            for (const QString &flow : { QString(), vision })
                cases << vps(rawC(security, flow).name, rawC(security, flow));
            cases << vps(QStringLiteral("refused/raw/%1/client flow udp443").arg(security), rawC(security, vision),
                         xraytest::clientFlow(QStringLiteral("xtls-rprx-vision-udp443")))
                             .expecting(Expect::Refused);
        }

        cases << vps("mkcp/tls", xrayexplore::mkcp());

        cases << vps("xhttp/packet-up/tls/cookie+GET", xhttpC(tls, packetUp),
                     all({ xh("uplinkDataPlacement", "cookie"), xh("uplinkHTTPMethod", "GET") }));
        cases << vps("xhttp/reality/xPadding obfs off", xhttpC(reality), xh("xPaddingObfsMode", false));
        cases << vps("xhttp/reality/xPadding 200-400", xhttpC(reality), xh("xPaddingBytes", "200-400"));
        cases << vps("xhttp/reality/path without slash", xhttpC(reality), xh("path", "amnezia"));
        cases << vps("xhttp/tls/xmux concurrency 16-32", xhttpC(tls),
                     xraytest::xmuxValue(QStringLiteral("maxConcurrency"), QStringLiteral("16-32")));
        cases << vps("xhttp/tls/uplinkHTTPMethod PUT + stored fingerprint firefox", xhttpC(tls),
                     xh("uplinkHTTPMethod", "PUT"))
                         .storedFingerprint(QStringLiteral("firefox"));

        cases << vps("refused/xmux concurrency and connections", xhttpC(tls),
                     all({ xraytest::xmuxValue(QStringLiteral("maxConcurrency"), 1),
                           xraytest::xmuxValue(QStringLiteral("maxConnections"), 1) }))
                         .expecting(Expect::Refused);
        cases << vps("refused/wildcard serverNames", rawC(reality),
                     xraytest::realityServerName(QStringLiteral("*.example.com")))
                         .expecting(Expect::Refused);
        cases << vps("refused/security none", rawC(tls), xraytest::streamValue(QStringLiteral("security"), "none"))
                         .expecting(Expect::Refused);
        cases << vps("refused/reality on kcp", rawC(reality), xraytest::streamValue(QStringLiteral("network"), "kcp"))
                         .expecting(Expect::Refused);
        cases << vps("refused/xhttp/tls/vision", xhttpC(tls), xraytest::clientFlow(vision)).expecting(Expect::Refused);

        cases << vps("xhttp/packet-up/reality", xhttpC(reality, packetUp));
        for (const QString &mode : { QStringLiteral("Stream-one"), streamUp })
            cases << vps(QStringLiteral("xhttp/%1/tls/http1.1").arg(mode), xhttpC(tls, mode), alpn("http/1.1"));

        for (const QString fp : { QStringLiteral("android"), QStringLiteral("360") }) {
            cases << vps(QStringLiteral("legacy/xhttp/packet-up/tls/%1+h2").arg(fp), xhttpC(tls, packetUp))
                             .storedFingerprint(fp)
                             .expecting(Expect::NoTraffic);
        }
        cases << vps("legacy/raw/tls/vision/android", rawC(tls, vision))
                         .storedFingerprint(QStringLiteral("android"))
                         .expecting(Expect::NoTraffic);
        cases << vps("broken/raw/reality/ip serverNames", rawC(reality),
                     xraytest::realityServerName(QStringLiteral("1.2.3.4")))
                         .expecting(Expect::RefusedAndBroken);

        cases << vps("boundary/xhttp/packet-up/tls/android+http1.1", xhttpC(tls, packetUp), alpn("http/1.1"))
                         .storedFingerprint(QStringLiteral("android"));
        cases << vps("boundary/raw/tls/noflow/android", rawC(tls)).storedFingerprint(QStringLiteral("android"));

        cases << vps("gen/xhttp/reality/obfs off + xPadding 200-400", xhttpC(reality),
                     all({ xh("xPaddingObfsMode", false), xh("xPaddingBytes", "200-400") }))
                         .checking([](const QJsonObject &client, const QJsonObject &server) {
                             for (const auto &side :
                                  { qMakePair(QStringLiteral("client"), xhttpOf(client, "outbounds")),
                                    qMakePair(QStringLiteral("server"), xhttpOf(server, "inbounds")) }) {
                                 const QString bytes = side.second.value(QStringLiteral("xPaddingBytes")).toString();
                                 if (bytes != QLatin1String("200-400"))
                                     return QStringLiteral("%1 xPaddingBytes is \"%2\", expected 200-400")
                                             .arg(side.first, bytes);
                             }
                             return QString();
                         });
        cases << vps("gen/xhttp/packet-up/tls/xPadding 0-0", xhttpC(tls, packetUp), xh("xPaddingBytes", "0-0"));
        cases << vps("gen/xhttp/packet-up/tls/scMaxBufferedPosts 0", xhttpC(tls, packetUp), xh("scMaxBufferedPosts", 0))
                         .checking([](const QJsonObject &client, const QJsonObject &) {
                             if (xhttpOf(client, "outbounds").contains(QStringLiteral("scMaxBufferedPosts")))
                                 return QStringLiteral("client still has scMaxBufferedPosts");
                             return QString();
                         });
        cases << vps("gen/xhttp/stream-up/tls/scStreamUpServerSecs removed", xhttpC(tls, streamUp),
                     xh("scStreamUpServerSecs", QJsonValue(QJsonValue::Undefined)))
                         .checking([](const QJsonObject &, const QJsonObject &server) {
                             if (xhttpOf(server, "inbounds").contains(QStringLiteral("scStreamUpServerSecs")))
                                 return QStringLiteral("server still has scStreamUpServerSecs, expected core default");
                             return QString();
                         });
        for (const QString value : { QStringLiteral("http/1.1"), QStringLiteral("h2") }) {
            const bool expectKey = value == QLatin1String("h2");
            cases << vps(QStringLiteral("gen/xhttp/packet-up/tls/%1 + hKeepAlivePeriod 30").arg(value),
                         xhttpC(tls, packetUp),
                         all({ alpn(value), xraytest::xmuxValue(QStringLiteral("hKeepAlivePeriod"), 30) }))
                             .checking([expectKey](const QJsonObject &client, const QJsonObject &) {
                                 const QJsonObject mux =
                                         xhttpOf(client, "outbounds").value(QStringLiteral("xmux")).toObject();
                                 if (mux.contains(QStringLiteral("hKeepAlivePeriod")) != expectKey)
                                     return QStringLiteral("client xmux.hKeepAlivePeriod present=%1, expected %2")
                                             .arg(mux.contains(QStringLiteral("hKeepAlivePeriod")))
                                             .arg(expectKey);
                                 return QString();
                             });
        }

        cases << vps("refused/xPadding 0-100", xhttpC(tls, packetUp), xh("xPaddingBytes", "0-100")).expecting(Expect::Refused);
        cases << vps("refused/scMaxBufferedPosts 200000", xhttpC(tls, packetUp), xh("scMaxBufferedPosts", 200000))
                         .expecting(Expect::Refused);
        cases << vps("refused/mkcp tti 5", xrayexplore::mkcp(), kcp("tti", 5)).expecting(Expect::Refused);

        cases << vps("rule/mkcp/tls/stored fingerprint android", xrayexplore::mkcp())
                         .storedFingerprint(QStringLiteral("android"))
                         .forRule("tls-fingerprint-kcp-ignored");
        cases << vps("rule/xhttp/stream-one/session header", xhttpC(tls),
                     all({ xh("sessionIDPlacement", "header"), xh("sessionIDKey", "X-Sess") }))
                         .forRule("use-session-not-stream-one");
        cases << vps("rule/xhttp/stream-up/seq header", xhttpC(tls, streamUp), xh("seqPlacement", "header"))
                         .forRule("use-seq-packet-up-only");
        cases << vps("rule/xhttp/stream-one/scMaxEachPostBytes 500000", xhttpC(tls),
                     xh("scMaxEachPostBytes", "500000-500000"))
                         .forRule("use-max-each-post-packet-up-only");
        cases << vps("rule/xhttp/stream-one/scMinPostsIntervalMs 10-20", xhttpC(tls), xh("scMinPostsIntervalMs", "10-20"))
                         .forRule("use-min-posts-interval-packet-up-only");
        cases << vps("rule/xhttp/stream-one/scMaxBufferedPosts 50", xhttpC(tls), xh("scMaxBufferedPosts", 50))
                         .forRule("use-max-buffered-posts-packet-up-only");
        cases << vps("rule/xhttp/packet-up/scStreamUpServerSecs 5-10", xhttpC(tls, packetUp),
                     xh("scStreamUpServerSecs", "5-10"))
                         .forRule("use-stream-up-secs-stream-up-only");
        cases << vps("rule/xhttp/packet-up/body + uplinkChunkSize 1024", xhttpC(tls, packetUp),
                     xh("uplinkChunkSize", 1024))
                         .forRule("use-chunk-size-header-cookie-only");
        cases << vps("rule/xhttp/packet-up/body + uplinkDataKey", xhttpC(tls, packetUp), xh("uplinkDataKey", "x_d"))
                         .forRule("use-uplink-data-key-body");
        cases << vps("rule/xhttp/obfs off + xPaddingKey", xhttpC(tls),
                     all({ xh("xPaddingObfsMode", false), xh("xPaddingKey", "x_pad") }))
                         .forRule("use-padding-obfs-fields");
        cases << vps("rule/xhttp/packet-up/gRPC header on", xhttpC(tls, packetUp), xh("noGRPCHeader", false))
                         .forRule("use-nogrpc-stream-only");
        cases << vps("rule/xhttp/packet-up/path placement + sessionIDKey", xhttpC(tls, packetUp),
                     all({ xh("sessionIDPlacement", "path"), xh("sessionIDKey", "x_s") }))
                         .forRule("use-session-seq-key-path");
        cases << vps("rule/xhttp/xPadding cookie + xPaddingHeader", xhttpC(tls),
                     all({ xh("xPaddingPlacement", "cookie"), xh("xPaddingHeader", "X-Pad") }))
                         .forRule("use-padding-key-header-placement");
        cases << vps("rule/xhttp/packet-up/scMinPostsIntervalMs 0-0", xhttpC(tls, packetUp),
                     xh("scMinPostsIntervalMs", "0-0"))
                         .forRule("xhttp-min-posts-interval-zero");
        cases << vps("rule/xhttp/packet-up/header placements + keys", xhttpC(tls, packetUp),
                     all({ xh("sessionIDPlacement", "header"), xh("seqPlacement", "header"),
                           xh("uplinkDataPlacement", "header"), xh("sessionIDKey", "X-S"), xh("seqKey", "X-Q"),
                           xh("uplinkDataKey", "X-D") }))
                         .forRule("xhttp-placement-enum");

        cases << vps("broken/xhttp/packet-up/scMaxEachPostBytes 0-1", xhttpC(tls, packetUp),
                     xh("scMaxEachPostBytes", "0-1"))
                         .expecting(Expect::RefusedAndBroken)
                         .forRule("xhttp-sc-max-each-post-bytes");
        cases << vps("broken/xhttp/packet-up/scMaxEachPostBytes 100-0", xhttpC(tls, packetUp),
                     xh("scMaxEachPostBytes", "100-0"))
                         .expecting(Expect::RefusedAndBroken)
                         .forRule("xhttp-range-reversed-zero");
        cases << vps("stricter/xhttp/stream-up/scStreamUpServerSecs 0-5", xhttpC(tls, streamUp),
                     xh("scStreamUpServerSecs", "0-5"))
                         .expecting(Expect::RefusedButWorks)
                         .forRule("xhttp-stream-up-server-secs-zero");
        cases << vps("stricter/mkcp uplinkCapacity 5000", xrayexplore::mkcp(), kcp("uplinkCapacity", 5000))
                         .expecting(Expect::RefusedButWorks)
                         .forRule("kcp-capacity-overflow");
        cases << vps("stricter/mkcp cwndMultiplier 6000", xrayexplore::mkcp(), kcp("cwndMultiplier", 6000))
                         .expecting(Expect::RefusedButWorks)
                         .forRule("kcp-cwnd-overflow");
        cases << vps("broken/xhttp/packet-up/sessionIDKey with space", xhttpC(tls, packetUp),
                     all({ xh("sessionIDPlacement", "header"), xh("sessionIDKey", "a b") }))
                         .expecting(Expect::RefusedAndBroken);
        cases << vps("broken/xhttp/xPaddingHeader with space", xhttpC(tls),
                     all({ xh("xPaddingPlacement", "header"), xh("xPaddingHeader", "a b") }))
                         .expecting(Expect::RefusedAndBroken);
        cases << vps("broken/xhttp/packet-up/uplinkDataKey with colon", xhttpC(tls, packetUp),
                     all({ xh("uplinkDataPlacement", "header"), xh("uplinkDataKey", "a:b") }))
                         .expecting(Expect::RefusedAndBroken);

        const QList<QPair<QString, Carrier>> realityBases = {
            { "raw-vision", rawC(reality, vision) },
            { "xhttp-stream-one", xhttpC(reality) },
        };
        QStringList storedFingerprints = xraytest::fingerprintOptions() + xraytest::realityFingerprintOptions()
                + xraytest::legacyRealityFingerprints();
        storedFingerprints.removeDuplicates();
        for (const auto &realityBase : realityBases) {
            for (const QString &fingerprint : storedFingerprints) {
                cases << vps(QStringLiteral("fingerprint/reality/%1/'%2'").arg(realityBase.first, fingerprint),
                             realityBase.second)
                                 .storedFingerprint(fingerprint);
            }
        }

        for (const QString &fingerprint :
             { QStringLiteral("hellochrome_120"), QStringLiteral("ios"), QStringLiteral("android") }) {
            cases << vps(QStringLiteral("old-client/raw-vision/%1").arg(fingerprint), rawC(reality, vision))
                             .expecting(Expect::NoTraffic)
                             .withClient(
                                     [fingerprint](QJsonObject &client) {
                                         xraytest::setClientRealityFingerprint(client, fingerprint);
                                     },
                                     fingerprint == QLatin1String("android")
                                             ? QString()
                                             : QStringLiteral("received real certificate"));
        }

        const QList<QPair<QString, Carrier>> tlsBases = {
            { "raw-noflow", rawC(tls) },
            { "xhttp-stream-one", xhttpC(tls) },
        };
        for (const auto &tlsBase : tlsBases) {
            for (const QString &fingerprint : xraytest::fingerprintOptions()) {
                const bool h2WithoutAlpn = tlsBase.first.startsWith(QLatin1String("xhttp"))
                        && (fingerprint == QLatin1String("android") || fingerprint == QLatin1String("360"));
                cases << vps(QStringLiteral("fingerprint/tls/%1/%2").arg(tlsBase.first, fingerprint), tlsBase.second)
                                 .storedFingerprint(fingerprint)
                                 .expecting(h2WithoutAlpn ? Expect::NoTraffic : Expect::Connects);
            }
        }

        if (!qEnvironmentVariableIsSet("VPS_MATRIX_FULL"))
            return cases;

        for (const xraytest::MatrixCase &full : xraytest::matrix()) {
            XrayServerConfig srv = full.srv;
            srv.sni = vpsSite;
            srv.site = vpsSite;
            VpsCase c;
            c.name = QStringLiteral("full/") + full.name;
            c.source = [srv](const QString &port) {
                XrayServerConfig edited = srv;
                edited.port = port;
                return xrayInbound::parse(xrayInbound::templateFromServerConfig(edited));
            };
            c.expect = templateIsValid(c.source(srv.port)) ? Expect::Connects : Expect::Refused;
            cases << c;
        }
        return cases;
    }

    struct LinkCase
    {
        QString name;
        Carrier carrier;
        InboundEdit edit;
        QString fingerprint;
        QStringList required;
        QStringList absent;
    };

    QList<LinkCase> linkCases()
    {
        const QString tls = QStringLiteral("tls");
        const QString reality = QStringLiteral("reality");
        const QString vision = QStringLiteral("xtls-rprx-vision");
        return {
            { "raw/reality/vision",
              rawC(reality, vision),
              {},
              QStringLiteral("chrome"),
              { "security=reality", "flow=xtls-rprx-vision", "fp=chrome", "pbk", "sid" },
              { "type", "alpn", "pcs" } },
            { "raw/tls/vision/firefox",
              rawC(tls, vision),
              {},
              QStringLiteral("firefox"),
              { "security=tls", "flow=xtls-rprx-vision", "fp=firefox", "alpn=h2", "pcs" },
              { "type", "pbk" } },
            { "xhttp/packet-up/tls/host+path",
              xhttpC(tls, QStringLiteral("Packet-up")),
              all({ xh("host", "www.googletagmanager.com"), xh("path", "/e2e") }),
              QStringLiteral("chrome"),
              { "type=xhttp", "security=tls", "host=www.googletagmanager.com", "path=/e2e", "mode=packet-up", "fp",
                "alpn=h2", "extra", "pcs" },
              { "flow" } },
            { "xhttp/stream-one/reality",
              xhttpC(reality),
              {},
              QStringLiteral("chrome"),
              { "type=xhttp", "security=reality", "mode=stream-one", "pbk", "sid", "extra" },
              { "flow", "pcs" } },
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
    struct Deploy
    {
        QString templateText;
        QStringList errors;
        bool refused = false;
        bool applied = true;
        bool touchesServer = false;
        bool fingerprintChanged = false;
        XrayProtocolConfig pending;
    };

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

    QJsonObject storedInbound() const
    {
        const XrayServerConfig stored = storedXray().serverConfig;
        return xrayInbound::parse(stored.inboundTemplate.isEmpty() ? xrayInbound::templateFromServerConfig(stored)
                                                                   : stored.inboundTemplate);
    }

    QString vpsPort() const
    {
        return m_originalSettings.port;
    }

    Deploy prepare(const QJsonObject &inbound, const QString &fingerprint)
    {
        Deploy d;
        d.templateText = xrayInbound::format(inbound);
        for (const XrayInboundIssue &issue : validateXrayInbound(d.templateText).issues) {
            if (issue.level == XrayInboundIssue::Level::Error)
                d.errors << QStringLiteral("line %1: %2").arg(issue.line).arg(issue.message);
        }

        const XrayProtocolConfig stored = storedXray();
        XrayPresetsModel presets(m_coreController->m_appSettingsRepository);
        XraySettingsModel model(&presets);
        model.updateModel(DockerContainer::Xray, stored);
        model.openEditor();
        model.setDraft(d.templateText);
        d.refused = !model.commitDraft();
        if (d.refused) {
            d.pending = stored;
            d.applied = xrayInbound::applyTemplate(d.templateText, d.pending.serverConfig);
            d.touchesServer = !d.pending.serverConfig.hasEqualServerSettings(stored.serverConfig);
            d.pending.clearClientConfig();
        } else {
            d.touchesServer = model.touchesServer();
            d.pending = model.getProtocolConfig();
        }

        const QString effective =
                fingerprint.isNull() ? QString::fromLatin1(protocols::xray::defaultFingerprint) : fingerprint;
        d.fingerprintChanged = stored.serverConfig.fingerprint != effective;
        d.pending.serverConfig.fingerprint = effective;
        if (d.fingerprintChanged)
            d.pending.clearClientConfig();
        return d;
    }

    ErrorCode save(const XrayProtocolConfig &protocolConfig, bool rebuildClient)
    {
        ContainerConfig oldConfig = storedContainer();
        ContainerConfig newConfig = oldConfig;
        newConfig.protocolConfig = protocolConfig;
        // The app rebuilds the client config only together with a server change, while a stored fingerprint
        // reaches the client only through that rebuild. Marking the old template as different forces it.
        if (rebuildClient) {
            if (auto *oldXray = oldConfig.getXrayProtocolConfig())
                oldXray->serverConfig.inboundTemplate = QStringLiteral("{}");
        }
        return m_coreController->m_installController->updateServerConfig(m_serverId, DockerContainer::Xray, oldConfig,
                                                                         newConfig, true);
    }

    ErrorCode save(const Deploy &d)
    {
        return save(d.pending, d.fingerprintChanged);
    }

    ErrorCode deploy(const QJsonObject &inbound, const QString &fingerprint = QString())
    {
        const Deploy d = prepare(inbound, fingerprint);
        if (d.refused) {
            qWarning().noquote() << "deploy refused:" << d.errors.join(QStringLiteral("; "));
            return ErrorCode::InternalError;
        }
        return save(d);
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
                "sudo docker inspect -f 'running={{.State.Running}} restarts={{.RestartCount}}' amnezia-xray 2>&1 || "
                "true\n"
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

        const QString path =
                m_dir.filePath(QStringLiteral("client-%1.json").arg(QUuid::createUuid().toString(QUuid::Id128)));
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

    void verifyTunnel()
    {
        QString why;
        const bool tunnelUp = !egressIpThroughTunnel(why).isEmpty();
        m_lastReason = tunnelUp ? QString() : why;
        QVERIFY2(tunnelUp, qPrintable(why));
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
        const QString serverVersion = runOnServer(QStringLiteral("sudo docker exec %1 xray version")
                                                          .arg(ContainerUtils::containerToString(DockerContainer::Xray)))
                                              .section(QLatin1Char('\n'), 0, 0);
        if (!serverVersion.contains(QStringLiteral("Xray ") + expected + QLatin1Char(' '))) {
            m_exploreSkip =
                    QStringLiteral("server xray is '%1', explore runs only against %2").arg(serverVersion, expected);
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

    TunnelCheck checkInternet(const ClientEdit &clientEdit)
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

    void runExploreCase(const QString &group, const QString &name, const QJsonObject &inbound,
                        const QString &fingerprint, const ClientEdit &clientEdit, bool dedupe)
    {
        const Deploy d = prepare(inbound, fingerprint);
        if (d.refused) {
            writeReportRow({ group, name, "refused by app", "-", "-", "-", "-", "-", "-", "-", "-",
                             d.errors.join(QStringLiteral("; ")) });
            return;
        }

        const bool portChanged = d.pending.serverConfig.port != storedXray().serverConfig.port;
        const bool unsaved = d.touchesServer || d.fingerprintChanged;
        const QString deployKind = portChanged ? QStringLiteral("reinstall")
                : d.touchesServer              ? QStringLiteral("server")
                : d.fingerprintChanged         ? QStringLiteral("fingerprint only, forced rebuild")
                                               : QStringLiteral("nothing to save");

        if (dedupe) {
            QJsonObject client = xraytest::clientJson(d.pending.serverConfig);
            if (clientEdit)
                clientEdit(client);
            const QString key = xrayInbound::normalized(d.templateText) + QLatin1Char('|') + xraytest::compact(client);
            const auto seen = m_exploreSeen.constFind(key);
            if (seen != m_exploreSeen.constEnd()) {
                writeReportRow({ group, name, QStringLiteral("duplicate of ") + seen.value(), deployKind, "-", "-", "-",
                                 "-", "-", "-", "-", "-" });
                return;
            }
            m_exploreSeen.insert(key, name);
        }

        QString saveError = QStringLiteral("-");
        QString saveMs = QStringLiteral("-");
        if (unsaved) {
            QElapsedTimer timer;
            timer.start();
            const ErrorCode error = save(d);
            saveMs = QString::number(timer.elapsed());
            saveError = QString::number(static_cast<int>(error));
            if (error != ErrorCode::NoError) {
                writeReportRow({ group, name, QStringLiteral("server rejected"), deployKind, saveError, saveMs, "-",
                                 "-", "-", "-", maskedAddresses(storedSummary()), "-" });
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
        writeReportRow({ group, name, verdict, deployKind, saveError, saveMs, check.egress, check.gstatic, check.http,
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
        if (m_originalSettings.inboundTemplate.isEmpty())
            m_originalSettings.inboundTemplate = xrayInbound::templateFromServerConfig(m_originalSettings);
        m_ready = true;
    }

    void cleanupTestCase()
    {
        if (m_ready) {
            const ErrorCode restoreError =
                    deploy(xrayInbound::parse(m_originalSettings.inboundTemplate), m_originalSettings.fingerprint);
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

        QJsonObject inbound = c.source(vpsPort());
        if (c.edit)
            c.edit(inbound);
        const Deploy d = prepare(inbound, c.fingerprint);

        qInfo().noquote() << "=====" << c.name << "=====";
        if (!c.rule.isEmpty())
            qInfo().noquote() << "rule:" << c.rule;
        qInfo().noquote() << "settings:" << settingsSummary(d.pending.serverConfig);
        qInfo().noquote() << "validation:"
                          << (d.errors.isEmpty() ? QStringLiteral("ok") : d.errors.join(QStringLiteral("; ")));

        if (c.expect == Expect::Refused) {
            m_lastReason = QStringLiteral("the editor must refuse this before anything reaches the server");
            QVERIFY2(d.refused, qPrintable(m_lastReason));
            return;
        }
        if (c.expect == Expect::RefusedAndBroken || c.expect == Expect::RefusedButWorks) {
            m_lastReason = QStringLiteral("the editor must refuse this combination");
            QVERIFY2(d.refused, qPrintable(m_lastReason));
            m_lastReason = QStringLiteral("the refused template cannot be applied to the stored settings");
            QVERIFY2(d.applied, qPrintable(m_lastReason));
        }
        if (c.expect == Expect::Connects || c.expect == Expect::NoTraffic) {
            m_lastReason = QStringLiteral("the editor refused it: ") + d.errors.join("; ");
            QVERIFY2(!d.refused, qPrintable(m_lastReason));
        }

        QElapsedTimer timer;
        timer.start();
        const ErrorCode saveError = save(d);
        const qint64 saveMs = timer.elapsed();
        const QJsonObject client = storedClientJson();
        const QJsonObject server = serverJson();
        const QString state = serverState();
        logSaved(client, server, saveError, saveMs, state);

        QString why;
        const QString ip = saveError == ErrorCode::NoError ? egressIpThroughTunnel(why, c.clientEdit) : QString();
        qInfo().noquote() << "traffic:"
                          << (ip.isEmpty() ? QStringLiteral("no traffic: ") + why : QStringLiteral("egress ") + ip);

        if (c.expect == Expect::RefusedAndBroken) {
            m_lastReason = QStringLiteral("the editor refuses a working combination: traffic left through ") + ip;
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

        QJsonObject inbound = xrayexplore::carrierInbound(c.carrier, vpsPort(), vpsSite);
        if (c.edit)
            c.edit(inbound);
        const Deploy d = prepare(inbound, c.fingerprint);
        qInfo().noquote() << "=====" << QStringLiteral("link/") + c.name << "=====";
        qInfo().noquote() << "settings:" << settingsSummary(d.pending.serverConfig);
        qInfo().noquote() << "validation:"
                          << (d.errors.isEmpty() ? QStringLiteral("ok") : d.errors.join(QStringLiteral("; ")));
        QVERIFY2(!d.refused, qPrintable(d.errors.join("; ")));

        QElapsedTimer timer;
        timer.start();
        const ErrorCode saveError = save(d);
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
        const QString shownLink =
                url.userName().isEmpty() ? link : QString(link).replace(url.userName(), QStringLiteral("***"));
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
            QVERIFY2(!query.hasQueryItem(key),
                     qPrintable(QStringLiteral("link must not have %1: %2").arg(key, shownLink)));

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
        qInfo().noquote() << "traffic:"
                          << (ip.isEmpty() ? QStringLiteral("no traffic: ") + why : QStringLiteral("egress ") + ip);
        revokeExportedClient(url.userName());
        if (ip.isEmpty())
            QFAIL(qPrintable(why + QStringLiteral("\nserver: ") + serverState()));
        QVERIFY2(m_expectedIps.isEmpty() || m_expectedIps.contains(ip),
                 qPrintable(QStringLiteral("traffic left through %1, expected %2")
                                    .arg(ip, m_expectedIps.join(QStringLiteral(", ")))));
    }

    void portChangeRecreatesContainer()
    {
        const Carrier realityVision = rawC(QStringLiteral("reality"), QStringLiteral("xtls-rprx-vision"));
        QCOMPARE(deploy(xrayexplore::carrierInbound(realityVision, vpsPort(), vpsSite)), ErrorCode::NoError);

        const QString originalPort = storedXray().serverConfig.port;
        const int newPort = originalPort.toInt() == 8443 ? 8444 : 8443;
        QJsonObject inbound = storedInbound();
        inbound[QStringLiteral("port")] = newPort;
        QCOMPARE(deploy(inbound), ErrorCode::NoError);
        QCOMPARE(storedXray().serverConfig.port, QString::number(newPort));
        verifyTunnel();

        inbound = storedInbound();
        inbound[QStringLiteral("port")] = originalPort.toInt();
        QCOMPARE(deploy(inbound), ErrorCode::NoError);
    }

    void serverSettingChangeKeepsContainer()
    {
        QCOMPARE(deploy(xrayexplore::carrierInbound(rawC(QStringLiteral("reality"), QStringLiteral("xtls-rprx-vision")),
                                                    vpsPort(), vpsSite)),
                 ErrorCode::NoError);
        const QString before = xrayContainerId();
        QVERIFY2(!before.isEmpty(), "cannot read the xray container id over ssh");

        const Deploy d =
                prepare(xrayexplore::carrierInbound(xhttpC(QStringLiteral("reality")), vpsPort(), vpsSite), QString());
        QVERIFY2(!d.refused, qPrintable(d.errors.join("; ")));
        QVERIFY(d.touchesServer);
        QCOMPARE(save(d), ErrorCode::NoError);
        m_lastReason = QStringLiteral("a server setting change without a port change recreated the container");
        QVERIFY2(xrayContainerId() == before, qPrintable(m_lastReason));
        verifyTunnel();
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
        QJsonObject inbound = xrayexplore::carrierInbound(c.carrier, vpsPort(), vpsSite);
        if (c.edit)
            c.edit(inbound);
        runExploreCase(c.group, c.name, inbound, c.fingerprint, c.clientEdit, true);
    }

    void exploreTransitions()
    {
        if (!exploreEnabled())
            QSKIP("Set VPS_EXPLORE=1 to run the explore transitions");
        prepareExplore();
        if (!m_exploreSkip.isEmpty())
            QSKIP(qPrintable(m_exploreSkip));

        const QString originalPort = vpsPort();
        const QString otherPort = originalPort == QLatin1String("2053") ? QStringLiteral("2054") : QStringLiteral("2053");
        const Carrier realityVision = rawC(QStringLiteral("reality"), QStringLiteral("xtls-rprx-vision"));
        const Carrier tlsVision = rawC(QStringLiteral("tls"), QStringLiteral("xtls-rprx-vision"));
        const Carrier tlsRaw = rawC(QStringLiteral("tls"));
        const Carrier kcpTls = xrayexplore::mkcp();
        const auto network = [](const QString &value) { return xraytest::streamValue(QStringLiteral("network"), value); };
        const auto port = [](const QString &value) { return xraytest::put({ QStringLiteral("port") }, value.toInt()); };

        struct Step
        {
            QString name;
            Carrier carrier;
            InboundEdit edit;
            QString fingerprint;
        };
        const auto fresh = [](const QString &name, const Carrier &carrier, const InboundEdit &edit = {},
                              const QString &fingerprint = QString()) {
            return Step { name, carrier, edit, fingerprint };
        };
        const auto change = [](const QString &name, const InboundEdit &edit, const QString &fingerprint = QString()) {
            return Step { name, Carrier {}, edit, fingerprint };
        };

        const QList<QPair<QString, QList<Step>>> chains = {
            { "reality-kcp-reality",
              { fresh("raw/reality/noflow", rawC(QStringLiteral("reality"))), change("network=kcp", network("kcp")),
                fresh("mkcp/tls", kcpTls), fresh("raw/reality/vision", realityVision) } },
            { "kcp-values-survive",
              { fresh("mkcp tti=20 mtu=1200 maxSendingWindow=1200", kcpTls,
                      all({ kcp("tti", 20), kcp("mtu", 1200), kcp("maxSendingWindow", 1200) })),
                change("network=raw", network("raw")), change("network=kcp", network("kcp")) } },
            { "kcp-edit-on-raw", { fresh("raw/tls", tlsRaw), change("kcpSettings.tti=20 while raw", kcp("tti", 20)) } },
            { "xhttp-kcp-xhttp",
              { fresh("xhttp/tls/Packet-up path=/keep", xhttpC(QStringLiteral("tls"), QStringLiteral("Packet-up")),
                      xh("path", "/keep")),
                change("network=kcp", network("kcp")), change("network=xhttp", network("xhttp")) } },
            { "tls-reality-tls",
              { fresh("raw/tls/vision", tlsVision), fresh("raw/reality/vision", realityVision),
                fresh("raw/tls/vision", tlsVision) } },
            { "port-and-kcp",
              { fresh("mkcp/tls", kcpTls),
                change("port=" + otherPort + " tti=30", all({ port(otherPort), kcp("tti", 30) })),
                change("port=" + originalPort, port(originalPort)) } },
            { "mtu-raise-with-msw",
              { fresh("mkcp mtu=1350 maxSendingWindow=1400", kcpTls,
                      all({ kcp("mtu", 1350), kcp("maxSendingWindow", 1400) })),
                change("mtu=1460", kcp("mtu", 1460)) } },
            { "equivalent-edits",
              { fresh("mkcp tti=50", kcpTls, kcp("tti", 50)),
                change("tti removed", kcp("tti", QJsonValue(QJsonValue::Undefined))), change("tti=5", kcp("tti", 5)),
                change("tti=\"05\"", kcp("tti", "05")) } },
            { "fingerprint-only",
              { fresh("raw/reality/vision stored fingerprint=chrome", realityVision, {}, QStringLiteral("chrome")),
                change("stored fingerprint=firefox", {}, QStringLiteral("firefox")),
                change("stored fingerprint=hellochrome_120", {}, QStringLiteral("hellochrome_120")) } },
        };

        for (const auto &chain : chains) {
            for (int i = 0; i < chain.second.size(); ++i) {
                const Step &step = chain.second.at(i);
                QJsonObject inbound = step.carrier.transport.isEmpty()
                        ? storedInbound()
                        : xrayexplore::carrierInbound(step.carrier, vpsPort(), vpsSite);
                if (step.edit)
                    step.edit(inbound);
                runExploreCase(QStringLiteral("transition"),
                               QStringLiteral("transition/%1 #%2 %3").arg(chain.first).arg(i + 1).arg(step.name),
                               inbound, step.fingerprint, {}, false);
            }
        }
    }

    void rescanKeepsTemplateAndFingerprint()
    {
        const Carrier realityVision = rawC(QStringLiteral("reality"), QStringLiteral("xtls-rprx-vision"));
        QCOMPARE(deploy(xrayexplore::carrierInbound(realityVision, vpsPort(), vpsSite), QStringLiteral("safari")),
                 ErrorCode::NoError);
        const QString templateBefore = storedXray().serverConfig.inboundTemplate;

        QCOMPARE(m_coreController->m_installController->scanServerForInstalledContainers(m_serverId), ErrorCode::NoError);

        const XrayServerConfig after = storedXray().serverConfig;
        QCOMPARE(after.fingerprint, QStringLiteral("safari"));
        QCOMPARE(xrayInbound::normalized(after.inboundTemplate), xrayInbound::normalized(templateBefore));
        verifyTunnel();
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
