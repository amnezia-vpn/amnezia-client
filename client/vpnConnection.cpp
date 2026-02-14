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

#include "core/utils/networkUtilities.h"
#include "vpnConnection.h"

using namespace ProtocolUtils;

VpnConnection::VpnConnection(SecureServersRepository* serversRepository, SecureAppSettingsRepository* appSettingsRepository, QObject *parent)
    : QObject(parent), m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository), m_checkTimer(new QTimer(this))
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
    IpcClient::withInterface([enabled](QSharedPointer<IpcInterfaceReplica> iface){
        QRemoteObjectPendingReply<bool> reply = iface->refreshKillSwitch(enabled);
        if (reply.waitForFinished(1000) && reply.returnValue())
            qDebug() << "VpnConnection::onKillSwitchModeChanged: Killswitch refreshed";
        else
            qWarning() << "VpnConnection::onKillSwitchModeChanged: Failed to execute remote refreshKillSwitch call";
    });
#endif
}

void VpnConnection::onConnectionStateChanged(Vpn::ConnectionState state)
{
#ifdef AMNEZIA_DESKTOP
    ServerConfig defaultServer = m_serversRepository->server(m_serversRepository->defaultServerIndex());
    DockerContainer container = defaultServer.defaultContainer();

    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        if (state == Vpn::ConnectionState::Connected) {
            iface->resetIpStack();
            iface->flushDns();

            if (!ContainerUtils::isAwgContainer(container) && 
                container != DockerContainer::WireGuard) {
                QString dns1 = m_vpnConfiguration.value(config_key::dns1).toString();
                QString dns2 = m_vpnConfiguration.value(config_key::dns2).toString();

                iface->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << dns1 << dns2);

                if (m_appSettingsRepository->isSitesSplitTunnelingEnabled()) {
                    iface->routeDeleteList(m_vpnProtocol->vpnGateway(), QStringList() << "0.0.0.0");
                    // qDebug() << "VpnConnection::onConnectionStateChanged :: adding custom routes, count:" << forwardIps.size();
                    RouteMode routeMode = m_appSettingsRepository->routeMode();
                    if (routeMode == amnezia::RouteMode::VpnOnlyForwardSites) {
                        QTimer::singleShot(1000, m_vpnProtocol.data(),
                                           [this, routeMode]() { addSitesRoutes(m_vpnProtocol->vpnGateway(), routeMode); });
                    } else if (routeMode == amnezia::RouteMode::VpnAllExceptSites) {
                        iface->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << "0.0.0.0/1");
                        iface->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << "128.0.0.0/1");

                        iface->routeAddList(m_vpnProtocol->routeGateway(), QStringList() << remoteAddress());
                        addSitesRoutes(m_vpnProtocol->routeGateway(), routeMode);
                    }
                }
            }
        } else if (state == Vpn::ConnectionState::Error) {
            iface->flushDns();

            if (m_appSettingsRepository->isSitesSplitTunnelingEnabled()) {
                if (m_appSettingsRepository->routeMode() == amnezia::RouteMode::VpnOnlyForwardSites) {
                    iface->clearSavedRoutes();
                }
            }
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
    emit connectionStateChanged(state);
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
    QStringList ips;
    QStringList sites;
    const QVariantMap &m = m_appSettingsRepository->vpnSites(mode);
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

    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        iface->routeAddList(gw, ips);
    });

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
                        IpcClient::withInterface([&gw, &ip](QSharedPointer<IpcInterfaceReplica> iface) {
                            iface->routeAddList(gw, QStringList() << ip);
                        });
                        m_appSettingsRepository->addVpnSite(mode, site, ip);
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
    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        if (connectionState() == Vpn::ConnectionState::Connected) {
            RouteMode routeMode = m_appSettingsRepository->routeMode();
            if (routeMode == amnezia::RouteMode::VpnOnlyForwardSites) {
                iface->routeAddList(m_vpnProtocol->vpnGateway(), ips);
            } else if (routeMode == amnezia::RouteMode::VpnAllExceptSites) {
                iface->routeAddList(m_vpnProtocol->routeGateway(), ips);
            }
        }
    });
#endif
}

void VpnConnection::deleteRoutes(const QStringList &ips)
{
#ifdef AMNEZIA_DESKTOP
    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        if (connectionState() == Vpn::ConnectionState::Connected) {
            RouteMode routeMode = m_appSettingsRepository->routeMode();
            if (routeMode == amnezia::RouteMode::VpnOnlyForwardSites) {
                iface->routeDeleteList(vpnProtocol()->vpnGateway(), ips);
            } else if (routeMode == amnezia::RouteMode::VpnAllExceptSites) {
                iface->routeDeleteList(m_vpnProtocol->routeGateway(), ips);
            }
        }
    });
#endif
}

void VpnConnection::flushDns()
{
#ifdef AMNEZIA_DESKTOP
    IpcClient::withInterface([](QSharedPointer<IpcInterfaceReplica> iface) {
        auto reply = iface->flushDns();
        if (reply.waitForFinished(1000) || !reply.returnValue()) {
            qWarning() << "VpnConnection::flushDns(): Failed to flush DNS";
        }
    });
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

    if (m_vpnProtocol.isNull()) {
        return ErrorCode::InternalError;
    }

    return m_vpnProtocol.data()->lastError();
}

