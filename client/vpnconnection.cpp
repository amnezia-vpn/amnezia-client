#include "qtimer.h"
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QHostInfo>
#include <QJsonObject>

#include <configurators/openvpn_configurator.h>
#include <configurators/cloak_configurator.h>
#include <configurators/shadowsocks_configurator.h>
#include <configurators/wireguard_configurator.h>
#include <configurators/vpn_configurator.h>
#include <core/servercontroller.h>

#ifdef AMNEZIA_DESKTOP
#include "ipc.h"
#include "core/ipcclient.h"
#include <protocols/wireguardprotocol.h>
#endif

#ifdef Q_OS_ANDROID
#include "../../platforms/android/android_controller.h"
#endif

#ifdef Q_OS_IOS
#include <protocols/ios_vpnprotocol.h>
#endif

#include "utilities.h"
#include "vpnconnection.h"

VpnConnection::VpnConnection(std::shared_ptr<Settings> settings,
    std::shared_ptr<VpnConfigurator> configurator, QObject* parent) : QObject(parent),
    m_settings(settings),
    m_configurator(configurator)
{
    m_checkTimer.setInterval(1000);
}

VpnConnection::~VpnConnection()
{
    if (m_vpnProtocol != nullptr) {
        m_vpnProtocol->deleteLater();
        m_vpnProtocol.clear();
    }
}

void VpnConnection::onBytesChanged(quint64 receivedBytes, quint64 sentBytes)
{
    emit bytesChanged(receivedBytes, sentBytes);
}

void VpnConnection::onConnectionStateChanged(VpnProtocol::VpnConnectionState state)
{

#ifdef AMNEZIA_DESKTOP
    if (IpcClient::Interface()) {
        if (state == VpnProtocol::Connected){
            IpcClient::Interface()->resetIpStack();
            IpcClient::Interface()->flushDns();

            if (m_settings->routeMode() != Settings::VpnAllSites) {
                IpcClient::Interface()->routeDeleteList(m_vpnProtocol->vpnGateway(), QStringList() << "0.0.0.0");
                //qDebug() << "VpnConnection::onConnectionStateChanged :: adding custom routes, count:" << forwardIps.size();
            }
            QString dns1 = m_vpnConfiguration.value(config_key::dns1).toString();
            QString dns2 = m_vpnConfiguration.value(config_key::dns1).toString();

            IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(),
                QStringList() << dns1 << dns2);


            if (m_settings->routeMode() == Settings::VpnOnlyForwardSites) {
                QTimer::singleShot(1000, m_vpnProtocol.data(), [this](){
                    addSitesRoutes(m_vpnProtocol->vpnGateway(), m_settings->routeMode());
                });
            }
            else if (m_settings->routeMode() == Settings::VpnAllExceptSites) {
                IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << "0.0.0.0/1");
                IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << "128.0.0.0/1");

                IpcClient::Interface()->routeAddList(m_vpnProtocol->routeGateway(), QStringList() << remoteAddress());
                addSitesRoutes(m_vpnProtocol->routeGateway(), m_settings->routeMode());
            }


        }
        else if (state == VpnProtocol::Error) {
            IpcClient::Interface()->flushDns();

            if (m_settings->routeMode() == Settings::VpnOnlyForwardSites) {
                IpcClient::Interface()->clearSavedRoutes();
            }
        }
    }
#endif

