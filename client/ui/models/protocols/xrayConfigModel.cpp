#include "xrayConfigModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/networkUtilities.h"
#include "core/utils/containers/containerUtils.h"

#include <QHostAddress>
#include <QRegularExpression>

using namespace amnezia;
using namespace ProtocolUtils;

namespace
{
    QJsonObject comparableJson(amnezia::XrayServerConfig config)
    {
        if (config.transport != QLatin1String(protocols::xray::transportXhttp)) {
            config.xhttp = {};
        }
        if (config.transport != QLatin1String(protocols::xray::transportMkcp)) {
            config.mkcp = {};
        }
        auto &x = config.xhttp.xmux;
        for (QString *limit : { &x.maxConcurrencyMin, &x.maxConcurrencyMax, &x.maxConnectionsMin, &x.maxConnectionsMax,
                                &x.cMaxReuseTimesMin, &x.cMaxReuseTimesMax, &x.hMaxRequestTimesMin, &x.hMaxRequestTimesMax,
                                &x.hMaxReusableSecsMin, &x.hMaxReusableSecsMax }) {
            if (limit->isEmpty()) {
                *limit = QStringLiteral("0");
            }
        }
        return config.toJson();
    }
}

XrayConfigModel::XrayConfigModel(QObject* parent) : QObject(parent)
{
}

QString XrayConfigModel::site() const
{
    return m_protocolConfig.serverConfig.site;
}

void XrayConfigModel::setSite(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.site, value);
}

QString XrayConfigModel::port() const
{
    return m_protocolConfig.serverConfig.port;
}

void XrayConfigModel::setPort(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.port, value);
}

QString XrayConfigModel::transport() const
{
    return m_protocolConfig.serverConfig.transport;
}

void XrayConfigModel::setTransport(const QString &value)
{
    auto &srv = m_protocolConfig.serverConfig;
    if (srv.transport == value) {
        return;
    }
    const bool wasUnsavedChanges = hasUnsavedChanges();
    srv.transport = value;
    if (value == QLatin1String(protocols::xray::transportXhttp)) {
        srv.applyDefaults(false);
        srv.xhttp.applyRangeDefaults();
    }
    if (value == QLatin1String(protocols::xray::transportMkcp)
        && srv.security == QLatin1String(protocols::xray::securityReality)) {
        srv.security = QString::fromLatin1(protocols::xray::securityTls);
    }
    emitChanged(wasUnsavedChanges);
}

QString XrayConfigModel::security() const
{
    return m_protocolConfig.serverConfig.security;
}

void XrayConfigModel::setSecurity(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.security, value);
}

QString XrayConfigModel::flow() const
{
    return m_protocolConfig.serverConfig.flow;
}

void XrayConfigModel::setFlow(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.flow, value);
}

QString XrayConfigModel::fingerprint() const
{
    return m_protocolConfig.serverConfig.fingerprint;
}

void XrayConfigModel::setFingerprint(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.fingerprint, value);
}

QString XrayConfigModel::realityFingerprint() const
{
    const auto &srv = m_protocolConfig.serverConfig;
    return srv.isThirdPartyConfig ? srv.fingerprint : xrayEffective::realityFingerprint(srv.fingerprint);
}

QString XrayConfigModel::sni() const
{
    return m_protocolConfig.serverConfig.sni;
}

void XrayConfigModel::setSni(const QString &value)
{
    auto &srv = m_protocolConfig.serverConfig;
    if (srv.sni == value && srv.site == value) {
        return;
    }
    const bool wasUnsavedChanges = hasUnsavedChanges();
    srv.sni = value;
    srv.site = value;
    emitChanged(wasUnsavedChanges);
}

QString XrayConfigModel::alpn() const
{
    return m_protocolConfig.serverConfig.alpn;
}

void XrayConfigModel::setAlpn(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.alpn, value);
}

QString XrayConfigModel::xhttpMode() const
{
    return m_protocolConfig.serverConfig.xhttp.mode;
}

void XrayConfigModel::setXhttpMode(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.mode, value);
}

QString XrayConfigModel::xhttpHost() const
{
    return m_protocolConfig.serverConfig.xhttp.host;
}

void XrayConfigModel::setXhttpHost(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.host, value);
}

QString XrayConfigModel::xhttpPath() const
{
    return m_protocolConfig.serverConfig.xhttp.path;
}

void XrayConfigModel::setXhttpPath(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.path, value);
}

QString XrayConfigModel::xhttpUplinkMethod() const
{
    return m_protocolConfig.serverConfig.xhttp.uplinkMethod;
}

void XrayConfigModel::setXhttpUplinkMethod(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.uplinkMethod, value);
}

