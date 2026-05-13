#include "vpnconnection.h"

#include <algorithm>

#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QSettings>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QThread>

#ifdef Q_OS_ANDROID
    #include <QGuiApplication>
#endif

#include <configurators/cloak_configurator.h>
#include <configurators/openvpn_configurator.h>
#include <configurators/shadowsocks_configurator.h>
#include <configurators/wireguard_configurator.h>

#ifdef AMNEZIA_DESKTOP
    #include "core/ipcclient.h"
    #include <protocols/wireguardprotocol.h>
#endif

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
#endif

#include "core/networkUtilities.h"
#include "vpnconnection.h"

namespace {
constexpr int kConnectWatchdogTimeoutMs = 28000;
constexpr int kDisconnectWatchdogTimeoutMs = 12000;
constexpr int kReconnectBaseDelayMs = 1200;
constexpr int kMaxRecoveryAttempts = 2;
constexpr int kFailureWindowSecs = 180;
constexpr int kSafeModeFailureThreshold = 3;
constexpr int kSafeModeDurationSecs = 1800;

constexpr char kSafeModeUntilEpochKey[] = "Conf/safeModeUntilEpochSec";
constexpr char kVIPEnabledProfilesCountKey[] = "Conf/vipEnabledProfilesCount";

bool isXrayBasedProtocolName(const QString &protocolName)
{
    return protocolName == ProtocolProps::protoToString(Proto::Xray)
           || protocolName == ProtocolProps::protoToString(Proto::SSXray);
}

QString currentProtocolName(const QJsonObject &vpnConfiguration)
{
    return vpnConfiguration.value(config_key::vpnproto).toString();
}

bool usesLegacyDesktopSiteRouting(const QJsonObject &vpnConfiguration)
{
    return !isXrayBasedProtocolName(currentProtocolName(vpnConfiguration));
}
}

VpnConnection::VpnConnection(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_settings(settings), m_checkTimer(this)
{
    m_connectionState = Vpn::ConnectionState::Disconnected;
    m_stateWatchdogTimer.setParent(this);
    m_recoveryTimer.setParent(this);
    m_switchWatchdog.setParent(this);

    m_stateWatchdogTimer.setSingleShot(true);
    connect(&m_stateWatchdogTimer, &QTimer::timeout, this, &VpnConnection::handleStateWatchdogTimeout);

    m_recoveryTimer.setSingleShot(true);
    connect(&m_recoveryTimer, &QTimer::timeout, this, [this]() {
        if (m_userRequestedDisconnect || m_reconnectScheduled == false) {
            return;
        }

        m_reconnectScheduled = false;
        connectToVpn(m_lastServerIndex, m_lastCredentials, m_lastContainer, m_lastVpnConfiguration);
    });

    m_switchWatchdog.setSingleShot(true);
    connect(&m_switchWatchdog, &QTimer::timeout, this, &VpnConnection::handleSwitchWatchdogTimeout);

#ifdef Q_OS_ANDROID
    if (auto *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        m_androidAppActive = (app->applicationState() == Qt::ApplicationActive);
        connect(app, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
            const bool isActive = (state == Qt::ApplicationActive);
            QMetaObject::invokeMethod(this, [this, isActive]() {
                setAndroidAppActive(isActive);
            }, Qt::QueuedConnection);
        });
    }
#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    m_checkTimer.setInterval(1000);
    connect(IosController::Instance(), &IosController::connectionStateChanged, this, &VpnConnection::onConnectionStateChanged);
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
        if (reply.waitForFinished(1500) && reply.returnValue())
            qDebug() << "VpnConnection::onKillSwitchModeChanged: Killswitch refreshed";
        else
            qWarning() << "VpnConnection::onKillSwitchModeChanged: Failed to execute remote refreshKillSwitch call (or 1500 ms IPC timeout)";
    });
#endif
}

