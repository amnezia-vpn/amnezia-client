#include "vpnConnection.h"

#include <QDebug>

#include "core/utils/constants/configKeys.h"
#include "core/utils/containers/containerUtils.h"

#ifdef AMNEZIA_DESKTOP
    #include "core/utils/ipcClient.h"
#endif

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
#endif

namespace
{
#ifndef AMNEZIA_DESKTOP
    QJsonObject platformTunnelConfig(const TunnelSettings &settings, const QJsonObject &vpnConfiguration)
    {
        QJsonObject config = settings.toJson();
        config.insert(configKey::description, vpnConfiguration.value(configKey::description));
        config.insert(configKey::hostName, vpnConfiguration.value(configKey::hostName));
        return config;
    }
#endif
}

VpnConnection::VpnConnection(SecureServersRepository *serversRepository,
                             SecureAppSettingsRepository *appSettingsRepository, QObject *parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository)
#if defined(Q_OS_IOS) || defined(MACOS_NE)
      ,
      m_checkTimer(this)
#endif
{
#ifdef Q_OS_ANDROID
    connect(AndroidController::instance(), &AndroidController::connectionStateChanged, this,
            &VpnConnection::onPlatformConnectionStateChanged);
    connect(AndroidController::instance(), &AndroidController::initConnectionState, this,
            &VpnConnection::setConnectionState);
#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    m_checkTimer.setInterval(1000);
    connect(&m_checkTimer, &QTimer::timeout, IosController::Instance(), &IosController::checkStatus);
    connect(IosController::Instance(), &IosController::connectionStateChanged, this,
            &VpnConnection::onPlatformConnectionStateChanged);
#endif
}

ErrorCode VpnConnection::lastError() const
{
    return m_lastError;
}

Vpn::ConnectionState VpnConnection::connectionState() const
{
    return m_connectionState;
}

void VpnConnection::setRepositories(SecureServersRepository *serversRepository,
                                    SecureAppSettingsRepository *appSettingsRepository)
{
    m_serversRepository = serversRepository;
    m_appSettingsRepository = appSettingsRepository;
}

void VpnConnection::connectToVpn(const QString &serverId, DockerContainer container, const QJsonObject &vpnConfiguration)
{
    if (!m_appSettingsRepository || !m_serversRepository) {
        qCritical() << "VpnConnection::connectToVpn: repositories not initialized";
        m_lastError = ErrorCode::InternalError;
        setConnectionState(Vpn::ConnectionState::Error);
        return;
    }

    qDebug() << "VpnConnection::connectToVpn: server id" << serverId << "container"
             << ContainerUtils::containerToString(container);

    m_lastError = ErrorCode::NoError;
    setConnectionState(Vpn::ConnectionState::Connecting);

    m_lastError = startTunnel(tunnelSettings(*m_appSettingsRepository, container, vpnConfiguration), vpnConfiguration);
    if (m_lastError != ErrorCode::NoError) {
        qWarning() << "VpnConnection::connectToVpn: tunnel failed with error" << m_lastError;
        setConnectionState(Vpn::ConnectionState::Error);
    }
}

void VpnConnection::disconnectFromVpn()
{
    if (m_connectionState == Vpn::ConnectionState::Disconnected) {
        setConnectionState(Vpn::ConnectionState::Disconnected);
        return;
    }

    setConnectionState(Vpn::ConnectionState::Disconnecting);
    stopTunnel();
}

void VpnConnection::setConnectionState(Vpn::ConnectionState state)
{
    m_connectionState = state;

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    if (state == Vpn::ConnectionState::Connected || state == Vpn::ConnectionState::Connecting
        || state == Vpn::ConnectionState::Reconnecting) {
        m_checkTimer.start();
    } else {
        m_checkTimer.stop();
    }
#endif

    emit connectionStateChanged(state);
}

