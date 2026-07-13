#include "vpnConnection.h"

#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QHostInfo>
#include <QJsonObject>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <core/configurators/openVpnConfigurator.h>
#include <core/configurators/wireguardConfigurator.h>

#ifdef AMNEZIA_DESKTOP
    #include "core/utils/ipcClient.h"
    #include <core/protocols/wireGuardProtocol.h>
#endif

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
    #include <QThread>

#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
#endif

#include "core/utils/constants/protocolConstants.h"
#include "core/utils/networkUtilities.h"

using namespace ProtocolUtils;

VpnConnection::VpnConnection(SecureServersRepository* serversRepository, SecureAppSettingsRepository* appSettingsRepository, QObject *parent)
    : QObject(parent), m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository), m_checkTimer(this), m_trafficGuard(new VpnTrafficGuard(appSettingsRepository, this))
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    m_checkTimer.setInterval(1000);
    connect(IosController::Instance(), &IosController::connectionStateChanged, this, &VpnConnection::setConnectionState);
    connect(IosController::Instance(), &IosController::bytesChanged, this, &VpnConnection::onBytesChanged);
#endif
}

VpnConnection::~VpnConnection()
{
}

void VpnConnection::onBytesChanged(quint64 receivedBytes, quint64 sentBytes)
{
    emit bytesChanged(receivedBytes, sentBytes);
}

void VpnConnection::onKillSwitchModeChanged(bool enabled)
{
#ifdef AMNEZIA_DESKTOP
    IpcClient::withInterface([enabled](QSharedPointer<IpcInterfaceReplica> iface){
        QRemoteObjectPendingReply<bool> reply = iface->refreshKillSwitch(enabled);
        if (reply.waitForFinished() && reply.returnValue())
            qDebug() << "VpnConnection::onKillSwitchModeChanged: Killswitch refreshed";
        else
            qWarning() << "VpnConnection::onKillSwitchModeChanged: Failed to execute remote refreshKillSwitch call";
    });
#endif
}

void VpnConnection::onConnectionStateChanged(Vpn::ConnectionState state)
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    if (state == Vpn::ConnectionState::Connected ||
        state == Vpn::ConnectionState::Connecting ||
        state == Vpn::ConnectionState::Reconnecting) {
        m_checkTimer.start();
    } else {
        m_checkTimer.stop();
    }
#endif
}

void VpnConnection::setRepositories(SecureServersRepository* serversRepository, SecureAppSettingsRepository* appSettingsRepository)
{
    m_serversRepository = serversRepository;
    m_appSettingsRepository = appSettingsRepository;
    m_trafficGuard.reset(new VpnTrafficGuard(appSettingsRepository, this));
}

QSharedPointer<VpnProtocol> VpnConnection::vpnProtocol() const
{
    return m_active ? m_active->protocol() : m_vpnProtocol;
}

void VpnConnection::disconnectSlots()
{
    if (auto proto = vpnProtocol()) {
        proto->disconnect();
    }
}

ErrorCode VpnConnection::lastError() const
{
#ifdef Q_OS_ANDROID
    return ErrorCode::AndroidError;
#endif

    if (m_lastError != ErrorCode::NoError) {
        return m_lastError;
    }
    auto proto = vpnProtocol();
    return proto.isNull() ? ErrorCode::InternalError : proto->lastError();
}

Vpn::ConnectionState VpnConnection::connectionState() const
{
    return m_connectionState;
}

QString VpnConnection::allocateIfname()
{
#ifdef Q_OS_MACOS
    QString kernelAssigned;
    IpcClient::withInterface([&kernelAssigned](QSharedPointer<IpcInterfaceReplica> iface) {
        auto reply = iface->reserveUtunName();
        if (reply.waitForFinished()) {
            kernelAssigned = reply.returnValue();
        }
    });
    if (kernelAssigned.isEmpty() || m_ifnamesInUse.contains(kernelAssigned)) {
        qCritical() << "allocateIfname: kernel utun reservation failed";
        return QString();
    }
    m_ifnamesInUse.insert(kernelAssigned);
    return kernelAssigned;
#else
    for (int i = 0; ; ++i) {
        const QString name = QStringLiteral("amn%1").arg(i);
        if (!m_ifnamesInUse.contains(name)) {
            m_ifnamesInUse.insert(name);
            return name;
        }
    }
#endif
}