void VpnConnection::onConnectionStateChanged(Vpn::ConnectionState state)
{
    // Defensive: same-state re-entries must not reissue desktop IPC RPCs.
    // setConnectionState already filters same-state transitions before calling
    // us, but we guard here too so a future reorder of setConnectionState's
    // body cannot double-issue flushDns / clearSavedRoutes / resetIpStack.
    if (state == m_connectionState) {
        return;
    }

#ifdef AMNEZIA_DESKTOP
    auto container = m_settings->defaultContainer(m_settings->defaultServerIndex());

    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        switch (state) {
            case Vpn::ConnectionState::Connected: {
                if (usesLegacyDesktopSiteRouting(m_vpnConfiguration)) {
                    iface->resetIpStack();
                } else {
                    qDebug() << "VpnConnection::onConnectionStateChanged: skipping IP stack reset for XRay session";
                }

                auto flushDns = iface->flushDns();
                if (flushDns.waitForFinished(1500) && flushDns.returnValue())
                    qDebug() << "VpnConnection::onConnectionStateChanged: Successfully flushed DNS";
                else
                    qWarning() << "VpnConnection::onConnectionStateChanged: Failed to flush DNS (or 1500 ms IPC timeout)";


                if (!ContainerProps::isAwgContainer(container) &&
                    container != DockerContainer::WireGuard) {
                    QString dns1 = m_vpnConfiguration.value(config_key::dns1).toString();
                    QString dns2 = m_vpnConfiguration.value(config_key::dns2).toString();

                    // TODO: add error code handling for all routeAddList (or rework the code below)
                    iface->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << dns1 << dns2);
                }
            } break;
            case Vpn::ConnectionState::Disconnected:
            case Vpn::ConnectionState::Error: {
                auto flushDns = iface->flushDns();
                if (flushDns.waitForFinished(1500) && flushDns.returnValue())
                    qDebug() << "VpnConnection::onConnectionStateChanged: Successfully flushed DNS";
                else
                    qWarning() << "VpnConnection::onConnectionStateChanged: Failed to flush DNS (or 1500 ms IPC timeout)";

                auto clearSavedRoutes = iface->clearSavedRoutes();
                if (clearSavedRoutes.waitForFinished(1500) && clearSavedRoutes.returnValue())
                    qDebug() << "VpnConnection::onConnectionStateChanged: Successfully cleared saved routes";
                else
                    qWarning() << "VpnConnection::onConnectionStateChanged: Failed to clear saved routes (or 1500 ms IPC timeout)";
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

void VpnConnection::connectToVpn(int serverIndex, const ServerCredentials &credentials, DockerContainer container,
                                 const QJsonObject &vpnConfiguration)
{
    if (!m_reconnectScheduled) {
        clearRecoveryState();
    }
    m_reconnectScheduled = false;
    m_userRequestedDisconnect = false;
    m_lastServerIndex = serverIndex;
    m_lastCredentials = credentials;
    m_lastContainer = container;
    m_lastVpnConfiguration = vpnConfiguration;

    qDebug() << QString("Trying to connect to VPN, server index is %1, container is %2")
                        .arg(serverIndex)
                        .arg(ContainerProps::containerToString(container));

    m_remoteAddress = NetworkUtilities::getIPAddress(credentials.hostName);
    m_vpnConfiguration = vpnConfiguration;

    // Show "Connecting" immediately so the UI doesn't flicker to Disconnected
    // during a server-switch handover while the old protocol is shutting down.
    setConnectionState(Vpn::ConnectionState::Connecting);

    // Note: on iOS / MACOS_NE, m_vpnProtocol is never set by startNewConnection
    // (IosController is used directly), so protoIsLive is always false and the
    // switch-wait block below is intentionally a no-op on those platforms.
    // If that changes, wire the iOS teardown through the same fence.
    // If a previous protocol exists and is still actively running or tearing
    // down, wait for it to reach a terminal state before starting the new one.
    // Otherwise the Android service sees the new CONNECT while still in
    // DISCONNECTING and silently drops it, which is the user-visible hang.
    if (m_vpnProtocol) {
        const Vpn::ConnectionState protoState = m_vpnProtocol->connectionState();
        const bool protoIsLive =
                protoState == Vpn::ConnectionState::Connecting
                || protoState == Vpn::ConnectionState::Connected
                || protoState == Vpn::ConnectionState::Reconnecting
                || protoState == Vpn::ConnectionState::Disconnecting;

        // Detach old protocol's signals so its terminal broadcast isn't
        // surfaced to the UI as a Disconnected flicker. Our one-shot handler
        // below observes the terminal transition we care about.
        disconnect(m_vpnProtocol.data(), nullptr, this, nullptr);

        if (protoIsLive) {
            qDebug() << "VpnConnection::connectToVpn(): waiting for previous protocol to shut down, state ="
                     << protoState;

            if (m_protocolShutdownConn) {
                QObject::disconnect(m_protocolShutdownConn);
                m_protocolShutdownConn = QMetaObject::Connection();
            }

            m_protocolShutdownConn = connect(
                m_vpnProtocol.data(), &VpnProtocol::connectionStateChanged, this,
                [this](Vpn::ConnectionState state) {
                    if (state != Vpn::ConnectionState::Disconnected
                        && state != Vpn::ConnectionState::Error
                        && state != Vpn::ConnectionState::Unknown) {
                        return;
                    }

                    qDebug() << "VpnConnection: previous protocol reached terminal state" << state
                             << "- starting deferred connection";

                    if (m_protocolShutdownConn) {
                        QObject::disconnect(m_protocolShutdownConn);
                        m_protocolShutdownConn = QMetaObject::Connection();
                    }
                    if (m_switchWatchdog.isActive()) {
                        m_switchWatchdog.stop();
                    }
                    m_vpnProtocol.reset();

                    QMetaObject::invokeMethod(
                        this,
                        [this]() {
                            startNewConnection(m_lastContainer, m_lastVpnConfiguration);
                        },
                        Qt::QueuedConnection);
                });

            // Arm the watchdog BEFORE stop() so that if stop() drives the
            // protocol synchronously to a terminal state (possible on
            // desktop), the one-shot lambda above observes an active
            // watchdog and can disarm it. Otherwise we would leave the
            // watchdog armed against the fresh connection we are about to
            // start, and at 4 s it would tear the new connection down.
            m_switchWatchdog.start(4000);
            m_vpnProtocol->stop();
            return;
        }

        // Previous protocol is Disconnected / Error / Unknown / Preparing,
        // safe to drop immediately without waiting.
        qDebug() << "VpnConnection::connectToVpn(): dropping already-inactive protocol, state =" << protoState;
        m_vpnProtocol.reset();
    }

#ifdef AMNEZIA_DESKTOP
    // Desktop grace window: if the user just tapped Disconnect and is now
    // re-tapping Connect, the fblink daemon may still be tearing down the
    // previous tunnel (deleteInterface() runs waitForFinished(5000) on the
    // wg process on Linux, WFP firewall rules are being refreshed on Windows,
    // pfctl is reloading its ruleset on macOS). Landing a new activate on
    // top of that in-flight teardown is what causes the hang. Defer by up
    // to 500 ms to let the daemon settle.
    if (m_vpnProtocol.isNull() && m_lastDisconnectMsec > 0) {
        const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_lastDisconnectMsec;
        if (elapsed >= 0 && elapsed < 500) {
            const int rawDelay = static_cast<int>(500 - elapsed);
            const int delayMs = std::max(50, std::min(500, rawDelay));
            qDebug() << "VpnConnection::connectToVpn(): daemon grace window, deferring" << delayMs << "ms";
            m_lastDisconnectMsec = 0;
            QTimer::singleShot(delayMs, this, [this]() {
                startNewConnection(m_lastContainer, m_lastVpnConfiguration);
            });
            return;
        }
    }
#endif

    startNewConnection(container, vpnConfiguration);
}

void VpnConnection::startNewConnection(DockerContainer container, const QJsonObject &vpnConfiguration)
{
    m_vpnConfiguration = vpnConfiguration;

#ifdef AMNEZIA_DESKTOP
    appendKillSwitchConfig();
#endif

    if (m_connectionState != Vpn::ConnectionState::Connecting) {
        setConnectionState(Vpn::ConnectionState::Connecting);
    }

    appendSplitTunnelingConfig();
    if (hasMissingManagedRoutingRules()) {
        qWarning() << "Managed routing profiles are enabled but rules are missing in config. Blocking unsafe full-tunnel fallback.";
        setConnectionState(Vpn::ConnectionState::Error);
        emit vpnProtocolError(ErrorCode::InternalError);
        return;
    }

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
    Proto proto = ContainerProps::defaultProtocol(container);
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

void VpnConnection::handleSwitchWatchdogTimeout()
{
    // Belt-and-braces: if the user initiated a Disconnect while the
    // switch-wait was pending, do not resurrect the connection. disconnectFromVpn()
    // stops this timer already, but a race between timer fire and
    // queued disconnect delivery is still possible on the worker thread.
    if (m_userRequestedDisconnect) {
        qDebug() << "VpnConnection: switch watchdog fired after user disconnect - not restarting";

        if (m_protocolShutdownConn) {
            QObject::disconnect(m_protocolShutdownConn);
            m_protocolShutdownConn = QMetaObject::Connection();
        }
        if (m_vpnProtocol) {
            disconnect(m_vpnProtocol.data(), nullptr, this, nullptr);
            m_vpnProtocol.reset();
        }
        return;
    }

    qWarning() << "VpnConnection: switch watchdog expired - previous protocol did not reach terminal state in time, forcing new connection";

    if (m_protocolShutdownConn) {
        QObject::disconnect(m_protocolShutdownConn);
        m_protocolShutdownConn = QMetaObject::Connection();
    }

    if (m_vpnProtocol) {
        disconnect(m_vpnProtocol.data(), nullptr, this, nullptr);
        m_vpnProtocol.reset();
    }

    startNewConnection(m_lastContainer, m_lastVpnConfiguration);
}

void VpnConnection::createProtocolConnections()
{
    connect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
    // Use Qt::QueuedConnection so a protocol that emits Disconnected
    // synchronously inside stop() (desktop OpenVPN / WireGuard do this) can
    // not re-enter setConnectionState -> onConnectionStateChanged and spin
    // a nested event loop while stop() is still on the stack.
    connect(m_vpnProtocol.data(), &VpnProtocol::connectionStateChanged, this, &VpnConnection::setConnectionState,
            Qt::QueuedConnection);
    connect(m_vpnProtocol.data(), SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));

#ifdef AMNEZIA_DESKTOP
    IpcClient::withInterface([this](QSharedPointer<IpcInterfaceReplica> rep) {
        connect(rep.data(), &IpcInterfaceReplica::networkChanged, this, &VpnConnection::reconnectToVpn,
                static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));
        connect(rep.data(), &IpcInterfaceReplica::wakeup, this, &VpnConnection::reconnectToVpn,
                static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));
    });
#endif
}