#ifndef AMNEZIA_DESKTOP
void VpnConnection::onPlatformConnectionStateChanged(Vpn::ConnectionState state)
{
    if (state == Vpn::ConnectionState::Error && m_lastError == ErrorCode::NoError) {
    #ifdef Q_OS_ANDROID
        m_lastError = ErrorCode::AndroidError;
    #else
        m_lastError = ErrorCode::InternalError;
    #endif
    }
    setConnectionState(state);
}
#endif

#ifdef AMNEZIA_DESKTOP

ErrorCode VpnConnection::startTunnel(const TunnelSettings &settings, const QJsonObject &vpnConfiguration)
{
    Q_UNUSED(vpnConfiguration)

    const TunnelConfig config(static_cast<int>(settings.proto), settings.config, settings.dnsServers,
                              static_cast<int>(settings.killSwitchMode), settings.allowedDnsServers,
                              static_cast<int>(settings.sitesRouteMode), settings.sites,
                              static_cast<int>(settings.appsRouteMode), settings.apps, settings.mtu);
    return IpcClient::withInterface(
            [this, &config](QSharedPointer<IpcInterfaceReplica> iface) {
                connect(iface.data(), &IpcInterfaceReplica::tunnelStatusChanged, this,
                        &VpnConnection::onTunnelStatusChanged, Qt::UniqueConnection);

                auto reply = iface->tunnelUp(config);
                if (!reply.waitForFinished()) {
                    return ErrorCode::AmneziaServiceConnectionFailed;
                }
                return static_cast<ErrorCode>(reply.returnValue());
            },
            []() { return ErrorCode::AmneziaServiceNotRunning; });
}

void VpnConnection::stopTunnel()
{
    IpcClient::withInterface([](QSharedPointer<IpcInterfaceReplica> iface) {
        auto reply = iface->tunnelDown();
        if (!reply.waitForFinished()) {
            qWarning() << "VpnConnection::stopTunnel: failed to execute remote tunnelDown call";
        }
    });

    setConnectionState(Vpn::ConnectionState::Disconnected);
}

void VpnConnection::onKillSwitchModeChanged(bool enabled)
{
    const KillSwitchMode mode = killSwitchMode(*m_appSettingsRepository, enabled);
    IpcClient::withInterface([mode](QSharedPointer<IpcInterfaceReplica> iface) {
        auto reply = iface->setKillSwitchMode(static_cast<int>(mode));
        if (!reply.waitForFinished() || !reply.returnValue()) {
            qWarning() << "VpnConnection::onKillSwitchModeChanged: failed to execute remote setKillSwitchMode call";
        }
    });
}

void VpnConnection::disconnectSlots()
{
    IpcClient::withInterface([this](QSharedPointer<IpcInterfaceReplica> iface) { iface->disconnect(this); });
}

void VpnConnection::onTunnelStatusChanged(int status)
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

#elif defined(Q_OS_ANDROID)

ErrorCode VpnConnection::startTunnel(const TunnelSettings &settings, const QJsonObject &vpnConfiguration)
{
    return AndroidController::instance()->start(platformTunnelConfig(settings, vpnConfiguration));
}

void VpnConnection::stopTunnel()
{
    AndroidController::instance()->stop();
}

void VpnConnection::onKillSwitchModeChanged(bool enabled)
{
    Q_UNUSED(enabled)
}

#else

ErrorCode VpnConnection::startTunnel(const TunnelSettings &settings, const QJsonObject &vpnConfiguration)
{
    return IosController::Instance()->connectVpn(platformTunnelConfig(settings, vpnConfiguration))
            ? ErrorCode::NoError
            : ErrorCode::InternalError;
}

void VpnConnection::stopTunnel()
{
    IosController::Instance()->disconnectVpn();
    setConnectionState(Vpn::ConnectionState::Disconnected);
}

void VpnConnection::onKillSwitchModeChanged(bool enabled)
{
    Q_UNUSED(enabled)
}

#endif
