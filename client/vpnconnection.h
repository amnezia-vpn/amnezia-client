#ifndef VPNCONNECTION_H
#define VPNCONNECTION_H

#include <QObject>
#include <QMetaObject>
#include <QString>
#include <QScopedPointer>
#include <QTimer>
#include <QDateTime>

#include "protocols/vpnprotocol.h"
#include "core/defs.h"
#include "settings.h"

#ifdef AMNEZIA_DESKTOP
#include <QRemoteObjectNode>
#include "core/ipcclient.h"
#endif

#ifdef Q_OS_ANDROID
#include "protocols/android_vpnprotocol.h"
#endif

using namespace fblink;

class VpnConnection : public QObject
{
    Q_OBJECT

public:
    explicit VpnConnection(std::shared_ptr<Settings> settings, QObject* parent = nullptr);
    ~VpnConnection() override;

    static QString bytesPerSecToText(quint64 bytes);

    ErrorCode lastError() const;

    QSharedPointer<VpnProtocol> vpnProtocol() const;

    const QString &remoteAddress() const;

#ifdef Q_OS_ANDROID
    void restoreConnection();
#endif

public slots:
    void connectToVpn(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &vpnConfiguration);
    void reconnectToVpn();
    void disconnectFromVpn();

    void onKillSwitchModeChanged(bool enabled);
    void disconnectSlots();

signals:
    void bytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void connectionStateChanged(Vpn::ConnectionState state);
    void vpnProtocolError(fblink::ErrorCode error);

    void serviceIsNotReady();

protected slots:
    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void onConnectionStateChanged(Vpn::ConnectionState state);

    void setConnectionState(Vpn::ConnectionState state);

protected:
    QSharedPointer<VpnProtocol> m_vpnProtocol;

private:
    std::shared_ptr<Settings> m_settings;
    QJsonObject m_vpnConfiguration;
    QJsonObject m_routeMode;
    QJsonObject m_lastVpnConfiguration;
    QString m_remoteAddress;

    // Only for iOS for now, check counters
    QTimer m_checkTimer;

#ifdef Q_OS_ANDROID
   AndroidVpnProtocol* androidVpnProtocol = nullptr;
   bool m_androidAppActive = true;

   AndroidVpnProtocol* createDefaultAndroidVpnProtocol();
   void createAndroidConnections();
   void setAndroidAppActive(bool active);
#endif

   Vpn::ConnectionState m_connectionState;
   QTimer m_stateWatchdogTimer;
   QTimer m_recoveryTimer;
   int m_recoveryAttempts = 0;
   int m_failureBurst = 0;
   qint64 m_failureWindowStartedAt = 0;
   int m_lastServerIndex = -1;
   ServerCredentials m_lastCredentials;
   DockerContainer m_lastContainer = DockerContainer::None;
   bool m_reconnectScheduled = false;
   bool m_userRequestedDisconnect = false;

   void createProtocolConnections();
   void armStateWatchdog(Vpn::ConnectionState state);
   void handleStateWatchdogTimeout();
   void clearRecoveryState();
   void scheduleRecoveryReconnect();
   void registerFailureAndMaybeEnterSafeMode();
   bool hasMissingManagedRoutingRules() const;

   void appendSplitTunnelingConfig();
   void appendKillSwitchConfig();
};

#endif // VPNCONNECTION_H
