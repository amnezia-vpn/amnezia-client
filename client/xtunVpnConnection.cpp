#include "xtunVpnConnection.h"

#include <QDebug>

#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/ipcClient.h"
#include "core/utils/networkUtilities.h"

XtunVpnConnection::XtunVpnConnection(SecureServersRepository *serversRepository,
                                     SecureAppSettingsRepository *appSettingsRepository, QObject *parent)
    : QObject(parent), m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository)
{
}

ErrorCode XtunVpnConnection::lastError() const
{
    return m_lastError;
}

Vpn::ConnectionState XtunVpnConnection::connectionState() const
{
    return m_connectionState;
}

void XtunVpnConnection::setRepositories(SecureServersRepository *serversRepository,
                                        SecureAppSettingsRepository *appSettingsRepository)
{
    m_serversRepository = serversRepository;
    m_appSettingsRepository = appSettingsRepository;
}

void XtunVpnConnection::connectToVpn(const QString &serverId, DockerContainer container,
                                     const QJsonObject &vpnConfiguration)
{
    if (!m_appSettingsRepository || !m_serversRepository) {
        qCritical() << "XtunVpnConnection::connectToVpn: repositories not initialized";
        m_lastError = ErrorCode::InternalError;
        setConnectionState(Vpn::ConnectionState::Error);
        return;
    }

    qDebug() << "XtunVpnConnection::connectToVpn: server id" << serverId << "container"
             << ContainerUtils::containerToString(container);

    m_lastError = ErrorCode::NoError;
    setConnectionState(Vpn::ConnectionState::Connecting);

    const TunnelConfig config = tunnelConfig(container, vpnConfiguration);
    m_lastError = IpcClient::withInterface(
            [this, &config](QSharedPointer<IpcInterfaceReplica> iface) {
                connect(iface.data(), &IpcInterfaceReplica::tunnelStatusChanged, this,
                        &XtunVpnConnection::onTunnelStatusChanged, Qt::UniqueConnection);

                auto reply = iface->tunnelUp(config);
                if (!reply.waitForFinished()) {
                    return ErrorCode::AmneziaServiceConnectionFailed;
                }
                return static_cast<ErrorCode>(reply.returnValue());
            },
            []() { return ErrorCode::AmneziaServiceNotRunning; });

    if (m_lastError != ErrorCode::NoError) {
        qWarning() << "XtunVpnConnection::connectToVpn: tunnel failed with error" << m_lastError;
        setConnectionState(Vpn::ConnectionState::Error);
    }
}

void XtunVpnConnection::disconnectFromVpn()
{
    if (m_connectionState == Vpn::ConnectionState::Disconnected) {
        setConnectionState(Vpn::ConnectionState::Disconnected);
        return;
    }

    setConnectionState(Vpn::ConnectionState::Disconnecting);

    IpcClient::withInterface([](QSharedPointer<IpcInterfaceReplica> iface) {
        auto reply = iface->tunnelDown();
        if (!reply.waitForFinished()) {
            qWarning() << "XtunVpnConnection::disconnectFromVpn: failed to execute remote tunnelDown call";
        }
    });

    setConnectionState(Vpn::ConnectionState::Disconnected);
}

void XtunVpnConnection::onKillSwitchModeChanged(bool enabled)
{
    const KillSwitchMode mode = killSwitchMode(enabled);
    IpcClient::withInterface([mode](QSharedPointer<IpcInterfaceReplica> iface) {
        auto reply = iface->setKillSwitchMode(static_cast<int>(mode));
        if (!reply.waitForFinished() || !reply.returnValue()) {
            qWarning() << "XtunVpnConnection::onKillSwitchModeChanged: failed to execute remote setKillSwitchMode call";
        }
    });
}

void XtunVpnConnection::disconnectSlots()
{
    IpcClient::withInterface([this](QSharedPointer<IpcInterfaceReplica> iface) { iface->disconnect(this); });
}

void XtunVpnConnection::setConnectionState(Vpn::ConnectionState state)
{
    m_connectionState = state;
    emit connectionStateChanged(state);
}

