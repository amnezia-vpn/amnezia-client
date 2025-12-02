#include "vpnconnection.h"

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
    #include <QThread>

#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
#endif

#include "core/networkUtilities.h"
#include "vpnconnection.h"

VpnConnection::VpnConnection(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_settings(settings), m_checkTimer(new QTimer(this))
{
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
    if (InterfaceReady()) {
        qDebug() << "Set KillSwitch Strict mode enabled " << enabled;
        IpcClient::Interface()->refreshKillSwitch(enabled);
    }
#endif
}

void VpnConnection::onConnectionStateChanged(Vpn::ConnectionState state)
{
#ifdef AMNEZIA_DESKTOP
    auto container = m_settings->defaultContainer(m_settings->defaultServerIndex());

    if (InterfaceReady()) {
        if (state == Vpn::ConnectionState::Connected) {
            IpcClient::Interface()->resetIpStack();
            IpcClient::Interface()->flushDns();

            if (container != DockerContainer::Awg && container != DockerContainer::WireGuard) {
                QString dns1 = m_vpnConfiguration.value(config_key::dns1).toString();
                QString dns2 = m_vpnConfiguration.value(config_key::dns2).toString();

                IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << dns1 << dns2);

                if (m_settings->isSitesSplitTunnelingEnabled()) {
                    IpcClient::Interface()->routeDeleteList(m_vpnProtocol->vpnGateway(), QStringList() << "0.0.0.0");
                    // qDebug() << "VpnConnection::onConnectionStateChanged :: adding custom routes, count:" << forwardIps.size();
                    if (m_settings->routeMode() == Settings::VpnOnlyForwardSites) {
                        QTimer::singleShot(1000, m_vpnProtocol.data(),
                                           [this]() { addSitesRoutes(m_vpnProtocol->vpnGateway(), m_settings->routeMode()); });
                    } else if (m_settings->routeMode() == Settings::VpnAllExceptSites) {
                        IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << "0.0.0.0/1");
                        IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << "128.0.0.0/1");

                        IpcClient::Interface()->routeAddList(m_vpnProtocol->routeGateway(), QStringList() << remoteAddress());
                        addSitesRoutes(m_vpnProtocol->routeGateway(), m_settings->routeMode());
                    }
                }
            }

            if (container != DockerContainer::Ipsec) {
                if (startNetworkCheckIfReady()) {
                    m_pendingNetworkCheck = false;
                } else {
                    m_pendingNetworkCheck = true;
                    qWarning() << "Deferring startNetworkCheck; missing gateway/local address"
                               << m_vpnProtocol->vpnGateway() << m_vpnProtocol->vpnLocalAddress();
                }
            } else {
                m_pendingNetworkCheck = false;
            }

        } else if (state == Vpn::ConnectionState::Error) {
            m_pendingNetworkCheck = false;
            IpcClient::Interface()->flushDns();

            if (m_settings->isSitesSplitTunnelingEnabled()) {
                if (m_settings->routeMode() == Settings::VpnOnlyForwardSites) {
                    IpcClient::Interface()->clearSavedRoutes();
                }
            }
        } else if (state == Vpn::ConnectionState::Connecting) {

        } else if (state == Vpn::ConnectionState::Disconnected) {
            m_pendingNetworkCheck = false;
            auto result = IpcClient::Interface()->stopNetworkCheck();
            result.waitForFinished(3000);
        }
    }
#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    if (state == Vpn::ConnectionState::Connected) {
        m_checkTimer.start();
    } else {
        m_checkTimer.stop();
    }
#endif
    emit connectionStateChanged(state);
}

const QString &VpnConnection::remoteAddress() const
{
    return m_remoteAddress;
}