void VpnConnection::releaseIfname(const QString& ifname)
{
    m_ifnamesInUse.remove(ifname);
}

void VpnConnection::wireTunnelSignals(Tunnel* tunnel, bool isActive)
{
    connect(tunnel, &Tunnel::prepared, this, &VpnConnection::onTunnelPrepared);
    connect(tunnel, &Tunnel::activated, this, &VpnConnection::onTunnelActivated);
    connect(tunnel, &Tunnel::failed, this, &VpnConnection::onTunnelFailed);

    if (isActive) {
        connect(tunnel, &Tunnel::bytesChanged, this, &VpnConnection::onBytesChanged);
    }
}

void VpnConnection::connectToVpn(const QString &serverId, DockerContainer container, const QJsonObject &vpnConfiguration)
{
    if (!m_appSettingsRepository || !m_serversRepository) {
        qCritical() << "VpnConnection::connectToVpn: repositories not initialized";
        setConnectionState(Vpn::ConnectionState::Error);
        return;
    }

    m_lastError = ErrorCode::NoError;

    qDebug() << QString("Trying to connect to VPN, server id is %1, container is %2, route mode is")
                        .arg(serverId)
                        .arg(ContainerUtils::containerToString(container))
             << m_appSettingsRepository->routeMode();

    const QString resolvedRemote =
        NetworkUtilities::getIPAddress(vpnConfiguration.value(configKey::hostName).toString());

#ifdef AMNEZIA_DESKTOP
    const bool isWg = VpnProtocol::isWireGuardBased(container);
    const bool isXray = VpnProtocol::isXrayBased(container);
    const bool targetIsSwitchable = isWg || isXray;
    const bool activeIsSwitchable = m_active
        && (VpnProtocol::isWireGuardBased(m_active->container())
            || VpnProtocol::isXrayBased(m_active->container()));
    const QString preAllocatedIfname = allocateIfname();
    if (preAllocatedIfname.isEmpty()) {
        setConnectionState(Vpn::ConnectionState::Error);
        emit vpnProtocolError(ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    if (m_active
        && m_connectionState == Vpn::ConnectionState::Connected
        && targetIsSwitchable
        && activeIsSwitchable) {
        if (!m_trafficGuard->allowEndpoint(resolvedRemote, preAllocatedIfname)) {
            releaseIfname(preAllocatedIfname);
            setConnectionState(Vpn::ConnectionState::Error);
            emit vpnProtocolError(ErrorCode::AmneziaServiceConnectionFailed);
            return;
        }
        startTunnelSwitch(container, vpnConfiguration, resolvedRemote, preAllocatedIfname);
        return;
    }

    if (!m_trafficGuard->allowEndpoint(resolvedRemote, preAllocatedIfname)) {
        releaseIfname(preAllocatedIfname);
        setConnectionState(Vpn::ConnectionState::Error);
        emit vpnProtocolError(ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }
#endif
    setConnectionState(Vpn::ConnectionState::Connecting);

    QJsonObject config = vpnConfiguration;

#ifdef AMNEZIA_DESKTOP
    if (m_active) {
        const QString oldIfname = m_active->ifname();
        m_trafficGuard->tearDown(m_active);
        m_trafficGuard->flushAll();
        delete m_active;
        m_active = nullptr;
        releaseIfname(oldIfname);
    }
#endif

    appendKillSwitchConfig(config);
    appendSplitTunnelingConfig(config);

    m_vpnConfiguration = config;
    m_remoteAddress = resolvedRemote;

#ifdef AMNEZIA_DESKTOP
    config.insert("ifname", preAllocatedIfname);
    if (isXray) {
        config.insert("tunName", preAllocatedIfname);
        config.insert("deviceIpv4Address", amnezia::protocols::xray::defaultLocalAddr);
    } else if (isWg) {
        const QString protoName = config.value("protocol").toString();
        const QJsonObject wgConfig = config.value(protoName + "_config_data").toObject();
        const QString clientIp = wgConfig.value(amnezia::configKey::clientIp).toString();
        if (!clientIp.isEmpty()) {
            config.insert("deviceIpv4Address", clientIp);
        }
    }
    m_vpnConfiguration = config;
    m_active = new Tunnel(preAllocatedIfname, container, config, resolvedRemote, this);
    wireTunnelSignals(m_active, /*isActive=*/true);
    IpcClient::withInterface([this](QSharedPointer<IpcInterfaceReplica> rep) {
        connect(rep.data(), &IpcInterfaceReplica::networkChanged, this, &VpnConnection::reconnectToVpn,
                static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));
        connect(rep.data(), &IpcInterfaceReplica::wakeup, this, &VpnConnection::reconnectToVpn,
                static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));
    });
    m_trafficGuard->setConfig(config);
    m_trafficGuard->bringUp(m_active);
    return;
#elif defined Q_OS_ANDROID
    androidVpnProtocol = createDefaultAndroidVpnProtocol();
    createAndroidConnections();

    m_vpnProtocol.reset(androidVpnProtocol);
#elif defined Q_OS_IOS || defined(MACOS_NE)
    Proto proto = ContainerUtils::defaultProtocol(container);
    IosController::Instance()->connectVpn(proto, m_vpnConfiguration);
    connect(&m_checkTimer, &QTimer::timeout, IosController::Instance(), &IosController::checkStatus);
    return;
#endif

    createProtocolConnections();

    if (ErrorCode err = m_vpnProtocol->start(); err != ErrorCode::NoError) {
        setConnectionState(Vpn::ConnectionState::Error);
        emit vpnProtocolError(err);
    }
}

void VpnConnection::createProtocolConnections()
{
    connect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
    connect(m_vpnProtocol.data(), &VpnProtocol::connectionStateChanged, this, &VpnConnection::setConnectionState);
    connect(m_vpnProtocol.data(), SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));
}