void XtunVpnConnection::onTunnelStatusChanged(int status)
{
    if (static_cast<TunnelStatus>(status) == TunnelStatus::Connected) {
        if (m_connectionState == Vpn::ConnectionState::Connecting
            || m_connectionState == Vpn::ConnectionState::Reconnecting) {
            setConnectionState(Vpn::ConnectionState::Connected);
        }
        return;
    }

    if (m_connectionState == Vpn::ConnectionState::Connected) {
        setConnectionState(Vpn::ConnectionState::Reconnecting);
    }
}

TunnelConfig XtunVpnConnection::tunnelConfig(DockerContainer container, const QJsonObject &vpnConfiguration) const
{
    const Proto proto = ContainerUtils::defaultProtocol(container);
    const QJsonObject protocolConfig = vpnConfiguration.value(ProtocolUtils::key_proto_config_data(proto)).toObject();

    const QString primaryDns = vpnConfiguration.value(configKey::dns1).toString();
    const QString secondaryDns = vpnConfiguration.value(configKey::dns2).toString();
    QStringList dnsServers { primaryDns };
    if (!primaryDns.contains(protocols::dns::amneziaDnsIp) && !secondaryDns.isEmpty()) {
        dnsServers.append(secondaryDns);
    }

    RouteMode routeMode = RouteMode::VpnAllSites;
    QStringList sites;
    if (m_appSettingsRepository->isSitesSplitTunnelingEnabled()) {
        sites = splitTunnelingSites(m_appSettingsRepository->routeMode(), vpnConfiguration);
        if (!sites.isEmpty()) {
            routeMode = m_appSettingsRepository->routeMode();
        }
    }

    AppsRouteMode appsRouteMode = AppsRouteMode::VpnAllApps;
    QStringList apps;
    if (m_appSettingsRepository->isAppsSplitTunnelingEnabled()) {
        apps = splitTunnelingApps(m_appSettingsRepository->appsRouteMode());
        if (!apps.isEmpty()) {
            appsRouteMode = m_appSettingsRepository->appsRouteMode();
        }
    }

    return TunnelConfig(static_cast<int>(proto), protocolConfig.value(configKey::config).toString(), dnsServers,
                        static_cast<int>(killSwitchMode(m_appSettingsRepository->isKillSwitchEnabled())),
                        m_appSettingsRepository->getAllowedDnsServers(), static_cast<int>(routeMode), sites,
                        static_cast<int>(appsRouteMode), apps, protocolConfig.value(configKey::mtu).toString().toInt());
}

KillSwitchMode XtunVpnConnection::killSwitchMode(bool killSwitchEnabled) const
{
    if (!killSwitchEnabled) {
        return KillSwitchMode::Disabled;
    }
    return m_appSettingsRepository->isStrictKillSwitchEnabled() ? KillSwitchMode::Hard : KillSwitchMode::Soft;
}

QStringList XtunVpnConnection::splitTunnelingSites(RouteMode routeMode, const QJsonObject &vpnConfiguration) const
{
    QStringList sites;
    const QVariantMap vpnSites = m_appSettingsRepository->vpnSites(routeMode);
    for (auto it = vpnSites.constBegin(); it != vpnSites.constEnd(); ++it) {
        if (NetworkUtilities::checkIpSubnetFormat(it.key())) {
            sites.append(it.key());
            continue;
        }
        for (const QString &ip : SecureAppSettingsRepository::siteIpList(it.value())) {
            if (NetworkUtilities::checkIpSubnetFormat(ip)) {
                sites.append(ip);
            }
        }
    }
    sites.removeDuplicates();

    if (!sites.isEmpty() && routeMode == RouteMode::VpnOnlyForwardSites) {
        sites.append(vpnConfiguration.value(configKey::dns1).toString());
        sites.append(vpnConfiguration.value(configKey::dns2).toString());
    }
    return sites;
}

QStringList XtunVpnConnection::splitTunnelingApps(AppsRouteMode appsRouteMode) const
{
    QStringList apps;
    for (const auto &app : m_appSettingsRepository->vpnApps(appsRouteMode)) {
        apps.append(app.appPath.isEmpty() ? app.packageName : app.appPath);
    }
    return apps;
}