void VpnConnection::addSitesRoutes(const QString &gw, Settings::RouteMode mode)
{
#ifdef AMNEZIA_DESKTOP
    QStringList ips;
    QStringList sites;
    const QVariantMap &m = m_settings->vpnSites(mode);
    for (auto i = m.constBegin(); i != m.constEnd(); ++i) {
        if (NetworkUtilities::checkIpSubnetFormat(i.key())) {
            ips.append(i.key());
        } else {
            if (NetworkUtilities::checkIpSubnetFormat(i.value().toString())) {
                ips.append(i.value().toString());
            }
            sites.append(i.key());
        }
    }
    ips.removeDuplicates();

    // add all IPs immediately
    IpcClient::Interface()->routeAddList(gw, ips);

    // re-resolve domains
    for (const QString &site : sites) {
        const auto &cbResolv = [this, site, gw, mode, ips](const QHostInfo &hostInfo) {
            const QList<QHostAddress> &addresses = hostInfo.addresses();
            QString ipv4Addr;
            for (const QHostAddress &addr : hostInfo.addresses()) {
                if (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
                    const QString &ip = addr.toString();
                    // qDebug() << "VpnConnection::addSitesRoutes updating site" << site << ip;
                    if (!ips.contains(ip)) {
                        IpcClient::Interface()->routeAddList(gw, QStringList() << ip);
                        m_settings->addVpnSite(mode, site, ip);
                    }
                    flushDns();
                    break;
                }
            }
        };
        QHostInfo::lookupHost(site, this, cbResolv);
    }
#endif
}

QSharedPointer<VpnProtocol> VpnConnection::vpnProtocol() const
{
    return m_vpnProtocol;
}

void VpnConnection::addRoutes(const QStringList &ips)
{
#ifdef AMNEZIA_DESKTOP
    if (connectionState() == Vpn::ConnectionState::Connected && IpcClient::Interface()) {
        if (m_settings->routeMode() == Settings::VpnOnlyForwardSites) {
            IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(), ips);
        } else if (m_settings->routeMode() == Settings::VpnAllExceptSites) {
            IpcClient::Interface()->routeAddList(m_vpnProtocol->routeGateway(), ips);
        }
    }
#endif
}

void VpnConnection::deleteRoutes(const QStringList &ips)
{
#ifdef AMNEZIA_DESKTOP
    if (connectionState() == Vpn::ConnectionState::Connected && IpcClient::Interface()) {
        if (m_settings->routeMode() == Settings::VpnOnlyForwardSites) {
            IpcClient::Interface()->routeDeleteList(vpnProtocol()->vpnGateway(), ips);
        } else if (m_settings->routeMode() == Settings::VpnAllExceptSites) {
            IpcClient::Interface()->routeDeleteList(m_vpnProtocol->routeGateway(), ips);
        }
    }
#endif
}

bool VpnConnection::InterfaceReady()
{
#ifdef AMNEZIA_DESKTOP
    if (m_IpcClient) {
        m_IpcClient->closeAndResetInstance(true);
        m_IpcClient->deleteLater();
        m_IpcClient = nullptr;
    }
    if (!m_IpcClient) {
        m_IpcClient = new IpcClient(this);
    }

    if (!m_IpcClient->isSocketConnected()) {
        if (!IpcClient::init(m_IpcClient)) {
            qWarning() << "Error occurred when init IPC client";
            emit serviceIsNotReady();
            return false;
        }
    }

    return IpcClient::Interface() != nullptr;
#endif
    return true;
}

void VpnConnection::flushDns()
{
#ifdef AMNEZIA_DESKTOP
    if (InterfaceReady())
        IpcClient::Interface()->flushDns();
#endif
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

    if (!m_vpnProtocol.data()) {
        return ErrorCode::InternalError;
    }

    return m_vpnProtocol.data()->lastError();
}