bool XrayConfigModel::xhttpDisableGrpc() const
{
    return m_protocolConfig.serverConfig.xhttp.disableGrpc;
}

void XrayConfigModel::setXhttpDisableGrpc(bool value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.disableGrpc, value);
}

bool XrayConfigModel::xhttpDisableSse() const
{
    return m_protocolConfig.serverConfig.xhttp.disableSse;
}

void XrayConfigModel::setXhttpDisableSse(bool value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.disableSse, value);
}

QString XrayConfigModel::xhttpSessionPlacement() const
{
    return m_protocolConfig.serverConfig.xhttp.sessionPlacement;
}

void XrayConfigModel::setXhttpSessionPlacement(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.sessionPlacement, value);
}

QString XrayConfigModel::xhttpSessionKey() const
{
    return m_protocolConfig.serverConfig.xhttp.sessionKey;
}

void XrayConfigModel::setXhttpSessionKey(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.sessionKey, value);
}

QString XrayConfigModel::xhttpSeqPlacement() const
{
    return m_protocolConfig.serverConfig.xhttp.seqPlacement;
}

void XrayConfigModel::setXhttpSeqPlacement(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.seqPlacement, value);
}

QString XrayConfigModel::xhttpSeqKey() const
{
    return m_protocolConfig.serverConfig.xhttp.seqKey;
}

void XrayConfigModel::setXhttpSeqKey(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.seqKey, value);
}

QString XrayConfigModel::xhttpUplinkDataPlacement() const
{
    return m_protocolConfig.serverConfig.xhttp.uplinkDataPlacement;
}

void XrayConfigModel::setXhttpUplinkDataPlacement(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.uplinkDataPlacement, value);
}

QString XrayConfigModel::xhttpUplinkDataKey() const
{
    return m_protocolConfig.serverConfig.xhttp.uplinkDataKey;
}

void XrayConfigModel::setXhttpUplinkDataKey(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.uplinkDataKey, value);
}

QString XrayConfigModel::xhttpUplinkChunkSize() const
{
    return m_protocolConfig.serverConfig.xhttp.uplinkChunkSize;
}

void XrayConfigModel::setXhttpUplinkChunkSize(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.uplinkChunkSize, value);
}

QString XrayConfigModel::xhttpScMaxBufferedPosts() const
{
    return m_protocolConfig.serverConfig.xhttp.scMaxBufferedPosts;
}

void XrayConfigModel::setXhttpScMaxBufferedPosts(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.scMaxBufferedPosts, value);
}

QString XrayConfigModel::xhttpScMaxEachPostBytesMin() const
{
    return m_protocolConfig.serverConfig.xhttp.scMaxEachPostBytesMin;
}

void XrayConfigModel::setXhttpScMaxEachPostBytesMin(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.scMaxEachPostBytesMin, value);
}

QString XrayConfigModel::xhttpScMaxEachPostBytesMax() const
{
    return m_protocolConfig.serverConfig.xhttp.scMaxEachPostBytesMax;
}

void XrayConfigModel::setXhttpScMaxEachPostBytesMax(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.scMaxEachPostBytesMax, value);
}

QString XrayConfigModel::xhttpScMinPostsIntervalMsMin() const
{
    return m_protocolConfig.serverConfig.xhttp.scMinPostsIntervalMsMin;
}

void XrayConfigModel::setXhttpScMinPostsIntervalMsMin(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.scMinPostsIntervalMsMin, value);
}

QString XrayConfigModel::xhttpScMinPostsIntervalMsMax() const
{
    return m_protocolConfig.serverConfig.xhttp.scMinPostsIntervalMsMax;
}

void XrayConfigModel::setXhttpScMinPostsIntervalMsMax(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.scMinPostsIntervalMsMax, value);
}

QString XrayConfigModel::xhttpScStreamUpServerSecsMin() const
{
    return m_protocolConfig.serverConfig.xhttp.scStreamUpServerSecsMin;
}

void XrayConfigModel::setXhttpScStreamUpServerSecsMin(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.scStreamUpServerSecsMin, value);
}

QString XrayConfigModel::xhttpScStreamUpServerSecsMax() const
{
    return m_protocolConfig.serverConfig.xhttp.scStreamUpServerSecsMax;
}

void XrayConfigModel::setXhttpScStreamUpServerSecsMax(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.scStreamUpServerSecsMax, value);
}

QString XrayConfigModel::mkcpTti() const
{
    return m_protocolConfig.serverConfig.mkcp.tti;
}

void XrayConfigModel::setMkcpTti(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.mkcp.tti, value);
}

QString XrayConfigModel::mkcpMtu() const
{
    return m_protocolConfig.serverConfig.mkcp.mtu;
}