#ifdef Q_OS_IOS
    if (state == VpnProtocol::Connected) {
        m_checkTimer.start();
    }
    else {
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
        if (Utils::checkIpSubnetFormat(i.key())) {
            ips.append(i.key());
        }
        else {
            if (Utils::checkIpSubnetFormat(i.value().toString())) {
                ips.append(i.value().toString());
            }
            sites.append(i.key());
        }
    }
    ips.removeDuplicates();

    // add all IPs immediately
    IpcClient::Interface()->routeAddList(gw, ips);

    // re-resolve domains
    for (const QString &site: sites) {
            const auto &cbResolv = [this, site, gw, mode, ips](const QHostInfo &hostInfo){
                const QList<QHostAddress> &addresses = hostInfo.addresses();
                QString ipv4Addr;
                for (const QHostAddress &addr: hostInfo.addresses()) {
                    if (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
                        const QString &ip = addr.toString();
                        //qDebug() << "VpnConnection::addSitesRoutes updating site" << site << ip;
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
    if (connectionState() == VpnProtocol::Connected && IpcClient::Interface()) {
        if (m_settings->routeMode() == Settings::VpnOnlyForwardSites) {
            IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(), ips);
        }
        else if (m_settings->routeMode() == Settings::VpnAllExceptSites) {
            IpcClient::Interface()->routeAddList(m_vpnProtocol->routeGateway(), ips);
        }
    }
#endif
}

void VpnConnection::deleteRoutes(const QStringList &ips)
{
#ifdef AMNEZIA_DESKTOP
    if (connectionState() == VpnProtocol::Connected && IpcClient::Interface()) {
        if (m_settings->routeMode() == Settings::VpnOnlyForwardSites) {
            IpcClient::Interface()->routeDeleteList(vpnProtocol()->vpnGateway(), ips);
        }
        else if (m_settings->routeMode() == Settings::VpnAllExceptSites) {
            IpcClient::Interface()->routeDeleteList(m_vpnProtocol->routeGateway(), ips);
        }
    }
#endif
}

void VpnConnection::flushDns()
{
#ifdef AMNEZIA_DESKTOP
    if (IpcClient::Interface()) IpcClient::Interface()->flushDns();
#endif
}

ErrorCode VpnConnection::lastError() const
{
    if (!m_vpnProtocol.data()) {
        return ErrorCode::InternalError;
    }

    return m_vpnProtocol.data()->lastError();
}

QMap<Proto, QString> VpnConnection::getLastVpnConfig(const QJsonObject &containerConfig)
{
    QMap<Proto, QString> configs;
    for (Proto proto: ProtocolProps::allProtocols()) {

        QString cfg = containerConfig.value(ProtocolProps::protoToString(proto)).toObject().value(config_key::last_config).toString();

        if (!cfg.isEmpty()) configs.insert(proto, cfg);
    }
    return configs;
}

QString VpnConnection::createVpnConfigurationForProto(int serverIndex,
    const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig, Proto proto,
    ErrorCode *errorCode)
{
    QMap<Proto, QString> lastVpnConfig = getLastVpnConfig(containerConfig);

    QString configData;
    if (lastVpnConfig.contains(proto)) {
        configData = lastVpnConfig.value(proto);
        configData = m_configurator->processConfigWithLocalSettings(serverIndex, container, proto, configData);
    }
    else {
        configData = m_configurator->genVpnProtocolConfig(credentials,
            container, containerConfig, proto, errorCode);

        if (errorCode && *errorCode) {
            return "";
        }

        QString configDataBeforeLocalProcessing = configData;

        configData = m_configurator->processConfigWithLocalSettings(serverIndex, container, proto, configData);

        if (serverIndex >= 0) {
            qDebug() << "VpnConnection::createVpnConfiguration: saving config for server #" << serverIndex << container << proto;
            QJsonObject protoObject = m_settings->protocolConfig(serverIndex, container, proto);
            protoObject.insert(config_key::last_config, configDataBeforeLocalProcessing);
            m_settings->setProtocolConfig(serverIndex, container, proto, protoObject);
        }
    }

    return configData;
}

QJsonObject VpnConnection::createVpnConfiguration(int serverIndex,
    const ServerCredentials &credentials, DockerContainer container,
    const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    QJsonObject vpnConfiguration;

    for (ProtocolEnumNS::Proto proto : ContainerProps::protocolsForContainer(container)) {
        QJsonObject vpnConfigData = QJsonDocument::fromJson(
            createVpnConfigurationForProto(
                serverIndex, credentials, container, containerConfig, proto, errorCode).toUtf8()).
                    object();

        if (errorCode && *errorCode) {
            return {};
        }

        vpnConfiguration.insert(ProtocolProps::key_proto_config_data(proto), vpnConfigData);
    }

    Proto proto = ContainerProps::defaultProtocol(container);
    vpnConfiguration[config_key::vpnproto] = ProtocolProps::protoToString(proto);

    auto dns = m_configurator->getDnsForConfig(serverIndex);

    vpnConfiguration[config_key::dns1] = dns.first;
    vpnConfiguration[config_key::dns2] = dns.second;

    return vpnConfiguration;
}

void VpnConnection::connectToVpn(int serverIndex,
    const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig)
{
    qDebug() << QString("ConnectToVpn, Server index is %1, container is %2, route mode is")
                .arg(serverIndex).arg(ContainerProps::containerToString(container)) << m_settings->routeMode();

#if !defined (Q_OS_ANDROID) && !defined (Q_OS_IOS)
    if (!m_IpcClient) {
        m_IpcClient = new IpcClient(this);
    }

    if (!m_IpcClient->isSocketConnected()) {
        if (!IpcClient::init(m_IpcClient)) {
            qWarning() << "Error occurred when init IPC client";
            emit serviceIsNotReady();
            emit connectionStateChanged(VpnProtocol::Error);
            return;
        }
    }
#endif

    m_remoteAddress = credentials.hostName;
    emit connectionStateChanged(VpnProtocol::Connecting);

    if (m_vpnProtocol) {
        disconnect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
        m_vpnProtocol->stop();
        m_vpnProtocol.reset();
    }

    ErrorCode e = ErrorCode::NoError;

    m_vpnConfiguration = createVpnConfiguration(serverIndex, credentials, container, containerConfig, &e);
    if (e) {
        emit connectionStateChanged(VpnProtocol::Error);
        return;
    }
    
#if !defined (Q_OS_ANDROID) && !defined (Q_OS_IOS)
    m_vpnProtocol.reset(VpnProtocol::factory(container, m_vpnConfiguration));
    if (!m_vpnProtocol) {
        emit VpnProtocol::Error;
        return;
    }
    m_vpnProtocol->prepare();
#elif defined Q_OS_ANDROID
    androidVpnProtocol = createDefaultAndroidVpnProtocol(container);
    createAndroidConnections(container);

    m_vpnProtocol.reset(androidVpnProtocol);
#elif defined Q_OS_IOS
    Proto proto = ContainerProps::defaultProtocol(container);
    auto iosVpnProtocol = new IOSVpnProtocol(proto, m_vpnConfiguration);

    if (!iosVpnProtocol->initialize()) {
         qDebug() << QString("Init failed") ;
         emit VpnProtocol::Error;
         iosVpnProtocol->deleteLater();
         return;
    }

    connect(&m_checkTimer, &QTimer::timeout, iosVpnProtocol, &IOSVpnProtocol::checkStatus);
    m_vpnProtocol.reset(iosVpnProtocol);

#endif

    createProtocolConnections();

    e = m_vpnProtocol.data()->start();
    if (e) emit VpnProtocol::Error;
}

void VpnConnection::createProtocolConnections() {
    connect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
    connect(m_vpnProtocol.data(), SIGNAL(connectionStateChanged(VpnProtocol::VpnConnectionState)), this, SLOT(onConnectionStateChanged(VpnProtocol::VpnConnectionState)));
    connect(m_vpnProtocol.data(), SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));
}

#ifdef Q_OS_ANDROID
void VpnConnection::restoreConnection() {
    createAndroidConnections();

    m_vpnProtocol.reset(androidVpnProtocol);

    createProtocolConnections();
}

void VpnConnection::createAndroidConnections()
{
    int serverIndex = m_settings->defaultServerIndex();
    DockerContainer container = m_settings->defaultContainer(serverIndex);

    createAndroidConnections(container);
}

void VpnConnection::createAndroidConnections(DockerContainer container)
{
    androidVpnProtocol = createDefaultAndroidVpnProtocol(container);

    connect(AndroidController::instance(), &AndroidController::connectionStateChanged, androidVpnProtocol, &AndroidVpnProtocol::setConnectionState);
    connect(AndroidController::instance(), &AndroidController::statusUpdated, androidVpnProtocol, &AndroidVpnProtocol::connectionDataUpdated);
}

AndroidVpnProtocol* VpnConnection::createDefaultAndroidVpnProtocol(DockerContainer container)
{
    Proto proto = ContainerProps::defaultProtocol(container);
    AndroidVpnProtocol *androidVpnProtocol = new AndroidVpnProtocol(proto, m_vpnConfiguration);

    return androidVpnProtocol;
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
    if (IpcClient::Interface()) {
        IpcClient::Interface()->flushDns();

        // delete cached routes
        QRemoteObjectPendingReply<bool> response = IpcClient::Interface()->clearSavedRoutes();
        response.waitForFinished(1000);
    }
#endif

    if (!m_vpnProtocol.data()) {
        emit connectionStateChanged(VpnProtocol::Disconnected);
#ifdef Q_OS_ANDROID
        AndroidController::instance()->stop();
#endif
        return;
    }

    if (m_vpnProtocol) {
        m_vpnProtocol->deleteLater();
    }
    m_vpnProtocol = nullptr;
}

VpnProtocol::VpnConnectionState VpnConnection::connectionState()
{
    if (!m_vpnProtocol) return VpnProtocol::Disconnected;
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
