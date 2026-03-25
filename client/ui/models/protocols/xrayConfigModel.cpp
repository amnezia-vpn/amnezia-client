#include "xrayConfigModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;
using namespace ProtocolUtils;

XrayConfigModel::XrayConfigModel(QObject* parent) : QAbstractListModel(parent)
{
}

int XrayConfigModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool XrayConfigModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerUtils::allContainers().size())
    {
        return false;
    }

    auto& srv = m_protocolConfig.serverConfig;
    auto& xhttp = srv.xhttp;
    auto& mkcp = srv.mkcp;
    auto& pad = xhttp.xPadding;
    auto& mux = xhttp.xmux;

    QString str = value.toString();

    switch (role)
    {
    // ── Main ──────────────────────────────────────────────────────────
    case Roles::SiteRole: srv.site = str;
        break;
    case Roles::PortRole: srv.port = str;
        break;
    case Roles::TransportRole: srv.transport = str;
        break;
    case Roles::SecurityRole: srv.security = str;
        break;
    case Roles::FlowRole: srv.flow = str;
        break;

    // ── Security ──────────────────────────────────────────────────────
    case Roles::FingerprintRole: srv.fingerprint = str;
        break;
    case Roles::SniRole: srv.sni = str;
        break;
    case Roles::AlpnRole: srv.alpn = str;
        break;

    // ── XHTTP ─────────────────────────────────────────────────────────
    case Roles::XhttpModeRole: xhttp.mode = str;
        break;
    case Roles::XhttpHostRole: xhttp.host = str;
        break;
    case Roles::XhttpPathRole: xhttp.path = str;
        break;
    case Roles::XhttpHeadersTemplateRole: xhttp.headersTemplate = str;
        break;
    case Roles::XhttpUplinkMethodRole: xhttp.uplinkMethod = str;
        break;
    case Roles::XhttpDisableGrpcRole: xhttp.disableGrpc = value.toBool();
        break;
    case Roles::XhttpDisableSseRole: xhttp.disableSse = value.toBool();
        break;

    case Roles::XhttpSessionPlacementRole: xhttp.sessionPlacement = str;
        break;
    case Roles::XhttpSessionKeyRole: xhttp.sessionKey = str;
        break;
    case Roles::XhttpSeqPlacementRole: xhttp.seqPlacement = str;
        break;
    case Roles::XhttpSeqKeyRole: xhttp.seqKey = str;
        break;
    case Roles::XhttpUplinkDataPlacementRole: xhttp.uplinkDataPlacement = str;
        break;
    case Roles::XhttpUplinkDataKeyRole: xhttp.uplinkDataKey = str;
        break;

    case Roles::XhttpUplinkChunkSizeRole: xhttp.uplinkChunkSize = str;
        break;
    case Roles::XhttpScMaxBufferedPostsRole: xhttp.scMaxBufferedPosts = str;
        break;
    case Roles::XhttpScMaxEachPostBytesMinRole: xhttp.scMaxEachPostBytesMin = str;
        break;
    case Roles::XhttpScMaxEachPostBytesMaxRole: xhttp.scMaxEachPostBytesMax = str;
        break;
    case Roles::XhttpScMinPostsIntervalMsMinRole: xhttp.scMinPostsIntervalMsMin = str;
        break;
    case Roles::XhttpScMinPostsIntervalMsMaxRole: xhttp.scMinPostsIntervalMsMax = str;
        break;
    case Roles::XhttpScStreamUpServerSecsMinRole: xhttp.scStreamUpServerSecsMin = str;
        break;
    case Roles::XhttpScStreamUpServerSecsMaxRole: xhttp.scStreamUpServerSecsMax = str;
        break;

    // ── mKCP ──────────────────────────────────────────────────────────
    case Roles::MkcpTtiRole: mkcp.tti = str;
        break;
    case Roles::MkcpUplinkCapacityRole: mkcp.uplinkCapacity = str;
        break;
    case Roles::MkcpDownlinkCapacityRole: mkcp.downlinkCapacity = str;
        break;
    case Roles::MkcpReadBufferSizeRole: mkcp.readBufferSize = str;
        break;
    case Roles::MkcpWriteBufferSizeRole: mkcp.writeBufferSize = str;
        break;
    case Roles::MkcpCongestionRole: mkcp.congestion = value.toBool();
        break;

    // ── xPadding ──────────────────────────────────────────────────────
    case Roles::XPaddingBytesMinRole: pad.bytesMin = str;
        break;
    case Roles::XPaddingBytesMaxRole: pad.bytesMax = str;
        break;
    case Roles::XPaddingObfsModeRole: pad.obfsMode = value.toBool();
        break;
    case Roles::XPaddingKeyRole: pad.key = str;
        break;
    case Roles::XPaddingHeaderRole: pad.header = str;
        break;
    case Roles::XPaddingPlacementRole: pad.placement = str;
        break;
    case Roles::XPaddingMethodRole: pad.method = str;
        break;

    // ── xmux ──────────────────────────────────────────────────────────
    case Roles::XmuxEnabledRole: mux.enabled = value.toBool();
        break;
    case Roles::XmuxMaxConcurrencyMinRole: mux.maxConcurrencyMin = str;
        break;
    case Roles::XmuxMaxConcurrencyMaxRole: mux.maxConcurrencyMax = str;
        break;
    case Roles::XmuxMaxConnectionsMinRole: mux.maxConnectionsMin = str;
        break;
    case Roles::XmuxMaxConnectionsMaxRole: mux.maxConnectionsMax = str;
        break;
    case Roles::XmuxCMaxReuseTimesMinRole: mux.cMaxReuseTimesMin = str;
        break;
    case Roles::XmuxCMaxReuseTimesMaxRole: mux.cMaxReuseTimesMax = str;
        break;
    case Roles::XmuxHMaxRequestTimesMinRole: mux.hMaxRequestTimesMin = str;
        break;
    case Roles::XmuxHMaxRequestTimesMaxRole: mux.hMaxRequestTimesMax = str;
        break;
    case Roles::XmuxHMaxReusableSecsMinRole: mux.hMaxReusableSecsMin = str;
        break;
    case Roles::XmuxHMaxReusableSecsMaxRole: mux.hMaxReusableSecsMax = str;
        break;
    case Roles::XmuxHKeepAlivePeriodRole: mux.hKeepAlivePeriod = str;
        break;

    default:
        return false;
    }

    emit dataChanged(index, index, QList{role});
    return true;
}