void XrayConfigModel::setMkcpMtu(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.mkcp.mtu, value);
}

QString XrayConfigModel::mkcpUplinkCapacity() const
{
    return m_protocolConfig.serverConfig.mkcp.uplinkCapacity;
}

void XrayConfigModel::setMkcpUplinkCapacity(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.mkcp.uplinkCapacity, value);
}

QString XrayConfigModel::mkcpDownlinkCapacity() const
{
    return m_protocolConfig.serverConfig.mkcp.downlinkCapacity;
}

void XrayConfigModel::setMkcpDownlinkCapacity(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.mkcp.downlinkCapacity, value);
}

QString XrayConfigModel::mkcpCwndMultiplier() const
{
    return m_protocolConfig.serverConfig.mkcp.cwndMultiplier;
}

void XrayConfigModel::setMkcpCwndMultiplier(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.mkcp.cwndMultiplier, value);
}

QString XrayConfigModel::mkcpMaxSendingWindow() const
{
    return m_protocolConfig.serverConfig.mkcp.maxSendingWindow;
}

void XrayConfigModel::setMkcpMaxSendingWindow(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.mkcp.maxSendingWindow, value);
}

QString XrayConfigModel::xPaddingBytesMin() const
{
    return m_protocolConfig.serverConfig.xhttp.xPadding.bytesMin;
}

void XrayConfigModel::setXPaddingBytesMin(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xPadding.bytesMin, value);
}

QString XrayConfigModel::xPaddingBytesMax() const
{
    return m_protocolConfig.serverConfig.xhttp.xPadding.bytesMax;
}

void XrayConfigModel::setXPaddingBytesMax(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xPadding.bytesMax, value);
}

bool XrayConfigModel::xPaddingObfsMode() const
{
    return m_protocolConfig.serverConfig.xhttp.xPadding.obfsMode;
}

void XrayConfigModel::setXPaddingObfsMode(bool value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xPadding.obfsMode, value);
}

QString XrayConfigModel::xPaddingKey() const
{
    return m_protocolConfig.serverConfig.xhttp.xPadding.key;
}

void XrayConfigModel::setXPaddingKey(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xPadding.key, value);
}

QString XrayConfigModel::xPaddingHeader() const
{
    return m_protocolConfig.serverConfig.xhttp.xPadding.header;
}

void XrayConfigModel::setXPaddingHeader(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xPadding.header, value);
}

QString XrayConfigModel::xPaddingPlacement() const
{
    return m_protocolConfig.serverConfig.xhttp.xPadding.placement;
}

void XrayConfigModel::setXPaddingPlacement(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xPadding.placement, value);
}

QString XrayConfigModel::xPaddingMethod() const
{
    return m_protocolConfig.serverConfig.xhttp.xPadding.method;
}

void XrayConfigModel::setXPaddingMethod(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xPadding.method, value);
}

bool XrayConfigModel::xmuxEnabled() const
{
    return m_protocolConfig.serverConfig.xhttp.xmux.enabled;
}

void XrayConfigModel::setXmuxEnabled(bool value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xmux.enabled, value);
}

QString XrayConfigModel::xmuxMaxConcurrencyMin() const
{
    return m_protocolConfig.serverConfig.xhttp.xmux.maxConcurrencyMin;
}

void XrayConfigModel::setXmuxMaxConcurrencyMin(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xmux.maxConcurrencyMin, value);
}

QString XrayConfigModel::xmuxMaxConcurrencyMax() const
{
    return m_protocolConfig.serverConfig.xhttp.xmux.maxConcurrencyMax;
}

void XrayConfigModel::setXmuxMaxConcurrencyMax(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xmux.maxConcurrencyMax, value);
}

QString XrayConfigModel::xmuxMaxConnectionsMin() const
{
    return m_protocolConfig.serverConfig.xhttp.xmux.maxConnectionsMin;
}

void XrayConfigModel::setXmuxMaxConnectionsMin(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xmux.maxConnectionsMin, value);
}

QString XrayConfigModel::xmuxMaxConnectionsMax() const
{
    return m_protocolConfig.serverConfig.xhttp.xmux.maxConnectionsMax;
}

void XrayConfigModel::setXmuxMaxConnectionsMax(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xmux.maxConnectionsMax, value);
}

QString XrayConfigModel::xmuxCMaxReuseTimesMin() const
{
    return m_protocolConfig.serverConfig.xhttp.xmux.cMaxReuseTimesMin;
}

void XrayConfigModel::setXmuxCMaxReuseTimesMin(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xmux.cMaxReuseTimesMin, value);
}

QString XrayConfigModel::xmuxCMaxReuseTimesMax() const
{
    return m_protocolConfig.serverConfig.xhttp.xmux.cMaxReuseTimesMax;
}

