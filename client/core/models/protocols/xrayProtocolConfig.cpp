#include "xrayProtocolConfig.h"

#include <QJsonDocument>
#include <QJsonArray>

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;
using namespace ProtocolUtils;

namespace amnezia
{
QJsonObject XrayXPaddingConfig::toJson() const
{
    QJsonObject obj;
    if (!bytesMin.isEmpty())   obj[configKey::xPaddingBytesMin]  = bytesMin;
    if (!bytesMax.isEmpty())   obj[configKey::xPaddingBytesMax]  = bytesMax;
    obj[configKey::xPaddingObfsMode]  = obfsMode;
    if (!key.isEmpty())        obj[configKey::xPaddingKey]       = key;
    if (!header.isEmpty())     obj[configKey::xPaddingHeader]    = header;
    if (!placement.isEmpty())  obj[configKey::xPaddingPlacement] = placement;
    if (!method.isEmpty())     obj[configKey::xPaddingMethod]    = method;
    return obj;
}

XrayXPaddingConfig XrayXPaddingConfig::fromJson(const QJsonObject &json)
{
    XrayXPaddingConfig c;
    c.bytesMin  = json.value(configKey::xPaddingBytesMin).toString();
    c.bytesMax  = json.value(configKey::xPaddingBytesMax).toString();
    c.obfsMode  = json.value(configKey::xPaddingObfsMode).toBool(true);
    c.key       = json.value(configKey::xPaddingKey).toString(protocols::xray::defaultSite);
    c.header    = json.value(configKey::xPaddingHeader).toString();
    c.placement = json.value(configKey::xPaddingPlacement).toString(protocols::xray::defaultXPaddingPlacement);
    c.method    = json.value(configKey::xPaddingMethod).toString(protocols::xray::defaultXPaddingMethod);
    return c;
}

QJsonObject XrayXmuxConfig::toJson() const
{
    QJsonObject obj;
    obj[configKey::xmuxEnabled] = enabled;
    if (!maxConcurrencyMin.isEmpty())   obj[configKey::xmuxMaxConcurrencyMin]   = maxConcurrencyMin;
    if (!maxConcurrencyMax.isEmpty())   obj[configKey::xmuxMaxConcurrencyMax]   = maxConcurrencyMax;
    if (!maxConnectionsMin.isEmpty())   obj[configKey::xmuxMaxConnectionsMin]   = maxConnectionsMin;
    if (!maxConnectionsMax.isEmpty())   obj[configKey::xmuxMaxConnectionsMax]   = maxConnectionsMax;
    if (!cMaxReuseTimesMin.isEmpty())   obj[configKey::xmuxCMaxReuseTimesMin]   = cMaxReuseTimesMin;
    if (!cMaxReuseTimesMax.isEmpty())   obj[configKey::xmuxCMaxReuseTimesMax]   = cMaxReuseTimesMax;
    if (!hMaxRequestTimesMin.isEmpty()) obj[configKey::xmuxHMaxRequestTimesMin] = hMaxRequestTimesMin;
    if (!hMaxRequestTimesMax.isEmpty()) obj[configKey::xmuxHMaxRequestTimesMax] = hMaxRequestTimesMax;
    if (!hMaxReusableSecsMin.isEmpty()) obj[configKey::xmuxHMaxReusableSecsMin] = hMaxReusableSecsMin;
    if (!hMaxReusableSecsMax.isEmpty()) obj[configKey::xmuxHMaxReusableSecsMax] = hMaxReusableSecsMax;
    if (!hKeepAlivePeriod.isEmpty())    obj[configKey::xmuxHKeepAlivePeriod]    = hKeepAlivePeriod;
    return obj;
}

XrayXmuxConfig XrayXmuxConfig::fromJson(const QJsonObject &json)
{
    XrayXmuxConfig c;
    c.enabled             = json.value(configKey::xmuxEnabled).toBool(true);
    c.maxConcurrencyMin   = json.value(configKey::xmuxMaxConcurrencyMin).toString("0");
    c.maxConcurrencyMax   = json.value(configKey::xmuxMaxConcurrencyMax).toString("0");
    c.maxConnectionsMin   = json.value(configKey::xmuxMaxConnectionsMin).toString("0");
    c.maxConnectionsMax   = json.value(configKey::xmuxMaxConnectionsMax).toString("0");
    c.cMaxReuseTimesMin   = json.value(configKey::xmuxCMaxReuseTimesMin).toString("0");
    c.cMaxReuseTimesMax   = json.value(configKey::xmuxCMaxReuseTimesMax).toString("0");
    c.hMaxRequestTimesMin = json.value(configKey::xmuxHMaxRequestTimesMin).toString("0");
    c.hMaxRequestTimesMax = json.value(configKey::xmuxHMaxRequestTimesMax).toString("0");
    c.hMaxReusableSecsMin = json.value(configKey::xmuxHMaxReusableSecsMin).toString("0");
    c.hMaxReusableSecsMax = json.value(configKey::xmuxHMaxReusableSecsMax).toString("0");
    c.hKeepAlivePeriod    = json.value(configKey::xmuxHKeepAlivePeriod).toString();
    return c;
}

QJsonObject XrayXhttpConfig::toJson() const
{
    QJsonObject obj;
    if (!mode.isEmpty())            obj[configKey::xhttpMode]            = mode;
    if (!host.isEmpty())            obj[configKey::xhttpHost]            = host;
    if (!path.isEmpty())            obj[configKey::xhttpPath]            = path;
    if (!headersTemplate.isEmpty()) obj[configKey::xhttpHeadersTemplate] = headersTemplate;
    if (!uplinkMethod.isEmpty())    obj[configKey::xhttpUplinkMethod]    = uplinkMethod;
    obj[configKey::xhttpDisableGrpc] = disableGrpc;
    obj[configKey::xhttpDisableSse]  = disableSse;

    if (!sessionPlacement.isEmpty())    obj[configKey::xhttpSessionPlacement]    = sessionPlacement;
    if (!sessionKey.isEmpty())          obj[configKey::xhttpSessionKey]          = sessionKey;
    if (!seqPlacement.isEmpty())        obj[configKey::xhttpSeqPlacement]        = seqPlacement;
    if (!seqKey.isEmpty())              obj[configKey::xhttpSeqKey]              = seqKey;
    if (!uplinkDataPlacement.isEmpty()) obj[configKey::xhttpUplinkDataPlacement] = uplinkDataPlacement;
    if (!uplinkDataKey.isEmpty())       obj[configKey::xhttpUplinkDataKey]       = uplinkDataKey;

    if (!uplinkChunkSize.isEmpty())         obj[configKey::xhttpUplinkChunkSize]         = uplinkChunkSize;
    if (!scMaxBufferedPosts.isEmpty())      obj[configKey::xhttpScMaxBufferedPosts]      = scMaxBufferedPosts;
    if (!scMaxEachPostBytesMin.isEmpty())   obj[configKey::xhttpScMaxEachPostBytesMin]   = scMaxEachPostBytesMin;
    if (!scMaxEachPostBytesMax.isEmpty())   obj[configKey::xhttpScMaxEachPostBytesMax]   = scMaxEachPostBytesMax;
    if (!scMinPostsIntervalMsMin.isEmpty()) obj[configKey::xhttpScMinPostsIntervalMsMin] = scMinPostsIntervalMsMin;
    if (!scMinPostsIntervalMsMax.isEmpty()) obj[configKey::xhttpScMinPostsIntervalMsMax] = scMinPostsIntervalMsMax;
    if (!scStreamUpServerSecsMin.isEmpty()) obj[configKey::xhttpScStreamUpServerSecsMin] = scStreamUpServerSecsMin;
    if (!scStreamUpServerSecsMax.isEmpty()) obj[configKey::xhttpScStreamUpServerSecsMax] = scStreamUpServerSecsMax;

    obj["xPadding"] = xPadding.toJson();
    obj["xmux"]     = xmux.toJson();

    return obj;
}

namespace
{
    XrayXhttpConfig clearedXhttpConfig()
    {
        XrayXhttpConfig c;
        c.mode = QString();
        c.host = QString();
        c.path = QString();
        c.headersTemplate = QString();
        c.uplinkMethod = QString();
        c.disableGrpc = false;
        c.disableSse = false;
        c.sessionPlacement = QString();
        c.sessionKey = QString();
        c.seqPlacement = QString();
        c.seqKey = QString();
        c.uplinkDataPlacement = QString();
        c.uplinkDataKey = QString();
        c.uplinkChunkSize = QString();
        c.scMaxBufferedPosts = QString();
        c.scMaxEachPostBytesMin = QString();
        c.scMaxEachPostBytesMax = QString();
        c.scMinPostsIntervalMsMin = QString();
        c.scMinPostsIntervalMsMax = QString();
        c.scStreamUpServerSecsMin = QString();
        c.scStreamUpServerSecsMax = QString();
        return c;
    }
} // namespace

XrayXhttpConfig XrayXhttpConfig::fromJson(const QJsonObject &json)
{
    if (json.isEmpty()) {
        return clearedXhttpConfig();
    }

    XrayXhttpConfig c = clearedXhttpConfig();

    if (json.contains(configKey::xhttpMode)) {
        c.mode = json.value(configKey::xhttpMode).toString();
    }
    if (json.contains(configKey::xhttpHost)) {
        c.host = json.value(configKey::xhttpHost).toString();
    }
    if (json.contains(configKey::xhttpPath)) {
        c.path = json.value(configKey::xhttpPath).toString();
    }
    if (json.contains(configKey::xhttpHeadersTemplate)) {
        c.headersTemplate = json.value(configKey::xhttpHeadersTemplate).toString();
    }
    if (json.contains(configKey::xhttpUplinkMethod)) {
        c.uplinkMethod = json.value(configKey::xhttpUplinkMethod).toString();
    }
    if (json.contains(configKey::xhttpDisableGrpc)) {
        c.disableGrpc = json.value(configKey::xhttpDisableGrpc).toBool();
    }
    if (json.contains(configKey::xhttpDisableSse)) {
        c.disableSse = json.value(configKey::xhttpDisableSse).toBool();
    }
    if (json.contains(configKey::xhttpSessionPlacement)) {
        c.sessionPlacement = json.value(configKey::xhttpSessionPlacement).toString();
    }
    if (json.contains(configKey::xhttpSessionKey)) {
        c.sessionKey = json.value(configKey::xhttpSessionKey).toString();
    }
    if (json.contains(configKey::xhttpSeqPlacement)) {
        c.seqPlacement = json.value(configKey::xhttpSeqPlacement).toString();
    }
    if (json.contains(configKey::xhttpSeqKey)) {
        c.seqKey = json.value(configKey::xhttpSeqKey).toString();
    }
    if (json.contains(configKey::xhttpUplinkDataPlacement)) {
        c.uplinkDataPlacement = json.value(configKey::xhttpUplinkDataPlacement).toString();
    }
    if (json.contains(configKey::xhttpUplinkDataKey)) {
        c.uplinkDataKey = json.value(configKey::xhttpUplinkDataKey).toString();
    }
    if (json.contains(configKey::xhttpUplinkChunkSize)) {
        c.uplinkChunkSize = json.value(configKey::xhttpUplinkChunkSize).toString();
    }
    if (json.contains(configKey::xhttpScMaxBufferedPosts)) {
        c.scMaxBufferedPosts = json.value(configKey::xhttpScMaxBufferedPosts).toString();
    }
    if (json.contains(configKey::xhttpScMaxEachPostBytesMin)) {
        c.scMaxEachPostBytesMin = json.value(configKey::xhttpScMaxEachPostBytesMin).toString();
    }
    if (json.contains(configKey::xhttpScMaxEachPostBytesMax)) {
        c.scMaxEachPostBytesMax = json.value(configKey::xhttpScMaxEachPostBytesMax).toString();
    }
    if (json.contains(configKey::xhttpScMinPostsIntervalMsMin)) {
        c.scMinPostsIntervalMsMin = json.value(configKey::xhttpScMinPostsIntervalMsMin).toString();
    }
    if (json.contains(configKey::xhttpScMinPostsIntervalMsMax)) {
        c.scMinPostsIntervalMsMax = json.value(configKey::xhttpScMinPostsIntervalMsMax).toString();
    }
    if (json.contains(configKey::xhttpScStreamUpServerSecsMin)) {
        c.scStreamUpServerSecsMin = json.value(configKey::xhttpScStreamUpServerSecsMin).toString();
    }
    if (json.contains(configKey::xhttpScStreamUpServerSecsMax)) {
        c.scStreamUpServerSecsMax = json.value(configKey::xhttpScStreamUpServerSecsMax).toString();
    }

    if (json.contains(QLatin1String("xPadding"))) {
        c.xPadding = XrayXPaddingConfig::fromJson(json.value(QLatin1String("xPadding")).toObject());
    }
    if (json.contains(QLatin1String("xmux"))) {
        c.xmux = XrayXmuxConfig::fromJson(json.value(QLatin1String("xmux")).toObject());
    }

    return c;
}

QJsonObject XrayMkcpConfig::toJson() const
{
    QJsonObject obj;
    if (!tti.isEmpty())              obj[configKey::mkcpTti]              = tti;
    if (!uplinkCapacity.isEmpty())   obj[configKey::mkcpUplinkCapacity]   = uplinkCapacity;
    if (!downlinkCapacity.isEmpty()) obj[configKey::mkcpDownlinkCapacity] = downlinkCapacity;
    if (!readBufferSize.isEmpty())   obj[configKey::mkcpReadBufferSize]   = readBufferSize;
    if (!writeBufferSize.isEmpty())  obj[configKey::mkcpWriteBufferSize]  = writeBufferSize;
    obj[configKey::mkcpCongestion] = congestion;
    return obj;
}

XrayMkcpConfig XrayMkcpConfig::fromJson(const QJsonObject &json)
{
    XrayMkcpConfig c;
    if (json.isEmpty()) {
        return c;
    }
    if (json.contains(configKey::mkcpTti)) {
        c.tti = json.value(configKey::mkcpTti).toString();
    }
    if (json.contains(configKey::mkcpUplinkCapacity)) {
        c.uplinkCapacity = json.value(configKey::mkcpUplinkCapacity).toString();
    }
    if (json.contains(configKey::mkcpDownlinkCapacity)) {
        c.downlinkCapacity = json.value(configKey::mkcpDownlinkCapacity).toString();
    }
    if (json.contains(configKey::mkcpReadBufferSize)) {
        c.readBufferSize = json.value(configKey::mkcpReadBufferSize).toString();
    }
    if (json.contains(configKey::mkcpWriteBufferSize)) {
        c.writeBufferSize = json.value(configKey::mkcpWriteBufferSize).toString();
    }
    if (json.contains(configKey::mkcpCongestion)) {
        c.congestion = json.value(configKey::mkcpCongestion).toBool();
    }
    return c;
}

QJsonObject XrayServerConfig::toJson() const
{
    QJsonObject obj;

    // Existing fields
    if (!port.isEmpty()) {
        obj[configKey::port] = port;
    }
    if (!transportProto.isEmpty()) {
        obj[configKey::transportProto] = transportProto;
    }
    if (!subnetAddress.isEmpty()) {
        obj[configKey::subnetAddress] = subnetAddress;
    }
    if (!site.isEmpty()) {
        obj[configKey::site] = site;
    }

    if (isThirdPartyConfig) {
        obj[configKey::isThirdPartyConfig] = isThirdPartyConfig;
    }

    // New: Security
    if (!security.isEmpty()) {
        obj[configKey::xraySecurity] = security;
    }
    if (!flow.isEmpty()) {
        obj[configKey::xrayFlow] = flow;
    }
    if (!fingerprint.isEmpty()) {
        obj[configKey::xrayFingerprint] = fingerprint;
    }
    if (!sni.isEmpty()) {
        obj[configKey::xraySni] = sni;
    }
    if (!alpn.isEmpty()) {
        obj[configKey::xrayAlpn] = alpn;
    }

    // New: Transport
    if (!transport.isEmpty()) {
        obj[configKey::xrayTransport] = transport;
    }
    const QJsonObject xhttpObj = xhttp.toJson();
    if (!xhttpObj.isEmpty()) {
        obj[QStringLiteral("xhttp")] = xhttpObj;
    }
    const QJsonObject mkcpObj = mkcp.toJson();
    if (!mkcpObj.isEmpty()) {
        obj[QStringLiteral("mkcp")] = mkcpObj;
    }

    return obj;
}

XrayServerConfig XrayServerConfig::fromJson(const QJsonObject &json)
{
    XrayServerConfig c;

    // Existing fields
    c.port = json.value(configKey::port).toString();
    c.transportProto = json.value(configKey::transportProto).toString();
    c.subnetAddress = json.value(configKey::subnetAddress).toString();
    c.site = json.value(configKey::site).toString();
    c.isThirdPartyConfig = json.value(configKey::isThirdPartyConfig).toBool(false);

    if (json.contains(configKey::xraySecurity)) {
        c.security = json.value(configKey::xraySecurity).toString();
    }
    if (json.contains(configKey::xrayFlow)) {
        c.flow = json.value(configKey::xrayFlow).toString();
    }
    if (json.contains(configKey::xrayFingerprint)) {
        c.fingerprint = json.value(configKey::xrayFingerprint).toString();
        if (c.fingerprint.contains(QLatin1String("Mozilla/5.0"), Qt::CaseInsensitive)) {
            c.fingerprint = QString::fromLatin1(protocols::xray::defaultFingerprint);
        }
    }
    if (json.contains(configKey::xraySni)) {
        c.sni = json.value(configKey::xraySni).toString();
    }
    if (json.contains(configKey::xrayAlpn)) {
        c.alpn = json.value(configKey::xrayAlpn).toString();
    }
    if (json.contains(configKey::xrayTransport)) {
        c.transport = json.value(configKey::xrayTransport).toString();
    }
    if (json.contains(QLatin1String("xhttp"))) {
        const QJsonObject xhttpJson = json.value(QLatin1String("xhttp")).toObject();
        if (!xhttpJson.isEmpty()) {
            c.xhttp = XrayXhttpConfig::fromJson(xhttpJson);
        }
    }
    if (json.contains(QLatin1String("mkcp"))) {
        const QJsonObject mkcpJson = json.value(QLatin1String("mkcp")).toObject();
        if (!mkcpJson.isEmpty()) {
            c.mkcp = XrayMkcpConfig::fromJson(mkcpJson);
        }
    }

    return c;
}

bool XrayServerConfig::hasEqualServerSettings(const XrayServerConfig &other) const
{
    return port == other.port
           && site == other.site
           && security == other.security
           && flow == other.flow
           && transport == other.transport
           && fingerprint == other.fingerprint
           && sni == other.sni
           && alpn == other.alpn
           && xhttp.toJson() == other.xhttp.toJson()
           && mkcp.toJson() == other.mkcp.toJson();
}

QJsonObject XrayClientConfig::toJson() const
{
    QJsonObject obj;
    if (!nativeConfig.isEmpty()) obj[configKey::config]   = nativeConfig;
    if (!localPort.isEmpty())    obj[configKey::localPort] = localPort;
    if (!id.isEmpty())           obj[configKey::clientId]  = id;
    return obj;
}

XrayClientConfig XrayClientConfig::fromJson(const QJsonObject &json)
{
    XrayClientConfig c;
    c.nativeConfig = json.value(configKey::config).toString();
    c.localPort    = json.value(configKey::localPort).toString();
    c.id           = json.value(configKey::clientId).toString();

    if (c.id.isEmpty() && !c.nativeConfig.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(c.nativeConfig.toUtf8());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject configObj = doc.object();
            if (configObj.contains(protocols::xray::outbounds)) {
                QJsonArray outbounds = configObj.value(protocols::xray::outbounds).toArray();
                if (!outbounds.isEmpty()) {
                    QJsonObject outbound = outbounds[0].toObject();
                    if (outbound.contains(protocols::xray::settings)) {
                        QJsonObject settings = outbound[protocols::xray::settings].toObject();
                        if (settings.contains(protocols::xray::vnext)) {
                            QJsonArray vnext = settings[protocols::xray::vnext].toArray();
                            if (!vnext.isEmpty()) {
                                QJsonObject vnextObj = vnext[0].toObject();
                                if (vnextObj.contains(protocols::xray::users)) {
                                    QJsonArray users = vnextObj[protocols::xray::users].toArray();
                                    if (!users.isEmpty()) {
                                        QJsonObject user = users[0].toObject();
                                        if (user.contains(protocols::xray::id)) {
                                            c.id = user[protocols::xray::id].toString();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return c;
}

QJsonObject XrayProtocolConfig::toJson() const
{
    QJsonObject obj = serverConfig.toJson();

    if (clientConfig.has_value()) {
        QJsonDocument doc = QJsonDocument::fromJson(clientConfig->nativeConfig.toUtf8());
        if (!doc.isNull() && doc.isObject() && doc.object().contains(protocols::xray::outbounds)
                && !doc.object().contains(configKey::config)) {
            obj[configKey::lastConfig] = clientConfig->nativeConfig;
        } else {
            QJsonObject clientJson = clientConfig->toJson();
            obj[configKey::lastConfig] = QString::fromUtf8(QJsonDocument(clientJson).toJson(QJsonDocument::Compact));
        }
    }

    return obj;
}

XrayProtocolConfig XrayProtocolConfig::fromJson(const QJsonObject &json)
{
    XrayProtocolConfig c;
    c.serverConfig = XrayServerConfig::fromJson(json);

    QString lastConfigStr = json.value(configKey::lastConfig).toString();
    if (!lastConfigStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(lastConfigStr.toUtf8());
        if (doc.isObject()) {
            QJsonObject parsed = doc.object();
            if (parsed.contains(protocols::xray::outbounds) && !parsed.contains(configKey::config)) {
                XrayClientConfig clientCfg;
                clientCfg.nativeConfig = lastConfigStr;
                if (parsed.contains(protocols::xray::inbounds)) {
                    QJsonArray inbounds = parsed.value(protocols::xray::inbounds).toArray();
                    if (!inbounds.isEmpty()) {
                        QJsonObject inbound = inbounds[0].toObject();
                        if (inbound.contains(protocols::xray::port)) {
                            clientCfg.localPort = QString::number(inbound.value(protocols::xray::port).toInt());
                        }
                    }
                }
                const QJsonArray outbounds = parsed.value(protocols::xray::outbounds).toArray();
                if (!outbounds.isEmpty()) {
                    const QJsonObject settings = outbounds[0].toObject().value(protocols::xray::settings).toObject();
                    const QJsonArray vnext = settings.value(protocols::xray::vnext).toArray();
                    if (!vnext.isEmpty()) {
                        const QJsonArray users = vnext[0].toObject().value(protocols::xray::users).toArray();
                        if (!users.isEmpty()) {
                            clientCfg.id = users[0].toObject().value(protocols::xray::id).toString();
                        }
                    }
                }
                c.clientConfig = clientCfg;
            } else {
                c.clientConfig = XrayClientConfig::fromJson(parsed);
            }
        }
    }

    c.needsClientHydration =
            c.hasClientConfig()
            && (!json.contains(configKey::xrayTransport) || c.serverConfig.isThirdPartyConfig);
    if (c.needsClientHydration) {
        c.hydrateServerConfigFromClientNative();
    }

    return c;
}

bool XrayProtocolConfig::hydrateServerConfigFromClientNative()
{
    if (!clientConfig.has_value() || clientConfig->nativeConfig.isEmpty()) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(clientConfig->nativeConfig.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    const QJsonObject root = doc.object();
    const QJsonArray outbounds = root.value(protocols::xray::outbounds).toArray();
    if (outbounds.isEmpty()) {
        return false;
    }

    const QJsonObject outbound = outbounds[0].toObject();
    const QJsonObject streamSettings = outbound.value(protocols::xray::streamSettings).toObject();
    if (streamSettings.isEmpty()) {
        return false;
    }

    XrayServerConfig &srv = serverConfig;

    const QJsonObject settings = outbound.value(protocols::xray::settings).toObject();
    const QJsonArray vnext = settings.value(protocols::xray::vnext).toArray();
    if (!vnext.isEmpty()) {
        const QJsonObject vnextEntry = vnext[0].toObject();
        if (vnextEntry.contains(protocols::xray::port)) {
            srv.port = QString::number(vnextEntry.value(protocols::xray::port).toInt());
        }
        const QJsonArray users = vnextEntry.value(protocols::xray::users).toArray();
        if (!users.isEmpty()) {
            srv.flow = users[0].toObject().value(protocols::xray::flow).toString();
        }
    }

    const QString networkVal = streamSettings.value(protocols::xray::network).toString(QStringLiteral("tcp"));
    if (networkVal == QLatin1String("xhttp")) {
        srv.transport = QStringLiteral("xhttp");
    } else if (networkVal == QLatin1String("kcp")) {
        srv.transport = QStringLiteral("mkcp");
    } else {
        srv.transport = QStringLiteral("raw");
    }

    if (streamSettings.contains(protocols::xray::security)) {
        srv.security = streamSettings.value(protocols::xray::security).toString();
    }

    if (srv.security == QLatin1String("reality")) {
        const QJsonObject rs = streamSettings.value(protocols::xray::realitySettings).toObject();
        srv.sni = rs.value(protocols::xray::serverName).toString();
        srv.site = srv.sni.isEmpty() ? srv.site : srv.sni;
        const QString fp = rs.value(protocols::xray::fingerprint).toString();
        if (!fp.isEmpty()) {
            srv.fingerprint = fp.contains(QLatin1String("Mozilla/5.0"), Qt::CaseInsensitive)
                    ? QString::fromLatin1(protocols::xray::defaultFingerprint)
                    : fp;
        }
    }

    if (srv.security == QLatin1String("tls")) {
        const QJsonObject tls = streamSettings.value(QStringLiteral("tlsSettings")).toObject();
        srv.sni = tls.value(protocols::xray::serverName).toString();
        const QString fp = tls.value(protocols::xray::fingerprint).toString();
        if (!fp.isEmpty()) {
            srv.fingerprint = fp;
        }
        QStringList alpnList;
        for (const QJsonValue &v : tls.value(QStringLiteral("alpn")).toArray()) {
            alpnList << v.toString();
        }
        if (!alpnList.isEmpty()) {
            srv.alpn = alpnList.join(QLatin1Char(','));
        }
    }

    if (srv.transport == QLatin1String("xhttp")) {
        const QJsonObject xhttpObj = streamSettings.value(QStringLiteral("xhttpSettings")).toObject();
        QJsonObject xhttpJson;
        const QString mode = xhttpObj.value(QStringLiteral("mode")).toString();
        if (!mode.isEmpty()) {
            if (mode == QLatin1String("auto")) {
                xhttpJson[configKey::xhttpMode] = QStringLiteral("Auto");
            } else if (mode == QLatin1String("packet-up")) {
                xhttpJson[configKey::xhttpMode] = QStringLiteral("Packet-up");
            } else if (mode == QLatin1String("stream-up")) {
                xhttpJson[configKey::xhttpMode] = QStringLiteral("Stream-up");
            } else if (mode == QLatin1String("stream-one")) {
                xhttpJson[configKey::xhttpMode] = QStringLiteral("Stream-one");
            } else {
                xhttpJson[configKey::xhttpMode] = mode;
            }
        }
        if (xhttpObj.contains(QStringLiteral("host"))) {
            xhttpJson[configKey::xhttpHost] = xhttpObj.value(QStringLiteral("host")).toString();
        }
        if (xhttpObj.contains(QStringLiteral("path"))) {
            xhttpJson[configKey::xhttpPath] = xhttpObj.value(QStringLiteral("path")).toString();
        }
        if (xhttpObj.contains(QStringLiteral("uplinkHTTPMethod"))) {
            xhttpJson[configKey::xhttpUplinkMethod] = xhttpObj.value(QStringLiteral("uplinkHTTPMethod")).toString();
        }
        xhttpJson[configKey::xhttpDisableGrpc] = xhttpObj.value(QStringLiteral("noGRPCHeader")).toBool(true);
        xhttpJson[configKey::xhttpDisableSse] = xhttpObj.value(QStringLiteral("noSSEHeader")).toBool(true);
        srv.xhttp = XrayXhttpConfig::fromJson(xhttpJson);
    }

    if (srv.transport == QLatin1String("mkcp")) {
        const QJsonObject kcpObj = streamSettings.value(QStringLiteral("kcpSettings")).toObject();
        XrayMkcpConfig mk;
        if (kcpObj.contains(QStringLiteral("tti"))) {
            mk.tti = QString::number(kcpObj.value(QStringLiteral("tti")).toInt());
        }
        if (kcpObj.contains(QStringLiteral("uplinkCapacity"))) {
            mk.uplinkCapacity = QString::number(kcpObj.value(QStringLiteral("uplinkCapacity")).toInt());
        }
        if (kcpObj.contains(QStringLiteral("downlinkCapacity"))) {
            mk.downlinkCapacity = QString::number(kcpObj.value(QStringLiteral("downlinkCapacity")).toInt());
        }
        if (kcpObj.contains(QStringLiteral("readBufferSize"))) {
            mk.readBufferSize = QString::number(kcpObj.value(QStringLiteral("readBufferSize")).toInt());
        }
        if (kcpObj.contains(QStringLiteral("writeBufferSize"))) {
            mk.writeBufferSize = QString::number(kcpObj.value(QStringLiteral("writeBufferSize")).toInt());
        }
        if (kcpObj.contains(QStringLiteral("congestion"))) {
            mk.congestion = kcpObj.value(QStringLiteral("congestion")).toBool(true);
        }
        srv.mkcp = mk;
    }

    needsClientHydration = false;
    return true;
}

bool XrayProtocolConfig::hasClientConfig() const
{
    return clientConfig.has_value();
}

void XrayProtocolConfig::setClientConfig(const XrayClientConfig &config)
{
    clientConfig = config;
}

void XrayProtocolConfig::clearClientConfig()
{
    clientConfig.reset();
}

} // namespace amnezia