void VpnConnection::appendKillSwitchConfig()
{
    m_vpnConfiguration.insert(config_key::killSwitchOption, QVariant(m_settings->isKillSwitchEnabled()).toString());
    m_vpnConfiguration.insert(config_key::allowedDnsServers, QVariant(m_settings->allowedDnsServers()).toJsonValue());
}

void VpnConnection::appendSplitTunnelingConfig()
{
    // this block is for old native configs and for old self-hosted configs
    auto protocolName = m_vpnConfiguration.value(config_key::vpnproto).toString();
    const bool isXrayBasedProtocol = isXrayBasedProtocolName(protocolName);
    bool xrayHasManagedRouting = false;

    if (isXrayBasedProtocol) {
        const QJsonObject protocolConfig = m_vpnConfiguration.value(protocolName + "_config_data").toObject();
        if (protocolConfig.contains("routing")) {
            xrayHasManagedRouting = true;
        } else {
            const QString lastConfig = protocolConfig.value(config_key::last_config).toString();
            if (!lastConfig.isEmpty()) {
                const QJsonDocument lastConfigDoc = QJsonDocument::fromJson(lastConfig.toUtf8());
                xrayHasManagedRouting = lastConfigDoc.isObject() && lastConfigDoc.object().contains("routing");
            }
        }
    }

    if (protocolName == ProtocolProps::protoToString(Proto::Awg) || protocolName == ProtocolProps::protoToString(Proto::WireGuard)) {
        auto configData = m_vpnConfiguration.value(protocolName + "_config_data").toObject();
        if (configData.value(config_key::allowed_ips).isString()) {
            QJsonArray allowedIpsJsonArray = QJsonArray::fromStringList(configData.value(config_key::allowed_ips).toString().split(", "));
            configData.insert(config_key::allowed_ips, allowedIpsJsonArray);
            m_vpnConfiguration.insert(protocolName + "_config_data", configData);
        } else if (configData.value(config_key::allowed_ips).isUndefined()) {
            auto nativeConfig = configData.value(config_key::config).toString();
            auto nativeConfigLines = nativeConfig.split("\n");
            for (auto &line : nativeConfigLines) {
                if (line.contains("AllowedIPs")) {
                    auto allowedIpsString = line.split(" = ");
                    if (allowedIpsString.size() < 1) {
                        break;
                    }
                    QString allowedIpsRaw = allowedIpsString.at(1).trimmed();
                    QJsonArray allowedIpsJsonArray;
                    for (const QString &ip : allowedIpsRaw.split(",")) {
                        allowedIpsJsonArray.append(ip.trimmed());
                    }
                    configData.insert(config_key::allowed_ips, allowedIpsJsonArray);
                    m_vpnConfiguration.insert(protocolName + "_config_data", configData);
                    break;
                }
            }
        }

        if (configData.value(config_key::persistent_keep_alive).isUndefined()) {
            auto nativeConfig = configData.value(config_key::config).toString();
            auto nativeConfigLines = nativeConfig.split("\n");
            for (auto &line : nativeConfigLines) {
                if (line.contains("PersistentKeepalive")) {
                    auto persistentKeepaliveString = line.split(" = ");
                    if (persistentKeepaliveString.size() < 1) {
                        break;
                    }
                    configData.insert(config_key::persistent_keep_alive, persistentKeepaliveString.at(1));
                    m_vpnConfiguration.insert(protocolName + "_config_data", configData);
                    break;
                }
            }
        }

        QJsonArray allowedIpsJsonArray = configData.value(config_key::allowed_ips).toArray();
        if (allowedIpsJsonArray.contains("0.0.0.0/0") && allowedIpsJsonArray.contains("::/0")) {
            // Full-tunnel WG/AWG config is valid. Legacy site split remains disabled.
        }
    }

    Settings::RouteMode routeMode = Settings::RouteMode::VpnAllSites;
    QJsonArray sitesJsonArray;
    QSettings subscriptionSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    const bool canUseAppSplitTunneling = subscriptionSettings.value("subscriptionCanUseAppSplitTunneling", false).toBool();
    const int enabledManagedProfilesCount = subscriptionSettings.value(kVIPEnabledProfilesCountKey, 0).toInt();

    const bool usesManagedXrayRouting = isXrayBasedProtocol && xrayHasManagedRouting;
    const bool missingExpectedManagedRouting = isXrayBasedProtocol && !xrayHasManagedRouting && enabledManagedProfilesCount > 0;

    if (usesManagedXrayRouting) {
        qDebug() << "XRay config contains managed routing rules; legacy site split tunneling is disabled";
    } else if (missingExpectedManagedRouting) {
        qWarning() << "XRay managed routing is expected, but rules are missing. Connection will be blocked until profiles sync.";
    } else if (isXrayBasedProtocol) {
        qWarning() << "XRay config has no managed routing rules. Legacy site split remains disabled.";
    }

    m_vpnConfiguration.insert(config_key::splitTunnelType, routeMode);
    m_vpnConfiguration.insert(config_key::splitTunnelSites, sitesJsonArray);
    m_vpnConfiguration.insert("vipRoutingRulesMissing", missingExpectedManagedRouting);

    Settings::AppsRouteMode appsRouteMode = Settings::AppsRouteMode::VpnAllApps;
    QJsonArray appsJsonArray;
    if (canUseAppSplitTunneling && m_settings->isAppsSplitTunnelingEnabled()) {
        appsRouteMode = m_settings->getAppsRouteMode();

        auto apps = m_settings->getVpnApps(appsRouteMode);
        for (const auto &app : apps) {
#ifdef Q_OS_ANDROID
            // Android VpnService.Builder accepts application package names only.
            if (!app.packageName.isEmpty()) {
                appsJsonArray.append(app.packageName);
            }
#else
            appsJsonArray.append(app.appPath.isEmpty() ? app.packageName : app.appPath);
#endif
        }

        if (appsJsonArray.isEmpty()) {
            appsRouteMode = Settings::AppsRouteMode::VpnAllApps;
        }
    }

    m_vpnConfiguration.insert(config_key::appSplitTunnelType, appsRouteMode);
    m_vpnConfiguration.insert(config_key::splitTunnelApps, appsJsonArray);

    QString siteSplitStatus = "deprecated (disabled)";
    if (missingExpectedManagedRouting) {
        siteSplitStatus = "temporarily unavailable (managed profiles not synced)";
    } else if (usesManagedXrayRouting) {
        siteSplitStatus = "managed by XRay routing profiles (legacy disabled)";
    } else if (isXrayBasedProtocol) {
        siteSplitStatus = "disabled (no XRay routing rules)";
    }

    qDebug() << QString("Site split tunneling is %1").arg(siteSplitStatus);
    qDebug() << QString("App split tunneling is %1, route mode is %2")
                        .arg((canUseAppSplitTunneling && m_settings->isAppsSplitTunnelingEnabled()) ? "enabled" : "disabled")
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
    if (m_vpnProtocol.isNull())
        return;

    // While a server-switch handover is pending, m_vpnProtocol still points
    // at the OLD server's protocol. Replaying stop()+start() here would send
    // the old config to the platform service and can race the switch-wait's
    // deferred startNewConnection, ending up connected to the wrong server.
    // m_protocolShutdownConn is the single source of truth for "switch-wait in flight".
    if (m_protocolShutdownConn
        || m_vpnProtocol->connectionState() == Vpn::ConnectionState::Disconnecting) {
        qDebug() << "Reconnect ignored: server-switch handover in progress";
        return;
    }

    // Allow reconnect only for Connected / Reconnecting states. Dropping
    // Connecting from the allowed set prevents networkChanged/wakeup from
    // tearing the tunnel down while the initial handshake is still in
    // progress: the desktop service fires networkChanged the moment the new
    // default route is installed, which collides with our own in-flight
    // Connecting attempt during a server switch.
    if (m_connectionState != Vpn::ConnectionState::Connected
        && m_connectionState != Vpn::ConnectionState::Reconnecting) {
        qWarning() << QString("Reconnect triggered during inappropriate state: %1; ignoring slot")
                              .arg(QMetaEnum::fromType<Vpn::ConnectionState>().valueToKey(m_connectionState));
        return;
    }

    qDebug() << "Reconnect triggered. Reconnecting to the server";

    setConnectionState(Vpn::ConnectionState::Reconnecting);

    m_vpnProtocol->stop();
    if (ErrorCode err = m_vpnProtocol->start(); err != ErrorCode::NoError) {
        setConnectionState(Vpn::ConnectionState::Error);
        emit vpnProtocolError(err);
    }
}