void VpnConnection::connectToVpn(int serverIndex, const ServerCredentials &credentials, DockerContainer container,
                                 const QJsonObject &vpnConfiguration)
{
    qDebug() << QString("ConnectToVpn, Server index is %1, container is %2, route mode is")
                        .arg(serverIndex)
                        .arg(ContainerProps::containerToString(container))
             << m_settings->routeMode();

    if (!InterfaceReady()) {
        emit connectionStateChanged(Vpn::ConnectionState::Error);
        return;
    } 

    m_remoteAddress = NetworkUtilities::getIPAddress(credentials.hostName);
    emit connectionStateChanged(Vpn::ConnectionState::Connecting);

    m_pendingNetworkCheck = false;
    m_vpnConfiguration = vpnConfiguration;
    m_serverIndex = serverIndex;
    m_serverCredentials = credentials;
    m_dockerContainer = container;

#ifdef AMNEZIA_DESKTOP
    if (m_vpnProtocol) {
        disconnect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
        m_vpnProtocol->stop();
        m_vpnProtocol.reset();
    }
    appendKillSwitchConfig();
#endif

    appendSplitTunnelingConfig();

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    m_vpnProtocol.reset(VpnProtocol::factory(container, m_vpnConfiguration));
    if (!m_vpnProtocol) {
        emit connectionStateChanged(Vpn::ConnectionState::Error);
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

    ErrorCode errorCode = m_vpnProtocol.data()->start();
    if (errorCode != ErrorCode::NoError)
        emit connectionStateChanged(Vpn::ConnectionState::Error);
}

void VpnConnection::restartConnection()
{
    // Only reconnect if VPN was connected before sleep/network change
    if (!m_wasConnectedBeforeSleep) {
        qDebug() << "VPN was not connected before sleep/network change, skipping reconnection";
        return;
    }
    
    qDebug() << "VPN was connected before sleep/network change, attempting reconnection";
    this->disconnectFromVpn();
#ifdef Q_OS_LINUX
    QThread::msleep(5000);
#endif
    this->connectToVpn(m_serverIndex, m_serverCredentials, m_dockerContainer, m_vpnConfiguration);
    
    // Reset the flag after reconnection attempt
    m_wasConnectedBeforeSleep = false;
}

void VpnConnection::createProtocolConnections()
{
    connect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
    connect(m_vpnProtocol.data(), SIGNAL(connectionStateChanged(Vpn::ConnectionState)), this,
            SLOT(onConnectionStateChanged(Vpn::ConnectionState)));
    connect(m_vpnProtocol.data(), SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));

#ifdef AMNEZIA_DESKTOP
    if (m_connectionLoseHandle)
        disconnect(m_connectionLoseHandle);
    if (m_networkChangeHandle)
        disconnect(m_networkChangeHandle);
    m_connectionLoseHandle = QMetaObject::Connection();
    m_networkChangeHandle = QMetaObject::Connection();

    m_connectionLoseHandle = connect(IpcClient::Interface().data(), &IpcInterfaceReplica::connectionLose,
            this, [this]() {
                qDebug() << "Connection Lose";
                auto result = IpcClient::Interface()->stopNetworkCheck();
                result.waitForFinished(3000);
                // Track VPN state before connection loss
                m_wasConnectedBeforeSleep = isConnected();
                qDebug() << "VPN was connected before connection loss:" << m_wasConnectedBeforeSleep;
                this->restartConnection();
            });
    m_networkChangeHandle = connect(IpcClient::Interface().data(), &IpcInterfaceReplica::networkChange,
            this, [this]() {
                qDebug() << "Network change";
                // Track VPN state before network change (including sleep/wake)
                m_wasConnectedBeforeSleep = isConnected();
                qDebug() << "VPN was connected before network change:" << m_wasConnectedBeforeSleep;
                this->restartConnection();
            });
    connect(m_vpnProtocol.data(), &VpnProtocol::tunnelAddressesUpdated,
            this, [this](const QString& gateway, const QString& localAddress) {
                Q_UNUSED(gateway)
                Q_UNUSED(localAddress)
                if (connectionState() != Vpn::ConnectionState::Connected) {
                    return;
                }
                if (startNetworkCheckIfReady()) {
                    m_pendingNetworkCheck = false;
                }
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
    bool allowSiteBasedSplitTunneling = true;

    // this block is for old native configs and for old self-hosted configs
    auto protocolName = m_vpnConfiguration.value(config_key::vpnproto).toString();
    if (protocolName == ProtocolProps::protoToString(Proto::Awg) || protocolName == ProtocolProps::protoToString(Proto::WireGuard)) {
        allowSiteBasedSplitTunneling = false;
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
                    QJsonArray allowedIpsJsonArray = QJsonArray::fromStringList(allowedIpsString.at(1).split(", "));
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
            allowSiteBasedSplitTunneling = true;
        }
    }

    Settings::RouteMode routeMode = Settings::RouteMode::VpnAllSites;
    QJsonArray sitesJsonArray;
    if (m_settings->isSitesSplitTunnelingEnabled()) {
        routeMode = m_settings->routeMode();

        if (allowSiteBasedSplitTunneling) {
            auto sites = m_settings->getVpnIps(routeMode);
            for (const auto &site : sites) {
                sitesJsonArray.append(site);
            }

            if (sitesJsonArray.isEmpty()) {
                routeMode = Settings::RouteMode::VpnAllSites;
            } else if (routeMode == Settings::VpnOnlyForwardSites) {
                // Allow traffic to Amnezia DNS
                sitesJsonArray.append(m_vpnConfiguration.value(config_key::dns1).toString());
                sitesJsonArray.append(m_vpnConfiguration.value(config_key::dns2).toString());
            }
        }
    }

    m_vpnConfiguration.insert(config_key::splitTunnelType, routeMode);
    m_vpnConfiguration.insert(config_key::splitTunnelSites, sitesJsonArray);

    Settings::AppsRouteMode appsRouteMode = Settings::AppsRouteMode::VpnAllApps;
    QJsonArray appsJsonArray;
    if (m_settings->isAppsSplitTunnelingEnabled()) {
        appsRouteMode = m_settings->getAppsRouteMode();

        auto apps = m_settings->getVpnApps(appsRouteMode);
        for (const auto &app : apps) {
            appsJsonArray.append(app.appPath.isEmpty() ? app.packageName : app.appPath);
        }

        if (appsJsonArray.isEmpty()) {
            appsRouteMode = Settings::AppsRouteMode::VpnAllApps;
        }
    }

    m_vpnConfiguration.insert(config_key::appSplitTunnelType, appsRouteMode);
    m_vpnConfiguration.insert(config_key::splitTunnelApps, appsJsonArray);
}

bool VpnConnection::startNetworkCheckIfReady()
{
#ifdef AMNEZIA_DESKTOP
    if (!m_vpnProtocol || m_dockerContainer == DockerContainer::Ipsec) {
        return false;
    }

    const QString gateway = m_vpnProtocol->vpnGateway();
    const QString localAddress = m_vpnProtocol->vpnLocalAddress();
    if (gateway.isEmpty() || localAddress.isEmpty()) {
        return false;
    }

    auto iface = IpcClient::Interface();
    if (!iface) {
        return false;
    }

    iface->startNetworkCheck(gateway, localAddress);
    return true;
#else
    return false;
#endif
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

void VpnConnection::disconnectFromVpn()
{
#ifdef AMNEZIA_DESKTOP
    if (InterfaceReady()) {

        m_vpnProtocol.data()->stop();
        qDebug() << "Interface is ready!";

        QRemoteObjectPendingReply<bool> flushDnsResp = IpcClient::Interface()->flushDns();
        flushDnsResp.waitForFinished(1000);

        qDebug() << "Flushed DNS";
        // delete cached routes
        QRemoteObjectPendingReply<bool> clearSavedRoutesResp = IpcClient::Interface()->clearSavedRoutes();
        clearSavedRoutesResp.waitForFinished(1000);
    }
#endif

#ifdef Q_OS_ANDROID
    if (m_vpnProtocol && m_vpnProtocol.data()) {
        auto *const connection = new QMetaObject::Connection;
        *connection = connect(AndroidController::instance(), &AndroidController::vpnStateChanged, this,
                              [this, connection](AndroidController::ConnectionState state) {
                                  if (state == AndroidController::ConnectionState::DISCONNECTED) {
                                      onConnectionStateChanged(Vpn::ConnectionState::Disconnected);
                                      disconnect(*connection);
                                      delete connection;
                                  }
                              });
        m_vpnProtocol.data()->stop();
    }
#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    IosController::Instance()->disconnectVpn();
    disconnect(&m_checkTimer, &QTimer::timeout, IosController::Instance(), &IosController::checkStatus);
#endif

    if (!m_vpnProtocol.data()) {
        emit connectionStateChanged(Vpn::ConnectionState::Disconnected);
        return;
    }

#if !defined(Q_OS_ANDROID) && !defined(AMNEZIA_DESKTOP)
    if (m_vpnProtocol) {
        m_vpnProtocol->deleteLater();
    }
#endif

    m_vpnProtocol = nullptr;
}

Vpn::ConnectionState VpnConnection::connectionState()
{
    if (!m_vpnProtocol)
        return Vpn::ConnectionState::Disconnected;
    return m_vpnProtocol->connectionState();
}

bool VpnConnection::isConnected() const
{
    if (!m_vpnProtocol.data()) {
        return false;
    }

    return m_vpnProtocol.data()->isConnected();
}

bool VpnConnection::isDisconnected() const
{
    if (!m_vpnProtocol.data()) {
        return true;
    }

    return m_vpnProtocol.data()->isDisconnected();
}
