#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QJsonObject>

#include <configurators/openvpn_configurator.h>
#include <configurators/cloak_configurator.h>
#include <configurators/shadowsocks_configurator.h>
#include <configurators/wireguard_configurator.h>
#include <configurators/vpn_configurator.h>
#include <core/servercontroller.h>
#include <protocols/wireguardprotocol.h>

#ifdef Q_OS_ANDROID
#include "android_controller.h"
#include "protocols/android_vpnprotocol.h"
#endif

#ifdef Q_OS_IOS
#include <protocols/ios_vpnprotocol.h>
#endif

#include "ipc.h"
#include "core/ipcclient.h"

#include "utils.h"
#include "vpnconnection.h"

VpnConnection::VpnConnection(QObject* parent) : QObject(parent),
    m_settings(this)

{
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
    if (IpcClient::Interface()) {
        if (state == VpnProtocol::Connected){
            IpcClient::Interface()->resetIpStack();
            IpcClient::Interface()->flushDns();

            if (m_settings.routeMode() != Settings::VpnAllSites) {
                IpcClient::Interface()->routeDeleteList(m_vpnProtocol->vpnGateway(), QStringList() << "0.0.0.0");
                //qDebug() << "VpnConnection::onConnectionStateChanged :: adding custom routes, count:" << forwardIps.size();
            }
            IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(),
                QStringList() << m_settings.primaryDns() << m_settings.secondaryDns());


            if (m_settings.routeMode() == Settings::VpnOnlyForwardSites) {
                IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(), m_settings.getVpnIps(Settings::VpnOnlyForwardSites));
            }
            else if (m_settings.routeMode() == Settings::VpnAllExceptSites) {
                IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << "0.0.0.0/1");
                IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << "128.0.0.0/1");

                IpcClient::Interface()->routeAddList(m_vpnProtocol->routeGateway(), QStringList() << remoteAddress());
                IpcClient::Interface()->routeAddList(m_vpnProtocol->routeGateway(), m_settings.getVpnIps(Settings::VpnAllExceptSites));
            }

        }
        else if (state == VpnProtocol::Error) {
            IpcClient::Interface()->flushDns();

            if (m_settings.routeMode() == Settings::VpnOnlyForwardSites) {
                IpcClient::Interface()->clearSavedRoutes();
            }
        }
    }

    emit connectionStateChanged(state);
}

const QString &VpnConnection::remoteAddress() const
{
    return m_remoteAddress;
}

QSharedPointer<VpnProtocol> VpnConnection::vpnProtocol() const
{
    return m_vpnProtocol;
}

void VpnConnection::addRoutes(const QStringList &ips)
{
    if (connectionState() == VpnProtocol::Connected && IpcClient::Interface()) {
        if (m_settings.routeMode() == Settings::VpnOnlyForwardSites) {
            IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(), ips);
        }
        else if (m_settings.routeMode() == Settings::VpnAllExceptSites) {
            IpcClient::Interface()->routeAddList(m_vpnProtocol->routeGateway(), ips);
        }
    }
}

void VpnConnection::deleteRoutes(const QStringList &ips)
{
    if (connectionState() == VpnProtocol::Connected && IpcClient::Interface()) {
        if (m_settings.routeMode() == Settings::VpnOnlyForwardSites) {
            IpcClient::Interface()->routeDeleteList(vpnProtocol()->vpnGateway(), ips);
        }
        else if (m_settings.routeMode() == Settings::VpnAllExceptSites) {
            IpcClient::Interface()->routeDeleteList(m_vpnProtocol->routeGateway(), ips);
        }
    }
}

void VpnConnection::flushDns()
{
    if (IpcClient::Interface()) IpcClient::Interface()->flushDns();
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
    ErrorCode e = ErrorCode::NoError;
    auto lastVpnConfig = getLastVpnConfig(containerConfig);

    QString configData;
    if (lastVpnConfig.contains(proto)) {
        configData = lastVpnConfig.value(proto);
        configData = VpnConfigurator::processConfigWithLocalSettings(container, proto, configData);

        qDebug() << "VpnConnection::createVpnConfiguration: using saved config for" << ProtocolProps::protoToString(proto);
    }
    else {
        qDebug() << "VpnConnection::createVpnConfiguration: gen new config for" << ProtocolProps::protoToString(proto);
        configData = VpnConfigurator::genVpnProtocolConfig(credentials,
            container, containerConfig, proto, &e);

        QString configDataBeforeLocalProcessing = configData;

        configData = VpnConfigurator::processConfigWithLocalSettings(container, proto, configData);


        if (errorCode && e) {
            *errorCode = e;
            return "";
        }


        if (serverIndex >= 0) {
            qDebug() << "VpnConnection::createVpnConfiguration: saving config for server #" << serverIndex << container << proto;
            QJsonObject protoObject = m_settings.protocolConfig(serverIndex, container, proto);
            protoObject.insert(config_key::last_config, configDataBeforeLocalProcessing);
            m_settings.setProtocolConfig(serverIndex, container, proto, protoObject);
        }
    }

    if (errorCode) *errorCode = e;
    return configData;
}