void VpnConnection::disconnectFromVpn()
{
    m_userRequestedDisconnect = true;
    clearRecoveryState();

    // Cancel any pending server-switch handover so the 4 s watchdog cannot
    // resurrect a connection after the user asked to tear it down.
    if (m_switchWatchdog.isActive()) {
        m_switchWatchdog.stop();
    }
    if (m_protocolShutdownConn) {
        QObject::disconnect(m_protocolShutdownConn);
        m_protocolShutdownConn = QMetaObject::Connection();
    }

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
    *connection = connect(AndroidController::instance(), &AndroidController::connectionStateChanged, this,
                          [this, connection](Vpn::ConnectionState state) {
                              if (state == Vpn::ConnectionState::Disconnected
                                  || state == Vpn::ConnectionState::Error) {
                                  setConnectionState(Vpn::ConnectionState::Disconnected);
                                  disconnect(*connection);
                                  delete connection;
                              }
                          });
#endif

    // Fence: detach protocol signals from this so the synchronous Disconnected
    // emission inside stop() on desktop protocols cannot re-enter
    // setConnectionState -> onConnectionStateChanged -> nested event loop.
    // On AMNEZIA_DESKTOP the protocol is destroyed synchronously via the
    // QSharedPointer ref drop below (no deleteLater) because desktop
    // protocols do not spin their own event loop and their destructors call
    // back into stop() if state != Disconnected - this fence keeps that
    // destructor path safe by ensuring no signals can be delivered into
    // `this` during the destruction window.
    disconnect(m_vpnProtocol.data(), nullptr, this, nullptr);
    m_vpnProtocol->stop();

#if !defined(Q_OS_ANDROID) && !defined(AMNEZIA_DESKTOP)
    m_vpnProtocol->deleteLater();
#endif

    m_vpnProtocol = nullptr;

#ifdef AMNEZIA_DESKTOP
    // Desktop: stop() has returned synchronously, force the terminal
    // state now. Signals from m_vpnProtocol were detached above so no
    // re-entrant onConnectionStateChanged can fire.
    setConnectionState(Vpn::ConnectionState::Disconnected);
#elif !defined(Q_OS_ANDROID)
    // iOS / other fallbacks: let the 200 ms timer settle the state.
    QTimer::singleShot(200, this, [this]() {
        if (m_connectionState == Vpn::ConnectionState::Disconnecting) {
            setConnectionState(Vpn::ConnectionState::Disconnected);
        }
    });
#endif

    // Stamp the disconnect time so connectToVpn's desktop grace window can
    // detect a rapid Connect-after-Disconnect and defer the new activate.
    m_lastDisconnectMsec = QDateTime::currentMSecsSinceEpoch();
}