void VpnConnection::appendKillSwitchConfig(QJsonObject &config)
{
#ifdef AMNEZIA_DESKTOP
    if (!m_appSettingsRepository) {
        qCritical() << "VpnConnection::appendKillSwitchConfig: repositories not initialized";
        return;
    }

    config.insert(configKey::killSwitchOption, QVariant(m_appSettingsRepository->isKillSwitchEnabled()).toString());
    config.insert(configKey::allowedDnsServers, QVariant(m_appSettingsRepository->getAllowedDnsServers()).toJsonValue());
#else
    Q_UNUSED(config)
#endif
}

void VpnConnection::appendSplitTunnelingConfig(QJsonObject &config)
{
    if (!m_appSettingsRepository) {
        qCritical() << "VpnConnection::appendSplitTunnelingConfig: repositories not initialized";
        return;
    }

    bool allowSiteBasedSplitTunneling = true;

    // this block is for old native configs and for old self-hosted configs
    auto protocolName = config.value(configKey::vpnProto).toString();
    if (protocolName == ProtocolUtils::protoToString(Proto::Awg) || protocolName == ProtocolUtils::protoToString(Proto::WireGuard)) {
        allowSiteBasedSplitTunneling = false;
        auto configData = config.value(protocolName + "_config_data").toObject();
        if (configData.value(configKey::allowedIps).isString()) {
            QJsonArray allowedIpsJsonArray = QJsonArray::fromStringList(configData.value(configKey::allowedIps).toString().split(", "));
            configData.insert(configKey::allowedIps, allowedIpsJsonArray);
            config.insert(protocolName + "_config_data", configData);
        } else if (configData.value(configKey::allowedIps).isUndefined()) {
            auto nativeConfig = configData.value(configKey::config).toString();
            auto nativeConfigLines = nativeConfig.split("\n");
            for (auto &line : nativeConfigLines) {
                if (line.contains("AllowedIPs")) {
                    auto allowedIpsString = line.split(" = ");
                    if (allowedIpsString.size() < 1) {
                        break;
                    }
                    QJsonArray allowedIpsJsonArray = QJsonArray::fromStringList(allowedIpsString.at(1).split(", "));
                    configData.insert(configKey::allowedIps, allowedIpsJsonArray);
                    config.insert(protocolName + "_config_data", configData);
                    break;
                }
            }
        }

        if (configData.value(configKey::persistentKeepAlive).isUndefined()) {
            auto nativeConfig = configData.value(configKey::config).toString();
            auto nativeConfigLines = nativeConfig.split("\n");
            for (auto &line : nativeConfigLines) {
                if (line.contains("PersistentKeepalive")) {
                    auto persistentKeepaliveString = line.split(" = ");
                    if (persistentKeepaliveString.size() < 1) {
                        break;
                    }
                    configData.insert(configKey::persistentKeepAlive, persistentKeepaliveString.at(1));
                    config.insert(protocolName + "_config_data", configData);
                    break;
                }
            }
        }

        QJsonArray allowedIpsJsonArray = configData.value(configKey::allowedIps).toArray();
        if (allowedIpsJsonArray.contains("0.0.0.0/0") && allowedIpsJsonArray.contains("::/0")) {
            allowSiteBasedSplitTunneling = true;
        }
    }

    amnezia::RouteMode routeMode = amnezia::RouteMode::VpnAllSites;
    QJsonArray sitesJsonArray;
    if (m_appSettingsRepository->isSitesSplitTunnelingEnabled()) {
        routeMode = m_appSettingsRepository->routeMode();

        if (allowSiteBasedSplitTunneling) {
            QStringList sites;
            const QVariantMap &m = m_appSettingsRepository->vpnSites(routeMode);
            for (auto i = m.constBegin(); i != m.constEnd(); ++i) {
                if (NetworkUtilities::checkIpSubnetFormat(i.key())) {
                    sites.append(i.key());
                } else if (NetworkUtilities::checkIpSubnetFormat(i.value().toString())) {
                    sites.append(i.value().toString());
                }
            }
            sites.removeDuplicates();
            for (const auto &site : sites) {
                sitesJsonArray.append(site);
            }

            if (sitesJsonArray.isEmpty()) {
                routeMode = amnezia::RouteMode::VpnAllSites;
            } else if (routeMode == amnezia::RouteMode::VpnOnlyForwardSites) {
                // Allow traffic to Amnezia DNS
                sitesJsonArray.append(config.value(configKey::dns1).toString());
                sitesJsonArray.append(config.value(configKey::dns2).toString());
            }
        }
    }

    config.insert(configKey::splitTunnelType, routeMode);
    config.insert(configKey::splitTunnelSites, sitesJsonArray);

    amnezia::AppsRouteMode appsRouteMode = amnezia::AppsRouteMode::VpnAllApps;
    QJsonArray appsJsonArray;
    if (m_appSettingsRepository->isAppsSplitTunnelingEnabled()) {
        appsRouteMode = m_appSettingsRepository->appsRouteMode();

        auto apps = m_appSettingsRepository->vpnApps(appsRouteMode);
        for (const auto &app : apps) {
            appsJsonArray.append(app.appPath.isEmpty() ? app.packageName : app.appPath);
        }

        if (appsJsonArray.isEmpty()) {
            appsRouteMode = amnezia::AppsRouteMode::VpnAllApps;
        }
    }

    config.insert(configKey::appSplitTunnelType, appsRouteMode);
    config.insert(configKey::splitTunnelApps, appsJsonArray);

    qDebug() << QString("Site split tunneling is %1, route mode is %2")
                        .arg(m_appSettingsRepository->isSitesSplitTunnelingEnabled() ? "enabled" : "disabled")
                        .arg(routeMode);
    qDebug() << QString("App split tunneling is %1, route mode is %2")
                        .arg(m_appSettingsRepository->isAppsSplitTunnelingEnabled() ? "enabled" : "disabled")
                        .arg(appsRouteMode);
}

