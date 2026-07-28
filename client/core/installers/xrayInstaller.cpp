#include "xrayInstaller.h"

#include <QJsonDocument>
#include <QJsonArray>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "logger.h"

namespace
{
    Logger logger("XrayInstaller");

    // Xray expects uTLS preset names (chrome, firefox, …). Old Amnezia/server templates used "Mozilla/5.0".
    QString normalizeXrayFingerprint(const QString &fp)
    {
        if (fp.isEmpty() || fp.contains(QLatin1String("Mozilla/5.0"), Qt::CaseInsensitive)) {
            return QString::fromLatin1(protocols::xray::defaultFingerprint);
        }
        return fp;
    }
}

using namespace amnezia;
using namespace ProtocolUtils;

XrayInstaller::XrayInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

ErrorCode XrayInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                     SshSession* sshSession, ContainerConfig &config)
{
    ErrorCode errorCode = ErrorCode::NoError;
    
    QString currentConfig = sshSession->getTextFileFromContainer(
            container, credentials, amnezia::protocols::xray::serverConfigPath, errorCode);

    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonDocument doc = QJsonDocument::fromJson(currentConfig.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        logger.error() << "Failed to parse server config JSON";
        return ErrorCode::InternalError;
    }
    QJsonObject serverConfig = doc.object();

    if (!serverConfig.contains(protocols::xray::inbounds)) {
        logger.error() << "Server config missing 'inbounds' field";
        return ErrorCode::InternalError;
    }

    QJsonArray inbounds = serverConfig[protocols::xray::inbounds].toArray();
    if (inbounds.isEmpty()) {
        logger.error() << "Server config has empty 'inbounds' array";
        return ErrorCode::InternalError;
    }

    QJsonObject inbound = inbounds[0].toObject();
    if (!inbound.contains(protocols::xray::streamSettings)) {
        logger.error() << "Inbound missing 'streamSettings' field";
        return ErrorCode::InternalError;
    }

    QJsonObject streamSettings = inbound[protocols::xray::streamSettings].toObject();
    auto *xrayConfig = config.getXrayProtocolConfig();
    if (!xrayConfig) {
        logger.error() << "No XrayProtocolConfig in ContainerConfig";
        return ErrorCode::InternalError;
    }

    XrayServerConfig &srv = xrayConfig->serverConfig;

    // ── Port ─────────────────────────────────────────────────────────
    if (inbound.contains(protocols::xray::port)) {
        srv.port = QString::number(inbound[protocols::xray::port].toInt());
    }

    // ── Network (transport) ───────────────────────────────────────────
    QString networkVal = streamSettings.value(protocols::xray::network).toString("tcp");
    if (networkVal == "xhttp") {
        srv.transport = "xhttp";
    } else if (networkVal == "kcp") {
        srv.transport = "mkcp";
    } else {
        srv.transport = "raw";
    }

    // ── Security ──────────────────────────────────────────────────────
    srv.security = streamSettings.value(protocols::xray::security).toString("reality");

    // ── Reality settings ──────────────────────────────────────────────
    if (srv.security == "reality") {
        QJsonObject rs = streamSettings.value(protocols::xray::realitySettings).toObject();

        // serverNames array → site + sni
        if (rs.contains(protocols::xray::serverNames)) {
            QString sniVal = rs[protocols::xray::serverNames].toArray().first().toString();
            srv.sni = sniVal;
            srv.site = sniVal;
        } else if (rs.contains(protocols::xray::serverName)) {
            srv.sni = rs[protocols::xray::serverName].toString();
            srv.site = srv.sni;
        }

        srv.fingerprint = normalizeXrayFingerprint(rs.value(protocols::xray::fingerprint).toString());
    }

    // ── TLS settings ──────────────────────────────────────────────────
    if (srv.security == "tls") {
        QJsonObject tls = streamSettings.value("tlsSettings").toObject();
        srv.sni = tls.value(protocols::xray::serverName).toString();
        srv.fingerprint = normalizeXrayFingerprint(tls.value(protocols::xray::fingerprint).toString());

        QJsonArray alpnArr = tls.value("alpn").toArray();
        QStringList alpnList;
        for (const QJsonValue &v : alpnArr) {
            alpnList << v.toString();
        }
        srv.alpn = alpnList.join(",");
    }

    // ── Flow (from users array) ───────────────────────────────────────
    if (inbound.contains(protocols::xray::settings)) {
        QJsonObject s = inbound[protocols::xray::settings].toObject();
        QJsonArray clientsArr = s.value(protocols::xray::clients).toArray();
        if (!clientsArr.isEmpty()) {
            srv.flow = clientsArr[0].toObject().value(protocols::xray::flow).toString();
        }
    }

    // ── XHTTP settings (Xray-core SplitHTTPConfig + legacy Amnezia keys) ──
    if (srv.transport == "xhttp") {
        QJsonObject xhttpObj = streamSettings.value("xhttpSettings").toObject();
        {
            const QString m = xhttpObj.value("mode").toString();
            if (m.isEmpty() || m == QLatin1String("auto"))
                srv.xhttp.mode = QStringLiteral("Auto");
            else if (m == QLatin1String("packet-up"))
                srv.xhttp.mode = QStringLiteral("Packet-up");
            else if (m == QLatin1String("stream-up"))
                srv.xhttp.mode = QStringLiteral("Stream-up");
            else if (m == QLatin1String("stream-one"))
                srv.xhttp.mode = QStringLiteral("Stream-one");
            else
                srv.xhttp.mode = m;
        }

        srv.xhttp.host = xhttpObj.value("host").toString();
        srv.xhttp.path = xhttpObj.value("path").toString();

        {
            const QJsonObject hdrs = xhttpObj.value("headers").toObject();
            if (hdrs.contains(QLatin1String("Host")) || !hdrs.isEmpty())
                srv.xhttp.headersTemplate = QStringLiteral("HTTP");
        }

        if (xhttpObj.contains(QLatin1String("uplinkHTTPMethod")))
            srv.xhttp.uplinkMethod = xhttpObj.value("uplinkHTTPMethod").toString();
        else
            srv.xhttp.uplinkMethod = xhttpObj.value("method").toString();

        srv.xhttp.disableGrpc = xhttpObj.value("noGRPCHeader").toBool(true);
        srv.xhttp.disableSse = xhttpObj.value("noSSEHeader").toBool(true);

        auto sessionSeqUi = [](const QString &core) -> QString {
            if (core.isEmpty() || core == QLatin1String("path"))
                return QStringLiteral("Path");
            if (core == QLatin1String("cookie"))
                return QStringLiteral("Cookie");
            if (core == QLatin1String("header"))
                return QStringLiteral("Header");
            if (core == QLatin1String("query"))
                return QStringLiteral("Query");
            return core;
        };
        QString sess = xhttpObj.value("sessionPlacement").toString();
        if (sess.isEmpty())
            sess = xhttpObj.value("scSessionPlacement").toString();
        srv.xhttp.sessionPlacement = sessionSeqUi(sess);

        QString seq = xhttpObj.value("seqPlacement").toString();
        if (seq.isEmpty())
            seq = xhttpObj.value("scSeqPlacement").toString();
        srv.xhttp.seqPlacement = sessionSeqUi(seq);

        auto uplinkDataUi = [](const QString &core) -> QString {
            if (core.isEmpty() || core == QLatin1String("body"))
                return QStringLiteral("Body");
            if (core == QLatin1String("auto"))
                return QStringLiteral("Auto");
            if (core == QLatin1String("header"))
                return QStringLiteral("Header");
            if (core == QLatin1String("cookie"))
                return QStringLiteral("Cookie");
            return core;
        };
        QString udata = xhttpObj.value("uplinkDataPlacement").toString();
        if (udata.isEmpty())
            udata = xhttpObj.value("scUplinkDataPlacement").toString();
        srv.xhttp.uplinkDataPlacement = uplinkDataUi(udata);

        srv.xhttp.sessionKey = xhttpObj.value("sessionKey").toString();
        srv.xhttp.seqKey = xhttpObj.value("seqKey").toString();
        srv.xhttp.uplinkDataKey = xhttpObj.value("uplinkDataKey").toString();

        if (xhttpObj.contains(QLatin1String("uplinkChunkSize"))) {
            QJsonObject uc = xhttpObj.value("uplinkChunkSize").toObject();
            if (!uc.isEmpty())
                srv.xhttp.uplinkChunkSize = QString::number(uc.value("from").toInt());
        } else if (xhttpObj.contains(QLatin1String("xhttpUplinkChunkSize"))) {
            srv.xhttp.uplinkChunkSize = QString::number(xhttpObj.value("xhttpUplinkChunkSize").toInt());
        }
        if (xhttpObj.contains(QLatin1String("scMaxBufferedPosts"))) {
            srv.xhttp.scMaxBufferedPosts = QString::number(xhttpObj.value("scMaxBufferedPosts").toVariant().toLongLong());
        }

        auto readRange = [&](const char *key, QString &minOut, QString &maxOut) {
            QJsonObject r = xhttpObj.value(QLatin1String(key)).toObject();
            if (!r.isEmpty()) {
                minOut = QString::number(r.value("from").toInt());
                maxOut = QString::number(r.value("to").toInt());
            }
        };
        readRange("scMaxEachPostBytes", srv.xhttp.scMaxEachPostBytesMin, srv.xhttp.scMaxEachPostBytesMax);
        readRange("scMinPostsIntervalMs", srv.xhttp.scMinPostsIntervalMsMin, srv.xhttp.scMinPostsIntervalMsMax);
        readRange("scStreamUpServerSecs", srv.xhttp.scStreamUpServerSecsMin, srv.xhttp.scStreamUpServerSecsMax);

        auto loadPaddingFromObject = [&](const QJsonObject &pad) {
            if (pad.contains(QLatin1String("xPaddingObfsMode")))
                srv.xhttp.xPadding.obfsMode = pad.value("xPaddingObfsMode").toBool(true);
            srv.xhttp.xPadding.key = pad.value("xPaddingKey").toString();
            srv.xhttp.xPadding.header = pad.value("xPaddingHeader").toString();
            srv.xhttp.xPadding.placement = pad.value("xPaddingPlacement").toString();
            srv.xhttp.xPadding.method = pad.value("xPaddingMethod").toString();
            QJsonObject bytesRange = pad.value("xPaddingBytes").toObject();
            if (!bytesRange.isEmpty()) {
                srv.xhttp.xPadding.bytesMin = QString::number(bytesRange.value("from").toInt());
                srv.xhttp.xPadding.bytesMax = QString::number(bytesRange.value("to").toInt());
            }
            QString pl = srv.xhttp.xPadding.placement.toLower();
            if (pl == QLatin1String("cookie"))
                srv.xhttp.xPadding.placement = QStringLiteral("Cookie");
            else if (pl == QLatin1String("header"))
                srv.xhttp.xPadding.placement = QStringLiteral("Header");
            else if (pl == QLatin1String("query"))
                srv.xhttp.xPadding.placement = QStringLiteral("Query");
            else if (pl == QLatin1String("queryinheader"))
                srv.xhttp.xPadding.placement = QStringLiteral("Query in header");
            QString met = srv.xhttp.xPadding.method.toLower();
            if (met == QLatin1String("repeat-x"))
                srv.xhttp.xPadding.method = QStringLiteral("Repeat-x");
            else if (met == QLatin1String("tokenish"))
                srv.xhttp.xPadding.method = QStringLiteral("Tokenish");
        };
        if (xhttpObj.contains(QLatin1String("xPaddingObfsMode")) || xhttpObj.contains(QLatin1String("xPaddingKey"))
            || !xhttpObj.value("xPaddingBytes").toObject().isEmpty()) {
            loadPaddingFromObject(xhttpObj);
        } else if (xhttpObj.contains(QLatin1String("xPadding")) && xhttpObj.value("xPadding").isObject()) {
            const QJsonObject nested = xhttpObj.value("xPadding").toObject();
            if (!nested.isEmpty()) {
                loadPaddingFromObject(nested);
                if (!nested.contains(QLatin1String("xPaddingObfsMode")))
                    srv.xhttp.xPadding.obfsMode = true;
            }
        }

        if (xhttpObj.contains(QLatin1String("xmux"))) {
            QJsonObject mux = xhttpObj.value("xmux").toObject();
            srv.xhttp.xmux.enabled = true;

            auto readMuxRange = [&](const char *key, QString &minOut, QString &maxOut) {
                QJsonObject r = mux.value(QLatin1String(key)).toObject();
                if (!r.isEmpty()) {
                    minOut = QString::number(r.value("from").toInt());
                    maxOut = QString::number(r.value("to").toInt());
                }
            };
            readMuxRange("maxConcurrency", srv.xhttp.xmux.maxConcurrencyMin, srv.xhttp.xmux.maxConcurrencyMax);
            readMuxRange("maxConnections", srv.xhttp.xmux.maxConnectionsMin, srv.xhttp.xmux.maxConnectionsMax);
            readMuxRange("cMaxReuseTimes", srv.xhttp.xmux.cMaxReuseTimesMin, srv.xhttp.xmux.cMaxReuseTimesMax);
            readMuxRange("hMaxRequestTimes", srv.xhttp.xmux.hMaxRequestTimesMin, srv.xhttp.xmux.hMaxRequestTimesMax);
            readMuxRange("hMaxReusableSecs", srv.xhttp.xmux.hMaxReusableSecsMin, srv.xhttp.xmux.hMaxReusableSecsMax);

            if (mux.contains(QLatin1String("hKeepAlivePeriod")))
                srv.xhttp.xmux.hKeepAlivePeriod = QString::number(mux.value("hKeepAlivePeriod").toVariant().toLongLong());
        }
    }

    // ── mKCP settings ─────────────────────────────────────────────────
    if (srv.transport == "mkcp") {
        QJsonObject kcp = streamSettings.value("kcpSettings").toObject();
        if (kcp.contains("tti")) {
            srv.mkcp.tti = QString::number(kcp["tti"].toInt());
        }
        if (kcp.contains("uplinkCapacity")) {
            srv.mkcp.uplinkCapacity = QString::number(kcp["uplinkCapacity"].toInt());
        }
        if (kcp.contains("downlinkCapacity")) {
            srv.mkcp.downlinkCapacity = QString::number(kcp["downlinkCapacity"].toInt());
        }
        if (kcp.contains("readBufferSize")) {
            srv.mkcp.readBufferSize = QString::number(kcp["readBufferSize"].toInt());
        }
        if (kcp.contains("writeBufferSize")) {
            srv.mkcp.writeBufferSize = QString::number(kcp["writeBufferSize"].toInt());
        }
        srv.mkcp.congestion = kcp.value("congestion").toBool(true);
    }

    return ErrorCode::NoError;
}