void VpnConnection::setConnectionState(Vpn::ConnectionState state)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, [this, state]() {
            setConnectionState(state);
        }, Qt::QueuedConnection);
        return;
    }

    const Vpn::ConnectionState previousState = m_connectionState;
    onConnectionStateChanged(state);

    if (state == Vpn::ConnectionState::Disconnected && previousState == Vpn::ConnectionState::Reconnecting) {
        return;
    }

    m_connectionState = state;
    armStateWatchdog(state);
    emit connectionStateChanged(state);

    if (state == Vpn::ConnectionState::Connected) {
        m_recoveryAttempts = 0;
        m_failureBurst = 0;
        m_failureWindowStartedAt = 0;
        m_userRequestedDisconnect = false;
        clearRecoveryState();
        return;
    }

    const bool failedTransition =
            state == Vpn::ConnectionState::Error
            || (state == Vpn::ConnectionState::Disconnected
                && (previousState == Vpn::ConnectionState::Connecting
                    || previousState == Vpn::ConnectionState::Preparing
                    || previousState == Vpn::ConnectionState::Reconnecting));

    if (failedTransition) {
        registerFailureAndMaybeEnterSafeMode();
        if (!m_userRequestedDisconnect) {
            scheduleRecoveryReconnect();
        }
        return;
    }

    if (state == Vpn::ConnectionState::Disconnected && previousState == Vpn::ConnectionState::Disconnecting) {
        m_userRequestedDisconnect = false;
        clearRecoveryState();
    }
}

