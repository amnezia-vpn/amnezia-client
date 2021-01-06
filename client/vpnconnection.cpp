#include <QDebug>
#include <QFile>

#include <core/openvpnconfigurator.h>
#include <core/servercontroller.h>

#include "protocols/openvpnprotocol.h"
#include "utils.h"
#include "vpnconnection.h"

VpnConnection::VpnConnection(QObject* parent) : QObject(parent)
{
    VpnProtocol::initializeCommunicator(parent);
}

void VpnConnection::onBytesChanged(quint64 receivedBytes, quint64 sentBytes)
{
    emit bytesChanged(receivedBytes, sentBytes);
}

void VpnConnection::onConnectionStateChanged(VpnProtocol::ConnectionState state)
{
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
    if (protocol == Protocol::OpenVpn) {
        QString configData = OpenVpnConfigurator::genOpenVpnConfig(credentials, &errorCode);
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
    else if (protocol == Protocol::ShadowSocks) {
        // Request OpenVPN config and ShadowSocks
        return ErrorCode::NotImplementedError;
    }
    return ErrorCode::NotImplementedError;

}

ErrorCode VpnConnection::connectToVpn(const ServerCredentials &credentials, Protocol protocol)
{
    // TODO: Try protocols one by one in case of Protocol::Any
    // TODO: Implement some behavior in case if connection not stable
    qDebug() << "Connect to VPN";

    if (protocol == Protocol::Any || protocol == Protocol::OpenVpn) {
        ErrorCode e = requestVpnConfig(credentials, Protocol::OpenVpn);
        if (e) {
            return e;
        }
        m_vpnProtocol.reset(new OpenVpnProtocol());
    }
    else if (protocol == Protocol::ShadowSocks) {
        return ErrorCode::NotImplementedError;
    }

    connect(m_vpnProtocol.data(), SIGNAL(connectionStateChanged(VpnProtocol::ConnectionState)), this, SLOT(onConnectionStateChanged(VpnProtocol::ConnectionState)));
    connect(m_vpnProtocol.data(), SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));

    return m_vpnProtocol.data()->start();
}

QString VpnConnection::bytesToText(quint64 bytes)
{
    return QString("%1 %2").arg(bytes / 1000000).arg(tr("Mbps"));
}

void VpnConnection::disconnectFromVpn()
{
    qDebug() << "Disconnect from VPN";

    if (!m_vpnProtocol.data()) {
        return;
    }
    m_vpnProtocol.data()->stop();
}

bool VpnConnection::connected() const
{
    if (!m_vpnProtocol.data()) {
        return false;
    }

    return m_vpnProtocol.data()->connected();
}

bool VpnConnection::disconnected() const
{
    if (!m_vpnProtocol.data()) {
        return true;
    }

    return m_vpnProtocol.data()->disconnected();
}
