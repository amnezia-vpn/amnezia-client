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

namespace {
    Logger logger("XrayInstaller");
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

        srv.fingerprint = rs.value(protocols::xray::fingerprint).toString("Mozilla/5.0");
    }

    // ── TLS settings ──────────────────────────────────────────────────
    if (srv.security == "tls") {
        QJsonObject tls = streamSettings.value("tlsSettings").toObject();
        srv.sni = tls.value(protocols::xray::serverName).toString();
        srv.fingerprint = tls.value(protocols::xray::fingerprint).toString("Mozilla/5.0");

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

    // ── XHTTP settings ────────────────────────────────────────────────
    if (srv.transport == "xhttp") {
        QJsonObject xhttpObj = streamSettings.value("xhttpSettings").toObject();
        srv.xhttp.mode = xhttpObj.value("mode").toString("Auto");
        srv.xhttp.host = xhttpObj.value("host").toString();
        srv.xhttp.path = xhttpObj.value("path").toString();
        srv.xhttp.uplinkMethod = xhttpObj.value("method").toString("POST");
        srv.xhttp.disableGrpc = xhttpObj.value("noGRPCHeader").toBool(true);
        srv.xhttp.disableSse = xhttpObj.value("noSSEHeader").toBool(true);

        srv.xhttp.sessionPlacement = xhttpObj.value("scSessionPlacement").toString("Path");
        srv.xhttp.seqPlacement = xhttpObj.value("scSeqPlacement").toString("Path");
        srv.xhttp.uplinkDataPlacement = xhttpObj.value("scUplinkDataPlacement").toString("Body");

        if (xhttpObj.contains("xhttpUplinkChunkSize")) {
            srv.xhttp.uplinkChunkSize = QString::number(xhttpObj["xhttpUplinkChunkSize"].toInt());
        }
        if (xhttpObj.contains("scMaxBufferedPosts")) {
            srv.xhttp.scMaxBufferedPosts = QString::number(xhttpObj["scMaxBufferedPosts"].toInt());
        }

        auto readRange = [&](const char *key, QString &minOut, QString &maxOut) {
            QJsonObject r = xhttpObj.value(key).toObject();
            if (!r.isEmpty()) {
                minOut = QString::number(r.value("from").toInt());
                maxOut = QString::number(r.value("to").toInt());
            }
        };
        readRange("scMaxEachPostBytes", srv.xhttp.scMaxEachPostBytesMin, srv.xhttp.scMaxEachPostBytesMax);
        readRange("scMinPostsIntervalMs", srv.xhttp.scMinPostsIntervalMsMin, srv.xhttp.scMinPostsIntervalMsMax);
        readRange("scStreamUpServerSecs", srv.xhttp.scStreamUpServerSecsMin, srv.xhttp.scStreamUpServerSecsMax);

        // xPadding
        if (xhttpObj.contains("xPadding")) {
            QJsonObject pad = xhttpObj["xPadding"].toObject();
            srv.xhttp.xPadding.obfsMode = true;
            srv.xhttp.xPadding.key = pad.value("xPaddingKey").toString();
            srv.xhttp.xPadding.header = pad.value("xPaddingHeader").toString();
            srv.xhttp.xPadding.placement = pad.value("xPaddingPlacement").toString("Cookie");
            srv.xhttp.xPadding.method = pad.value("xPaddingMethod").toString("Repeat-x");
            QJsonObject bytesRange = pad.value("xPaddingBytes").toObject();
            if (!bytesRange.isEmpty()) {
                srv.xhttp.xPadding.bytesMin = QString::number(bytesRange.value("from").toInt());
                srv.xhttp.xPadding.bytesMax = QString::number(bytesRange.value("to").toInt());
            }
        }

        // xmux
        if (xhttpObj.contains("xmux")) {
            QJsonObject mux = xhttpObj["xmux"].toObject();
            srv.xhttp.xmux.enabled = mux.value("enabled").toBool(true);

            auto readMuxRange = [&](const char *key, QString &minOut, QString &maxOut) {
                QJsonObject r = mux.value(key).toObject();
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

            if (mux.contains("hKeepAlivePeriod"))
                srv.xhttp.xmux.hKeepAlivePeriod = QString::number(mux["hKeepAlivePeriod"].toInt());
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

