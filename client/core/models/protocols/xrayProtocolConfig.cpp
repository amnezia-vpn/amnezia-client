#include "xrayProtocolConfig.h"

#include <QCryptographicHash>
#include <QDateTime>
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
    obj[QStringLiteral("server_fingerprint")] = serverFingerprint;
    obj[QStringLiteral("updated_at")] = updatedAt;
    obj[QStringLiteral("pending_server_upload")] = pendingServerUpload;
    return obj;
}

XrayClientTemplate XrayClientTemplate::fromJson(const QJsonObject &json)
{
    XrayClientTemplate c;
    c.formatVersion = json.value(QStringLiteral("format_version")).toInt(0);
    c.fingerprint = json.value(QStringLiteral("fingerprint")).toString();
    c.uplinkMethod = json.value(QStringLiteral("uplink_method")).toString();
    c.uplinkChunkSize = json.value(QStringLiteral("uplink_chunk_size")).toString();
    c.scMinPostsIntervalMsMin = json.value(QStringLiteral("sc_min_posts_interval_ms_min")).toString();
    c.scMinPostsIntervalMsMax = json.value(QStringLiteral("sc_min_posts_interval_ms_max")).toString();
    c.xmux = XrayXmuxConfig::fromJson(json.value(QStringLiteral("xmux")).toObject());
    c.serverFingerprint = json.value(QStringLiteral("server_fingerprint")).toString();
    c.updatedAt = json.value(QStringLiteral("updated_at")).toString();
    c.pendingServerUpload = json.value(QStringLiteral("pending_server_upload")).toBool(false);
    return c;
}

