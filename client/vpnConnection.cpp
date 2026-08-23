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
    #include <QRemoteObjectPendingCallWatcher>
#endif

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
    #include <QThread>

#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
#endif

#include "core/utils/networkUtilities.h"
#include "core/utils/serverConfigUtils.h"
#include "vpnConnection.h"

using namespace ProtocolUtils;

#ifdef AMNEZIA_DESKTOP
namespace {
// A reconnect attempt that has not reached Connected within this time is
// considered failed and is retried (some failures — e.g. a daemon-side
// activation error — produce no client-visible event at all).
constexpr int RECONNECT_ATTEMPT_TIMEOUT_MSEC = 30 * 1000;
constexpr int RECONNECT_RETRY_BASE_MSEC = 1000;
constexpr int RECONNECT_RETRY_MAX_MSEC = 60 * 1000;
// A fresh trigger does not restart an attempt younger than this: such an
// attempt was started under (almost) the same network conditions anyway.
constexpr int RECONNECT_ATTEMPT_MIN_AGE_MSEC = 1000;
}
#endif

VpnConnection::VpnConnection(SecureServersRepository* serversRepository, SecureAppSettingsRepository* appSettingsRepository, QObject *parent)
    : QObject(parent), m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository), m_checkTimer(this)
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    m_checkTimer.setInterval(1000);
    connect(IosController::Instance(), &IosController::connectionStateChanged, this, &VpnConnection::setConnectionState);
    connect(IosController::Instance(), &IosController::bytesChanged, this, &VpnConnection::onBytesChanged);
#endif