QVariant XrayConfigModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount())
    {
        return QVariant();
    }

    const auto& srv = m_protocolConfig.serverConfig;
    const auto& xhttp = srv.xhttp;
    const auto& mkcp = srv.mkcp;
    const auto& pad = xhttp.xPadding;
    const auto& mux = xhttp.xmux;

    switch (role)
    {
    // ── Main ──────────────────────────────────────────────────────────
    case Roles::SiteRole: return srv.site;
    case Roles::PortRole: return srv.port;
    case Roles::TransportRole: return srv.transport;
    case Roles::SecurityRole: return srv.security;
    case Roles::FlowRole: return srv.flow;

    // ── Security ──────────────────────────────────────────────────────
    case Roles::FingerprintRole: return srv.fingerprint;
    case Roles::SniRole: return srv.sni;
    case Roles::AlpnRole: return srv.alpn;

    // ── XHTTP ─────────────────────────────────────────────────────────
    case Roles::XhttpModeRole: return xhttp.mode;
    case Roles::XhttpHostRole: return xhttp.host;
    case Roles::XhttpPathRole: return xhttp.path;
    case Roles::XhttpHeadersTemplateRole: return xhttp.headersTemplate;
    case Roles::XhttpUplinkMethodRole: return xhttp.uplinkMethod;
    case Roles::XhttpDisableGrpcRole: return xhttp.disableGrpc;
    case Roles::XhttpDisableSseRole: return xhttp.disableSse;

    case Roles::XhttpSessionPlacementRole: return xhttp.sessionPlacement;
    case Roles::XhttpSessionKeyRole: return xhttp.sessionKey;
    case Roles::XhttpSeqPlacementRole: return xhttp.seqPlacement;
    case Roles::XhttpSeqKeyRole: return xhttp.seqKey;
    case Roles::XhttpUplinkDataPlacementRole: return xhttp.uplinkDataPlacement;
    case Roles::XhttpUplinkDataKeyRole: return xhttp.uplinkDataKey;

    case Roles::XhttpUplinkChunkSizeRole: return xhttp.uplinkChunkSize;
    case Roles::XhttpScMaxBufferedPostsRole: return xhttp.scMaxBufferedPosts;
    case Roles::XhttpScMaxEachPostBytesMinRole: return xhttp.scMaxEachPostBytesMin;
    case Roles::XhttpScMaxEachPostBytesMaxRole: return xhttp.scMaxEachPostBytesMax;
    case Roles::XhttpScMinPostsIntervalMsMinRole: return xhttp.scMinPostsIntervalMsMin;
    case Roles::XhttpScMinPostsIntervalMsMaxRole: return xhttp.scMinPostsIntervalMsMax;
    case Roles::XhttpScStreamUpServerSecsMinRole: return xhttp.scStreamUpServerSecsMin;
    case Roles::XhttpScStreamUpServerSecsMaxRole: return xhttp.scStreamUpServerSecsMax;

    // ── mKCP ──────────────────────────────────────────────────────────
    case Roles::MkcpTtiRole: return mkcp.tti;
    case Roles::MkcpUplinkCapacityRole: return mkcp.uplinkCapacity;
    case Roles::MkcpDownlinkCapacityRole: return mkcp.downlinkCapacity;
    case Roles::MkcpReadBufferSizeRole: return mkcp.readBufferSize;
    case Roles::MkcpWriteBufferSizeRole: return mkcp.writeBufferSize;
    case Roles::MkcpCongestionRole: return mkcp.congestion;

    // ── xPadding ──────────────────────────────────────────────────────
    case Roles::XPaddingBytesMinRole: return pad.bytesMin;
    case Roles::XPaddingBytesMaxRole: return pad.bytesMax;
    case Roles::XPaddingObfsModeRole: return pad.obfsMode;
    case Roles::XPaddingKeyRole: return pad.key;
    case Roles::XPaddingHeaderRole: return pad.header;
    case Roles::XPaddingPlacementRole: return pad.placement;
    case Roles::XPaddingMethodRole: return pad.method;

    // ── xmux ──────────────────────────────────────────────────────────
    case Roles::XmuxEnabledRole: return mux.enabled;
    case Roles::XmuxMaxConcurrencyMinRole: return mux.maxConcurrencyMin;
    case Roles::XmuxMaxConcurrencyMaxRole: return mux.maxConcurrencyMax;
    case Roles::XmuxMaxConnectionsMinRole: return mux.maxConnectionsMin;
    case Roles::XmuxMaxConnectionsMaxRole: return mux.maxConnectionsMax;
    case Roles::XmuxCMaxReuseTimesMinRole: return mux.cMaxReuseTimesMin;
    case Roles::XmuxCMaxReuseTimesMaxRole: return mux.cMaxReuseTimesMax;
    case Roles::XmuxHMaxRequestTimesMinRole: return mux.hMaxRequestTimesMin;
    case Roles::XmuxHMaxRequestTimesMaxRole: return mux.hMaxRequestTimesMax;
    case Roles::XmuxHMaxReusableSecsMinRole: return mux.hMaxReusableSecsMin;
    case Roles::XmuxHMaxReusableSecsMaxRole: return mux.hMaxReusableSecsMax;
    case Roles::XmuxHKeepAlivePeriodRole: return mux.hKeepAlivePeriod;
    }

    return QVariant();
}

