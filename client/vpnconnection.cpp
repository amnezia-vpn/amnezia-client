#include <QApplication>
#include <QDebug>
#include <QFile>

#include <core/openvpnconfigurator.h>
#include <core/servercontroller.h>

#include "ipc.h"
#include "protocols/openvpnprotocol.h"
#include "protocols/shadowsocksvpnprotocol.h"
#include "utils.h"
#include "vpnconnection.h"
#include "communicator.h"

VpnConnection::VpnConnection(QObject* parent) : QObject(parent)
{
    //VpnProtocol::initializeCommunicator(parent);
}

void VpnConnection::onBytesChanged(quint64 receivedBytes, quint64 sentBytes)
{
    emit bytesChanged(receivedBytes, sentBytes);
}

void VpnConnection::onConnectionStateChanged(VpnProtocol::ConnectionState state)
{
//    if (state == VpnProtocol::ConnectionState::Connected){
//        m_vpnProtocol->communicator()->sendMessage(Message(Message::State::FlushDnsRequest, QStringList()));

//        // add routes
//        const QStringList &black_custom = m_settings.customIps();
//        qDebug() << "onConnect :: adding custom black routes, count:" << black_custom.size();


//        QStringList args;
//        args << m_vpnProtocol->vpnGateway();
//        args << black_custom;

//        Message m(Message::State::RoutesAddRequest, args);
//        m_vpnProtocol->communicator()->sendMessage(m);
//    }
//    else if (state == VpnProtocol::ConnectionState::Error) {
//        m_vpnProtocol->communicator()->sendMessage(Message(Message::State::ClearSavedRoutesRequest, QStringList()));
//        m_vpnProtocol->communicator()->sendMessage(Message(Message::State::FlushDnsRequest, QStringList()));
//    }
    emit connectionStateChanged(state);
}

ErrorCode VpnConnection::lastError() const
{
    if (!m_vpnProtocol.data()) {
        return ErrorCode::InternalError;
    }

    return m_vpnProtocol.data()->lastError();
}

ErrorCode VpnConnection::requestVpnConfig(const ServerCredentials &credentials, Protocol protocol)
{
    ErrorCode errorCode = ErrorCode::NoError;
    if (protocol == Protocol::OpenVpn || protocol == Protocol::ShadowSocks) {
        QString configData = OpenVpnConfigurator::genOpenVpnConfig(credentials, protocol, &errorCode);
        if (errorCode) {
            return errorCode;
        }

        QFile file(Utils::defaultVpnConfigFileName());
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
            QTextStream stream(&file);
            stream << configData << endl;
            return ErrorCode::NoError;
        }

        return ErrorCode::FailedToSaveConfigData;
    }
    else {
        return ErrorCode::NotImplementedError;
    }
    return ErrorCode::NotImplementedError;

}

ErrorCode VpnConnection::connectToVpn(const ServerCredentials &credentials, Protocol protocol)
{
    // protocol = Protocol::ShadowSocks;

    // TODO: Try protocols one by one in case of Protocol::Any
    // TODO: Implement some behavior in case if connection not stable
    qDebug() << "Connect to VPN";

    emit connectionStateChanged(VpnProtocol::ConnectionState::Connecting);
    qApp->processEvents();

    if (protocol == Protocol::Any || protocol == Protocol::OpenVpn) {
        ErrorCode e = requestVpnConfig(credentials, Protocol::OpenVpn);
        if (e) {
            emit connectionStateChanged(VpnProtocol::ConnectionState::Error);
            return e;
        }
        if (m_vpnProtocol) {
            disconnect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
        }
        m_vpnProtocol.reset(new OpenVpnProtocol());
        connect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
    }
    else if (protocol == Protocol::ShadowSocks) {
        ErrorCode e = requestVpnConfig(credentials, Protocol::ShadowSocks);
        if (e) {
            emit connectionStateChanged(VpnProtocol::ConnectionState::Error);
            return e;
        }
        if (m_vpnProtocol) {
            disconnect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
        }

        m_vpnProtocol.reset(new ShadowSocksVpnProtocol(ShadowSocksVpnProtocol::genShadowSocksConfig(credentials)));
        connect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);

    }

    connect(m_vpnProtocol.data(), SIGNAL(connectionStateChanged(VpnProtocol::ConnectionState)), this, SLOT(onConnectionStateChanged(VpnProtocol::ConnectionState)));
    connect(m_vpnProtocol.data(), SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));

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

//    m_vpnProtocol->communicator()->sendMessage(Message(Message::State::ClearSavedRoutesRequest, QStringList()));
//    m_vpnProtocol->communicator()->sendMessage(Message(Message::State::FlushDnsRequest, QStringList()));

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

bool VpnConnection::onConnected() const
{
    if (!m_vpnProtocol.data()) {
        return false;
    }

    return m_vpnProtocol.data()->onConnected();
}

bool VpnConnection::onDisconnected() const
{
    if (!m_vpnProtocol.data()) {
        return true;
    }

    return m_vpnProtocol.data()->onDisconnected();
}