void VpnConnection::armStateWatchdog(Vpn::ConnectionState state)
{
#ifdef Q_OS_ANDROID
    if (!m_androidAppActive) {
        if (m_stateWatchdogTimer.isActive()) {
            m_stateWatchdogTimer.stop();
        }
        return;
    }
#endif

    switch (state) {
    case Vpn::ConnectionState::Preparing:
    case Vpn::ConnectionState::Connecting:
    case Vpn::ConnectionState::Reconnecting:
        m_stateWatchdogTimer.start(kConnectWatchdogTimeoutMs);
        break;
    case Vpn::ConnectionState::Disconnecting:
        m_stateWatchdogTimer.start(kDisconnectWatchdogTimeoutMs);
        break;
    default:
        m_stateWatchdogTimer.stop();
        break;
    }
}

void VpnConnection::handleStateWatchdogTimeout()
{
#ifdef Q_OS_ANDROID
    if (!m_androidAppActive) {
        qDebug() << "Watchdog timeout ignored because Android app is in background";
        return;
    }
#endif

    if (m_connectionState == Vpn::ConnectionState::Disconnecting) {
        qWarning() << "Disconnect watchdog timeout: forcing disconnected state";
        setConnectionState(Vpn::ConnectionState::Disconnected);
        return;
    }

    if (m_connectionState == Vpn::ConnectionState::Preparing
        || m_connectionState == Vpn::ConnectionState::Connecting
        || m_connectionState == Vpn::ConnectionState::Reconnecting) {
        qWarning() << "Connection watchdog timeout while state is" << m_connectionState
                   << "- attempting recovery";
        if (m_vpnProtocol) {
            m_vpnProtocol->stop();
            m_vpnProtocol.reset();
        }
        setConnectionState(Vpn::ConnectionState::Error);
        emit vpnProtocolError(ErrorCode::InternalError);
    }
}

