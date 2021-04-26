#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QJsonObject>

#include <configurators/openvpn_configurator.h>
#include <configurators/cloak_configurator.h>
#include <core/servercontroller.h>

#include "ipc.h"
#include "core/ipcclient.h"
#include "protocols/openvpnprotocol.h"
#include "protocols/openvpnovercloakprotocol.h"
#include "protocols/shadowsocksvpnprotocol.h"

#include "utils.h"
#include "vpnconnection.h"

VpnConnection::VpnConnection(QObject* parent) : QObject(parent)
{
    QTimer::singleShot(0, this, [this](){
        if (!IpcClient::init()) {
            qWarning() << "Error occured when init IPC client";
            emit serviceIsNotReady();
        }
    });
}

VpnConnection::~VpnConnection()
{
    m_vpnProtocol.clear();
}

void VpnConnection::onBytesChanged(quint64 receivedBytes, quint64 sentBytes)
{
    emit bytesChanged(receivedBytes, sentBytes);
}

void VpnConnection::onConnectionStateChanged(VpnProtocol::ConnectionState state)
{
    if (IpcClient::Interface()) {
        if (state == VpnProtocol::ConnectionState::Connected && IpcClient::Interface()){
            IpcClient::Interface()->flushDns();

            if (m_settings.customRouting()) {
                IpcClient::Interface()->routeDelete("0.0.0.0", m_vpnProtocol->vpnGateway());

                IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(),
                    QStringList() << m_settings.primaryDns() << m_settings.secondaryDns());

                const QStringList &black_custom = m_settings.customIps();
                qDebug() << "VpnConnection::onConnectionStateChanged :: adding custom routes, count:" << black_custom.size();

                IpcClient::Interface()->routeAddList(m_vpnProtocol->vpnGateway(), black_custom);
            }
        }
        else if (state == VpnProtocol::ConnectionState::Error) {
            IpcClient::Interface()->flushDns();

            if (m_settings.customRouting()) {
                IpcClient::Interface()->clearSavedRoutes();
            }
        }
    }

    emit connectionStateChanged(state);
}

QSharedPointer<VpnProtocol> VpnConnection::vpnProtocol() const
{
    return m_vpnProtocol;
}

ErrorCode VpnConnection::lastError() const
{
    if (!m_vpnProtocol.data()) {
        return ErrorCode::InternalError;
    }

    return m_vpnProtocol.data()->lastError();
}

ErrorCode VpnConnection::createVpnConfiguration(const ServerCredentials &credentials, DockerContainer container)
{
    ErrorCode errorCode = ErrorCode::NoError;
    if (container == DockerContainer::OpenVpn || container == DockerContainer::ShadowSocksOverOpenVpn || container == DockerContainer::OpenVpnOverCloak) {
        QString openVpnConfigData = OpenVpnConfigurator::genOpenVpnConfig(credentials, container, &errorCode);
        m_vpnConfiguration.insert(config::key_openvpn_config_data, openVpnConfigData);
        if (errorCode) {
            return errorCode;
        }

        QFile file(Utils::defaultVpnConfigFileName());
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
            QTextStream stream(&file);
            stream << openVpnConfigData << endl;
            file.close();
        }
        else {
            return ErrorCode::FailedToSaveConfigData;
        }
    }

    if (container == DockerContainer::ShadowSocksOverOpenVpn) {
        QJsonObject ssConfigData = ShadowSocksVpnProtocol::genShadowSocksConfig(credentials);
        m_vpnConfiguration.insert(config::key_shadowsocks_config_data, ssConfigData);
    }

    if (container == DockerContainer::OpenVpnOverCloak) {
        QJsonObject cloakConfigData = CloakConfigurator::genCloakConfig(credentials, DockerContainer::OpenVpnOverCloak, &errorCode);
        m_vpnConfiguration.insert(config::key_cloak_config_data, cloakConfigData);
    }

    //qDebug().noquote() << "VPN config" << QJsonDocument(m_vpnConfiguration).toJson();
    return ErrorCode::NoError;
}