void XrayConfigModel::setXmuxCMaxReuseTimesMax(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xmux.cMaxReuseTimesMax, value);
}

QString XrayConfigModel::xmuxHMaxRequestTimesMin() const
{
    return m_protocolConfig.serverConfig.xhttp.xmux.hMaxRequestTimesMin;
}

void XrayConfigModel::setXmuxHMaxRequestTimesMin(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xmux.hMaxRequestTimesMin, value);
}

QString XrayConfigModel::xmuxHMaxRequestTimesMax() const
{
    return m_protocolConfig.serverConfig.xhttp.xmux.hMaxRequestTimesMax;
}

void XrayConfigModel::setXmuxHMaxRequestTimesMax(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xmux.hMaxRequestTimesMax, value);
}

QString XrayConfigModel::xmuxHMaxReusableSecsMin() const
{
    return m_protocolConfig.serverConfig.xhttp.xmux.hMaxReusableSecsMin;
}

void XrayConfigModel::setXmuxHMaxReusableSecsMin(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xmux.hMaxReusableSecsMin, value);
}

QString XrayConfigModel::xmuxHMaxReusableSecsMax() const
{
    return m_protocolConfig.serverConfig.xhttp.xmux.hMaxReusableSecsMax;
}

void XrayConfigModel::setXmuxHMaxReusableSecsMax(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xmux.hMaxReusableSecsMax, value);
}

QString XrayConfigModel::xmuxHKeepAlivePeriod() const
{
    return m_protocolConfig.serverConfig.xhttp.xmux.hKeepAlivePeriod;
}

void XrayConfigModel::setXmuxHKeepAlivePeriod(const QString &value)
{
    assignField(m_protocolConfig.serverConfig.xhttp.xmux.hKeepAlivePeriod, value);
}

bool XrayConfigModel::xmuxKeepAliveApplies() const
{
    return !xrayEffective::xhttpUsesHttp11(m_protocolConfig.serverConfig);
}

void XrayConfigModel::assignField(QString &field, const QString &value)
{
    if (field == value) {
        return;
    }
    const bool wasUnsavedChanges = hasUnsavedChanges();
    field = value;
    emitChanged(wasUnsavedChanges);
}

void XrayConfigModel::assignField(bool &field, bool value)
{
    if (field == value) {
        return;
    }
    const bool wasUnsavedChanges = hasUnsavedChanges();
    field = value;
    emitChanged(wasUnsavedChanges);
}

void XrayConfigModel::emitChanged(bool wasUnsavedChanges)
{
    emit changed();
    if (wasUnsavedChanges != hasUnsavedChanges()) {
        emit hasUnsavedChangesChanged();
    }
}

void XrayConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::XrayProtocolConfig& protocolConfig)
{
    const bool wasUnsavedChanges = hasUnsavedChanges();

    m_container = container;

    m_protocolConfig = protocolConfig;
    if (m_protocolConfig.needsClientHydration) {
        m_protocolConfig.hydrateServerConfigFromClientNative();
    }

    if (!m_protocolConfig.serverConfig.isThirdPartyConfig) {
        applyDefaultsToServerConfig(m_protocolConfig.serverConfig, false);
    }

    m_originalProtocolConfig = m_protocolConfig;

    emitChanged(wasUnsavedChanges);
}

void XrayConfigModel::applyDefaultsToServerConfig(amnezia::XrayServerConfig &config, bool fillFlowDefault)
{
    config.applyDefaults(fillFlowDefault);
}

amnezia::XrayProtocolConfig XrayConfigModel::getProtocolConfig()
{
    if (!m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig)) {
        m_protocolConfig.clearClientConfig();
        return m_protocolConfig;
    }

    if (comparableJson(m_protocolConfig.serverConfig) == comparableJson(m_originalProtocolConfig.serverConfig)) {
        return m_protocolConfig;
    }

    if (!m_protocolConfig.regenerateClientConfigFromNative()) {
        m_protocolConfig.clearClientConfig();
    }
    return m_protocolConfig;
}

amnezia::XrayServerConfig XrayConfigModel::pendingServerConfig(const QString &pendingPort) const
{
    amnezia::XrayServerConfig pending = m_protocolConfig.serverConfig;
    if (!pendingPort.isEmpty()) {
        pending.port = pendingPort;
    }
    return pending;
}

bool XrayConfigModel::pendingChangeTouchesServer(const QString &pendingPort) const
{
    return !pendingServerConfig(pendingPort).hasEqualServerSettings(m_originalProtocolConfig.serverConfig);
}

