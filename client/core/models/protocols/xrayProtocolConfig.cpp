#include "xrayProtocolConfig.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "logger.h"

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
    c.key       = json.value(configKey::xPaddingKey).toString();
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

QJsonObject XrayClientTemplate::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("format_version")] = formatVersion;
    obj[QStringLiteral("fingerprint")] = fingerprint;
    obj[QStringLiteral("uplink_method")] = uplinkMethod;
    obj[QStringLiteral("uplink_chunk_size")] = uplinkChunkSize;
    obj[QStringLiteral("sc_min_posts_interval_ms_min")] = scMinPostsIntervalMsMin;
    obj[QStringLiteral("sc_min_posts_interval_ms_max")] = scMinPostsIntervalMsMax;
    obj[QStringLiteral("xmux")] = xmux.toJson();
    obj[QStringLiteral("updated_at")] = updatedAt;
    return obj;
}

XrayClientTemplate XrayClientTemplate::fromJson(const QJsonObject &json)
{
    XrayClientTemplate c;
    c.formatVersion = json.value(QStringLiteral("format_version")).toInt(0);
    c.fingerprint = json.value(QStringLiteral("fingerprint")).toString(c.fingerprint);
    c.uplinkMethod = json.value(QStringLiteral("uplink_method")).toString(c.uplinkMethod);
    c.uplinkChunkSize = json.value(QStringLiteral("uplink_chunk_size")).toString(c.uplinkChunkSize);
    c.scMinPostsIntervalMsMin = json.value(QStringLiteral("sc_min_posts_interval_ms_min")).toString(c.scMinPostsIntervalMsMin);
    c.scMinPostsIntervalMsMax = json.value(QStringLiteral("sc_min_posts_interval_ms_max")).toString(c.scMinPostsIntervalMsMax);
    c.xmux = XrayXmuxConfig::fromJson(json.value(QStringLiteral("xmux")).toObject());
    c.updatedAt = json.value(QStringLiteral("updated_at")).toString();
    return c;
}

QString XrayClientTemplate::contentFingerprint() const
{
    QJsonObject content = toJson();
    content.remove(QStringLiteral("updated_at"));
    content.remove(QStringLiteral("format_version"));

    const QByteArray canonical = QJsonDocument(content).toJson(QJsonDocument::Compact);
    return QString::fromLatin1(QCryptographicHash::hash(canonical, QCryptographicHash::Sha256).toHex());
}

void XrayClientTemplate::materializeFromLegacy(const QJsonObject &storedServerJson)
{
    const QJsonObject xhttpJson = storedServerJson.value(QStringLiteral("xhttp")).toObject();

    if (storedServerJson.contains(configKey::xrayFingerprint)) {
        fingerprint = storedServerJson.value(configKey::xrayFingerprint).toString();
        if (fingerprint.contains(QLatin1String("Mozilla/5.0"), Qt::CaseInsensitive)) {
            fingerprint = QString::fromLatin1(protocols::xray::defaultFingerprint);
        }
    }
    if (xhttpJson.contains(configKey::xhttpUplinkMethod)) {
        uplinkMethod = xhttpJson.value(configKey::xhttpUplinkMethod).toString();
    }
    if (xhttpJson.contains(configKey::xhttpUplinkChunkSize)) {
        uplinkChunkSize = xhttpJson.value(configKey::xhttpUplinkChunkSize).toString();
    }
    if (xhttpJson.contains(configKey::xhttpScMinPostsIntervalMsMin)) {
        scMinPostsIntervalMsMin = xhttpJson.value(configKey::xhttpScMinPostsIntervalMsMin).toString();
    }
    if (xhttpJson.contains(configKey::xhttpScMinPostsIntervalMsMax)) {
        scMinPostsIntervalMsMax = xhttpJson.value(configKey::xhttpScMinPostsIntervalMsMax).toString();
    }
    if (xhttpJson.contains(QLatin1String("xmux"))) {
        xmux = XrayXmuxConfig::fromJson(xhttpJson.value(QLatin1String("xmux")).toObject());
    }

    formatVersion = 1;
}

QJsonObject XrayXhttpConfig::toJson() const
{
    QJsonObject obj;
    if (!mode.isEmpty())            obj[configKey::xhttpMode]            = mode;
    if (!host.isEmpty())            obj[configKey::xhttpHost]            = host;
    if (!path.isEmpty())            obj[configKey::xhttpPath]            = path;
    obj[configKey::xhttpDisableGrpc] = disableGrpc;
    obj[configKey::xhttpDisableSse]  = disableSse;

    if (!sessionPlacement.isEmpty())    obj[configKey::xhttpSessionPlacement]    = sessionPlacement;
    if (!sessionKey.isEmpty())          obj[configKey::xhttpSessionKey]          = sessionKey;
    if (!seqPlacement.isEmpty())        obj[configKey::xhttpSeqPlacement]        = seqPlacement;
    if (!seqKey.isEmpty())              obj[configKey::xhttpSeqKey]              = seqKey;
    if (!uplinkDataPlacement.isEmpty()) obj[configKey::xhttpUplinkDataPlacement] = uplinkDataPlacement;
    if (!uplinkDataKey.isEmpty())       obj[configKey::xhttpUplinkDataKey]       = uplinkDataKey;

    if (!scMaxBufferedPosts.isEmpty())      obj[configKey::xhttpScMaxBufferedPosts]      = scMaxBufferedPosts;
    if (!scMaxEachPostBytesMin.isEmpty())   obj[configKey::xhttpScMaxEachPostBytesMin]   = scMaxEachPostBytesMin;
    if (!scMaxEachPostBytesMax.isEmpty())   obj[configKey::xhttpScMaxEachPostBytesMax]   = scMaxEachPostBytesMax;
    if (!scStreamUpServerSecsMin.isEmpty()) obj[configKey::xhttpScStreamUpServerSecsMin] = scStreamUpServerSecsMin;
    if (!scStreamUpServerSecsMax.isEmpty()) obj[configKey::xhttpScStreamUpServerSecsMax] = scStreamUpServerSecsMax;

    obj["xPadding"] = xPadding.toJson();

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
        c.disableSse = false;
        c.sessionPlacement = QString();
        c.sessionKey = QString();
        c.seqPlacement = QString();
        c.seqKey = QString();
        c.uplinkDataPlacement = QString();
        c.uplinkDataKey = QString();
        c.scMaxBufferedPosts = QString();
        c.scMaxEachPostBytesMin = QString();
        c.scMaxEachPostBytesMax = QString();
        c.scStreamUpServerSecsMin = QString();
        c.scStreamUpServerSecsMax = QString();
        return c;
    }

    QString coerceXhttpModeToSupported(const QString &mode)
    {
        if (mode.isEmpty() || mode.compare(QLatin1String("Stream-up"), Qt::CaseInsensitive) == 0
            || mode.compare(QLatin1String("Stream-one"), Qt::CaseInsensitive) == 0) {
            return mode;
        }
        const QString coerced = QString::fromLatin1(protocols::xray::defaultXhttpMode);
        return coerced;
    }

    QString coerceUplinkDataPlacementToSupported(const QString &placement)
    {
        if (placement.isEmpty() || placement.compare(QLatin1String("Body"), Qt::CaseInsensitive) == 0
            || placement.compare(QLatin1String("Auto"), Qt::CaseInsensitive) == 0) {
            return placement;
        }
        const QString coerced = QString::fromLatin1(protocols::xray::defaultXhttpUplinkDataPlacement);
        return coerced;
    }
} // namespace