#ifdef AMNEZIA_DESKTOP
    m_reconnectRetryTimer.setSingleShot(true);
    connect(&m_reconnectRetryTimer, &QTimer::timeout, this, &VpnConnection::startReconnectAttempt);
    m_reconnectWatchdogTimer.setSingleShot(true);
    connect(&m_reconnectWatchdogTimer, &QTimer::timeout, this, &VpnConnection::onReconnectWatchdogTimeout);
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
#ifdef AMNEZIA_DESKTOP
    if (!m_serversRepository || !m_appSettingsRepository) {
        qCritical() << "VpnConnection::onConnectionStateChanged: repositories not initialized";
        return;
    }

    const QString defaultServerId = m_serversRepository->defaultServerId();
    DockerContainer container = DockerContainer::None;
    switch (m_serversRepository->serverKind(defaultServerId)) {
    case serverConfigUtils::ConfigType::SelfHostedAdmin: {
        const auto cfg = m_serversRepository->selfHostedAdminConfig(defaultServerId);
        if (cfg.has_value()) {
            container = cfg->defaultContainer;
        }
        break;
    }
    case serverConfigUtils::ConfigType::SelfHostedUser: {
        const auto cfg = m_serversRepository->selfHostedUserConfig(defaultServerId);
        if (cfg.has_value()) {
            container = cfg->defaultContainer;
        }
        break;
    }
    case serverConfigUtils::ConfigType::Native: {
        const auto cfg = m_serversRepository->nativeConfig(defaultServerId);
        if (cfg.has_value()) {
            container = cfg->defaultContainer;
        }
        break;
    }
    case serverConfigUtils::ConfigType::AmneziaPremiumV2:
    case serverConfigUtils::ConfigType::AmneziaFreeV3:
    case serverConfigUtils::ConfigType::ExternalPremium: {
        const auto cfg = m_serversRepository->apiV2Config(defaultServerId);
        if (cfg.has_value()) {
            container = cfg->defaultContainer;
        }
        break;
    }
    case serverConfigUtils::ConfigType::AmneziaPremiumV1:
    case serverConfigUtils::ConfigType::AmneziaFreeV2:
        break;
    case serverConfigUtils::ConfigType::Invalid:
    default:
        break;
    }

    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        switch (state) {
            case Vpn::ConnectionState::Connected: {
                iface->resetIpStack();

                auto flushDns = iface->flushDns();
                if (flushDns.waitForFinished() && flushDns.returnValue())
                    qDebug() << "VpnConnection::onConnectionStateChanged: Successfully flushed DNS";
                else
                    qWarning() << "VpnConnection::onConnectionStateChanged: Failed to flush DNS";

                if (!ContainerUtils::isAwgContainer(container) && container != DockerContainer::WireGuard) {
                    QString dns1 = m_vpnConfiguration.value(configKey::dns1).toString();
                    QString dns2 = m_vpnConfiguration.value(configKey::dns2).toString();

#ifdef Q_OS_MACOS
                    if (!m_appSettingsRepository->isSitesSplitTunnelingEnabled() || m_appSettingsRepository->routeMode() != amnezia::RouteMode::VpnAllExceptSites) {
                        iface->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << dns1 << dns2);
                    }
#else
                    iface->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << dns1 << dns2);
#endif

                    if (m_appSettingsRepository->isSitesSplitTunnelingEnabled()) {
                        iface->routeDeleteList(m_vpnProtocol->vpnGateway(), QStringList() << "0.0.0.0");
                        RouteMode routeMode = m_appSettingsRepository->routeMode();
                        if (routeMode == amnezia::RouteMode::VpnOnlyForwardSites) {
                            QTimer::singleShot(1000, m_vpnProtocol.data(),
                                               [this, routeMode]() { addSitesRoutes(m_vpnProtocol->vpnGateway(), routeMode); });
                        } else if (routeMode == amnezia::RouteMode::VpnAllExceptSites) {
                            iface->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << "0.0.0.0/1");
                            iface->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << "128.0.0.0/1");

                            iface->routeAddList(m_vpnProtocol->routeGateway(), QStringList() << remoteAddress());
#ifdef Q_OS_MACOS
                            iface->routeAddList(m_vpnProtocol->routeGateway(), QStringList() << dns1 << dns2);
#endif
                            addSitesRoutes(m_vpnProtocol->routeGateway(), routeMode);
                        }
                    }
                }
            } break;
            case Vpn::ConnectionState::Disconnected:
            case Vpn::ConnectionState::Error: {
                auto flushDns = iface->flushDns();
                if (flushDns.waitForFinished() && flushDns.returnValue())
                    qDebug() << "VpnConnection::onConnectionStateChanged: Successfully flushed DNS";
                else
                    qWarning() << "VpnConnection::onConnectionStateChanged: Failed to flush DNS";

                auto clearSavedRoutes = iface->clearSavedRoutes();
                if (clearSavedRoutes.waitForFinished() && clearSavedRoutes.returnValue())
                    qDebug() << "VpnConnection::onConnectionStateChanged: Successfully cleared saved routes";
                else
                    qWarning() << "VpnConnection::onConnectionStateChanged: Failed to clear saved routes";
            } break;
            default:
                break;
        }
    });
#endif

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

const QString &VpnConnection::remoteAddress() const
{
    return m_remoteAddress;
}

void VpnConnection::setRepositories(SecureServersRepository* serversRepository, SecureAppSettingsRepository* appSettingsRepository)
{
    m_serversRepository = serversRepository;
    m_appSettingsRepository = appSettingsRepository;
}

