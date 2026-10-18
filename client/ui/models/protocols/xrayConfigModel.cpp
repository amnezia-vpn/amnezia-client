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

    if (m_protocolConfig.serverConfig.toJson() == m_originalProtocolConfig.serverConfig.toJson()) {
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
            return tr("The server configuration will be updated. The container will not be recreated.");
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
    return m_protocolConfig.serverConfig.toJson() != m_originalProtocolConfig.serverConfig.toJson();
}

void XrayConfigModel::resetToDefaults()
{
    const bool wasUnsavedChanges = hasUnsavedChanges();

    m_protocolConfig.serverConfig = amnezia::XrayServerConfig{};
    applyDefaultsToServerConfig(m_protocolConfig.serverConfig, true);

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
    return { "none", "tls", "reality" };
}

QStringList XrayConfigModel::transportOptions()
{
    return { "raw", "xhttp", "mkcp" };
}

QStringList XrayConfigModel::fingerprintOptions()
{
    return { "chrome", "firefox", "safari", "ios", "android", "edge", "360", "qq", "random" };
}

QStringList XrayConfigModel::alpnOptions()
{
    return { "h2", "http/1.1", "h2,http/1.1" };
}

QStringList XrayConfigModel::xhttpModeOptions()
{
    return { "Stream-up", "Stream-one" };
}

QStringList XrayConfigModel::xhttpUplinkMethodOptions()
{
    return { "POST", "PUT", "PATCH" };
}

QStringList XrayConfigModel::xhttpSessionPlacementOptions()
{
    return { "Path", "Header", "Cookie", "Query", "None" };
}

QStringList XrayConfigModel::xhttpSeqPlacementOptions()
{
    return { "Path", "Header", "Cookie", "Query", "None" };
}

QStringList XrayConfigModel::xhttpUplinkDataPlacementOptions()
{
    return { "Body", "Auto" };
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
    return QString::fromLatin1(protocols::xray::defaultXhttpScMaxEachPostBytesMin);
}

QString XrayConfigModel::scMaxEachPostBytesMaxDefault()
{
    return QString::fromLatin1(protocols::xray::defaultXhttpScMaxEachPostBytesMax);
}

QString XrayConfigModel::scMinPostsIntervalMsMinDefault()
{
    return QString::fromLatin1(protocols::xray::defaultXhttpScMinPostsIntervalMsMin);
}

QString XrayConfigModel::scMinPostsIntervalMsMaxDefault()
{
    return QString::fromLatin1(protocols::xray::defaultXhttpScMinPostsIntervalMsMax);
}

QString XrayConfigModel::scStreamUpServerSecsMinDefault()
{
    return QString::fromLatin1(protocols::xray::defaultXhttpScStreamUpServerSecsMin);
}

QString XrayConfigModel::scStreamUpServerSecsMaxDefault()
{
    return QString::fromLatin1(protocols::xray::defaultXhttpScStreamUpServerSecsMax);
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
    if (t.startsWith(QLatin1String("*."))) {
        return isValidSingleHost(t.mid(2));
    }
    return isValidSingleHost(t);
}

bool XrayConfigModel::isValidPath(const QString &path)
{
    const QString t = path.trimmed();
    if (t.isEmpty()) {
        return true;
    }
    return t.startsWith(QLatin1Char('/'));
}

QStringList XrayConfigModel::validationErrors() const
{
    QStringList errs;
    const auto &srv = m_protocolConfig.serverConfig;

    if (!srv.port.isEmpty()) {
        bool ok = false;
        const int p = srv.port.toInt(&ok);
        if (!ok || p < 1 || p > 65535) {
            errs << tr("Port must be in the range of 1 to 65535");
        }
    }

    if (srv.security == QLatin1String("tls") || srv.security == QLatin1String("reality")) {
        if (!isValidSni(srv.sni)) {
            errs << tr("SNI: enter a valid IP address or domain name");
        }
    }

    if (srv.transport == QLatin1String("xhttp")) {
        if (!isValidHost(srv.xhttp.host)) {
            errs << tr("Host: enter a valid IP address or domain name");
        }
        if (!isValidPath(srv.xhttp.path)) {
            errs << tr("Path must start with \"/\"");
        }
    }

    return errs;
}