ErrorCode VpnConnection::connectToVpn(const ServerCredentials &credentials, DockerContainer container)
{
    qDebug() << "connectToVpn, CustomRouting is" << m_settings.customRouting();
//    qDebug() << "Cred" << m_settings.serverCredentials().hostName <<
//                m_settings.serverCredentials().password;
    //protocol = Protocol::ShadowSocks;
    container = DockerContainer::OpenVpnOverCloak;

    // TODO: Try protocols one by one in case of Protocol::Any
    // TODO: Implement some behavior in case if connection not stable
    qDebug() << "Connect to VPN";

    emit connectionStateChanged(VpnProtocol::ConnectionState::Connecting);

    if (m_vpnProtocol) {
        disconnect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
        m_vpnProtocol->stop();
        m_vpnProtocol.reset();
        //m_vpnProtocol->deleteLater();
    }

    //qApp->processEvents();

    if (container == DockerContainer::None || container == DockerContainer::OpenVpn) {
        ErrorCode e = createVpnConfiguration(credentials, DockerContainer::OpenVpn);
        if (e) {
            emit connectionStateChanged(VpnProtocol::ConnectionState::Error);
            return e;
        }

        m_vpnProtocol.reset(new OpenVpnProtocol(m_vpnConfiguration));
        e = static_cast<OpenVpnProtocol *>(m_vpnProtocol.data())->checkAndSetupTapDriver();
        if (e) {
            emit connectionStateChanged(VpnProtocol::ConnectionState::Error);
            return e;
        }
    }
    else if (container == DockerContainer::ShadowSocksOverOpenVpn) {
        ErrorCode e = createVpnConfiguration(credentials, DockerContainer::ShadowSocksOverOpenVpn);
        if (e) {
            emit connectionStateChanged(VpnProtocol::ConnectionState::Error);
            return e;
        }

        m_vpnProtocol.reset(new ShadowSocksVpnProtocol(m_vpnConfiguration));
        e = static_cast<OpenVpnProtocol *>(m_vpnProtocol.data())->checkAndSetupTapDriver();
        if (e) {
            emit connectionStateChanged(VpnProtocol::ConnectionState::Error);
            return e;
        }
    }
    else if (container == DockerContainer::OpenVpnOverCloak) {
        ErrorCode e = createVpnConfiguration(credentials, DockerContainer::OpenVpnOverCloak);
        if (e) {
            emit connectionStateChanged(VpnProtocol::ConnectionState::Error);
            return e;
        }

        m_vpnProtocol.reset(new OpenVpnOverCloakProtocol(m_vpnConfiguration));
        e = static_cast<OpenVpnProtocol *>(m_vpnProtocol.data())->checkAndSetupTapDriver();
        if (e) {
            emit connectionStateChanged(VpnProtocol::ConnectionState::Error);
            return e;
        }
    }

    connect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
    connect(m_vpnProtocol.data(), SIGNAL(connectionStateChanged(VpnProtocol::ConnectionState)), this, SLOT(onConnectionStateChanged(VpnProtocol::ConnectionState)));
    connect(m_vpnProtocol.data(), SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));

    ServerController::disconnectFromHost(credentials);

    return m_vpnProtocol.data()->start();
}

QString VpnConnection::bytesPerSecToText(quint64 bytes)
{
    double mbps = bytes * 8 / 1e6;
    return QString("%1 %2").arg(QString::number(mbps, 'f', 2)).arg(tr("Mbps")); // Mbit/s
}

void VpnConnection::disconnectFromVpn()
{
    qDebug() << "Disconnect from VPN";

    IpcClient::Interface()->flushDns();

    if (m_settings.customRouting()) {
        IpcClient::Interface()->clearSavedRoutes();
    }

    if (!m_vpnProtocol.data()) {
        return;
    }
    m_vpnProtocol.data()->stop();
}

VpnProtocol::ConnectionState VpnConnection::connectionState()
{
    if (!m_vpnProtocol) return VpnProtocol::ConnectionState::Disconnected;
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