void VpnConnection::addSitesRoutes(const QString &gw, amnezia::RouteMode mode)
{
#ifdef AMNEZIA_DESKTOP
    if (!m_appSettingsRepository) {
        qCritical() << "VpnConnection::addSitesRoutes: repositories not initialized";
        return;
    }

    QStringList ips;
    QStringList sites;
    const QVariantMap &m = m_appSettingsRepository->vpnSites(mode);
    for (auto i = m.constBegin(); i != m.constEnd(); ++i) {
        if (NetworkUtilities::checkIpSubnetFormat(i.key())) {
            ips.append(i.key());
        } else {
            const QStringList siteIps = SecureAppSettingsRepository::siteIpList(i.value());
            for (const QString &ip : siteIps) {
                if (NetworkUtilities::checkIpSubnetFormat(ip)) {
                    ips.append(ip);
                }
            }
            sites.append(i.key());
        }
    }
    ips.removeDuplicates();

    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        iface->routeAddList(gw, ips);
    });

    auto remainingLookups = QSharedPointer<int>::create(sites.size());
    auto needFlush = QSharedPointer<bool>::create(false);

    // re-resolve domains
    for (const QString &site : sites) {
        const auto &cbResolv = [this, site, gw, mode, ips, remainingLookups, needFlush](const QHostInfo &hostInfo) {
            QStringList resolvedIps;
            for (const QHostAddress &addr : hostInfo.addresses()) {
                if (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
                    resolvedIps.append(addr.toString());
                }
            }
            resolvedIps.removeDuplicates();
            qDebug() << "[SplitTunneling] addSitesRoutes resolved" << site << "->" << resolvedIps;

            QStringList newIps;
            for (const QString &ip : resolvedIps) {
                if (!ips.contains(ip)) {
                    IpcClient::withInterface([gw, ip](QSharedPointer<IpcInterfaceReplica> iface) {
                        iface->routeAddList(gw, QStringList() << ip);
                    });
                    newIps.append(ip);
                }
            }

            if (!newIps.isEmpty()) {
                m_appSettingsRepository->addVpnSite(mode, site, newIps);
                *needFlush = true;
            }

            if (--(*remainingLookups) > 0)
                return;

            if (!*needFlush)
                return;

            // Async flush: never waitForFinished() here — that re-enters the event loop and
            // can re-enter this QHostInfo callback until the stack overflows (0xc00000fd).
            IpcClient::withInterface([this](QSharedPointer<IpcInterfaceReplica> iface) {
                QRemoteObjectPendingReply<bool> reply = iface->flushDns();
                auto *watcher = new QRemoteObjectPendingCallWatcher(reply, this);
                QObject::connect(watcher, &QRemoteObjectPendingCallWatcher::finished, this,
                        [](QRemoteObjectPendingCallWatcher *call) {
                            if (call->error() != QRemoteObjectPendingCall::NoError
                                || !call->returnValue().toBool()) {
                                qWarning() << "VpnConnection::addSitesRoutes: Failed to flush DNS";
                            }
                            call->deleteLater();
                        });
            });
        };
        QHostInfo::lookupHost(site, this, cbResolv);
    }
#endif
}

QSharedPointer<VpnProtocol> VpnConnection::vpnProtocol() const
{
    return m_vpnProtocol;
}

void VpnConnection::disconnectSlots()
{
    if (m_vpnProtocol) {
        m_vpnProtocol->disconnect();
    }
}

ErrorCode VpnConnection::lastError() const
{
#ifdef Q_OS_ANDROID
    return ErrorCode::AndroidError;
#endif

    if (m_vpnProtocol.isNull()) {
        return ErrorCode::InternalError;
    }

    return m_vpnProtocol.data()->lastError();
}

Vpn::ConnectionState VpnConnection::connectionState() const
{
    return m_connectionState;
}

void VpnConnection::connectToVpn(const QString &serverId, DockerContainer container, const QJsonObject &vpnConfiguration)
{
    if (!m_appSettingsRepository || !m_serversRepository) {
        qCritical() << "VpnConnection::connectToVpn: repositories not initialized";
        setConnectionState(Vpn::ConnectionState::Error);
        return;
    }

    qDebug() << QString("Trying to connect to VPN, server id is %1, container is %2, route mode is")
                        .arg(serverId)
                        .arg(ContainerUtils::containerToString(container))
             << m_appSettingsRepository->routeMode();

    m_remoteAddress = NetworkUtilities::getIPAddress(vpnConfiguration.value(configKey::hostName).toString());
    setConnectionState(Vpn::ConnectionState::Connecting);

    m_vpnConfiguration = vpnConfiguration;

#ifdef AMNEZIA_DESKTOP
    cancelReconnect();
    if (m_vpnProtocol) {
        // Detach every slot of ours (state, bytes, errors) so tearing the old
        // protocol down doesn't inject events into the new connection flow.
        m_vpnProtocol->disconnect(this);
        m_vpnProtocol->stop();
        m_vpnProtocol.reset();
    }
    appendKillSwitchConfig();
#endif

    appendSplitTunnelingConfig();

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    m_vpnProtocol.reset(VpnProtocol::factory(container, m_vpnConfiguration));
    if (!m_vpnProtocol) {
        setConnectionState(Vpn::ConnectionState::Error);
        return;
    }
    m_vpnProtocol->prepare();
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
    connect(m_vpnProtocol.data(), &VpnProtocol::connectionStateChanged, this, &VpnConnection::onProtocolConnectionStateChanged);
    connect(m_vpnProtocol.data(), SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));

