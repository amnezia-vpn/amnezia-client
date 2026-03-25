#ifndef XRAYCONFIGMODEL_H
#define XRAYCONFIGMODEL_H

#include <QAbstractListModel>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/xrayProtocolConfig.h"

class XrayConfigModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles
    {
        // ── Main page ─────────────────────────────────────────────────
        SiteRole = Qt::UserRole + 1,
        PortRole,
        TransportRole, // "raw" | "xhttp" | "mkcp"   (display in main page row)
        SecurityRole, // "none" | "tls" | "reality" (display in main page row)
        FlowRole, // "" | "xtls-rprx-vision" | "xtls-rprx-vision-udp443"

        // ── Security ──────────────────────────────────────────────────
        FingerprintRole,
        SniRole,
        AlpnRole,

        // ── Transport — XHTTP ─────────────────────────────────────────
        XhttpModeRole,
        XhttpHostRole,
        XhttpPathRole,
        XhttpHeadersTemplateRole,
        XhttpUplinkMethodRole,
        XhttpDisableGrpcRole,
        XhttpDisableSseRole,

        // Session & Sequence
        XhttpSessionPlacementRole,
        XhttpSessionKeyRole,
        XhttpSeqPlacementRole,
        XhttpSeqKeyRole,
        XhttpUplinkDataPlacementRole,
        XhttpUplinkDataKeyRole,

        // Traffic Shaping
        XhttpUplinkChunkSizeRole,
        XhttpScMaxBufferedPostsRole,
        XhttpScMaxEachPostBytesMinRole,
        XhttpScMaxEachPostBytesMaxRole,
        XhttpScMinPostsIntervalMsMinRole,
        XhttpScMinPostsIntervalMsMaxRole,
        XhttpScStreamUpServerSecsMinRole,
        XhttpScStreamUpServerSecsMaxRole,

        // ── Transport — mKCP ──────────────────────────────────────────
        MkcpTtiRole,
        MkcpUplinkCapacityRole,
        MkcpDownlinkCapacityRole,
        MkcpReadBufferSizeRole,
        MkcpWriteBufferSizeRole,
        MkcpCongestionRole,

        // ── xPadding ──────────────────────────────────────────────────
        XPaddingBytesMinRole,
        XPaddingBytesMaxRole,
        XPaddingObfsModeRole,
        XPaddingKeyRole,
        XPaddingHeaderRole,
        XPaddingPlacementRole,
        XPaddingMethodRole,

        // ── xmux ──────────────────────────────────────────────────────
        XmuxEnabledRole,
        XmuxMaxConcurrencyMinRole,
        XmuxMaxConcurrencyMaxRole,
        XmuxMaxConnectionsMinRole,
        XmuxMaxConnectionsMaxRole,
        XmuxCMaxReuseTimesMinRole,
        XmuxCMaxReuseTimesMaxRole,
        XmuxHMaxRequestTimesMinRole,
        XmuxHMaxRequestTimesMaxRole,
        XmuxHMaxReusableSecsMinRole,
        XmuxHMaxReusableSecsMaxRole,
        XmuxHKeepAlivePeriodRole,
    };

    explicit XrayConfigModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::XrayProtocolConfig& protocolConfig);
    amnezia::XrayProtocolConfig getProtocolConfig();
    bool isServerSettingsEqual();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    amnezia::DockerContainer m_container;
    amnezia::XrayProtocolConfig m_protocolConfig;
    amnezia::XrayProtocolConfig m_originalProtocolConfig;

    void applyDefaultsToServerConfig(amnezia::XrayServerConfig& config);
};

#endif // XRAYCONFIGMODEL_H