void XrayConfigModel::updateModel(amnezia::DockerContainer container,
                                  const amnezia::XrayProtocolConfig& protocolConfig)
{
    beginResetModel();
    m_container = container;
    m_protocolConfig = protocolConfig;
    applyDefaultsToServerConfig(m_protocolConfig.serverConfig);
    m_originalProtocolConfig = m_protocolConfig;
    endResetModel();
}

void XrayConfigModel::applyDefaultsToServerConfig(amnezia::XrayServerConfig& config)
{
    if (config.port.isEmpty())
        config.port = protocols::xray::defaultPort;

    if (config.site.isEmpty())
        config.site = protocols::xray::defaultSite;

    if (config.transport.isEmpty())
        config.transport = "raw";

    if (config.security.isEmpty())
        config.security = "reality";

    if (config.flow.isEmpty())
        config.flow = "xtls-rprx-vision";

    if (config.fingerprint.isEmpty())
        config.fingerprint = "Mozilla/5.0";

    if (config.sni.isEmpty())
        config.sni = "cdn.example.com";

    if (config.alpn.isEmpty())
        config.alpn = "HTTP/2";
}

amnezia::XrayProtocolConfig XrayConfigModel::getProtocolConfig()
{
    if (!m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig))
    {
        m_protocolConfig.clearClientConfig();
    }
    return m_protocolConfig;
}

bool XrayConfigModel::isServerSettingsEqual()
{
    return m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig);
}