#ifdef AMNEZIA_DESKTOP
    IpcClient::withInterface([this](QSharedPointer<IpcInterfaceReplica> rep) {
        // The replica is thread-local and long-lived while this method runs on
        // every connect — UniqueConnection keeps these from piling up.
        const auto queuedUnique = static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection);
        connect(rep.data(), &IpcInterfaceReplica::networkChanged, this, &VpnConnection::onIpcNetworkChanged, queuedUnique);
        connect(rep.data(), &IpcInterfaceReplica::wakeup, this, &VpnConnection::onIpcWakeup, queuedUnique);
    });
#endif
}

void VpnConnection::appendKillSwitchConfig()
{
    if (!m_appSettingsRepository) {
        qCritical() << "VpnConnection::appendKillSwitchConfig: repositories not initialized";
        return;
    }

    m_vpnConfiguration.insert(configKey::killSwitchOption, QVariant(m_appSettingsRepository->isKillSwitchEnabled()).toString());
    m_vpnConfiguration.insert(configKey::allowedDnsServers, QVariant(m_appSettingsRepository->getAllowedDnsServers()).toJsonValue());
}

void VpnConnection::appendSplitTunnelingConfig()
{
    if (!m_appSettingsRepository) {
        qCritical() << "VpnConnection::appendSplitTunnelingConfig: repositories not initialized";
        return;
    }

    bool allowSiteBasedSplitTunneling = true;

    // this block is for old native configs and for old self-hosted configs
    auto protocolName = m_vpnConfiguration.value(configKey::vpnProto).toString();
    if (protocolName == ProtocolUtils::protoToString(Proto::Awg) || protocolName == ProtocolUtils::protoToString(Proto::WireGuard)) {
        allowSiteBasedSplitTunneling = false;
        auto configData = m_vpnConfiguration.value(protocolName + "_config_data").toObject();
        if (configData.value(configKey::allowedIps).isString()) {
            QJsonArray allowedIpsJsonArray = QJsonArray::fromStringList(configData.value(configKey::allowedIps).toString().split(", "));
            configData.insert(configKey::allowedIps, allowedIpsJsonArray);
            m_vpnConfiguration.insert(protocolName + "_config_data", configData);
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
                    m_vpnConfiguration.insert(protocolName + "_config_data", configData);
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
                    if (persistentKeepaliveString.size() > 1) {
                        configData.insert(configKey::persistentKeepAlive, persistentKeepaliveString.at(1));
                        m_vpnConfiguration.insert(protocolName + "_config_data", configData);
                    }
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
                } else {
                    const QStringList siteIps = SecureAppSettingsRepository::siteIpList(i.value());
                    for (const QString &ip : siteIps) {
                        if (NetworkUtilities::checkIpSubnetFormat(ip)) {
                            sites.append(ip);
                        }
                    }
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
                sitesJsonArray.append(m_vpnConfiguration.value(configKey::dns1).toString());
                sitesJsonArray.append(m_vpnConfiguration.value(configKey::dns2).toString());
            }
        }
    }

    m_vpnConfiguration.insert(configKey::splitTunnelType, routeMode);
    m_vpnConfiguration.insert(configKey::splitTunnelSites, sitesJsonArray);

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

    m_vpnConfiguration.insert(configKey::appSplitTunnelType, appsRouteMode);
    m_vpnConfiguration.insert(configKey::splitTunnelApps, appsJsonArray);

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

