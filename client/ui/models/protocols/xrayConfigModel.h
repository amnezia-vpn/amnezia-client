#ifndef XRAYCONFIGMODEL_H
#define XRAYCONFIGMODEL_H

#include <QObject>
#include <QStringList>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/xrayProtocolConfig.h"

class XrayConfigModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY hasUnsavedChangesChanged)
    Q_PROPERTY(QString site READ site WRITE setSite NOTIFY changed)
    Q_PROPERTY(QString port READ port WRITE setPort NOTIFY changed)
    Q_PROPERTY(QString transport READ transport WRITE setTransport NOTIFY changed)
    Q_PROPERTY(QString security READ security WRITE setSecurity NOTIFY changed)
    Q_PROPERTY(QString flow READ flow WRITE setFlow NOTIFY changed)
    Q_PROPERTY(QString fingerprint READ fingerprint WRITE setFingerprint NOTIFY changed)
    Q_PROPERTY(QString sni READ sni WRITE setSni NOTIFY changed)
    Q_PROPERTY(QString alpn READ alpn WRITE setAlpn NOTIFY changed)
    Q_PROPERTY(QString xhttpMode READ xhttpMode WRITE setXhttpMode NOTIFY changed)
    Q_PROPERTY(QString xhttpHost READ xhttpHost WRITE setXhttpHost NOTIFY changed)
    Q_PROPERTY(QString xhttpPath READ xhttpPath WRITE setXhttpPath NOTIFY changed)
    Q_PROPERTY(QString xhttpUplinkMethod READ xhttpUplinkMethod WRITE setXhttpUplinkMethod NOTIFY changed)
    Q_PROPERTY(bool xhttpDisableGrpc READ xhttpDisableGrpc WRITE setXhttpDisableGrpc NOTIFY changed)
    Q_PROPERTY(bool xhttpDisableSse READ xhttpDisableSse WRITE setXhttpDisableSse NOTIFY changed)
    Q_PROPERTY(QString xhttpSessionPlacement READ xhttpSessionPlacement WRITE setXhttpSessionPlacement NOTIFY changed)
    Q_PROPERTY(QString xhttpSessionKey READ xhttpSessionKey WRITE setXhttpSessionKey NOTIFY changed)
    Q_PROPERTY(QString xhttpSeqPlacement READ xhttpSeqPlacement WRITE setXhttpSeqPlacement NOTIFY changed)
    Q_PROPERTY(QString xhttpSeqKey READ xhttpSeqKey WRITE setXhttpSeqKey NOTIFY changed)
    Q_PROPERTY(QString xhttpUplinkDataPlacement READ xhttpUplinkDataPlacement WRITE setXhttpUplinkDataPlacement NOTIFY changed)
    Q_PROPERTY(QString xhttpUplinkDataKey READ xhttpUplinkDataKey WRITE setXhttpUplinkDataKey NOTIFY changed)
    Q_PROPERTY(QString xhttpUplinkChunkSize READ xhttpUplinkChunkSize WRITE setXhttpUplinkChunkSize NOTIFY changed)
    Q_PROPERTY(QString xhttpScMaxBufferedPosts READ xhttpScMaxBufferedPosts WRITE setXhttpScMaxBufferedPosts NOTIFY changed)
    Q_PROPERTY(QString xhttpScMaxEachPostBytesMin READ xhttpScMaxEachPostBytesMin WRITE setXhttpScMaxEachPostBytesMin NOTIFY changed)
    Q_PROPERTY(QString xhttpScMaxEachPostBytesMax READ xhttpScMaxEachPostBytesMax WRITE setXhttpScMaxEachPostBytesMax NOTIFY changed)
    Q_PROPERTY(QString xhttpScMinPostsIntervalMsMin READ xhttpScMinPostsIntervalMsMin WRITE setXhttpScMinPostsIntervalMsMin NOTIFY changed)
    Q_PROPERTY(QString xhttpScMinPostsIntervalMsMax READ xhttpScMinPostsIntervalMsMax WRITE setXhttpScMinPostsIntervalMsMax NOTIFY changed)
    Q_PROPERTY(QString xhttpScStreamUpServerSecsMin READ xhttpScStreamUpServerSecsMin WRITE setXhttpScStreamUpServerSecsMin NOTIFY changed)
    Q_PROPERTY(QString xhttpScStreamUpServerSecsMax READ xhttpScStreamUpServerSecsMax WRITE setXhttpScStreamUpServerSecsMax NOTIFY changed)
    Q_PROPERTY(QString mkcpTti READ mkcpTti WRITE setMkcpTti NOTIFY changed)
    Q_PROPERTY(QString mkcpMtu READ mkcpMtu WRITE setMkcpMtu NOTIFY changed)
    Q_PROPERTY(QString mkcpUplinkCapacity READ mkcpUplinkCapacity WRITE setMkcpUplinkCapacity NOTIFY changed)
    Q_PROPERTY(QString mkcpDownlinkCapacity READ mkcpDownlinkCapacity WRITE setMkcpDownlinkCapacity NOTIFY changed)
    Q_PROPERTY(QString mkcpCwndMultiplier READ mkcpCwndMultiplier WRITE setMkcpCwndMultiplier NOTIFY changed)
    Q_PROPERTY(QString mkcpMaxSendingWindow READ mkcpMaxSendingWindow WRITE setMkcpMaxSendingWindow NOTIFY changed)
    Q_PROPERTY(QString xPaddingBytesMin READ xPaddingBytesMin WRITE setXPaddingBytesMin NOTIFY changed)
    Q_PROPERTY(QString xPaddingBytesMax READ xPaddingBytesMax WRITE setXPaddingBytesMax NOTIFY changed)
    Q_PROPERTY(bool xPaddingObfsMode READ xPaddingObfsMode WRITE setXPaddingObfsMode NOTIFY changed)
    Q_PROPERTY(QString xPaddingKey READ xPaddingKey WRITE setXPaddingKey NOTIFY changed)
    Q_PROPERTY(QString xPaddingHeader READ xPaddingHeader WRITE setXPaddingHeader NOTIFY changed)
    Q_PROPERTY(QString xPaddingPlacement READ xPaddingPlacement WRITE setXPaddingPlacement NOTIFY changed)
    Q_PROPERTY(QString xPaddingMethod READ xPaddingMethod WRITE setXPaddingMethod NOTIFY changed)
    Q_PROPERTY(bool xmuxEnabled READ xmuxEnabled WRITE setXmuxEnabled NOTIFY changed)
    Q_PROPERTY(QString xmuxMaxConcurrencyMin READ xmuxMaxConcurrencyMin WRITE setXmuxMaxConcurrencyMin NOTIFY changed)
    Q_PROPERTY(QString xmuxMaxConcurrencyMax READ xmuxMaxConcurrencyMax WRITE setXmuxMaxConcurrencyMax NOTIFY changed)
    Q_PROPERTY(QString xmuxMaxConnectionsMin READ xmuxMaxConnectionsMin WRITE setXmuxMaxConnectionsMin NOTIFY changed)
    Q_PROPERTY(QString xmuxMaxConnectionsMax READ xmuxMaxConnectionsMax WRITE setXmuxMaxConnectionsMax NOTIFY changed)
    Q_PROPERTY(QString xmuxCMaxReuseTimesMin READ xmuxCMaxReuseTimesMin WRITE setXmuxCMaxReuseTimesMin NOTIFY changed)
    Q_PROPERTY(QString xmuxCMaxReuseTimesMax READ xmuxCMaxReuseTimesMax WRITE setXmuxCMaxReuseTimesMax NOTIFY changed)
    Q_PROPERTY(QString xmuxHMaxRequestTimesMin READ xmuxHMaxRequestTimesMin WRITE setXmuxHMaxRequestTimesMin NOTIFY changed)
    Q_PROPERTY(QString xmuxHMaxRequestTimesMax READ xmuxHMaxRequestTimesMax WRITE setXmuxHMaxRequestTimesMax NOTIFY changed)
    Q_PROPERTY(QString xmuxHMaxReusableSecsMin READ xmuxHMaxReusableSecsMin WRITE setXmuxHMaxReusableSecsMin NOTIFY changed)
    Q_PROPERTY(QString xmuxHMaxReusableSecsMax READ xmuxHMaxReusableSecsMax WRITE setXmuxHMaxReusableSecsMax NOTIFY changed)
    Q_PROPERTY(QString xmuxHKeepAlivePeriod READ xmuxHKeepAlivePeriod WRITE setXmuxHKeepAlivePeriod NOTIFY changed)