QHash<int, QByteArray> XrayConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    // Main
    roles[SiteRole] = "site";
    roles[PortRole] = "port";
    roles[TransportRole] = "transport";
    roles[SecurityRole] = "security";
    roles[FlowRole] = "flow";

    // Security
    roles[FingerprintRole] = "fingerprint";
    roles[SniRole] = "sni";
    roles[AlpnRole] = "alpn";

    // XHTTP
    roles[XhttpModeRole] = "xhttpMode";
    roles[XhttpHostRole] = "xhttpHost";
    roles[XhttpPathRole] = "xhttpPath";
    roles[XhttpHeadersTemplateRole] = "xhttpHeadersTemplate";
    roles[XhttpUplinkMethodRole] = "xhttpUplinkMethod";
    roles[XhttpDisableGrpcRole] = "xhttpDisableGrpc";
    roles[XhttpDisableSseRole] = "xhttpDisableSse";

    roles[XhttpSessionPlacementRole] = "xhttpSessionPlacement";
    roles[XhttpSessionKeyRole] = "xhttpSessionKey";
    roles[XhttpSeqPlacementRole] = "xhttpSeqPlacement";
    roles[XhttpSeqKeyRole] = "xhttpSeqKey";
    roles[XhttpUplinkDataPlacementRole] = "xhttpUplinkDataPlacement";
    roles[XhttpUplinkDataKeyRole] = "xhttpUplinkDataKey";

    roles[XhttpUplinkChunkSizeRole] = "xhttpUplinkChunkSize";
    roles[XhttpScMaxBufferedPostsRole] = "xhttpScMaxBufferedPosts";
    roles[XhttpScMaxEachPostBytesMinRole] = "xhttpScMaxEachPostBytesMin";
    roles[XhttpScMaxEachPostBytesMaxRole] = "xhttpScMaxEachPostBytesMax";
    roles[XhttpScMinPostsIntervalMsMinRole] = "xhttpScMinPostsIntervalMsMin";
    roles[XhttpScMinPostsIntervalMsMaxRole] = "xhttpScMinPostsIntervalMsMax";
    roles[XhttpScStreamUpServerSecsMinRole] = "xhttpScStreamUpServerSecsMin";
    roles[XhttpScStreamUpServerSecsMaxRole] = "xhttpScStreamUpServerSecsMax";

    // mKCP
    roles[MkcpTtiRole] = "mkcpTti";
    roles[MkcpUplinkCapacityRole] = "mkcpUplinkCapacity";
    roles[MkcpDownlinkCapacityRole] = "mkcpDownlinkCapacity";
    roles[MkcpReadBufferSizeRole] = "mkcpReadBufferSize";
    roles[MkcpWriteBufferSizeRole] = "mkcpWriteBufferSize";
    roles[MkcpCongestionRole] = "mkcpCongestion";

    // xPadding
    roles[XPaddingBytesMinRole] = "xPaddingBytesMin";
    roles[XPaddingBytesMaxRole] = "xPaddingBytesMax";
    roles[XPaddingObfsModeRole] = "xPaddingObfsMode";
    roles[XPaddingKeyRole] = "xPaddingKey";
    roles[XPaddingHeaderRole] = "xPaddingHeader";
    roles[XPaddingPlacementRole] = "xPaddingPlacement";
    roles[XPaddingMethodRole] = "xPaddingMethod";

    // xmux
    roles[XmuxEnabledRole] = "xmuxEnabled";
    roles[XmuxMaxConcurrencyMinRole] = "xmuxMaxConcurrencyMin";
    roles[XmuxMaxConcurrencyMaxRole] = "xmuxMaxConcurrencyMax";
    roles[XmuxMaxConnectionsMinRole] = "xmuxMaxConnectionsMin";
    roles[XmuxMaxConnectionsMaxRole] = "xmuxMaxConnectionsMax";
    roles[XmuxCMaxReuseTimesMinRole] = "xmuxCMaxReuseTimesMin";
    roles[XmuxCMaxReuseTimesMaxRole] = "xmuxCMaxReuseTimesMax";
    roles[XmuxHMaxRequestTimesMinRole] = "xmuxHMaxRequestTimesMin";
    roles[XmuxHMaxRequestTimesMaxRole] = "xmuxHMaxRequestTimesMax";
    roles[XmuxHMaxReusableSecsMinRole] = "xmuxHMaxReusableSecsMin";
    roles[XmuxHMaxReusableSecsMaxRole] = "xmuxHMaxReusableSecsMax";
    roles[XmuxHKeepAlivePeriodRole] = "xmuxHKeepAlivePeriod";

    return roles;
}