#ifdef AMNEZIA_DESKTOP
void VpnConnection::onIpcWakeup()
{
    requestReconnect(QStringLiteral("wakeup"));
}

void VpnConnection::onIpcNetworkChanged()
{
    requestReconnect(QStringLiteral("network change"));
}

void VpnConnection::requestReconnect(const QString &trigger)
{
    if (m_vpnProtocol.isNull())
        return;

    if (m_reconnectActive) {
        // Conditions changed (e.g. the network actually came back after
        // wakeup) — restart the backoff sequence and try again right away.
        // An in-flight attempt that was started before this trigger is likely
        // doomed (it raced the network coming up), so restart it too instead
        // of waiting out its watchdog; a just-started attempt is left alone.
        //
        // The age check applies regardless of whether the last attempt is
        // still in flight or has already failed and is waiting on the retry
        // timer: a failing attempt can complete in milliseconds (e.g. no
        // gateway yet), and NetworkManager can keep emitting networkChanged/
        // wakeup in a burst while it settles after wakeup. Gating only on
        // "in flight" let every such trigger cancel the backoff timer and
        // restart immediately, turning the intended 1s/2s/4s.. backoff into a
        // tight retry loop for as long as the network kept flapping.
        if (m_reconnectAttemptAge.isValid()
            && m_reconnectAttemptAge.elapsed() < RECONNECT_ATTEMPT_MIN_AGE_MSEC) {
            qDebug() << "Reconnect: new trigger" << trigger << "ignored, last attempt started too recently";
            m_reconnectAttempt = 0;
            return;
        }

        qDebug() << "Reconnect: new trigger" << trigger << "while already reconnecting, retrying immediately";
        m_reconnectAttempt = 0;
        m_reconnectAttemptInFlight = false;
        m_reconnectRetryTimer.stop();
        m_reconnectWatchdogTimer.stop();
        startReconnectAttempt();
        return;
    }

    if (m_connectionState != Vpn::ConnectionState::Connected) {
        qWarning() << QString("Reconnect triggered by %1 during inappropriate state: %2; ignoring")
                              .arg(trigger)
                              .arg(QMetaEnum::fromType<Vpn::ConnectionState>().valueToKey(m_connectionState));
        return;
    }

    qDebug() << "Reconnect triggered by" << trigger << ". Reconnecting to the server";

    m_reconnectActive = true;
    m_reconnectAttempt = 0;
    setConnectionState(Vpn::ConnectionState::Reconnecting);
    startReconnectAttempt();
}

void VpnConnection::startReconnectAttempt()
{
    if (!m_reconnectActive)
        return;

    if (m_vpnProtocol.isNull()) {
        cancelReconnect();
        return;
    }

    ++m_reconnectAttempt;
    qDebug() << "Reconnect: attempt" << m_reconnectAttempt;
    m_reconnectAttemptAge.start();

    // stop() may synchronously emit Disconnected; while the machine is active
    // (and no attempt is in flight yet) onProtocolConnectionStateChanged
    // suppresses it so the UI stays in Reconnecting.
    m_vpnProtocol->stop();

    m_reconnectAttemptInFlight = true;
    if (ErrorCode err = m_vpnProtocol->start(); err != ErrorCode::NoError) {
        qWarning() << "Reconnect: attempt" << m_reconnectAttempt << "failed to start, error" << err;
        scheduleReconnectRetry();
        return;
    }

    // start() may have failed synchronously through a protocol event, in which
    // case the retry is already scheduled and the watchdog must stay off.
    if (m_reconnectAttemptInFlight) {
        m_reconnectWatchdogTimer.start(RECONNECT_ATTEMPT_TIMEOUT_MSEC);
    }
}