QJsonObject VpnConnection::createVpnConfiguration(int serverIndex,
    const ServerCredentials &credentials, DockerContainer container,
    const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    ErrorCode e = ErrorCode::NoError;
    QJsonObject vpnConfiguration;


    for (ProtocolEnumNS::Proto proto : ContainerProps::protocolsForContainer(container)) {
        QJsonObject vpnConfigData = QJsonDocument::fromJson(
            createVpnConfigurationForProto(
                serverIndex, credentials, container, containerConfig, proto, &e).toUtf8()).
                    object();

        if (e) {
            if (errorCode) *errorCode = e;
            return {};
        }

        vpnConfiguration.insert(ProtocolProps::key_proto_config_data(proto), vpnConfigData);
    }

    Proto proto = ContainerProps::defaultProtocol(container);
    vpnConfiguration[config_key::vpnproto] = ProtocolProps::protoToString(proto);

    return vpnConfiguration;
}

void VpnConnection::connectToVpn(int serverIndex,
    const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig)
{
    qDebug() << QString("СonnectToVpn, Server index is %1, container is %2, route mode is")
                .arg(serverIndex).arg(ContainerProps::containerToString(container)) << m_settings.routeMode();

#if !defined (Q_OS_ANDROID) && !defined (Q_OS_IOS)
    if (!m_IpcClient) {
        m_IpcClient = new IpcClient(this);
    }

    if (!m_IpcClient->isSocketConnected()) {
        if (!IpcClient::init(m_IpcClient)) {
            qWarning() << "Error occured when init IPC client";
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
        //qDebug() << "VpnConnection::connectToVpn 1";
        m_vpnProtocol->stop();
        //qDebug() << "VpnConnection::connectToVpn 2";
        m_vpnProtocol.reset();
        //qDebug() << "VpnConnection::connectToVpn 3";
    }

    ErrorCode e = ErrorCode::NoError;

    m_vpnConfiguration = createVpnConfiguration(serverIndex, credentials, container, containerConfig);
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
    Proto proto = ContainerProps::defaultProtocol(container);
    AndroidVpnProtocol *androidVpnProtocol = new AndroidVpnProtocol(proto, m_vpnConfiguration);
    connect(AndroidController::instance(), &AndroidController::connectionStateChanged, androidVpnProtocol, &AndroidVpnProtocol::setConnectionState);

    m_vpnProtocol.reset(androidVpnProtocol);
#elif defined Q_OS_IOS
    Proto proto = ContainerProps::defaultProtocol(container);
    IOSVpnProtocol *iosVpnProtocol = new IOSVpnProtocol(proto, m_vpnConfiguration);
    if (!iosVpnProtocol->initialize()) {
         qDebug() << QString("Init failed") ;
         emit VpnProtocol::Error;
         return;
    }
    m_vpnProtocol.reset(iosVpnProtocol);
#endif

    connect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
    connect(m_vpnProtocol.data(), SIGNAL(connectionStateChanged(VpnProtocol::VpnConnectionState)), this, SLOT(onConnectionStateChanged(VpnProtocol::VpnConnectionState)));
    connect(m_vpnProtocol.data(), SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));

    ServerController::disconnectFromHost(credentials);

    e = m_vpnProtocol.data()->start();
    if (e) emit VpnProtocol::Error;
}

QString VpnConnection::bytesPerSecToText(quint64 bytes)
{
    double mbps = bytes * 8 / 1e6;
    return QString("%1 %2").arg(QString::number(mbps, 'f', 2)).arg(tr("Mbps")); // Mbit/s
}

void VpnConnection::disconnectFromVpn()
{
    // qDebug() << "Disconnect from VPN 1";

    if (IpcClient::Interface()) {
        IpcClient::Interface()->flushDns();

        // delete cached routes
        QRemoteObjectPendingReply<bool> response = IpcClient::Interface()->clearSavedRoutes();
        response.waitForFinished(1000);
    }

    if (!m_vpnProtocol.data()) {
        return;
    }
    m_vpnProtocol.data()->stop();
    // qDebug() << "Disconnect from VPN 2";
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
