#ifndef XRAYCONFIGMODEL_H
#define XRAYCONFIGMODEL_H

#include <QAbstractListModel>
#include <QStringList>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/xrayProtocolConfig.h"

class XrayConfigModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY hasUnsavedChangesChanged)

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

    // ── Static option lists (for QML DropDown models) ─────────────────
    Q_INVOKABLE static QStringList flowOptions();
    Q_INVOKABLE static QStringList securityOptions();
    Q_INVOKABLE static QStringList transportOptions();
    Q_INVOKABLE static QStringList fingerprintOptions();
    Q_INVOKABLE static QStringList alpnOptions();
    Q_INVOKABLE static QStringList xhttpModeOptions();
    Q_INVOKABLE static QStringList xhttpHeadersTemplateOptions();
    Q_INVOKABLE static QStringList xhttpUplinkMethodOptions();
    Q_INVOKABLE static QStringList xhttpSessionPlacementOptions();
    Q_INVOKABLE static QStringList xhttpSessionKeyOptions();
    Q_INVOKABLE static QStringList xhttpSeqPlacementOptions();
    Q_INVOKABLE static QStringList xhttpUplinkDataPlacementOptions();
    Q_INVOKABLE static QStringList xPaddingPlacementOptions();
    Q_INVOKABLE static QStringList xPaddingMethodOptions();

    // mKCP display defaults (protocolConstants.h — must match xrayConfigurator empty-field behavior)
    Q_INVOKABLE static QString mkcpDefaultTti();
    Q_INVOKABLE static QString mkcpDefaultUplinkCapacity();
    Q_INVOKABLE static QString mkcpDefaultDownlinkCapacity();
    Q_INVOKABLE static QString mkcpDefaultReadBufferSize();
    Q_INVOKABLE static QString mkcpDefaultWriteBufferSize();

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::XrayProtocolConfig& protocolConfig);
    amnezia::XrayProtocolConfig getProtocolConfig();
    bool isServerSettingsEqual() const;
    bool hasUnsavedChanges() const;
    void resetToDefaults();
    void applyServerConfig(const amnezia::XrayServerConfig &serverConfig);

signals:
    void hasUnsavedChangesChanged();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    amnezia::DockerContainer m_container;
    amnezia::XrayProtocolConfig m_protocolConfig;
    amnezia::XrayProtocolConfig m_originalProtocolConfig;

    void applyDefaultsToServerConfig(amnezia::XrayServerConfig& config, bool fillFlowDefault = true);
};

#endif // XRAYCONFIGMODEL_H