void VpnConnection::onReconnectWatchdogTimeout()
{
    if (!m_reconnectActive || !m_reconnectAttemptInFlight)
        return;

    qWarning() << "Reconnect: attempt" << m_reconnectAttempt << "did not reach the Connected state in time";
    scheduleReconnectRetry();
}

void VpnConnection::scheduleReconnectRetry()
{
    m_reconnectWatchdogTimer.stop();
    m_reconnectAttemptInFlight = false;

    if (!m_reconnectActive)
        return;

    const int delay = reconnectRetryDelayMsec();
    qDebug() << "Reconnect: next attempt in" << delay << "ms";
    m_reconnectRetryTimer.start(delay);
}

void VpnConnection::cancelReconnect()
{
    m_reconnectActive = false;
    m_reconnectAttemptInFlight = false;
    m_reconnectRetryTimer.stop();
    m_reconnectWatchdogTimer.stop();
}

int VpnConnection::reconnectRetryDelayMsec() const
{
    // 1s, 2s, 4s, ... capped at RECONNECT_RETRY_MAX_MSEC; a fresh trigger
    // resets m_reconnectAttempt and thus the sequence.
    const int exponent = qMin(m_reconnectAttempt > 0 ? m_reconnectAttempt - 1 : 0, 6);
    return qMin(RECONNECT_RETRY_BASE_MSEC << exponent, RECONNECT_RETRY_MAX_MSEC);
}
#endif

void VpnConnection::onProtocolConnectionStateChanged(Vpn::ConnectionState state)
{
#ifdef AMNEZIA_DESKTOP
    if (m_reconnectActive) {
        switch (state) {
        case Vpn::ConnectionState::Connected:
            qDebug() << "Reconnect: succeeded on attempt" << m_reconnectAttempt;
            cancelReconnect();
            break; // propagate below
        case Vpn::ConnectionState::Disconnected:
        case Vpn::ConnectionState::Error:
            // Keep the between-attempts cleanup the old code used to run for
            // a swallowed Disconnected (DNS flush, saved-routes cleanup), but
            // hold the UI in Reconnecting and keep retrying.
            onConnectionStateChanged(state);
            if (m_reconnectAttemptInFlight) {
                qWarning() << "Reconnect: attempt" << m_reconnectAttempt << "failed, protocol reported"
                           << QMetaEnum::fromType<Vpn::ConnectionState>().valueToKey(state);
                scheduleReconnectRetry();
            }
            return;
        default:
            // Transient states while retrying — keep showing Reconnecting.
            return;
        }
    }
#endif

    setConnectionState(state);
}

void VpnConnection::disconnectFromVpn()
{
#ifdef AMNEZIA_DESKTOP
    cancelReconnect();
#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    // iOS/macOS NE use IosController directly; m_vpnProtocol is not set there.
    IosController::Instance()->disconnectVpn();
    disconnect(&m_checkTimer, &QTimer::timeout, IosController::Instance(), &IosController::checkStatus);
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
    // Drive the final state ourselves: a protocol that is already internally
    // Disconnected (e.g. after failed reconnect attempts) will not emit
    // another Disconnected, which used to leave the UI stuck in Disconnecting.
    m_vpnProtocol->disconnect(this);
#endif

    m_vpnProtocol->stop();

#if !defined(Q_OS_ANDROID) && !defined(AMNEZIA_DESKTOP)
    m_vpnProtocol->deleteLater();
#endif

    m_vpnProtocol = nullptr;

#ifdef AMNEZIA_DESKTOP
    setConnectionState(Vpn::ConnectionState::Disconnected);
#endif
}

void VpnConnection::setConnectionState(Vpn::ConnectionState state) {
    onConnectionStateChanged(state);

#ifndef AMNEZIA_DESKTOP
    // On desktop the reconnect machine decides which protocol events are
    // propagated (see onProtocolConnectionStateChanged); on mobile keep the
    // historical behavior of hiding the stop() blip during a reconnect.
    if (state == Vpn::Disconnected && m_connectionState == Vpn::Reconnecting)
        return;
#endif

    m_connectionState = state;
    emit connectionStateChanged(state);
}
