#ifndef XTUNVPNCONNECTION_H
#define XTUNVPNCONNECTION_H

#include <QJsonObject>
#include <QObject>

#include "core/protocols/vpnProtocol.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"
#include "core/utils/errorCodes.h"
#include "ipc.h"

using namespace amnezia;

class TunnelConfig;

class XtunVpnConnection : public QObject
{
    Q_OBJECT

public:
    explicit XtunVpnConnection(SecureServersRepository *serversRepository,
                               SecureAppSettingsRepository *appSettingsRepository, QObject *parent = nullptr);

    ErrorCode lastError() const;
    Vpn::ConnectionState connectionState() const;

public slots:
    void setRepositories(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository);
    void connectToVpn(const QString &serverId, DockerContainer container, const QJsonObject &vpnConfiguration);
    void disconnectFromVpn();
    void onKillSwitchModeChanged(bool enabled);
    void disconnectSlots();

signals:
    void connectionStateChanged(Vpn::ConnectionState state);

private:
    void setConnectionState(Vpn::ConnectionState state);
    void onTunnelStatusChanged(int status);

    TunnelConfig tunnelConfig(DockerContainer container, const QJsonObject &vpnConfiguration) const;
    KillSwitchMode killSwitchMode(bool killSwitchEnabled) const;
    QStringList splitTunnelingSites(RouteMode routeMode, const QJsonObject &vpnConfiguration) const;
    QStringList splitTunnelingApps(AppsRouteMode appsRouteMode) const;

    SecureServersRepository *m_serversRepository;
    SecureAppSettingsRepository *m_appSettingsRepository;

    Vpn::ConnectionState m_connectionState = Vpn::ConnectionState::Disconnected;
    ErrorCode m_lastError = ErrorCode::NoError;
};

#endif // XTUNVPNCONNECTION_H
