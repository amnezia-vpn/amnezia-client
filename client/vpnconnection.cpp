#include "qtimer.h"

#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QHostInfo>
#include <QJsonObject>

#include "core/controllers/serverController.h"
#include <configurators/cloak_configurator.h>
#include <configurators/openvpn_configurator.h>
#include <configurators/shadowsocks_configurator.h>
#include <configurators/wireguard_configurator.h>

#ifdef AMNEZIA_DESKTOP
    #include "core/ipcclient.h"
    #include "ipc.h"
    #include <protocols/wireguardprotocol.h>
#endif

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif

#ifdef Q_OS_IOS
    #include "platforms/ios/ios_controller.h"
#endif

#include "core/networkUtilities.h"
#include "vpnconnection.h"

VpnConnection::VpnConnection(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_settings(settings), m_checkTimer(new QTimer(this))
{
    m_checkTimer.setInterval(1000);
#ifdef Q_OS_IOS
    connect(IosController::Instance(), &IosController::connectionStateChanged, this, &VpnConnection::onConnectionStateChanged);
    connect(IosController::Instance(), &IosController::bytesChanged, this, &VpnConnection::onBytesChanged);

#endif
}

VpnConnection::~VpnConnection()
{
#if defined AMNEZIA_DESKTOP
    disconnectFromVpn();
#endif
}

void VpnConnection::onBytesChanged(quint64 receivedBytes, quint64 sentBytes)
{
    emit bytesChanged(receivedBytes, sentBytes);
}

void VpnConnection::onConnectionStateChanged(Vpn::ConnectionState state)
{

#ifdef AMNEZIA_DESKTOP
    auto container = m_settings->defaultContainer(m_settings->defaultServerIndex());

    if (IpcClient::Interface()) {
        if (state == Vpn::ConnectionState::Connected) {
            IpcClient::Interface()->resetIpStack();
            IpcClient::Interface()->flushDns();

            if (!m_vpnConfiguration.value(config_key::configVersion).toInt() && container != DockerContainer::Awg
                && container != DockerContainer::WireGuard) {
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

        } else if (state == Vpn::ConnectionState::Error) {
            IpcClient::Interface()->flushDns();

            if (m_settings->isSitesSplitTunnelingEnabled()) {
                if (m_settings->routeMode() == Settings::VpnOnlyForwardSites) {
                    IpcClient::Interface()->clearSavedRoutes();
                }
            }
        } else if (state == Vpn::ConnectionState::Connecting) {

        } else if (state == Vpn::ConnectionState::Disconnected) {
        }
    }
#endif

#ifdef Q_OS_IOS
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

void VpnConnection::flushDns()
{
#ifdef AMNEZIA_DESKTOP
    if (IpcClient::Interface())
        IpcClient::Interface()->flushDns();
#endif
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
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    if (!m_IpcClient) {
        m_IpcClient = new IpcClient(this);
    }

    if (!m_IpcClient->isSocketConnected()) {
        if (!IpcClient::init(m_IpcClient)) {
            qWarning() << "Error occurred when init IPC client";
            emit serviceIsNotReady();
            emit connectionStateChanged(Vpn::ConnectionState::Error);
            return;
        }
    }
#endif

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

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
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
#elif defined Q_OS_IOS
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

void VpnConnection::createProtocolConnections()
{
    connect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
    connect(m_vpnProtocol.data(), SIGNAL(connectionStateChanged(Vpn::ConnectionState)), this,
            SLOT(onConnectionStateChanged(Vpn::ConnectionState)));
    connect(m_vpnProtocol.data(), SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));
}

void VpnConnection::appendKillSwitchConfig()
{
    m_vpnConfiguration.insert(config_key::killSwitchOption, QVariant(m_settings->isKillSwitchEnabled()).toString());
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

            // Allow traffic to Amnezia DNS
            if (routeMode == Settings::VpnOnlyForwardSites) {
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
    }

    m_vpnConfiguration.insert(config_key::appSplitTunnelType, appsRouteMode);
    m_vpnConfiguration.insert(config_key::splitTunnelApps, appsJsonArray);
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
    QString proto = m_settings->defaultContainerName(m_settings->defaultServerIndex());
    if (IpcClient::Interface()) {
        IpcClient::Interface()->flushDns();

        // delete cached routes
        QRemoteObjectPendingReply<bool> response = IpcClient::Interface()->clearSavedRoutes();
        response.waitForFinished(1000);
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

#ifdef Q_OS_IOS
    IosController::Instance()->disconnectVpn();
    disconnect(&m_checkTimer, &QTimer::timeout, IosController::Instance(), &IosController::checkStatus);
#endif

    if (!m_vpnProtocol.data()) {
        emit connectionStateChanged(Vpn::ConnectionState::Disconnected);
        return;
    }

#ifndef Q_OS_ANDROID
    if (m_vpnProtocol) {
        m_vpnProtocol->deleteLater();
    }
    m_vpnProtocol = nullptr;
#endif
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