#ifdef Q_OS_ANDROID
void VpnConnection::restoreConnection()
{
    createAndroidConnections();

    m_vpnProtocol.reset(androidVpnProtocol);

    createProtocolConnections();
}

void VpnConnection::createAndroidConnections()
{
    androidVpnProtocol = createDefaultAndroidVpnProtocol();

    connect(AndroidController::instance(), &AndroidController::connectionStateChanged, androidVpnProtocol,
            &AndroidVpnProtocol::setConnectionState);
    connect(AndroidController::instance(), &AndroidController::statisticsUpdated, androidVpnProtocol, &AndroidVpnProtocol::setBytesChanged);
}

AndroidVpnProtocol *VpnConnection::createDefaultAndroidVpnProtocol()
{
    return new AndroidVpnProtocol(m_vpnConfiguration);
}
#endif

QString VpnConnection::bytesPerSecToText(quint64 bytes)
{
    double mbps = bytes * 8 / 1e6;
    return QString("%1 %2").arg(QString::number(mbps, 'f', 2)).arg(tr("Mbps")); // Mbit/s
}

void VpnConnection::reconnectToVpn() {
    if (m_connectionState != Vpn::ConnectionState::Connected) {
        qWarning() << QString("Reconnect triggered on %1 during inappropriate state: %2; ignoring slot")
                              .arg(QMetaEnum::fromType<Vpn::ConnectionState>().valueToKey(m_connectionState));
        return;
    }

    qDebug() << "Reconnect triggered. Reconnecting to the server";

    setConnectionState(Vpn::ConnectionState::Reconnecting);

#ifdef AMNEZIA_DESKTOP
    if (m_active) {
        m_active->restart();
        return;
    }
#endif

    if (m_vpnProtocol.isNull())
        return;

    m_vpnProtocol->stop();
    if (ErrorCode err = m_vpnProtocol->start(); err != ErrorCode::NoError) {
        setConnectionState(Vpn::ConnectionState::Error);
        emit vpnProtocolError(err);
    }
}