void VpnConnection::clearRecoveryState()
{
    m_recoveryTimer.stop();
    m_reconnectScheduled = false;
}

void VpnConnection::scheduleRecoveryReconnect()
{
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    const qint64 safeModeUntil = settings.value(kSafeModeUntilEpochKey, 0).toLongLong();
    if (safeModeUntil > QDateTime::currentSecsSinceEpoch()) {
        qWarning() << "Recovery reconnect skipped: safe mode is active";
        return;
    }

    if (m_reconnectScheduled || m_recoveryAttempts >= kMaxRecoveryAttempts) {
        return;
    }

    if (m_lastServerIndex < 0 || m_lastCredentials.hostName.isEmpty() || m_lastVpnConfiguration.isEmpty()) {
        return;
    }

    ++m_recoveryAttempts;
    const int delayMs = kReconnectBaseDelayMs * (1 << (m_recoveryAttempts - 1));
    m_reconnectScheduled = true;
    qWarning() << "Scheduling recovery reconnect attempt" << m_recoveryAttempts << "in" << delayMs << "ms";
    m_recoveryTimer.start(delayMs);
}

void VpnConnection::registerFailureAndMaybeEnterSafeMode()
{
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (m_failureWindowStartedAt == 0 || (now - m_failureWindowStartedAt) > kFailureWindowSecs) {
        m_failureWindowStartedAt = now;
        m_failureBurst = 0;
    }

    ++m_failureBurst;
    if (m_failureBurst < kSafeModeFailureThreshold) {
        return;
    }

    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    const qint64 safeModeUntil = now + kSafeModeDurationSecs;
    settings.setValue(kSafeModeUntilEpochKey, safeModeUntil);
    settings.sync();

    if (m_settings) {
        m_settings->setAutoConnect(false);
    }

    qWarning() << "Safe mode enabled due to repeated VPN failures until" << safeModeUntil;
    clearRecoveryState();
    m_recoveryAttempts = 0;
    m_failureBurst = 0;
    m_failureWindowStartedAt = 0;
}

bool VpnConnection::hasMissingManagedRoutingRules() const
{
    return m_vpnConfiguration.value("vipRoutingRulesMissing").toBool(false);
}

#ifdef Q_OS_ANDROID
void VpnConnection::setAndroidAppActive(bool active)
{
    if (m_androidAppActive == active) {
        return;
    }

    m_androidAppActive = active;

    if (!m_androidAppActive) {
        if (m_stateWatchdogTimer.isActive()) {
            qDebug() << "Android app moved to background: pausing VPN watchdog";
            m_stateWatchdogTimer.stop();
        }
        return;
    }

    qDebug() << "Android app returned to foreground: reconciling VPN state";

    AndroidController::instance()->requestVpnStatus();

    armStateWatchdog(m_connectionState);
}
#endif