public:
    explicit XrayConfigModel(QObject* parent = nullptr);

    QString site() const;
    void setSite(const QString &value);

    QString port() const;
    void setPort(const QString &value);

    QString transport() const;
    void setTransport(const QString &value);

    QString security() const;
    void setSecurity(const QString &value);

    QString flow() const;
    void setFlow(const QString &value);

    QString fingerprint() const;
    void setFingerprint(const QString &value);

    QString sni() const;
    void setSni(const QString &value);

    QString alpn() const;
    void setAlpn(const QString &value);

    QString xhttpMode() const;
    void setXhttpMode(const QString &value);

    QString xhttpHost() const;
    void setXhttpHost(const QString &value);

    QString xhttpPath() const;
    void setXhttpPath(const QString &value);

    QString xhttpUplinkMethod() const;
    void setXhttpUplinkMethod(const QString &value);

    bool xhttpDisableGrpc() const;
    void setXhttpDisableGrpc(bool value);

    bool xhttpDisableSse() const;
    void setXhttpDisableSse(bool value);

    QString xhttpSessionPlacement() const;
    void setXhttpSessionPlacement(const QString &value);

    QString xhttpSessionKey() const;
    void setXhttpSessionKey(const QString &value);

    QString xhttpSeqPlacement() const;
    void setXhttpSeqPlacement(const QString &value);

    QString xhttpSeqKey() const;
    void setXhttpSeqKey(const QString &value);

    QString xhttpUplinkDataPlacement() const;
    void setXhttpUplinkDataPlacement(const QString &value);

    QString xhttpUplinkDataKey() const;
    void setXhttpUplinkDataKey(const QString &value);

    QString xhttpUplinkChunkSize() const;
    void setXhttpUplinkChunkSize(const QString &value);

    QString xhttpScMaxBufferedPosts() const;
    void setXhttpScMaxBufferedPosts(const QString &value);

    QString xhttpScMaxEachPostBytesMin() const;
    void setXhttpScMaxEachPostBytesMin(const QString &value);

    QString xhttpScMaxEachPostBytesMax() const;
    void setXhttpScMaxEachPostBytesMax(const QString &value);

    QString xhttpScMinPostsIntervalMsMin() const;
    void setXhttpScMinPostsIntervalMsMin(const QString &value);

    QString xhttpScMinPostsIntervalMsMax() const;
    void setXhttpScMinPostsIntervalMsMax(const QString &value);

    QString xhttpScStreamUpServerSecsMin() const;
    void setXhttpScStreamUpServerSecsMin(const QString &value);

    QString xhttpScStreamUpServerSecsMax() const;
    void setXhttpScStreamUpServerSecsMax(const QString &value);

    QString mkcpTti() const;
    void setMkcpTti(const QString &value);

    QString mkcpMtu() const;
    void setMkcpMtu(const QString &value);

    QString mkcpUplinkCapacity() const;
    void setMkcpUplinkCapacity(const QString &value);

    QString mkcpDownlinkCapacity() const;
    void setMkcpDownlinkCapacity(const QString &value);

    QString mkcpCwndMultiplier() const;
    void setMkcpCwndMultiplier(const QString &value);

    QString mkcpMaxSendingWindow() const;
    void setMkcpMaxSendingWindow(const QString &value);

    QString xPaddingBytesMin() const;
    void setXPaddingBytesMin(const QString &value);

    QString xPaddingBytesMax() const;
    void setXPaddingBytesMax(const QString &value);

    bool xPaddingObfsMode() const;
    void setXPaddingObfsMode(bool value);

    QString xPaddingKey() const;
    void setXPaddingKey(const QString &value);

    QString xPaddingHeader() const;
    void setXPaddingHeader(const QString &value);

    QString xPaddingPlacement() const;
    void setXPaddingPlacement(const QString &value);

    QString xPaddingMethod() const;
    void setXPaddingMethod(const QString &value);

    bool xmuxEnabled() const;
    void setXmuxEnabled(bool value);

    QString xmuxMaxConcurrencyMin() const;
    void setXmuxMaxConcurrencyMin(const QString &value);

    QString xmuxMaxConcurrencyMax() const;
    void setXmuxMaxConcurrencyMax(const QString &value);

    QString xmuxMaxConnectionsMin() const;
    void setXmuxMaxConnectionsMin(const QString &value);

    QString xmuxMaxConnectionsMax() const;
    void setXmuxMaxConnectionsMax(const QString &value);

    QString xmuxCMaxReuseTimesMin() const;
    void setXmuxCMaxReuseTimesMin(const QString &value);

    QString xmuxCMaxReuseTimesMax() const;
    void setXmuxCMaxReuseTimesMax(const QString &value);

    QString xmuxHMaxRequestTimesMin() const;
    void setXmuxHMaxRequestTimesMin(const QString &value);

    QString xmuxHMaxRequestTimesMax() const;
    void setXmuxHMaxRequestTimesMax(const QString &value);

    QString xmuxHMaxReusableSecsMin() const;
    void setXmuxHMaxReusableSecsMin(const QString &value);

    QString xmuxHMaxReusableSecsMax() const;
    void setXmuxHMaxReusableSecsMax(const QString &value);

    QString xmuxHKeepAlivePeriod() const;
    void setXmuxHKeepAlivePeriod(const QString &value);

    // ── Static option lists (for QML DropDown models) ─────────────────
    Q_INVOKABLE static QStringList flowOptions();
    Q_INVOKABLE static QStringList securityOptions();
    Q_INVOKABLE static QStringList transportOptions();
    Q_INVOKABLE static QStringList fingerprintOptions();
    Q_INVOKABLE static QStringList alpnOptions();
    Q_INVOKABLE static QStringList xhttpModeOptions();
    Q_INVOKABLE static QStringList xhttpUplinkMethodOptions();
    Q_INVOKABLE static QStringList xhttpSessionPlacementOptions();
    Q_INVOKABLE static QStringList xhttpSeqPlacementOptions();
    Q_INVOKABLE static QStringList xhttpUplinkDataPlacementOptions();
    Q_INVOKABLE static QStringList xPaddingPlacementOptions();
    Q_INVOKABLE static QStringList xPaddingMethodOptions();

    // mKCP display defaults (protocolConstants.h — must match xrayConfigurator empty-field behavior)
    Q_INVOKABLE static QString mkcpDefaultTti();
    Q_INVOKABLE static QString mkcpDefaultUplinkCapacity();
    Q_INVOKABLE static QString mkcpDefaultDownlinkCapacity();
    Q_INVOKABLE static QString mkcpDefaultMtu();
    Q_INVOKABLE static QString mkcpDefaultCwndMultiplier();

    Q_INVOKABLE static QString portDefault();
    Q_INVOKABLE static QString sniDefault();
    Q_INVOKABLE static QString xhttpHostDefault();
    Q_INVOKABLE static QString xhttpUplinkChunkSizeDefault();
    Q_INVOKABLE static QString scMaxEachPostBytesMinDefault();
    Q_INVOKABLE static QString scMaxEachPostBytesMaxDefault();
    Q_INVOKABLE static QString scMinPostsIntervalMsMinDefault();
    Q_INVOKABLE static QString scMinPostsIntervalMsMaxDefault();
    Q_INVOKABLE static QString scStreamUpServerSecsMinDefault();
    Q_INVOKABLE static QString scStreamUpServerSecsMaxDefault();
    Q_INVOKABLE static QString xPaddingKeyDefault();
    Q_INVOKABLE static QString xPaddingHeaderDefault();
    Q_INVOKABLE static QString xPaddingBytesMinDefault();
    Q_INVOKABLE static QString xPaddingBytesMaxDefault();

    Q_INVOKABLE static bool isValidHost(const QString &host);
    Q_INVOKABLE static bool isValidSni(const QString &sni);
    Q_INVOKABLE QStringList validationErrors() const;

    Q_INVOKABLE bool pendingChangeTouchesServer(const QString &pendingPort) const;
    Q_INVOKABLE bool pendingChangeBreaksIssuedConfigs(const QString &pendingPort) const;
    Q_INVOKABLE bool pendingChangeRequiresReinstall(const QString &pendingPort) const;

    Q_INVOKABLE QString saveDescription(const QString &pendingPort) const;

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::XrayProtocolConfig& protocolConfig);
    amnezia::XrayProtocolConfig getProtocolConfig();
    amnezia::XrayServerConfig pendingServerConfig(const QString &pendingPort) const;

    bool isServerSettingsEqual() const;
    bool hasUnsavedChanges() const;
    void resetToDefaults();
    void applyServerConfig(const amnezia::XrayServerConfig &serverConfig);

signals:
    void changed();
    void hasUnsavedChangesChanged();

private:
    amnezia::DockerContainer m_container;
    amnezia::XrayProtocolConfig m_protocolConfig;
    amnezia::XrayProtocolConfig m_originalProtocolConfig;

    void applyDefaultsToServerConfig(amnezia::XrayServerConfig& config, bool fillFlowDefault);
    void assignField(QString &field, const QString &value);
    void assignField(bool &field, bool value);
    void emitChanged(bool wasUnsavedChanges);
};

#endif // XRAYCONFIGMODEL_H