XrayXhttpConfig XrayXhttpConfig::fromJson(const QJsonObject &json)
{
    if (json.isEmpty()) {
        return clearedXhttpConfig();
    }

    XrayXhttpConfig c = clearedXhttpConfig();

    if (json.contains(configKey::xhttpMode)) {
        c.mode = coerceXhttpModeToSupported(json.value(configKey::xhttpMode).toString());
    }
    if (json.contains(configKey::xhttpHost)) {
        c.host = json.value(configKey::xhttpHost).toString();
    }
    if (json.contains(configKey::xhttpPath)) {
        c.path = json.value(configKey::xhttpPath).toString();
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
        c.uplinkDataPlacement =
                coerceUplinkDataPlacementToSupported(json.value(configKey::xhttpUplinkDataPlacement).toString());
    }
    if (json.contains(configKey::xhttpUplinkDataKey)) {
        c.uplinkDataKey = json.value(configKey::xhttpUplinkDataKey).toString();
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
    if (json.contains(configKey::xhttpScStreamUpServerSecsMin)) {
        c.scStreamUpServerSecsMin = json.value(configKey::xhttpScStreamUpServerSecsMin).toString();
    }
    if (json.contains(configKey::xhttpScStreamUpServerSecsMax)) {
        c.scStreamUpServerSecsMax = json.value(configKey::xhttpScStreamUpServerSecsMax).toString();
    }

    if (json.contains(QLatin1String("xPadding"))) {
        c.xPadding = XrayXPaddingConfig::fromJson(json.value(QLatin1String("xPadding")).toObject());
    }

    if ((c.scMaxEachPostBytesMin.isEmpty() && c.scMaxEachPostBytesMax.isEmpty())
        || (c.scMaxEachPostBytesMin == QLatin1String("1") && c.scMaxEachPostBytesMax == QLatin1String("100"))) {
        c.scMaxEachPostBytesMin = QString::fromLatin1(protocols::xray::defaultXhttpScMaxEachPostBytesMin);
        c.scMaxEachPostBytesMax = QString::fromLatin1(protocols::xray::defaultXhttpScMaxEachPostBytesMax);
    }

    return c;
}

QJsonObject XrayMkcpConfig::toJson() const
{
    QJsonObject obj;
    if (!tti.isEmpty())              obj[configKey::mkcpTti]              = tti;
    if (!mtu.isEmpty())              obj[configKey::mkcpMtu]              = mtu;
    if (!uplinkCapacity.isEmpty())   obj[configKey::mkcpUplinkCapacity]   = uplinkCapacity;
    if (!downlinkCapacity.isEmpty()) obj[configKey::mkcpDownlinkCapacity] = downlinkCapacity;
    if (!cwndMultiplier.isEmpty())   obj[configKey::mkcpCwndMultiplier]   = cwndMultiplier;
    if (!maxSendingWindow.isEmpty()) obj[configKey::mkcpMaxSendingWindow] = maxSendingWindow;
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
    if (json.contains(configKey::mkcpMtu)) {
        c.mtu = json.value(configKey::mkcpMtu).toString();
    }
    if (json.contains(configKey::mkcpUplinkCapacity)) {
        c.uplinkCapacity = json.value(configKey::mkcpUplinkCapacity).toString();
    }
    if (json.contains(configKey::mkcpDownlinkCapacity)) {
        c.downlinkCapacity = json.value(configKey::mkcpDownlinkCapacity).toString();
    }
    if (json.contains(configKey::mkcpCwndMultiplier)) {
        c.cwndMultiplier = json.value(configKey::mkcpCwndMultiplier).toString();
    }
    if (json.contains(configKey::mkcpMaxSendingWindow)) {
        c.maxSendingWindow = json.value(configKey::mkcpMaxSendingWindow).toString();
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

void XrayServerConfig::applyDefaults(bool fillFlowDefault)
{
    if (port.isEmpty()) {
        port = protocols::xray::defaultPort;
    }
    if (transportProto.isEmpty()) {
        transportProto = ProtocolUtils::transportProtoToString(ProtocolUtils::defaultTransportProto(Proto::Xray),
                                                               Proto::Xray);
    }
    if (site.isEmpty()) {
        site = protocols::xray::defaultSite;
    }
    if (transport.isEmpty()) {
        transport = protocols::xray::defaultTransport;
    }
    if (security.isEmpty()) {
        security = protocols::xray::defaultSecurity;
    }
    if (fillFlowDefault && flow.isEmpty()) {
        flow = protocols::xray::defaultFlow;
    }
    if (sni.isEmpty()) {
        sni = site.isEmpty() ? QString::fromLatin1(protocols::xray::defaultSni) : site;
    }
    if (alpn.isEmpty()) {
        alpn = protocols::xray::defaultAlpn;
    }

    if (xhttp.host.isEmpty()) {
        xhttp.host = protocols::xray::defaultXhttpHost;
    }
    if (xhttp.mode.isEmpty()) {
        xhttp.mode = protocols::xray::defaultXhttpMode;
    }
    if (xhttp.sessionPlacement.isEmpty()) {
        xhttp.sessionPlacement = protocols::xray::defaultXhttpSessionPlacement;
    }
    if (xhttp.sessionKey.isEmpty()) {
        xhttp.sessionKey = protocols::xray::defaultXhttpSessionKey;
    }
    if (xhttp.seqPlacement.isEmpty()) {
        xhttp.seqPlacement = protocols::xray::defaultXhttpSeqPlacement;
    }
    if (xhttp.uplinkDataPlacement.isEmpty()) {
        xhttp.uplinkDataPlacement = protocols::xray::defaultXhttpUplinkDataPlacement;
    }
    if (xhttp.scMaxEachPostBytesMin.isEmpty()) {
        xhttp.scMaxEachPostBytesMin = protocols::xray::defaultXhttpScMaxEachPostBytesMin;
    }
    if (xhttp.scMaxEachPostBytesMax.isEmpty()) {
        xhttp.scMaxEachPostBytesMax = protocols::xray::defaultXhttpScMaxEachPostBytesMax;
    }

    if (xhttp.xPadding.placement.isEmpty()) {
        xhttp.xPadding.placement = protocols::xray::defaultXPaddingPlacement;
    }
    if (xhttp.xPadding.method.isEmpty()) {
        xhttp.xPadding.method = protocols::xray::defaultXPaddingMethod;
    }
}

namespace xrayEffective
{
    QString xhttpMode(const QString &mode)
    {
        const QString t = mode.trimmed();
        if (t.isEmpty() || t.compare(QLatin1String("Auto"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("auto");
        if (t.compare(QLatin1String("Packet-up"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("packet-up");
        if (t.compare(QLatin1String("Stream-up"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("stream-up");
        if (t.compare(QLatin1String("Stream-one"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("stream-one");
        return t.toLower();
    }

    QString sessionSeqPlacement(const QString &placement)
    {
        if (placement.isEmpty() || placement.compare(QLatin1String("None"), Qt::CaseInsensitive) == 0)
            return {};
        return placement.toLower();
    }

    QString uplinkDataPlacement(const QString &placement)
    {
        if (placement.isEmpty() || placement.compare(QLatin1String("Body"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("body");
        if (placement.compare(QLatin1String("Auto"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("auto");
        return placement.toLower();
    }

    QString xPaddingPlacement(const QString &placement)
    {
        QString t = placement.trimmed();
        if (t.isEmpty())
            return QString::fromLatin1(protocols::xray::defaultXPaddingPlacement).toLower();
        if (t.contains(QLatin1String("queryInHeader"), Qt::CaseInsensitive)
            || t.compare(QLatin1String("Query in header"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("queryInHeader");
        return t.toLower();
    }

    QString xPaddingMethod(const QString &method)
    {
        QString t = method.trimmed();
        if (t.isEmpty() || t.compare(QLatin1String("Repeat-x"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("repeat-x");
        if (t.compare(QLatin1String("Tokenish"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("tokenish");
        if (t.compare(QLatin1String("Random"), Qt::CaseInsensitive) == 0
            || t.compare(QLatin1String("Zero"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("repeat-x");
        return t.toLower();
    }

    QString range(const QString &minV, const QString &maxV)
    {
        return minV + QLatin1Char('-') + maxV;
    }

    void putRangeIfAny(QJsonObject &obj, const char *key, QString minV, QString maxV, const char *fallbackMin,
                       const char *fallbackMax)
    {
        if (minV.isEmpty() && maxV.isEmpty())
            return;
        if (minV.isEmpty())
            minV = QString::fromLatin1(fallbackMin);
        if (maxV.isEmpty())
            maxV = QString::fromLatin1(fallbackMax);
        obj[QString::fromUtf8(key)] = range(minV, maxV);
    }

    QString security(const XrayServerConfig &srv)
    {
        namespace px = protocols::xray;
        if (srv.transport == QLatin1String(px::transportMkcp)
            && srv.security == QLatin1String(px::securityReality)) {
            return QString::fromLatin1(px::securityNone);
        }
        return srv.security;
    }

    QString clientFlow(const XrayServerConfig &srv)
    {
        namespace px = protocols::xray;
        const bool rawTransport = srv.transport.isEmpty() || srv.transport == QLatin1String(px::transportRaw);
        const bool secureFlow =
                srv.security == QLatin1String(px::securityTls) || srv.security == QLatin1String(px::securityReality);
        return (rawTransport && secureFlow) ? srv.flow : QString();
    }

    QString network(const XrayServerConfig &srv)
    {
        namespace px = protocols::xray;
        if (srv.transport == QLatin1String(px::transportXhttp))
            return QString::fromLatin1(px::networkXhttp);
        if (srv.transport == QLatin1String(px::transportMkcp))
            return QString::fromLatin1(px::networkKcp);
        return QString::fromLatin1(px::networkTcp);
    }

    QString xhttpModeSent(const XrayServerConfig &srv)
    {
        return xhttpMode(srv.xhttp.mode);
    }
}

QJsonObject XrayServerConfig::serverStreamSettings() const
{
    return streamSettingsJson(XrayStreamSide::Server, XrayClientTemplate {});
}

QJsonObject XrayServerConfig::clientStreamSettings(const XrayClientTemplate &clientTemplate) const
{
    return streamSettingsJson(XrayStreamSide::Client, clientTemplate);
}

QJsonObject XrayServerConfig::streamSettingsJson(XrayStreamSide side, const XrayClientTemplate &clientTemplate) const
{
    namespace px = protocols::xray;

    const bool clientSide = side == XrayStreamSide::Client;
    const QString fingerprintEff = clientTemplate.fingerprint.isEmpty()
            ? QString::fromLatin1(px::defaultFingerprint)
            : clientTemplate.fingerprint;

    QJsonObject streamSettings;
    streamSettings[px::network] = xrayEffective::network(*this);

    const QString securityEff = xrayEffective::security(*this);
    streamSettings[px::security] = securityEff;

    if (securityEff == QLatin1String(px::securityTls)) {
        QJsonObject tlsSettings;
        tlsSettings[px::serverName] = sni.isEmpty() ? QString::fromLatin1(px::defaultSni) : sni;
        const QString alpnEff = alpn.isEmpty() ? QString::fromLatin1(px::defaultAlpn) : alpn;
        QJsonArray alpnArray;
        for (const QString &a : alpnEff.split(QLatin1Char(','))) {
            QString t = a.trimmed();
            if (t.isEmpty())
                continue;
            if (t.compare(QLatin1String("HTTP/2"), Qt::CaseInsensitive) == 0)
                t = QStringLiteral("h2");
            else if (t.compare(QLatin1String("HTTP/1.1"), Qt::CaseInsensitive) == 0)
                t = QStringLiteral("http/1.1");
            alpnArray.append(t);
        }
        if (!alpnArray.isEmpty())
            tlsSettings[px::alpn] = alpnArray;
        if (clientSide)
            tlsSettings[px::fingerprint] = fingerprintEff;
        if (!clientSide && !isThirdPartyConfig) {
            QJsonObject cert;
            cert[px::certificateFile] = QString::fromLatin1(px::tlsCertPath);
            cert[px::keyFile] = QString::fromLatin1(px::tlsKeyPath);
            tlsSettings[px::certificates] = QJsonArray { cert };
        }
        streamSettings[px::tlsSettings] = tlsSettings;
    }

    if (clientSide && securityEff == QLatin1String(px::securityReality)) {
        QJsonObject realitySettings;
        realitySettings[px::fingerprint] = fingerprintEff;
        realitySettings[px::serverName] = sni.isEmpty() ? QString::fromLatin1(px::defaultSni) : sni;
        streamSettings[px::realitySettings] = realitySettings;
    }

    if (transport == QLatin1String(px::transportXhttp)) {
        QJsonObject xo;
        xo[px::xhttpHost] = xhttp.host.isEmpty() ? QString::fromLatin1(px::defaultXhttpHost) : xhttp.host;
        const QString pathEff = xhttp.path.trimmed().isEmpty()
                ? QString::fromLatin1(px::defaultXhttpPath)
                : xhttp.path;
        xo[px::xhttpPath] = pathEff;
        const QString modeEff = xrayEffective::xhttpModeSent(*this);
        xo[px::xhttpMode] = modeEff;

        if (clientSide) {
            QString methodEff = clientTemplate.uplinkMethod.isEmpty()
                    ? QString::fromLatin1(px::defaultXhttpUplinkMethod)
                    : clientTemplate.uplinkMethod;
            methodEff = methodEff.toUpper();
            if ((modeEff == QLatin1String("stream-one") || modeEff == QLatin1String("auto"))
                && methodEff == QLatin1String("PUT")) {
                methodEff = QString::fromLatin1(px::defaultXhttpUplinkMethod);
            }
            xo[px::uplinkHttpMethod] = methodEff;
        }

        xo[px::noGrpcHeader] = xhttp.disableGrpc;
        xo[px::noSseHeader] = xhttp.disableSse;

        const QString sessPl = xrayEffective::sessionSeqPlacement(xhttp.sessionPlacement);
        if (!sessPl.isEmpty())
            xo[px::sessionIdPlacement] = sessPl;
        const QString seqPl = xrayEffective::sessionSeqPlacement(xhttp.seqPlacement);
        if (!seqPl.isEmpty())
            xo[px::seqPlacement] = seqPl;
        if (!xhttp.sessionKey.isEmpty())
            xo[px::sessionIdKey] = xhttp.sessionKey;
        if (!xhttp.seqKey.isEmpty())
            xo[px::seqKey] = xhttp.seqKey;

        const QString uDataPl = xrayEffective::uplinkDataPlacement(xhttp.uplinkDataPlacement);
        const bool uDataNeedsPacketUp = uDataPl == QLatin1String("header") || uDataPl == QLatin1String("cookie");
        if (!(uDataNeedsPacketUp && modeEff != QLatin1String("packet-up")))
            xo[px::uplinkDataPlacement] = uDataPl;
        if (!xhttp.uplinkDataKey.isEmpty())
            xo[px::uplinkDataKey] = xhttp.uplinkDataKey;

        if (clientSide) {
            const QString chunkSizeEff = clientTemplate.uplinkChunkSize.isEmpty()
                    ? QString::fromLatin1(px::defaultXhttpUplinkChunkSize)
                    : clientTemplate.uplinkChunkSize;
            if (!chunkSizeEff.isEmpty() && chunkSizeEff != QLatin1String("0"))
                xo[px::uplinkChunkSize] = chunkSizeEff.toInt();
        }

        if (!xhttp.scMaxBufferedPosts.isEmpty())
            xo[px::scMaxBufferedPosts] = xhttp.scMaxBufferedPosts.toLongLong();
        xrayEffective::putRangeIfAny(xo, px::scMaxEachPostBytes, xhttp.scMaxEachPostBytesMin,
                                     xhttp.scMaxEachPostBytesMax, px::defaultXhttpScMaxEachPostBytesMin,
                                     px::defaultXhttpScMaxEachPostBytesMax);
        if (clientSide) {
            xrayEffective::putRangeIfAny(xo, px::scMinPostsIntervalMs, clientTemplate.scMinPostsIntervalMsMin,
                                         clientTemplate.scMinPostsIntervalMsMax,
                                         px::defaultXhttpScMinPostsIntervalMsMin,
                                         px::defaultXhttpScMinPostsIntervalMsMax);
        }
        xrayEffective::putRangeIfAny(xo, px::scStreamUpServerSecs, xhttp.scStreamUpServerSecsMin,
                                     xhttp.scStreamUpServerSecsMax, px::defaultXhttpScStreamUpServerSecsMin,
                                     px::defaultXhttpScStreamUpServerSecsMax);

        const auto &pad = xhttp.xPadding;
        xo[px::xPaddingObfsMode] = pad.obfsMode;
        if (pad.obfsMode) {
            if (!pad.bytesMin.isEmpty() || !pad.bytesMax.isEmpty()) {
                const int fromV = pad.bytesMin.isEmpty() ? QString::fromLatin1(px::defaultXPaddingBytesMin).toInt()
                                                         : pad.bytesMin.toInt();
                int toV = pad.bytesMax.isEmpty() ? QString::fromLatin1(px::defaultXPaddingBytesMax).toInt()
                                                 : pad.bytesMax.toInt();
                if (toV < fromV)
                    toV = fromV;
                xo[px::xPaddingBytes] = xrayEffective::range(QString::number(fromV), QString::number(toV));
            }
            xo[px::xPaddingKey] = pad.key.isEmpty() ? QString::fromLatin1(px::defaultXPaddingKey) : pad.key;
            xo[px::xPaddingHeader] = pad.header.isEmpty() ? QString::fromLatin1(px::defaultXPaddingHeader) : pad.header;
            xo[px::xPaddingPlacement] = xrayEffective::xPaddingPlacement(
                    pad.placement.isEmpty() ? QString::fromLatin1(px::defaultXPaddingPlacement) : pad.placement);
            xo[px::xPaddingMethod] = xrayEffective::xPaddingMethod(
                    pad.method.isEmpty() ? QString::fromLatin1(px::defaultXPaddingMethod) : pad.method);
        }

        if (clientSide && clientTemplate.xmux.enabled) {
            const XrayXmuxConfig &xmux = clientTemplate.xmux;
            QJsonObject mux;
            auto addMuxRange = [&mux](const char *key, const QString &from, const QString &to) {
                const bool fromZero = from.isEmpty() || from == QLatin1String("0");
                const bool toZero = to.isEmpty() || to == QLatin1String("0");
                if (fromZero && toZero)
                    return;
                mux[QString::fromUtf8(key)] = xrayEffective::range(from.isEmpty() ? QStringLiteral("0") : from,
                                                                   to.isEmpty() ? QStringLiteral("0") : to);
            };
            addMuxRange(px::xmuxMaxConcurrency, xmux.maxConcurrencyMin, xmux.maxConcurrencyMax);
            addMuxRange(px::xmuxMaxConnections, xmux.maxConnectionsMin, xmux.maxConnectionsMax);
            addMuxRange(px::xmuxCMaxReuseTimes, xmux.cMaxReuseTimesMin, xmux.cMaxReuseTimesMax);
            addMuxRange(px::xmuxHMaxRequestTimes, xmux.hMaxRequestTimesMin, xmux.hMaxRequestTimesMax);
            addMuxRange(px::xmuxHMaxReusableSecs, xmux.hMaxReusableSecsMin, xmux.hMaxReusableSecsMax);
            if (!xmux.hKeepAlivePeriod.isEmpty())
                mux[px::xmuxHKeepAlivePeriod] = xmux.hKeepAlivePeriod.toLongLong();
            if (!mux.isEmpty())
                xo[px::xmux] = mux;
        }

        streamSettings[px::xhttpSettings] = xo;
    }

    if (transport == QLatin1String(px::transportMkcp)) {
        auto clamped = [](const QString &value, const char *fallback, int lo, int hi) {
            const QString effective = value.isEmpty() ? QString::fromLatin1(fallback) : value;
            bool ok = false;
            int number = effective.toInt(&ok);
            if (!ok) {
                number = QString::fromLatin1(fallback).toInt();
            }
            return qBound(lo, number, hi);
        };

        QJsonObject kcpObj;
        const int mtuEff = clamped(mkcp.mtu, px::defaultMkcpMtu, 576, 1460);
        kcpObj[px::kcpTti] = clamped(mkcp.tti, px::defaultMkcpTti, 10, 100);
        kcpObj[px::kcpMtu] = mtuEff;
        kcpObj[px::kcpUplinkCapacity] =
                (mkcp.uplinkCapacity.isEmpty() ? QString::fromLatin1(px::defaultMkcpUplinkCapacity)
                                               : mkcp.uplinkCapacity).toInt();
        kcpObj[px::kcpDownlinkCapacity] =
                (mkcp.downlinkCapacity.isEmpty() ? QString::fromLatin1(px::defaultMkcpDownlinkCapacity)
                                                 : mkcp.downlinkCapacity).toInt();
        kcpObj[px::kcpCwndMultiplier] = clamped(mkcp.cwndMultiplier, px::defaultMkcpCwndMultiplier, 1, 2147483647);
        if (!mkcp.maxSendingWindow.isEmpty() && mkcp.maxSendingWindow.toInt() >= mtuEff) {
            kcpObj[px::kcpMaxSendingWindow] = mkcp.maxSendingWindow.toInt();
        }
        streamSettings[px::kcpSettings] = kcpObj;
    }

    return streamSettings;
}

namespace
{
    QString normalizedFingerprint(const QString &fingerprint)
    {
        if (fingerprint.isEmpty() || fingerprint.contains(QLatin1String("Mozilla/5.0"), Qt::CaseInsensitive)) {
            return QString::fromLatin1(protocols::xray::defaultFingerprint);
        }
        return fingerprint;
    }

    void parseIntRange(const QJsonValue &value, QString &minOut, QString &maxOut)
    {
        namespace px = protocols::xray;

        if (value.isString()) {
            const QString text = value.toString().trimmed();
            const int dash = text.indexOf(QLatin1Char('-'), 1);
            if (dash > 0) {
                minOut = text.left(dash).trimmed();
                maxOut = text.mid(dash + 1).trimmed();
            } else if (!text.isEmpty()) {
                minOut = text;
                maxOut = text;
            }
        } else if (value.isDouble()) {
            minOut = QString::number(value.toInt());
            maxOut = minOut;
        } else if (value.isObject()) {
            const QJsonObject range = value.toObject();
            if (range.contains(px::legacyRangeFrom) || range.contains(px::legacyRangeTo)) {
                minOut = QString::number(range.value(px::legacyRangeFrom).toInt());
                maxOut = QString::number(range.value(px::legacyRangeTo).toInt());
            }
        }
    }

    QString xhttpModeUi(const QString &coreMode)
    {
        if (coreMode.isEmpty() || coreMode == QLatin1String("auto"))
            return QStringLiteral("Auto");
        if (coreMode == QLatin1String("packet-up"))
            return QStringLiteral("Packet-up");
        if (coreMode == QLatin1String("stream-up"))
            return QStringLiteral("Stream-up");
        if (coreMode == QLatin1String("stream-one"))
            return QStringLiteral("Stream-one");
        return coreMode;
    }

    QString sessionSeqPlacementUi(const QString &corePlacement)
    {
        if (corePlacement.isEmpty() || corePlacement == QLatin1String("path"))
            return QStringLiteral("Path");
        if (corePlacement == QLatin1String("cookie"))
            return QStringLiteral("Cookie");
        if (corePlacement == QLatin1String("header"))
            return QStringLiteral("Header");
        if (corePlacement == QLatin1String("query"))
            return QStringLiteral("Query");
        return corePlacement;
    }

    QString uplinkDataPlacementUi(const QString &corePlacement)
    {
        if (corePlacement.isEmpty() || corePlacement == QLatin1String("body"))
            return QStringLiteral("Body");
        if (corePlacement == QLatin1String("auto"))
            return QStringLiteral("Auto");
        if (corePlacement == QLatin1String("header"))
            return QStringLiteral("Header");
        if (corePlacement == QLatin1String("cookie"))
            return QStringLiteral("Cookie");
        return corePlacement;
    }

    void readPaddingFromObject(const QJsonObject &padding, XrayXPaddingConfig &out)
    {
        namespace px = protocols::xray;

        if (padding.contains(px::xPaddingObfsMode))
            out.obfsMode = padding.value(px::xPaddingObfsMode).toBool(true);
        out.key = padding.value(px::xPaddingKey).toString();
        out.header = padding.value(px::xPaddingHeader).toString();
        out.placement = padding.value(px::xPaddingPlacement).toString();
        out.method = padding.value(px::xPaddingMethod).toString();

        QString bytesMin;
        QString bytesMax;
        parseIntRange(padding.value(px::xPaddingBytes), bytesMin, bytesMax);
        if (!bytesMin.isEmpty()) {
            out.bytesMin = bytesMin;
            out.bytesMax = bytesMax;
        }

        const QString placement = out.placement.toLower();
        if (placement == QLatin1String("cookie"))
            out.placement = QStringLiteral("Cookie");
        else if (placement == QLatin1String("header"))
            out.placement = QStringLiteral("Header");
        else if (placement == QLatin1String("query"))
            out.placement = QStringLiteral("Query");
        else if (placement == QLatin1String("queryinheader"))
            out.placement = QStringLiteral("Query in header");

        const QString method = out.method.toLower();
        if (method == QLatin1String("repeat-x"))
            out.method = QStringLiteral("Repeat-x");
        else if (method == QLatin1String("tokenish"))
            out.method = QStringLiteral("Tokenish");
    }

    QString jsonPortToString(const QJsonValue &value)
    {
        if (value.isString()) {
            const QString text = value.toString().trimmed();
            if (!text.isEmpty()) {
                return text;
            }
        }
        if (value.isDouble()) {
            return QString::number(value.toInt());
        }
        return {};
    }

    QString hostFromDest(const QString &dest)
    {
        const QString trimmed = dest.trimmed();
        const int colon = trimmed.lastIndexOf(QLatin1Char(':'));
        if (colon > 0) {
            return trimmed.left(colon);
        }
        return trimmed;
    }

    void applyStreamSettingsFromJson(const QJsonObject &streamSettings, XrayServerConfig &srv,
                                     XrayClientTemplate &tpl)
    {
        namespace px = protocols::xray;

        const QString networkVal = streamSettings.value(px::network).toString(px::networkTcp);
        if (networkVal == QLatin1String(px::networkXhttp)) {
            srv.transport = px::transportXhttp;
        } else if (networkVal == QLatin1String(px::networkKcp)) {
            srv.transport = px::transportMkcp;
        } else {
            srv.transport = px::transportRaw;
        }

        srv.security = streamSettings.value(px::security).toString(px::securityReality);


        if (srv.security == QLatin1String(px::securityReality)) {
            const QJsonObject realitySettings = streamSettings.value(px::realitySettings).toObject();
            if (realitySettings.contains(px::dest)) {
                const QString siteFromDest = hostFromDest(realitySettings.value(px::dest).toString());
                if (!siteFromDest.isEmpty()) {
                    srv.site = siteFromDest;
                }
            }
            if (realitySettings.contains(px::serverNames)) {
                srv.sni = realitySettings.value(px::serverNames).toArray().first().toString();
            } else if (realitySettings.contains(px::serverName)) {
                srv.sni = realitySettings.value(px::serverName).toString();
            }
            if (srv.site.isEmpty() && !srv.sni.isEmpty()) {
                srv.site = srv.sni;
            }
            if (realitySettings.contains(px::fingerprint)) {
                tpl.fingerprint = normalizedFingerprint(realitySettings.value(px::fingerprint).toString());
            }
        }

        if (srv.security == QLatin1String(px::securityTls)) {
            const QJsonObject tls = streamSettings.value(px::tlsSettings).toObject();
            if (tls.contains(px::serverName)) {
                srv.sni = tls.value(px::serverName).toString();
            }
            if (tls.contains(px::fingerprint)) {
                tpl.fingerprint = normalizedFingerprint(tls.value(px::fingerprint).toString());
            }

            QStringList alpnList;
            for (const QJsonValue &value : tls.value(px::alpn).toArray()) {
                QString entry = value.toString().trimmed();
                if (entry.compare(QLatin1String("HTTP/2"), Qt::CaseInsensitive) == 0)
                    entry = QStringLiteral("h2");
                else if (entry.compare(QLatin1String("HTTP/1.1"), Qt::CaseInsensitive) == 0)
                    entry = QStringLiteral("http/1.1");
                if (!entry.isEmpty())
                    alpnList << entry;
            }
            srv.alpn = alpnList.join(QLatin1Char(','));
        }

        if (srv.transport == QLatin1String(px::transportXhttp)) {
            const QJsonObject xhttpObj = streamSettings.value(px::xhttpSettings).toObject();

            srv.xhttp.mode = coerceXhttpModeToSupported(xhttpModeUi(xhttpObj.value(px::xhttpMode).toString()));
            srv.xhttp.host = xhttpObj.value(px::xhttpHost).toString();
            srv.xhttp.path = xhttpObj.value(px::xhttpPath).toString();

            if (xhttpObj.contains(px::uplinkHttpMethod)) {
                tpl.uplinkMethod = xhttpObj.value(px::uplinkHttpMethod).toString();
            } else if (xhttpObj.contains(px::legacyXhttpMethod)) {
                tpl.uplinkMethod = xhttpObj.value(px::legacyXhttpMethod).toString();
            }

            if (xhttpObj.contains(px::noGrpcHeader)) {
                srv.xhttp.disableGrpc = xhttpObj.value(px::noGrpcHeader).toBool(true);
            }
            srv.xhttp.disableSse = xhttpObj.value(px::noSseHeader).toBool(true);

            QString sessionPlacement = xhttpObj.value(px::sessionIdPlacement).toString();
            if (sessionPlacement.isEmpty())
                sessionPlacement = xhttpObj.value(px::legacySessionPlacement).toString();
            if (sessionPlacement.isEmpty())
                sessionPlacement = xhttpObj.value(px::legacyScSessionPlacement).toString();
            srv.xhttp.sessionPlacement = sessionSeqPlacementUi(sessionPlacement);

            QString seqPlacement = xhttpObj.value(px::seqPlacement).toString();
            if (seqPlacement.isEmpty())
                seqPlacement = xhttpObj.value(px::legacyScSeqPlacement).toString();
            srv.xhttp.seqPlacement = sessionSeqPlacementUi(seqPlacement);

            QString uplinkPlacement = xhttpObj.value(px::uplinkDataPlacement).toString();
            if (uplinkPlacement.isEmpty())
                uplinkPlacement = xhttpObj.value(px::legacyScUplinkDataPlacement).toString();
            srv.xhttp.uplinkDataPlacement =
                    coerceUplinkDataPlacementToSupported(uplinkDataPlacementUi(uplinkPlacement));

            srv.xhttp.sessionKey = xhttpObj.value(px::sessionIdKey).toString();
            if (srv.xhttp.sessionKey.isEmpty())
                srv.xhttp.sessionKey = xhttpObj.value(px::legacySessionKey).toString();
            srv.xhttp.seqKey = xhttpObj.value(px::seqKey).toString();
            srv.xhttp.uplinkDataKey = xhttpObj.value(px::uplinkDataKey).toString();

            if (xhttpObj.contains(px::uplinkChunkSize)) {
                QString chunkMin;
                QString chunkMax;
                parseIntRange(xhttpObj.value(px::uplinkChunkSize), chunkMin, chunkMax);
                if (!chunkMin.isEmpty())
                    tpl.uplinkChunkSize = chunkMin;
            } else if (xhttpObj.contains(px::legacyUplinkChunkSize)) {
                tpl.uplinkChunkSize = QString::number(xhttpObj.value(px::legacyUplinkChunkSize).toInt());
            }

            if (xhttpObj.contains(px::scMaxBufferedPosts)) {
                srv.xhttp.scMaxBufferedPosts =
                        QString::number(xhttpObj.value(px::scMaxBufferedPosts).toVariant().toLongLong());
            }

            parseIntRange(xhttpObj.value(px::scMaxEachPostBytes), srv.xhttp.scMaxEachPostBytesMin,
                          srv.xhttp.scMaxEachPostBytesMax);
            if (xhttpObj.contains(px::scMinPostsIntervalMs)) {
                parseIntRange(xhttpObj.value(px::scMinPostsIntervalMs), tpl.scMinPostsIntervalMsMin,
                              tpl.scMinPostsIntervalMsMax);
            }
            parseIntRange(xhttpObj.value(px::scStreamUpServerSecs), srv.xhttp.scStreamUpServerSecsMin,
                          srv.xhttp.scStreamUpServerSecsMax);

            if (xhttpObj.contains(px::xPaddingObfsMode) || xhttpObj.contains(px::xPaddingKey)
                || xhttpObj.contains(px::xPaddingBytes)) {
                readPaddingFromObject(xhttpObj, srv.xhttp.xPadding);
            } else if (xhttpObj.value(px::legacyXPaddingBlock).isObject()) {
                const QJsonObject nested = xhttpObj.value(px::legacyXPaddingBlock).toObject();
                if (!nested.isEmpty()) {
                    readPaddingFromObject(nested, srv.xhttp.xPadding);
                    if (!nested.contains(px::xPaddingObfsMode))
                        srv.xhttp.xPadding.obfsMode = true;
                }
            }

            if (xhttpObj.contains(px::xmux)) {
                const QJsonObject mux = xhttpObj.value(px::xmux).toObject();
                tpl.xmux.enabled = true;
                parseIntRange(mux.value(px::xmuxMaxConcurrency), tpl.xmux.maxConcurrencyMin,
                              tpl.xmux.maxConcurrencyMax);
                parseIntRange(mux.value(px::xmuxMaxConnections), tpl.xmux.maxConnectionsMin,
                              tpl.xmux.maxConnectionsMax);
                parseIntRange(mux.value(px::xmuxCMaxReuseTimes), tpl.xmux.cMaxReuseTimesMin, tpl.xmux.cMaxReuseTimesMax);
                parseIntRange(mux.value(px::xmuxHMaxRequestTimes), tpl.xmux.hMaxRequestTimesMin,
                              tpl.xmux.hMaxRequestTimesMax);
                parseIntRange(mux.value(px::xmuxHMaxReusableSecs), tpl.xmux.hMaxReusableSecsMin,
                              tpl.xmux.hMaxReusableSecsMax);
                if (mux.contains(px::xmuxHKeepAlivePeriod)) {
                    tpl.xmux.hKeepAlivePeriod =
                            QString::number(mux.value(px::xmuxHKeepAlivePeriod).toVariant().toLongLong());
                }
            }
        }

        if (srv.transport == QLatin1String(px::transportMkcp)) {
            const QJsonObject kcp = streamSettings.value(px::kcpSettings).toObject();
            if (kcp.contains(px::kcpTti)) {
                srv.mkcp.tti = QString::number(kcp.value(px::kcpTti).toInt());
            }
            if (kcp.contains(px::kcpUplinkCapacity)) {
                srv.mkcp.uplinkCapacity = QString::number(kcp.value(px::kcpUplinkCapacity).toInt());
            }
            if (kcp.contains(px::kcpDownlinkCapacity)) {
                srv.mkcp.downlinkCapacity = QString::number(kcp.value(px::kcpDownlinkCapacity).toInt());
            }
            if (kcp.contains(px::kcpMtu)) {
                srv.mkcp.mtu = QString::number(kcp.value(px::kcpMtu).toInt());
            }
            if (kcp.contains(px::kcpCwndMultiplier)) {
                srv.mkcp.cwndMultiplier = QString::number(kcp.value(px::kcpCwndMultiplier).toInt());
            }
            if (kcp.contains(px::kcpMaxSendingWindow)) {
                srv.mkcp.maxSendingWindow = QString::number(kcp.value(px::kcpMaxSendingWindow).toInt());
            }
        }
    }
}

QJsonObject XrayServerConfig::toServerInboundJson(const XrayServerInboundInputs &inputs) const
{
    namespace px = protocols::xray;

    QJsonObject streamSettings = serverStreamSettings();
    if (xrayEffective::security(*this) == QLatin1String(px::securityReality)) {
        const QString siteEff = site.isEmpty() ? QString::fromLatin1(px::defaultSite) : site;
        QJsonObject realitySettings;
        realitySettings[px::dest] = siteEff + QStringLiteral(":443");
        realitySettings[px::privateKey] = inputs.realityPrivateKey;
        realitySettings[px::serverNames] = QJsonArray { sni.isEmpty() ? siteEff : sni };
        realitySettings[px::shortIds] = QJsonArray { inputs.realityShortId };
        streamSettings[px::realitySettings] = realitySettings;
    }

    QJsonObject settings;
    settings[px::clients] = inputs.clients;
    settings[px::decryption] = QString::fromLatin1(px::decryptionNone);

    QJsonObject inbound;
    inbound[px::port] = port.isEmpty() ? QString::fromLatin1(px::defaultPort).toInt() : port.toInt();
    inbound[px::protocol] = QString::fromLatin1(px::protocolVless);
    inbound[px::settings] = settings;
    inbound[px::streamSettings] = streamSettings;

    QJsonObject serverJson;
    serverJson[px::logBlock] = QJsonObject { { px::logLevel, px::logLevelError } };
    serverJson[px::inbounds] = QJsonArray { inbound };
    serverJson[px::outbounds] = QJsonArray { QJsonObject { { px::protocol, px::protocolFreedom } } };

    return serverJson;
}

XrayServerJsonStatus XrayServerConfig::fromServerInboundJson(const QJsonObject &serverJson,
                                                             XrayServerConfig &outServerConfig,
                                                             XrayClientTemplate &outClientTemplate)
{
    namespace px = protocols::xray;

    if (!serverJson.contains(px::inbounds)) {
        return XrayServerJsonStatus::MissingInbounds;
    }
    const QJsonArray inbounds = serverJson.value(px::inbounds).toArray();
    if (inbounds.isEmpty()) {
        return XrayServerJsonStatus::EmptyInbounds;
    }

    const QJsonObject inbound = inbounds[0].toObject();
    if (!inbound.contains(px::streamSettings)) {
        return XrayServerJsonStatus::MissingStreamSettings;
    }
    const QJsonObject streamSettings = inbound.value(px::streamSettings).toObject();

    XrayServerConfig &srv = outServerConfig;
    XrayClientTemplate &tpl = outClientTemplate;

    if (inbound.contains(px::port)) {
        srv.port = QString::number(inbound.value(px::port).toInt());
    }

    applyStreamSettingsFromJson(streamSettings, srv, tpl);

    if (inbound.contains(px::settings)) {
        const QJsonArray clients = inbound.value(px::settings).toObject().value(px::clients).toArray();
        if (!clients.isEmpty()) {
            srv.flow = clients[0].toObject().value(px::flow).toString();
        }
    }

    return XrayServerJsonStatus::Ok;
}

QJsonArray XrayServerConfig::clientsFromServerInboundJson(const QJsonObject &serverJson)
{
    namespace px = protocols::xray;

    const QJsonArray inbounds = serverJson.value(px::inbounds).toArray();
    if (inbounds.isEmpty()) {
        return {};
    }
    return inbounds[0].toObject().value(px::settings).toObject().value(px::clients).toArray();
}

XrayServerJsonStatus XrayServerConfig::setClientsInServerInboundJson(QJsonObject &serverJson,
                                                                     const QJsonArray &clients)
{
    namespace px = protocols::xray;

    if (!serverJson.contains(px::inbounds)) {
        return XrayServerJsonStatus::MissingInbounds;
    }
    QJsonArray inbounds = serverJson.value(px::inbounds).toArray();
    if (inbounds.isEmpty()) {
        return XrayServerJsonStatus::EmptyInbounds;
    }

    QJsonObject inbound = inbounds[0].toObject();
    if (!inbound.contains(px::settings)) {
        return XrayServerJsonStatus::MissingSettings;
    }

    QJsonObject settings = inbound.value(px::settings).toObject();
    settings[px::clients] = clients;
    inbound[px::settings] = settings;
    inbounds[0] = inbound;
    serverJson[px::inbounds] = inbounds;

    return XrayServerJsonStatus::Ok;
}

QJsonObject XrayServerConfig::makeClientEntry(const QString &clientId, const QString &flowValue)
{
    namespace px = protocols::xray;

    QJsonObject client;
    client[px::id] = clientId;
    if (!flowValue.isEmpty()) {
        client[px::flow] = flowValue;
    }
    return client;
}

QJsonObject XrayServerConfig::applyFlowToClient(const QJsonObject &client, const QString &flowValue)
{
    namespace px = protocols::xray;

    QJsonObject updated = client;
    if (flowValue.isEmpty()) {
        updated.remove(px::flow);
    } else {
        updated[px::flow] = flowValue;
    }
    return updated;
}

int XrayServerConfig::indexOfClient(const QJsonArray &clients, const QString &clientId)
{
    namespace px = protocols::xray;

    if (clientId.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < clients.size(); ++i) {
        if (clients[i].toObject().value(px::id).toString() == clientId) {
            return i;
        }
    }
    return -1;
}

QString XrayServerConfig::firstClientId(const QJsonArray &clients)
{
    namespace px = protocols::xray;

    if (clients.isEmpty()) {
        return {};
    }
    return clients[0].toObject().value(px::id).toString();
}

QJsonArray XrayServerConfig::applyFlowToClients(const QJsonArray &clients, const QString &flowValue,
                                                 XrayClientListFilter filter)
{
    namespace px = protocols::xray;

    QJsonArray updated;
    for (const QJsonValue &value : clients) {
        const QJsonObject client = value.toObject();
        if (filter == XrayClientListFilter::DropWithoutId && client.value(px::id).toString().isEmpty()) {
            continue;
        }
        updated.append(applyFlowToClient(client, flowValue));
    }
    return updated;
}

QJsonObject XrayServerConfig::serverView() const
{
    namespace px = protocols::xray;

    QJsonObject view;
    view[QStringLiteral("port")] = port.isEmpty() ? QString::fromLatin1(px::defaultPort) : port;
    view[QStringLiteral("transportProto")] = transportProto;

    view[px::flow] = xrayEffective::clientFlow(*this);

    view[px::streamSettings] = serverStreamSettings();

    if (xrayEffective::security(*this) == QLatin1String(px::securityReality)) {
        const QString siteEff = site.isEmpty() ? QString::fromLatin1(px::defaultSite) : site;
        QJsonObject reality;
        reality[px::dest] = siteEff;
        reality[px::serverName] = sni.isEmpty() ? siteEff : sni;
        view[QStringLiteral("reality")] = reality;
    }

    return view;
}

QJsonObject XrayServerConfig::issuedConfigView() const
{
    namespace px = protocols::xray;

    QJsonObject view = serverView();
    view.remove(QStringLiteral("transportProto"));

    QJsonObject stream = view.value(px::streamSettings).toObject();
    if (stream.contains(px::xhttpSettings)) {
        QJsonObject xo = stream.value(px::xhttpSettings).toObject();
        xo.remove(px::scMaxBufferedPosts);
        xo.remove(px::scStreamUpServerSecs);
        stream[px::xhttpSettings] = xo;
    }

    stream.remove(px::kcpSettings);
    view[px::streamSettings] = stream;

    return view;
}

bool XrayServerConfig::breaksIssuedConfigs(const XrayServerConfig &other) const
{
    XrayServerConfig a = *this;
    XrayServerConfig b = other;
    a.applyDefaults();
    b.applyDefaults();

    return a.issuedConfigView() != b.issuedConfigView();
}

bool XrayServerConfig::hasEqualServerSettings(const XrayServerConfig &other) const
{
    XrayServerConfig a = *this;
    XrayServerConfig b = other;
    a.applyDefaults();
    b.applyDefaults();

    return a.serverView() == b.serverView();
}

QJsonObject XrayClientConfig::toJson() const
{
    QJsonObject obj;
    if (!nativeConfig.isEmpty()) obj[configKey::config]   = nativeConfig;
    if (!localPort.isEmpty())    obj[configKey::localPort] = localPort;
    if (!id.isEmpty())           obj[configKey::clientId]  = id;
    if (!templateFingerprint.isEmpty()) obj[QStringLiteral("template_fingerprint")] = templateFingerprint;
    return obj;
}

XrayClientConfig XrayClientConfig::fromJson(const QJsonObject &json)
{
    XrayClientConfig c;
    c.nativeConfig = json.value(configKey::config).toString();
    c.localPort    = json.value(configKey::localPort).toString();
    c.id           = json.value(configKey::clientId).toString();
    c.templateFingerprint = json.value(QStringLiteral("template_fingerprint")).toString();

    if (c.id.isEmpty() && !c.nativeConfig.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(c.nativeConfig.toUtf8());
        if (!doc.isNull() && doc.isObject()) {
            c.id = idFromNativeJson(doc.object());
        }
    }

    return c;
}

QString XrayClientConfig::idFromNativeJson(const QJsonObject &nativeJson)
{
    namespace px = protocols::xray;

    const QJsonArray outbounds = nativeJson.value(px::outbounds).toArray();
    if (outbounds.isEmpty()) {
        return {};
    }
    const QJsonArray vnext =
            outbounds[0].toObject().value(px::settings).toObject().value(px::vnext).toArray();
    if (vnext.isEmpty()) {
        return {};
    }
    const QJsonArray users = vnext[0].toObject().value(px::users).toArray();
    if (users.isEmpty()) {
        return {};
    }
    return users[0].toObject().value(px::id).toString();
}

QString XrayClientConfig::localPortFromNativeJson(const QJsonObject &nativeJson)
{
    namespace px = protocols::xray;

    const QJsonArray inbounds = nativeJson.value(px::inbounds).toArray();
    if (inbounds.isEmpty()) {
        return {};
    }
    const QJsonObject inbound = inbounds[0].toObject();
    if (!inbound.contains(px::port)) {
        return {};
    }
    return jsonPortToString(inbound.value(px::port));
}

QJsonObject XrayProtocolConfig::toJson() const
{
    QJsonObject obj = serverConfig.toJson();
    obj[QStringLiteral("client_template")] = clientTemplate.toJson();

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
                clientCfg.localPort = XrayClientConfig::localPortFromNativeJson(parsed);
                clientCfg.id = XrayClientConfig::idFromNativeJson(parsed);
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

    if (json.contains(QStringLiteral("client_template"))) {
        c.clientTemplate = XrayClientTemplate::fromJson(json.value(QStringLiteral("client_template")).toObject());
        c.needsTemplateMaterialization = c.clientTemplate.formatVersion == 0;
    } else {
        c.needsTemplateMaterialization = true;
    }
    if (c.needsTemplateMaterialization) {
        c.clientTemplate.materializeFromLegacy(json);
        c.stampClientTemplateFormatVersion();
    }

    return c;
}

bool XrayProtocolConfig::stampClientTemplateFormatVersion()
{
    if (!needsTemplateMaterialization) {
        return false;
    }

    clientTemplate.formatVersion = 1;

    needsTemplateMaterialization = false;
    return true;
}

QJsonObject XrayProtocolConfig::toClientOutboundJson(const XrayClientOutboundInputs &inputs) const
{
    namespace px = protocols::xray;

    const XrayServerConfig &srv = serverConfig;

    QJsonObject userObj;
    userObj[px::id] = inputs.clientId;
    userObj[px::encryption] = QString::fromLatin1(px::decryptionNone);
    const QString flowValue = xrayEffective::clientFlow(srv);
    if (!flowValue.isEmpty()) {
        userObj[px::flow] = flowValue;
    }

    QJsonObject vnextEntry;
    vnextEntry[px::address] = inputs.serverAddress;
    vnextEntry[px::port] = srv.port.isEmpty() ? QString::fromLatin1(px::defaultPort).toInt() : srv.port.toInt();
    vnextEntry[px::users] = QJsonArray { userObj };

    QJsonObject outboundSettings;
    outboundSettings[px::vnext] = QJsonArray { vnextEntry };

    QJsonObject streamSettings = srv.clientStreamSettings(clientTemplate);
    if (xrayEffective::security(srv) == QLatin1String(px::securityReality)) {
        QJsonObject realitySettings = streamSettings.value(px::realitySettings).toObject();
        realitySettings[px::publicKey] = inputs.realityPublicKey;
        realitySettings[px::shortId] = inputs.realityShortId;
        realitySettings[px::spiderX] = QString();
        streamSettings[px::realitySettings] = realitySettings;
    }
    if (xrayEffective::security(srv) == QLatin1String(px::securityTls) && !srv.isThirdPartyConfig
        && !inputs.tlsPinnedPeerCertSha256.isEmpty()) {
        QJsonObject tlsSettings = streamSettings.value(px::tlsSettings).toObject();
        tlsSettings[px::pinnedPeerCertSha256] = inputs.tlsPinnedPeerCertSha256;
        streamSettings[px::tlsSettings] = tlsSettings;
    }

    QJsonObject outbound;
    outbound[px::protocol] = QString::fromLatin1(px::protocolVless);
    outbound[px::settings] = outboundSettings;
    outbound[px::streamSettings] = streamSettings;

    QJsonObject inbound;
    inbound[px::listen] = px::defaultLocalListenAddr;
    inbound[px::port] = QString::fromLatin1(px::defaultLocalProxyPort).toInt();
    inbound[px::protocol] = QString::fromLatin1(px::protocolSocks);
    inbound[px::settings] = QJsonObject { { px::udp, true } };

    QJsonObject clientJson;
    clientJson[px::logBlock] = QJsonObject { { px::logLevel, px::logLevelError } };
    clientJson[px::inbounds] = QJsonArray { inbound };
    clientJson[px::outbounds] = QJsonArray { outbound };

    const QString securityEff = xrayEffective::security(srv);
    return clientJson;
}

bool XrayProtocolConfig::hydrateServerConfigFromClientNative()
{
    if (!clientConfig.has_value() || clientConfig->nativeConfig.isEmpty()) {
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(clientConfig->nativeConfig.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    if (!fromClientOutboundJson(doc.object())) {
        return false;
    }

    needsClientHydration = false;
    return true;
}

bool XrayProtocolConfig::fromClientOutboundJson(const QJsonObject &nativeJson)
{
    namespace px = protocols::xray;

    const QJsonArray outbounds = nativeJson.value(px::outbounds).toArray();
    if (outbounds.isEmpty()) {
        return false;
    }

    const QJsonObject outbound = outbounds[0].toObject();
    const QJsonObject streamSettings = outbound.value(px::streamSettings).toObject();
    if (streamSettings.isEmpty()) {
        return false;
    }

    XrayServerConfig &srv = serverConfig;

    if (clientConfig.has_value()) {
        if (clientConfig->id.isEmpty()) {
            clientConfig->id = XrayClientConfig::idFromNativeJson(nativeJson);
        }
        if (clientConfig->localPort.isEmpty()) {
            clientConfig->localPort = XrayClientConfig::localPortFromNativeJson(nativeJson);
        }
    }

    const QJsonObject settings = outbound.value(px::settings).toObject();
    const QJsonArray vnext = settings.value(px::vnext).toArray();
    if (!vnext.isEmpty()) {
        const QJsonObject vnextEntry = vnext[0].toObject();
        if (vnextEntry.contains(px::port)) {
            srv.port = QString::number(vnextEntry.value(px::port).toInt());
        }
        const QJsonArray users = vnextEntry.value(px::users).toArray();
        if (!users.isEmpty()) {
            srv.flow = users[0].toObject().value(px::flow).toString();
        }
    }

    applyStreamSettingsFromJson(streamSettings, srv, clientTemplate);

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
