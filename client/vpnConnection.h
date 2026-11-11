#ifndef VPNCONNECTION_H
#define VPNCONNECTION_H

#include <QJsonObject>
#include <QObject>
#include <QTimer>

#include "core/protocols/vpnProtocol.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"
#include "core/utils/errorCodes.h"
#include "core/utils/tunnelSettings.h"

using namespace amnezia;

class VpnConnection : public QObject
{
    Q_OBJECT

public:
    explicit VpnConnection(SecureServersRepository *serversRepository,
                           SecureAppSettingsRepository *appSettingsRepository, QObject *parent = nullptr);

    ErrorCode lastError() const;
    Vpn::ConnectionState connectionState() const;

public slots:
    void setRepositories(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository);
    void connectToVpn(const QString &serverId, DockerContainer container, const QJsonObject &vpnConfiguration);
    void disconnectFromVpn();
    void onKillSwitchModeChanged(bool enabled);
#ifdef AMNEZIA_DESKTOP
    void disconnectSlots();
#endif

signals:
    void connectionStateChanged(Vpn::ConnectionState state);

private:
    void setConnectionState(Vpn::ConnectionState state);
    ErrorCode startTunnel(const TunnelSettings &settings, const QJsonObject &vpnConfiguration);
    void stopTunnel();
#ifdef AMNEZIA_DESKTOP
    void onTunnelStatusChanged(int status);
#else
    void onPlatformConnectionStateChanged(Vpn::ConnectionState state);
#endif

    SecureServersRepository *m_serversRepository;
    SecureAppSettingsRepository *m_appSettingsRepository;

    Vpn::ConnectionState m_connectionState = Vpn::ConnectionState::Disconnected;
    ErrorCode m_lastError = ErrorCode::NoError;

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    QTimer m_checkTimer;
#endif
};

#endif // VPNCONNECTION_H