bool XrayConfigModel::pendingChangeBreaksIssuedConfigs(const QString &pendingPort) const
{
    return pendingServerConfig(pendingPort).breaksIssuedConfigs(m_originalProtocolConfig.serverConfig);
}

bool XrayConfigModel::pendingChangeRequiresReinstall(const QString &pendingPort) const
{
    const auto effectivePort = [](const QString &port) -> QString {
        return port.isEmpty() ? QString::fromLatin1(protocols::xray::defaultPort) : port;
    };
    const amnezia::XrayServerConfig pending = pendingServerConfig(pendingPort);
    return effectivePort(pending.port) != effectivePort(m_originalProtocolConfig.serverConfig.port);
}

QString XrayConfigModel::saveDescription(const QString &pendingPort) const
{
    const bool touchesServer = pendingChangeTouchesServer(pendingPort);
    const bool breaksIssued = pendingChangeBreaksIssuedConfigs(pendingPort);
    const bool requiresReinstall = pendingChangeRequiresReinstall(pendingPort);

    if (requiresReinstall) {
        if (breaksIssued) {
            return tr("All users with whom you shared a connection with will no longer be able to connect to it. You will need to share the connection again.");
        } else {
            return tr("The server will be recreated. This takes up to a minute, and connections that were already shared keep working.");
        }
    } else if (touchesServer) {
        if (breaksIssued) {
            return tr("The server configuration will be updated. All users with whom you shared a connection with will no longer be able to connect to it. You will need to share the connection again.");
        } else {
            return tr("The server configuration will be updated. If XRay on the server is outdated, the container will be recreated with the current version; connections that were already shared keep working.");
        }
    } else {
        return tr("The server will not be changed now. The new settings apply the next time you connect.");
    }
}

bool XrayConfigModel::isServerSettingsEqual() const
{
    return m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig);
}

bool XrayConfigModel::hasUnsavedChanges() const
{
    return comparableJson(m_protocolConfig.serverConfig) != comparableJson(m_originalProtocolConfig.serverConfig);
}

void XrayConfigModel::resetToDefaults()
{
    const bool wasUnsavedChanges = hasUnsavedChanges();

    m_protocolConfig.serverConfig = amnezia::XrayServerConfig{};
    applyDefaultsToServerConfig(m_protocolConfig.serverConfig, true);
    m_protocolConfig.serverConfig.xhttp.applyRangeDefaults();

    emitChanged(wasUnsavedChanges);
}

void XrayConfigModel::applyServerConfig(const amnezia::XrayServerConfig &serverConfig)
{
    const bool wasUnsavedChanges = hasUnsavedChanges();

    m_protocolConfig.serverConfig = serverConfig;
    if (!m_protocolConfig.serverConfig.isThirdPartyConfig) {
        m_protocolConfig.serverConfig.applyDefaults(false);
    }

    emitChanged(wasUnsavedChanges);
}

QStringList XrayConfigModel::flowOptions()
{
    return {
        "",                        // Empty (no flow)
        "xtls-rprx-vision",
        "xtls-rprx-vision-udp443"
    };
}

QStringList XrayConfigModel::securityOptions()
{
    return { "tls", "reality" };
}

QStringList XrayConfigModel::transportOptions()
{
    return { "raw", "xhttp", "mkcp" };
}

QStringList XrayConfigModel::fingerprintOptions()
{
    return { "chrome", "firefox", "safari", "ios", "android", "edge", "360", "qq", "random" };
}

QStringList XrayConfigModel::realityFingerprintOptions()
{
    return { "chrome", "firefox", "safari" };
}

QStringList XrayConfigModel::realityFingerprintOptions()
{
    return { "hellochrome_120", "chrome", "firefox", "safari", "ios", "edge", "qq", "random" };
}

QStringList XrayConfigModel::alpnOptions()
{
    return { "h2", "http/1.1", "h2,http/1.1" };
}

QStringList XrayConfigModel::xhttpModeOptions()
{
    return { "Auto", "Packet-up", "Stream-up", "Stream-one" };
}

QStringList XrayConfigModel::xhttpUplinkMethodOptions()
{
    return { "POST", "PUT", "PATCH" };
}

QStringList XrayConfigModel::xhttpSessionPlacementOptions()
{
    return { "Path", "Header", "Cookie", "Query" };
}

QStringList XrayConfigModel::xhttpSeqPlacementOptions()
{
    return { "Path", "Header", "Cookie", "Query" };
}

QStringList XrayConfigModel::xhttpUplinkDataPlacementOptions()
{
    return { "Body", "Auto", "Header", "Cookie" };
}

QStringList XrayConfigModel::xPaddingPlacementOptions()
{
    // Xray-core: cookie | header | query | queryInHeader (not "body")
    return { "Cookie", "Header", "Query", "Query in header" };
}

