#include <QDebug>
#include <QTimer>

#include "communicator.h"
#include "vpnprotocol.h"
#include "core/errorstrings.h"

//Communicator* VpnProtocol::m_communicator = nullptr;

VpnProtocol::VpnProtocol(const QString& args, QObject* parent)
    : QObject(parent),
      m_connectionState(ConnectionState::Unknown),
      m_timeoutTimer(new QTimer(this)),
      m_receivedBytes(0),
      m_sentBytes(0)
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &VpnProtocol::onTimeout);

    Q_UNUSED(args)
}

//void VpnProtocol::initializeCommunicator(QObject* parent)
//{
//    if (!m_communicator) {
//        m_communicator = new Communicator(parent);
//    }
//}

//Communicator* VpnProtocol::communicator()
//{
//    return m_communicator;
//}

void VpnProtocol::setLastError(ErrorCode lastError)
{
    m_lastError = lastError;
    if (lastError){
        setConnectionState(ConnectionState::Disconnected);
    }
    qCritical().noquote() << "VpnProtocol error, code" << m_lastError << errorString(m_lastError);
}

ErrorCode VpnProtocol::lastError() const
{
    return m_lastError;
}

void VpnProtocol::onTimeout()
{
    qDebug() << "Timeout";

    emit timeoutTimerEvent();
    stop();
}

void VpnProtocol::startTimeoutTimer()
{
    m_timeoutTimer->start(30000);
}

void VpnProtocol::stopTimeoutTimer()
{
    m_timeoutTimer->stop();
}

VpnProtocol::ConnectionState VpnProtocol::connectionState() const
{
    return m_connectionState;
}

void VpnProtocol::setBytesChanged(quint64 receivedBytes, quint64 sentBytes)
{
    emit bytesChanged(receivedBytes - m_receivedBytes, sentBytes - m_sentBytes);

    m_receivedBytes = receivedBytes;
    m_sentBytes = sentBytes;
}

void VpnProtocol::setConnectionState(VpnProtocol::ConnectionState state)
{
    if (m_connectionState == state) {
        return;
    }

    m_connectionState = state;
    if (m_connectionState == ConnectionState::Disconnected) {
        m_receivedBytes = 0;
        m_sentBytes = 0;
    }

    qDebug().noquote() << QString("Connection state: '%1'").arg(textConnectionState());

    emit connectionStateChanged(m_connectionState);
}

QString VpnProtocol::vpnGateway() const
{
    return m_vpnGateway;
}

QString VpnProtocol::routeGateway() const
{
    return m_routeGateway;
}

QString VpnProtocol::textConnectionState(ConnectionState connectionState)
{
    switch (connectionState) {
    case ConnectionState::Unknown: return tr("Unknown");
    case ConnectionState::Disconnected: return tr("Disconnected");
    case ConnectionState::Preparing: return tr("Preparing");
    case ConnectionState::Connecting: return tr("Connecting...");
    case ConnectionState::Connected: return tr("Connected");
    case ConnectionState::Disconnecting: return tr("Disconnecting...");
    case ConnectionState::TunnelReconnecting: return tr("Reconnecting...");
    case ConnectionState::Error: return tr("Error");
    default:
        ;
    }

    return QString();
}

QString VpnProtocol::textConnectionState() const
{
    return textConnectionState(m_connectionState);
}

bool VpnProtocol::onConnected() const
{
    return m_connectionState == ConnectionState::Connected;
}

bool VpnProtocol::onDisconnected() const
{
    return m_connectionState == ConnectionState::Disconnected;
}
