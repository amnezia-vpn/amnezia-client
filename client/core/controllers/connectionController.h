#ifndef CONNECTIONCONTROLLER_H
#define CONNECTIONCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QPair>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <memory>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/secureServersRepository.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/protocols/vpnProtocol.h"
#include "vpnConnection.h"

using namespace amnezia;

class ConnectionController : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionController(SecureServersRepository* serversRepository,
                                 SecureAppSettingsRepository* appSettingsRepository,
                                 VpnConnection* vpnConnection,
                                 QObject* parent = nullptr);
    ~ConnectionController() = default;

    ErrorCode prepareConnection(const QString &serverId,
                               QJsonObject& vpnConfiguration,
                               DockerContainer& container);

    ErrorCode openConnection(const QString &serverId);

    void closeConnection();

#ifdef Q_OS_ANDROID
    void restoreConnection(Vpn::ConnectionState state, int serverIndex);
#endif

    void onKillSwitchModeChanged(bool enabled);
    void onManagedSplitTunnelingRulesPublished(int serverIndex);

    ErrorCode lastConnectionError() const;

    bool isConnected() const;
    void setConnectionState(Vpn::ConnectionState state);

    QJsonObject createConnectionConfiguration(int serverIndex,
                                             const QPair<QString, QString> &dns,
                                             bool isApiConfig,
                                             const QString &hostName,
                                             const QString &description,
                                             int configVersion,
                                             const ContainerConfig &containerConfig,
                                             DockerContainer container);

    bool isServiceReady() const;

    bool isContainerSupported(DockerContainer container) const;

signals:
    void connectionStateChanged(Vpn::ConnectionState state);
    void serverRoutingRulesChanged(int serverIndex);
    void openConnectionRequested(const QString &serverId, DockerContainer container, const QJsonObject &vpnConfiguration);
    void closeConnectionRequested();
    void setConnectionStateRequested(Vpn::ConnectionState state);
    void killSwitchModeChangedRequested(bool enabled);

#ifdef Q_OS_ANDROID
    void restoreConnectionRequested(int serverIndex, DockerContainer container, const QJsonObject &vpnConfiguration,
                                    Vpn::ConnectionState state);
#endif

private:
    void onVpnConnectionStateChanged(Vpn::ConnectionState state);
    void scheduleServerRoutingRulesSync(int intervalMs);
    void scheduleNextServerRoutingRulesSync(bool success);
    void finishServerRoutingRulesSync(bool success);
    void syncServerRoutingRules();
    void syncServerRoutingRulesFromUrls(const QList<QUrl> &syncUrls, int urlIndex, int serverIndex,
                                        RouteMode oldRouteMode, const QStringList &oldSplitTunnelIps,
                                        int syncGeneration);
    bool applyServerRoutingRulesPayload(int serverIndex, const QJsonObject &payload);
    QStringList effectiveSplitTunnelIpsForSync(int serverIndex, RouteMode routeMode) const;
    int currentConnectionServerIndex() const;
    bool isCurrentConnectionServerIndex(int serverIndex) const;

    void scheduleClientManagedSitesResolve(int serverIndex);
    void startClientManagedSitesResolve();
    void resolveNextClientManagedSite();
    void finishClientManagedSitesResolve();
    void cancelClientManagedSitesResolve();

    SecureServersRepository* m_serversRepository;
    SecureAppSettingsRepository* m_appSettingsRepository;
    VpnConnection* m_vpnConnection;

    QTimer m_serverRoutingRulesSyncTimer;
    QTimer m_serverRoutingRulesClientResolveTimer;
    bool m_isServerRoutingRulesSyncInProgress = false;
    bool m_serverRoutingRulesSyncPendingRefresh = false;
    int m_serverRoutingRulesSyncFastRetryCount = 0;
    int m_serverRoutingRulesSyncGeneration = 0;

    bool m_isClientManagedSitesResolveInProgress = false;
    int m_clientManagedSitesResolveServerIndex = -1;
    QStringList m_clientManagedSitesResolveQueue;
    QJsonObject m_clientManagedSitesResolvedCache;
    RouteMode m_clientManagedSitesResolveOldRouteMode = RouteMode::VpnAllSites;
    QStringList m_clientManagedSitesResolveOldSplitTunnelIps;
};

#endif