QStringList XrayConfigModel::xPaddingMethodOptions()
{
    return { "Repeat-x", "Tokenish" };
}

QString XrayConfigModel::mkcpDefaultTti()
{
    return QString::fromLatin1(protocols::xray::defaultMkcpTti);
}

QString XrayConfigModel::mkcpDefaultUplinkCapacity()
{
    return QString::fromLatin1(protocols::xray::defaultMkcpUplinkCapacity);
}

QString XrayConfigModel::mkcpDefaultDownlinkCapacity()
{
    return QString::fromLatin1(protocols::xray::defaultMkcpDownlinkCapacity);
}

QString XrayConfigModel::mkcpDefaultMtu()
{
    return QString::fromLatin1(protocols::xray::defaultMkcpMtu);
}

QString XrayConfigModel::mkcpDefaultCwndMultiplier()
{
    return QString::fromLatin1(protocols::xray::defaultMkcpCwndMultiplier);
}

QString XrayConfigModel::portDefault()
{
    return QString::fromLatin1(protocols::xray::defaultPort);
}

QString XrayConfigModel::sniDefault()
{
    return QString::fromLatin1(protocols::xray::defaultSni);
}

QString XrayConfigModel::xhttpHostDefault()
{
    return QString::fromLatin1(protocols::xray::defaultXhttpHost);
}

QString XrayConfigModel::xhttpUplinkChunkSizeDefault()
{
    return QString::fromLatin1(protocols::xray::defaultXhttpUplinkChunkSize);
}

QString XrayConfigModel::scMaxEachPostBytesMinDefault()
{
    return QString::fromLatin1(protocols::xray::coreDefaultXhttpScMaxEachPostBytesMin);
}

QString XrayConfigModel::scMaxEachPostBytesMaxDefault()
{
    return QString::fromLatin1(protocols::xray::coreDefaultXhttpScMaxEachPostBytesMax);
}

QString XrayConfigModel::scMinPostsIntervalMsMinDefault()
{
    return QString::fromLatin1(protocols::xray::coreDefaultXhttpScMinPostsIntervalMsMin);
}

QString XrayConfigModel::scMinPostsIntervalMsMaxDefault()
{
    return QString::fromLatin1(protocols::xray::coreDefaultXhttpScMinPostsIntervalMsMax);
}

QString XrayConfigModel::scStreamUpServerSecsMinDefault()
{
    return QString::fromLatin1(protocols::xray::coreDefaultXhttpScStreamUpServerSecsMin);
}

QString XrayConfigModel::scStreamUpServerSecsMaxDefault()
{
    return QString::fromLatin1(protocols::xray::coreDefaultXhttpScStreamUpServerSecsMax);
}

QString XrayConfigModel::xPaddingKeyDefault()
{
    return QString::fromLatin1(protocols::xray::defaultXPaddingKey);
}

QString XrayConfigModel::xPaddingHeaderDefault()
{
    return QString::fromLatin1(protocols::xray::defaultXPaddingHeader);
}

QString XrayConfigModel::xPaddingBytesMinDefault()
{
    return QString::fromLatin1(protocols::xray::defaultXPaddingBytesMin);
}

QString XrayConfigModel::xPaddingBytesMaxDefault()
{
    return QString::fromLatin1(protocols::xray::defaultXPaddingBytesMax);
}

namespace {
    bool isValidSingleHost(const QString &t)
    {
        if (t.isEmpty() || t.length() > 253) {
            return false;
        }
        QHostAddress a(t);
        if (a.protocol() == QHostAddress::IPv4Protocol) {
            return NetworkUtilities::checkIPv4Format(t);
        }
        if (a.protocol() == QHostAddress::IPv6Protocol) {
            return true;
        }
        static const QRegularExpression onlyDigits(QStringLiteral(R"(^\d+$)"));
        if (onlyDigits.match(t).hasMatch()) {
            return false;
        }
        QRegExp re = NetworkUtilities::domainRegExp();
        re.setCaseSensitivity(Qt::CaseInsensitive);
        return re.exactMatch(t);
    }
}

bool XrayConfigModel::isValidHost(const QString &host)
{
    const QString t = host.trimmed();
    if (t.isEmpty()) {
        return true;
    }
    return isValidSingleHost(t);
}

bool XrayConfigModel::isValidSni(const QString &sni)
{
    const QString t = sni.trimmed();
    if (t.isEmpty()) {
        return true;
    }
    if (t.contains(QLatin1Char('*'))) {
        return false;
    }
    return isValidSingleHost(t);
}