void VpnConnection::disconnectFromVpn()
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    // iOS/macOS NE use IosController directly; m_vpnProtocol is not set there.
    IosController::Instance()->disconnectVpn();
    disconnect(&m_checkTimer, &QTimer::timeout, IosController::Instance(), &IosController::checkStatus);
#endif

#ifdef AMNEZIA_DESKTOP
    if (m_staging) {
        m_trafficGuard->tearDown(m_staging);
        releaseIfname(m_staging->ifname());
        delete m_staging;
        m_staging = nullptr;
    }

    if (m_active) {
        setConnectionState(Vpn::ConnectionState::Disconnecting);
        m_trafficGuard->tearDown(m_active);
        m_trafficGuard->flushAll();
        releaseIfname(m_active->ifname());
        delete m_active;
        m_active = nullptr;
        setConnectionState(Vpn::ConnectionState::Disconnected);
        return;
    }
#endif

    if (m_vpnProtocol.isNull()) {
        setConnectionState(Vpn::ConnectionState::Disconnected);
        return;
    }

    setConnectionState(Vpn::ConnectionState::Disconnecting);

#ifdef Q_OS_ANDROID
    auto *const connection = new QMetaObject::Connection;
    *connection = connect(AndroidController::instance(), &AndroidController::vpnStateChanged, this,
                          [this, connection](AndroidController::ConnectionState state) {
                              if (state == AndroidController::ConnectionState::DISCONNECTED) {
                                  setConnectionState(Vpn::ConnectionState::Disconnected);
                                  disconnect(*connection);
                                  delete connection;
                              }
                          });
#endif
#ifdef AMNEZIA_DESKTOP
    m_trafficGuard->flushAll();
#endif
    m_vpnProtocol->stop();

#if !defined(Q_OS_ANDROID) && !defined(AMNEZIA_DESKTOP)
    m_vpnProtocol->deleteLater();