void VpnConnection::connectToVpn(int serverIndex, const ServerCredentials &credentials, DockerContainer container,
                                 const QJsonObject &vpnConfiguration)
{
    qDebug() << QString("Trying to connect to VPN, server index is %1, container is %2, route mode is")
                        .arg(serverIndex)
                        .arg(ContainerUtils::containerToString(container))
             << m_appSettingsRepository->routeMode();

    m_remoteAddress = NetworkUtilities::getIPAddress(credentials.hostName);
    emit connectionStateChanged(Vpn::ConnectionState::Connecting);

    m_vpnConfiguration = vpnConfiguration;

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
    Proto proto = ContainerUtils::defaultProtocol(container);
    IosController::Instance()->connectVpn(proto, m_vpnConfiguration);
    connect(&m_checkTimer, &QTimer::timeout, IosController::Instance(), &IosController::checkStatus);
    return;
#endif

    createProtocolConnections();

    ErrorCode errorCode = m_vpnProtocol->start();
    if (errorCode != ErrorCode::NoError)
        emit connectionStateChanged(Vpn::ConnectionState::Error);
}

void VpnConnection::createProtocolConnections()
{
    connect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
    connect(m_vpnProtocol.data(), SIGNAL(connectionStateChanged(Vpn::ConnectionState)), this,
            SLOT(onConnectionStateChanged(Vpn::ConnectionState)));
    connect(m_vpnProtocol.data(), SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));
}

void VpnConnection::appendKillSwitchConfig()
{
    m_vpnConfiguration.insert(config_key::killSwitchOption, QVariant(m_appSettingsRepository->isKillSwitchEnabled()).toString());
    m_vpnConfiguration.insert(config_key::allowedDnsServers, QVariant(m_appSettingsRepository->getAllowedDnsServers()).toJsonValue());
}

void VpnConnection::appendSplitTunnelingConfig()
{
    bool allowSiteBasedSplitTunneling = true;

    // this block is for old native configs and for old self-hosted configs
    auto protocolName = m_vpnConfiguration.value(config_key::vpnproto).toString();
    if (protocolName == ProtocolUtils::protoToString(Proto::Awg) || protocolName == ProtocolUtils::protoToString(Proto::WireGuard)) {
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
                sitesJsonArray.append(m_vpnConfiguration.value(config_key::dns1).toString());
                sitesJsonArray.append(m_vpnConfiguration.value(config_key::dns2).toString());
            }
        }
    }

    m_vpnConfiguration.insert(config_key::splitTunnelType, routeMode);
    m_vpnConfiguration.insert(config_key::splitTunnelSites, sitesJsonArray);

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

    m_vpnConfiguration.insert(config_key::appSplitTunnelType, appsRouteMode);
    m_vpnConfiguration.insert(config_key::splitTunnelApps, appsJsonArray);

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

void VpnConnection::disconnectFromVpn()
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    // iOS/macOS NE use IosController directly; m_vpnProtocol is not set there.
    IosController::Instance()->disconnectVpn();
    disconnect(&m_checkTimer, &QTimer::timeout, IosController::Instance(), &IosController::checkStatus);
#endif

    if (m_vpnProtocol.isNull()) {
        emit connectionStateChanged(Vpn::ConnectionState::Disconnected);
        return;
    }

    m_vpnProtocol->stop();

#ifdef AMNEZIA_DESKTOP
    IpcClient::withInterface([](QSharedPointer<IpcInterfaceReplica> iface) {
        QRemoteObjectPendingReply<bool> flushReply = iface->flushDns();
        if (flushReply.waitForFinished(5000) && flushReply.returnValue())
            qDebug() << "VpnConnection::disconnectFromVpn(): Successfully flushed DNS";
        else
            qWarning() << "VpnConnection::disconnectFromVpn(): Failed to flush DNS";

        QRemoteObjectPendingReply<bool> clearSavedRoutesReply = iface->clearSavedRoutes();
        if (clearSavedRoutesReply.waitForFinished(5000) && clearSavedRoutesReply.returnValue())
            qDebug() << "VpnConnection::disconnectFromVpn(): Successfully cleared saved routes";
        else
            qWarning() << "VpnConnection::disconnectFromVpn(): Failed to clear saved routes";
    });
#endif

#ifdef Q_OS_ANDROID
    auto *const connection = new QMetaObject::Connection;
    *connection = connect(AndroidController::instance(), &AndroidController::vpnStateChanged, this,
                          [this, connection](AndroidController::ConnectionState state) {
                              if (state == AndroidController::ConnectionState::DISCONNECTED) {
                                  onConnectionStateChanged(Vpn::ConnectionState::Disconnected);
                                  disconnect(*connection);
                                  delete connection;
                              }
                          });
    m_vpnProtocol->stop();
#endif

#if !defined(Q_OS_ANDROID) && !defined(AMNEZIA_DESKTOP)
    m_vpnProtocol->deleteLater();
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
    if (m_vpnProtocol.isNull()) {
        return false;
    }

    return m_vpnProtocol->isConnected();
}

bool VpnConnection::isDisconnected() const
{
    if (m_vpnProtocol.isNull()) {
        return true;
    }

    return m_vpnProtocol->isDisconnected();
}