QStringList XrayConfigModel::validationErrors() const
{
    QStringList errs;
    const auto &srv = m_protocolConfig.serverConfig;

    if (!srv.port.isEmpty() || !srv.isThirdPartyConfig) {
        bool ok = false;
        const int p = srv.port.toInt(&ok);
        if (!ok || p < 1 || p > 65535) {
            errs << tr("Port must be in the range of 1 to 65535");
        }
    }

    const QString security = xrayEffective::security(srv);
    if (security == QLatin1String("tls") || security == QLatin1String("reality")) {
        if (!isValidSni(srv.sni)) {
            errs << tr("SNI: enter a valid IP address or domain name");
        }
    } else if (!srv.isThirdPartyConfig) {
        errs << tr("None is not supported by Xray 26.10. Choose TLS or REALITY.");
    }

    const QString fingerprint = srv.fingerprint.trimmed().toLower();
    const bool tls12OnlyFingerprint = fingerprint == QLatin1String("android") || fingerprint == QLatin1String("360");
    if (security == QLatin1String("reality")) {
        if (tls12OnlyFingerprint) {
            errs << tr("Fingerprints android and 360 do not support TLS 1.3 and do not work with REALITY");
        }
        const QHostAddress sniAddress(srv.sni.trimmed());
        if (sniAddress.protocol() == QHostAddress::IPv4Protocol || sniAddress.protocol() == QHostAddress::IPv6Protocol) {
            errs << tr("REALITY needs a domain name in SNI, not an IP address");
        }
    }
    if (security == QLatin1String("tls") && tls12OnlyFingerprint && !xrayEffective::clientFlow(srv).isEmpty()) {
        errs << tr("Fingerprints android and 360 do not support TLS 1.3 and do not work with xtls-rprx-vision");
    }

    if (!srv.isThirdPartyConfig) {
        auto checkValue = [&errs](const QString &field, const QString &value, const QStringList &allowed) {
            if (!allowed.contains(value)) {
                errs << tr("%1: unsupported value \"%2\"").arg(field, value);
            }
        };
        checkValue(QStringLiteral("Transport"), srv.transport, { "", "raw", "xhttp", "mkcp" });
        checkValue(QStringLiteral("Security"), srv.security, { "", "none", "tls", "reality" });
        checkValue(QStringLiteral("Flow"), srv.flow, flowOptions());
        if (!fingerprint.isEmpty()) {
            checkValue(QStringLiteral("Fingerprint"), fingerprint, fingerprintOptions());
        }
        if (security == QLatin1String("tls")) {
            const QString alpnEff = srv.alpn.isEmpty() ? QString::fromLatin1(protocols::xray::defaultAlpn) : srv.alpn;
            for (const QString &a : xrayEffective::alpnList(alpnEff)) {
                checkValue(QStringLiteral("ALPN"), a, { "h2", "http/1.1" });
            }
        }
        if (srv.transport == QLatin1String("xhttp")) {
            const auto &x = srv.xhttp;
            const QString modeEff = xrayEffective::xhttpMode(x.mode);
            checkValue(QStringLiteral("Mode"), modeEff, { "auto", "packet-up", "stream-up", "stream-one" });
            QString method = x.uplinkMethod.toUpper();
            if (method == QLatin1String("GET") && modeEff == QLatin1String("packet-up"))
                method.clear();
            checkValue(QStringLiteral("UplinkHTTPMethod"), method, { "", "POST", "PUT", "PATCH" });
            checkValue(QStringLiteral("SessionPlacement"), xrayEffective::sessionSeqPlacement(x.sessionPlacement),
                       { "", "path", "header", "cookie", "query" });
            checkValue(QStringLiteral("SeqPlacement"), xrayEffective::sessionSeqPlacement(x.seqPlacement),
                       { "", "path", "header", "cookie", "query" });
            checkValue(QStringLiteral("UplinkDataPlacement"), xrayEffective::uplinkDataPlacement(x.uplinkDataPlacement),
                       { "body", "auto", "header", "cookie" });
            if (x.xPadding.obfsMode) {
                checkValue(QStringLiteral("xPaddingPlacement"), xrayEffective::xPaddingPlacement(x.xPadding.placement),
                           { "cookie", "header", "query", "queryInHeader" });
                checkValue(QStringLiteral("xPaddingMethod"), xrayEffective::xPaddingMethod(x.xPadding.method),
                           { "repeat-x", "tokenish" });
            }
        }
    }

    if (srv.transport == QLatin1String("xhttp")) {
        if (security == QLatin1String("tls") && tls12OnlyFingerprint && !xrayEffective::xhttpUsesHttp11(srv)) {
            errs << tr("Fingerprints android and 360 do not offer HTTP/2; with XHTTP use ALPN http/1.1");
        }
        if (!isValidHost(srv.xhttp.host)) {
            errs << tr("Host: enter a valid IP address or domain name");
        }
        const auto &xmux = srv.xhttp.xmux;
        auto isSet = [](const QString &v) { return !v.isEmpty() && v != QLatin1String("0"); };
        if (xmux.enabled && (isSet(xmux.maxConcurrencyMin) || isSet(xmux.maxConcurrencyMax))
            && (isSet(xmux.maxConnectionsMin) || isSet(xmux.maxConnectionsMax))) {
            errs << tr("XMux: set either maxConcurrency or maxConnections, not both");
        }

        auto rangeValue = [](const QString &v, const char *fallback) {
            return (v.isEmpty() ? QString::fromLatin1(fallback) : v).toLongLong();
        };
        const auto &pad = srv.xhttp.xPadding;
        if (!pad.bytesMin.isEmpty() || !pad.bytesMax.isEmpty()) {
            const qint64 from = rangeValue(pad.bytesMin, protocols::xray::defaultXPaddingBytesMin);
            const qint64 to = qMax(from, rangeValue(pad.bytesMax, protocols::xray::defaultXPaddingBytesMax));
            if (from == 0 && to > 0) {
                errs << tr("xPaddingBytes: Min can be 0 only if Max is 0 too (0–0 = core default 100–1000)");
            }
        }
        const QString mode = xrayEffective::xhttpMode(srv.xhttp.mode);
        const bool packetUpEffective = mode == QLatin1String("packet-up")
                || (mode == QLatin1String("auto") && security != QLatin1String("reality"));
        if (packetUpEffective
            && (!srv.xhttp.scMaxEachPostBytesMin.isEmpty() || !srv.xhttp.scMaxEachPostBytesMax.isEmpty())) {
            const qint64 from = rangeValue(srv.xhttp.scMaxEachPostBytesMin,
                                           protocols::xray::coreDefaultXhttpScMaxEachPostBytesMin);
            const qint64 to = rangeValue(srv.xhttp.scMaxEachPostBytesMax,
                                         protocols::xray::coreDefaultXhttpScMaxEachPostBytesMax);
            if (qMin(from, to) == 0 && qMax(from, to) > 0) {
                errs << tr("scMaxEachPostBytes: Min can be 0 only if Max is 0 too (0–0 = core default 1000000)");
            }
        }
        if (mode == QLatin1String("stream-up")
            && (!srv.xhttp.scStreamUpServerSecsMin.isEmpty() || !srv.xhttp.scStreamUpServerSecsMax.isEmpty())) {
            const qint64 from = rangeValue(srv.xhttp.scStreamUpServerSecsMin,
                                           protocols::xray::coreDefaultXhttpScStreamUpServerSecsMin);
            const qint64 to = rangeValue(srv.xhttp.scStreamUpServerSecsMax,
                                         protocols::xray::coreDefaultXhttpScStreamUpServerSecsMax);
            if (qMin(from, to) == 0 && qMax(from, to) > 0) {
                errs << tr("scStreamUpServerSecs: Min can be 0 only if Max is 0 too (0–0 = core default 20–80)");
            }
        }
        if (srv.xhttp.scMaxBufferedPosts.toLongLong() > 100000) {
            errs << tr("scMaxBufferedPosts: maximum is 100000 (Packet-up settings)");
        }
    }

    if (srv.transport == QLatin1String("mkcp")) {
        auto checkInt = [&errs](const QString &field, const QString &value, qint64 lo, qint64 hi) -> qint64 {
            if (value.isEmpty()) {
                return -1;
            }
            bool ok = false;
            const qint64 n = value.toLongLong(&ok);
            if (!ok || n < lo || n > hi) {
                errs << tr("%1: allowed range is %2–%3").arg(field).arg(lo).arg(hi);
                return -1;
            }
            return n;
        };
        const auto &k = srv.mkcp;
        checkInt(QStringLiteral("TTI"), k.tti, 10, 100);
        const qint64 mtu = checkInt(QStringLiteral("mtu"), k.mtu, 576, 1460);
        checkInt(QStringLiteral("uplinkCapacity"), k.uplinkCapacity, 0, 4095);
        checkInt(QStringLiteral("downlinkCapacity"), k.downlinkCapacity, 0, 4095);
        checkInt(QStringLiteral("cwndMultiplier"), k.cwndMultiplier, 1, 5000);
        const qint64 mtuEff = mtu > 0 ? mtu : QString::fromLatin1(protocols::xray::defaultMkcpMtu).toLongLong();
        checkInt(QStringLiteral("maxSendingWindow"), k.maxSendingWindow, mtuEff, 2147483647);
    }

    return errs;
}