#endif

    m_vpnProtocol = nullptr;
}

void VpnConnection::setConnectionState(Vpn::ConnectionState state) {
    onConnectionStateChanged(state);

    if (state == Vpn::Disconnected && m_connectionState == Vpn::Reconnecting)
        return;

    m_connectionState = state;
    emit connectionStateChanged(state);
}

void VpnConnection::startTunnelSwitch(DockerContainer container,
                                      const QJsonObject &vpnConfiguration,
                                      const QString &resolvedRemote,
                                      const QString &stagingIfname)
{
    QJsonObject config = vpnConfiguration;
    config.insert("ifname", stagingIfname);
    if (VpnProtocol::isXrayBased(container)) {
        config.insert("tunName", stagingIfname);
        config.insert("deviceIpv4Address", amnezia::protocols::xray::defaultLocalAddr);
    } else if (VpnProtocol::isWireGuardBased(container)) {
        const QString protoName = config.value("protocol").toString();
        const QJsonObject wgConfig = config.value(protoName + "_config_data").toObject();
        const QString clientIp = wgConfig.value(amnezia::configKey::clientIp).toString();
        if (!clientIp.isEmpty()) {
            config.insert("deviceIpv4Address", clientIp);
        }
    }
    appendKillSwitchConfig(config);
    appendSplitTunnelingConfig(config);

    m_staging = new Tunnel(stagingIfname, container, config, resolvedRemote, this);
    if (m_active) {
        m_staging->setHandoverIfname(m_active->ifname());
    }
    wireTunnelSignals(m_staging, /*isActive=*/false);

    setConnectionState(Vpn::ConnectionState::Switching);
    m_trafficGuard->bringUp(m_staging);
}

void VpnConnection::onTunnelPrepared()
{
    Tunnel* tunnel = qobject_cast<Tunnel*>(sender());
    if (!tunnel) return;

    if (tunnel == m_staging && m_active) {
        Tunnel* oldTunnel = m_active;
        const QString oldIfname = oldTunnel->ifname();

        m_active = m_staging;
        m_staging = nullptr;
        connect(m_active, &Tunnel::bytesChanged, this, &VpnConnection::onBytesChanged);
        m_vpnConfiguration = m_active->config();
        m_remoteAddress = m_active->remoteAddress();
        m_trafficGuard->setConfig(m_vpnConfiguration);

        QMetaObject::invokeMethod(this, [this, oldTunnel, oldIfname]() {
            m_trafficGuard->swap(oldTunnel, m_active);
            delete oldTunnel;
            releaseIfname(oldIfname);
            emit serverSwitchSucceeded();
        }, Qt::QueuedConnection);
        return;
    }

    m_trafficGuard->commit(tunnel);
}

void VpnConnection::onTunnelActivated()
{
    Tunnel* tunnel = qobject_cast<Tunnel*>(sender());
    if (!tunnel) return;

    if (tunnel == m_active) {
        setConnectionState(Vpn::ConnectionState::Connected);
    }
}

void VpnConnection::onTunnelFailed(amnezia::ErrorCode error)
{
    Tunnel* tunnel = qobject_cast<Tunnel*>(sender());
    if (!tunnel) return;

    if (tunnel == m_staging) {
        m_trafficGuard->release(m_staging);
        m_staging->deactivate();
        releaseIfname(m_staging->ifname());
        m_staging->deleteLater();
        m_staging = nullptr;
        setConnectionState(Vpn::ConnectionState::Connected);
        emit serverSwitchFailed();
        return;
    }

    if (tunnel == m_active) {
        m_trafficGuard->tearDown(m_active);
        m_trafficGuard->flushAll();
        releaseIfname(m_active->ifname());
        m_active->deleteLater();
        m_active = nullptr;
        if (error == ErrorCode::NoError) {
            emit serverConnectionTimeout();
            return;
        }
        m_lastError = error;
        setConnectionState(Vpn::ConnectionState::Error);
    }
}
