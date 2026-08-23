#ifndef VPNCONNECTION_H
#define VPNCONNECTION_H

#include <QObject>
#include <QMetaObject>
#include <QString>
#include <QScopedPointer>
#include <QRemoteObjectNode>
#include <QElapsedTimer>
#include <QTimer>

#include "core/protocols/vpnProtocol.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/secureServersRepository.h"
#include "core/repositories/secureAppSettingsRepository.h"

#ifdef AMNEZIA_DESKTOP
#include "core/utils/ipcClient.h"
#endif

#ifdef Q_OS_ANDROID
#include "core/protocols/androidVpnProtocol.h"
#endif

using namespace amnezia;

class VpnConnection : public QObject
{
    Q_OBJECT

public:
    explicit VpnConnection(SecureServersRepository* serversRepository, SecureAppSettingsRepository* appSettingsRepository, QObject* parent = nullptr);
    ~VpnConnection() override;

    static QString bytesPerSecToText(quint64 bytes);

    ErrorCode lastError() const;
    Vpn::ConnectionState connectionState() const;

    QSharedPointer<VpnProtocol> vpnProtocol() const;

    const QString &remoteAddress() const;
    void addSitesRoutes(const QString &gw, amnezia::RouteMode mode);

#ifdef Q_OS_ANDROID
    void restoreConnection();
#endif

public slots:
    void setRepositories(SecureServersRepository* serversRepository, SecureAppSettingsRepository* appSettingsRepository);
    void connectToVpn(const QString &serverId, DockerContainer container, const QJsonObject &vpnConfiguration);
    void disconnectFromVpn();

    void onKillSwitchModeChanged(bool enabled);
    void disconnectSlots();

    void setConnectionState(Vpn::ConnectionState state);

signals:
    void bytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void connectionStateChanged(Vpn::ConnectionState state);
    void vpnProtocolError(amnezia::ErrorCode error);

    void serviceIsNotReady();

protected slots:
    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void onConnectionStateChanged(Vpn::ConnectionState state);

private slots:
    void onProtocolConnectionStateChanged(Vpn::ConnectionState state);
#ifdef AMNEZIA_DESKTOP
    void onIpcWakeup();
    void onIpcNetworkChanged();
    void startReconnectAttempt();
    void onReconnectWatchdogTimeout();
#endif

protected:
    QSharedPointer<VpnProtocol> m_vpnProtocol;

private:
    SecureServersRepository* m_serversRepository;
    SecureAppSettingsRepository* m_appSettingsRepository;

    QJsonObject m_vpnConfiguration;
    QJsonObject m_routeMode;
    QString m_remoteAddress;

    // Only for iOS for now, check counters
    QTimer m_checkTimer;

#ifdef Q_OS_ANDROID
   AndroidVpnProtocol* androidVpnProtocol = nullptr;

   AndroidVpnProtocol* createDefaultAndroidVpnProtocol();
   void createAndroidConnections();
#endif

   Vpn::ConnectionState m_connectionState;

   void createProtocolConnections();

   void appendSplitTunnelingConfig();
   void appendKillSwitchConfig();

#ifdef AMNEZIA_DESKTOP
   // Auto-reconnect state machine (wakeup / network change). While it is
   // active the UI is held in the Reconnecting state and stop()/start()
   // attempts are retried with backoff until the protocol reports Connected
   // or the user cancels via connect/disconnect.
   void requestReconnect(const QString &trigger);
   void scheduleReconnectRetry();
   void cancelReconnect();
   int reconnectRetryDelayMsec() const;

   bool m_reconnectActive = false;          // machine engaged, UI shows Reconnecting
   bool m_reconnectAttemptInFlight = false; // start() issued, waiting for the outcome
   int m_reconnectAttempt = 0;              // attempts since the last trigger, drives backoff
   QTimer m_reconnectRetryTimer{this};      // single-shot, schedules the next attempt
   QTimer m_reconnectWatchdogTimer{this};   // single-shot, bounds a single attempt
   QElapsedTimer m_reconnectAttemptAge;     // how long the in-flight attempt has been running
#endif
};

#endif // VPNCONNECTION_H