QString XrayClientTemplate::contentFingerprint() const
{
    QJsonObject content = toJson();
    content.remove(QStringLiteral("server_fingerprint"));
    content.remove(QStringLiteral("updated_at"));
    content.remove(QStringLiteral("format_version"));
    content.remove(QStringLiteral("pending_server_upload"));

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

    if (xhttp.mode.compare(QLatin1String("Auto"), Qt::CaseInsensitive) == 0
        || xhttp.mode.compare(QLatin1String("Packet-up"), Qt::CaseInsensitive) == 0) {
        xhttp.mode = QString::fromLatin1(protocols::xray::defaultXhttpMode);
    }
    if (xhttp.uplinkDataPlacement.compare(QLatin1String("Header"), Qt::CaseInsensitive) == 0
        || xhttp.uplinkDataPlacement.compare(QLatin1String("Cookie"), Qt::CaseInsensitive) == 0) {
        xhttp.uplinkDataPlacement = QString::fromLatin1(protocols::xray::defaultXhttpUplinkDataPlacement);
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
        if (placement.compare(QLatin1String("Query"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("header");
        return placement.toLower();
    }

    QString xPaddingPlacement(const QString &placement)
    {
        QString t = placement.trimmed();
        if (t.isEmpty())
            return QString::fromLatin1(protocols::xray::defaultXPaddingPlacement).toLower();
        if (t.compare(QLatin1String("Body"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("queryInHeader");
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
        if (srv.transport == QLatin1String("mkcp") && srv.security == QLatin1String("reality")) {
            return QStringLiteral("none");
        }
        return srv.security;
    }

    QString clientFlow(const XrayServerConfig &srv)
    {
        const bool rawTransport = srv.transport.isEmpty() || srv.transport == QLatin1String("raw");
        const bool secureFlow = srv.security == QLatin1String("tls") || srv.security == QLatin1String("reality");
        return (rawTransport && secureFlow) ? srv.flow : QString();
    }

    QString network(const XrayServerConfig &srv)
    {
        if (srv.transport == QLatin1String("xhttp"))
            return QStringLiteral("xhttp");
        if (srv.transport == QLatin1String("mkcp"))
            return QStringLiteral("kcp");
        return QStringLiteral("tcp");
    }

    QString xhttpModeSent(const XrayServerConfig &srv)
    {
        QString mode = xhttpMode(srv.xhttp.mode);
        if (mode == QLatin1String("auto") || mode == QLatin1String("packet-up")) {
            mode = QStringLiteral("stream-one");
        }
        return mode;
    }
} // namespace xrayEffective

QJsonObject XrayServerConfig::serverStreamSettings() const
{
    namespace px = protocols::xray;

    QJsonObject streamSettings;
    streamSettings[px::network] = xrayEffective::network(*this);

    const QString securityEff = xrayEffective::security(*this);
    streamSettings[px::security] = securityEff;

    if (securityEff == QLatin1String("tls")) {
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
            tlsSettings[QStringLiteral("alpn")] = alpnArray;
        streamSettings[QStringLiteral("tlsSettings")] = tlsSettings;
    }

    if (transport == QLatin1String("xhttp")) {
        QJsonObject xo;
        xo[QStringLiteral("host")] = xhttp.host.isEmpty() ? QString::fromLatin1(px::defaultXhttpHost) : xhttp.host;
        if (!xhttp.path.isEmpty())
            xo[QStringLiteral("path")] = xhttp.path;
        const QString modeEff = xrayEffective::xhttpModeSent(*this);
        xo[QStringLiteral("mode")] = modeEff;
        xo[QStringLiteral("noGRPCHeader")] = xhttp.disableGrpc;
        xo[QStringLiteral("noSSEHeader")] = xhttp.disableSse;

        const QString sessPl = xrayEffective::sessionSeqPlacement(xhttp.sessionPlacement);
        if (!sessPl.isEmpty())
            xo[QStringLiteral("sessionIDPlacement")] = sessPl;
        const QString seqPl = xrayEffective::sessionSeqPlacement(xhttp.seqPlacement);
        if (!seqPl.isEmpty())
            xo[QStringLiteral("seqPlacement")] = seqPl;
        if (!xhttp.sessionKey.isEmpty())
            xo[QStringLiteral("sessionIDKey")] = xhttp.sessionKey;
        if (!xhttp.seqKey.isEmpty())
            xo[QStringLiteral("seqKey")] = xhttp.seqKey;

        const QString uDataPl = xrayEffective::uplinkDataPlacement(xhttp.uplinkDataPlacement);
        const bool uDataNeedsPacketUp = uDataPl == QLatin1String("header") || uDataPl == QLatin1String("cookie");
        if (!(uDataNeedsPacketUp && modeEff != QLatin1String("packet-up")))
            xo[QStringLiteral("uplinkDataPlacement")] = uDataPl;
        if (!xhttp.uplinkDataKey.isEmpty())
            xo[QStringLiteral("uplinkDataKey")] = xhttp.uplinkDataKey;

        if (!xhttp.scMaxBufferedPosts.isEmpty())
            xo[QStringLiteral("scMaxBufferedPosts")] = xhttp.scMaxBufferedPosts.toLongLong();
        xrayEffective::putRangeIfAny(xo, "scMaxEachPostBytes", xhttp.scMaxEachPostBytesMin,
                                     xhttp.scMaxEachPostBytesMax, px::defaultXhttpScMaxEachPostBytesMin,
                                     px::defaultXhttpScMaxEachPostBytesMax);
        xrayEffective::putRangeIfAny(xo, "scStreamUpServerSecs", xhttp.scStreamUpServerSecsMin,
                                     xhttp.scStreamUpServerSecsMax, px::defaultXhttpScStreamUpServerSecsMin,
                                     px::defaultXhttpScStreamUpServerSecsMax);

        const auto &pad = xhttp.xPadding;
        xo[QStringLiteral("xPaddingObfsMode")] = pad.obfsMode;
        if (pad.obfsMode) {
            if (!pad.bytesMin.isEmpty() || !pad.bytesMax.isEmpty()) {
                const int fromV = pad.bytesMin.isEmpty() ? QString::fromLatin1(px::defaultXPaddingBytesMin).toInt()
                                                         : pad.bytesMin.toInt();
                int toV = pad.bytesMax.isEmpty() ? QString::fromLatin1(px::defaultXPaddingBytesMax).toInt()
                                                 : pad.bytesMax.toInt();
                if (toV < fromV)
                    toV = fromV;
                xo[QStringLiteral("xPaddingBytes")] =
                        xrayEffective::range(QString::number(fromV), QString::number(toV));
            }
            xo[QStringLiteral("xPaddingKey")] = pad.key.isEmpty() ? QString::fromLatin1(px::defaultXPaddingKey) : pad.key;
            xo[QStringLiteral("xPaddingHeader")] =
                    pad.header.isEmpty() ? QString::fromLatin1(px::defaultXPaddingHeader) : pad.header;
            xo[QStringLiteral("xPaddingPlacement")] = xrayEffective::xPaddingPlacement(
                    pad.placement.isEmpty() ? QString::fromLatin1(px::defaultXPaddingPlacement) : pad.placement);
            xo[QStringLiteral("xPaddingMethod")] = xrayEffective::xPaddingMethod(
                    pad.method.isEmpty() ? QString::fromLatin1(px::defaultXPaddingMethod) : pad.method);
        }

        streamSettings[QStringLiteral("xhttpSettings")] = xo;
    }

    if (transport == QLatin1String("mkcp")) {
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
        const QString mtuEff = QString::number(clamped(mkcp.mtu, px::defaultMkcpMtu, 576, 1460));
        kcpObj[QStringLiteral("tti")] = clamped(mkcp.tti, px::defaultMkcpTti, 10, 100);
        kcpObj[QStringLiteral("mtu")] = mtuEff.toInt();
        kcpObj[QStringLiteral("uplinkCapacity")] =
                (mkcp.uplinkCapacity.isEmpty() ? QString::fromLatin1(px::defaultMkcpUplinkCapacity)
                                               : mkcp.uplinkCapacity).toInt();
        kcpObj[QStringLiteral("downlinkCapacity")] =
                (mkcp.downlinkCapacity.isEmpty() ? QString::fromLatin1(px::defaultMkcpDownlinkCapacity)
                                                 : mkcp.downlinkCapacity).toInt();
        kcpObj[QStringLiteral("cwndMultiplier")] =
                clamped(mkcp.cwndMultiplier, px::defaultMkcpCwndMultiplier, 1, 2147483647);
        if (!mkcp.maxSendingWindow.isEmpty() && mkcp.maxSendingWindow.toInt() >= mtuEff.toInt()) {
            kcpObj[QStringLiteral("maxSendingWindow")] = mkcp.maxSendingWindow.toInt();
        }
        streamSettings[QStringLiteral("kcpSettings")] = kcpObj;
    }

    return streamSettings;
}

QJsonObject XrayServerConfig::serverView() const
{
    namespace px = protocols::xray;

    QJsonObject view;
    view[QStringLiteral("port")] = port.isEmpty() ? QString::fromLatin1(px::defaultPort) : port;
    view[QStringLiteral("transportProto")] = transportProto;

    view[QStringLiteral("flow")] = xrayEffective::clientFlow(*this);

    view[QStringLiteral("streamSettings")] = serverStreamSettings();

    if (xrayEffective::security(*this) == QLatin1String("reality")) {
        const QString siteEff = site.isEmpty() ? QString::fromLatin1(px::defaultSite) : site;
        QJsonObject reality;
        reality[QStringLiteral("dest")] = siteEff;
        reality[QStringLiteral("serverName")] = sni.isEmpty() ? siteEff : sni;
        view[QStringLiteral("reality")] = reality;
    }

    return view;
}

QJsonObject XrayServerConfig::issuedConfigView() const
{
    QJsonObject view = serverView();
    view.remove(QStringLiteral("transportProto"));

    QJsonObject stream = view.value(QStringLiteral("streamSettings")).toObject();
    if (stream.contains(QStringLiteral("xhttpSettings"))) {
        QJsonObject xo = stream.value(QStringLiteral("xhttpSettings")).toObject();
        xo.remove(QStringLiteral("scMaxBufferedPosts"));
        xo.remove(QStringLiteral("scStreamUpServerSecs"));
        stream[QStringLiteral("xhttpSettings")] = xo;
    }

    stream.remove(QStringLiteral("kcpSettings"));
    view[QStringLiteral("streamSettings")] = stream;

    return view;
}

QString XrayServerConfig::sharedBlockFingerprint() const
{
    XrayServerConfig normalized = *this;
    normalized.applyDefaults();

    const QByteArray canonical = QJsonDocument(normalized.issuedConfigView()).toJson(QJsonDocument::Compact);
    return QString::fromLatin1(QCryptographicHash::hash(canonical, QCryptographicHash::Sha256).toHex());
}

bool XrayServerConfig::breaksIssuedConfigs(const XrayServerConfig &other) const
{
    XrayServerConfig a = *this;
    XrayServerConfig b = other;
    a.applyDefaults();
    b.applyDefaults();

    return a.issuedConfigView() != b.issuedConfigView();
}

QStringList XrayServerConfig::serverViewDifferences(const XrayServerConfig &other) const
{
    XrayServerConfig a = *this;
    XrayServerConfig b = other;
    a.applyDefaults();
    b.applyDefaults();

    const QJsonObject viewA = a.serverView();
    const QJsonObject viewB = b.serverView();

    QStringList differences;
    QStringList keys = viewA.keys();
    for (const QString &key : viewB.keys()) {
        if (!keys.contains(key)) {
            keys.append(key);
        }
    }
    keys.sort();

    for (const QString &key : keys) {
        const QJsonValue left = viewA.value(key);
        const QJsonValue right = viewB.value(key);
        if (left == right) {
            continue;
        }
        if (left.isObject() && right.isObject()) {
            const QJsonObject leftObj = left.toObject();
            const QJsonObject rightObj = right.toObject();
            QStringList innerKeys = leftObj.keys();
            for (const QString &innerKey : rightObj.keys()) {
                if (!innerKeys.contains(innerKey)) {
                    innerKeys.append(innerKey);
                }
            }
            innerKeys.sort();
            for (const QString &innerKey : innerKeys) {
                if (leftObj.value(innerKey) != rightObj.value(innerKey)) {
                    differences << key + QLatin1Char('.') + innerKey;
                }
            }
            continue;
        }
        differences << key;
    }

    return differences;
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

    if (json.contains(QStringLiteral("client_template"))) {
        c.clientTemplate = XrayClientTemplate::fromJson(json.value(QStringLiteral("client_template")).toObject());
        c.needsTemplateMaterialization = c.clientTemplate.formatVersion == 0;
    } else {
        c.needsTemplateMaterialization = true;
    }
    if (c.needsTemplateMaterialization) {
        c.clientTemplate.materializeFromLegacy(json);
        c.materializeTemplateFromServerConfig();
    }

    return c;
}

bool XrayProtocolConfig::materializeTemplateFromServerConfig()
{
    if (!needsTemplateMaterialization) {
        return false;
    }

    clientTemplate.formatVersion = 1;

    clientTemplate.serverFingerprint.clear();
    clientTemplate.formatVersion = 1;

    needsTemplateMaterialization = false;
    templateWasMaterialized = true;
    return true;
}

XrayTemplateSyncState XrayProtocolConfig::templateSyncState(bool serverReadable) const
{
    if (clientTemplate.formatVersion == 0) {
        return XrayTemplateSyncState::NoLocalCopy;
    }
    if (!serverReadable) {
        return XrayTemplateSyncState::Unknown;
    }
    if (clientTemplate.serverFingerprint.isEmpty()) {
        return XrayTemplateSyncState::InAgreement;
    }
    return clientTemplate.serverFingerprint == serverConfig.sharedBlockFingerprint()
            ? XrayTemplateSyncState::InAgreement
            : XrayTemplateSyncState::Drifted;
}

void XrayProtocolConfig::syncTemplateWithServer()
{
    clientTemplate.serverFingerprint = serverConfig.sharedBlockFingerprint();
    clientTemplate.updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (clientTemplate.formatVersion == 0) {
        clientTemplate.formatVersion = 1;
    }
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
            clientTemplate.fingerprint = fp.contains(QLatin1String("Mozilla/5.0"), Qt::CaseInsensitive)
                    ? QString::fromLatin1(protocols::xray::defaultFingerprint)
                    : fp;
        }
    }

    if (srv.security == QLatin1String("tls")) {
        const QJsonObject tls = streamSettings.value(QStringLiteral("tlsSettings")).toObject();
        srv.sni = tls.value(protocols::xray::serverName).toString();
        const QString fp = tls.value(protocols::xray::fingerprint).toString();
        if (!fp.isEmpty()) {
            clientTemplate.fingerprint = fp;
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
            clientTemplate.uplinkMethod = xhttpObj.value(QStringLiteral("uplinkHTTPMethod")).toString();
        }
        srv.xhttp.disableGrpc = xhttpObj.value(QStringLiteral("noGRPCHeader")).toBool(true);
        if (xhttpObj.contains(QStringLiteral("xmux"))) {
            clientTemplate.xmux = XrayXmuxConfig::fromJson(xhttpObj.value(QStringLiteral("xmux")).toObject());
        }
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
        if (kcpObj.contains(QStringLiteral("mtu"))) {
            mk.mtu = QString::number(kcpObj.value(QStringLiteral("mtu")).toInt());
        }
        if (kcpObj.contains(QStringLiteral("cwndMultiplier"))) {
            mk.cwndMultiplier = QString::number(kcpObj.value(QStringLiteral("cwndMultiplier")).toInt());
        }
        if (kcpObj.contains(QStringLiteral("maxSendingWindow"))) {
            mk.maxSendingWindow = QString::number(kcpObj.value(QStringLiteral("maxSendingWindow")).toInt());
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
