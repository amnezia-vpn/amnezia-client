#include <QDebug>
#include <QTimer>

#include "communicator.h"
#include "vpnprotocol.h"

VpnProtocol::VpnProtocol(const QString& args, QObject* parent)
    : QObject(parent),
      m_connectionState(ConnectionState::Unknown),
      m_communicator(new Communicator),
      m_timeoutTimer(new QTimer(this))
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &VpnProtocol::onTimeout);

    Q_UNUSED(args)
}

VpnProtocol::~VpnProtocol()
{

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
    emit bytesChanged(receivedBytes, sentBytes);
}

void VpnProtocol::setConnectionState(VpnProtocol::ConnectionState state)
{
    if (m_connectionState == state) {
        return;
    }

    m_connectionState = state;
    emit connectionStateChanged(m_connectionState);
}

QString VpnProtocol::textConnectionState(ConnectionState connectionState)
{
    switch (connectionState) {
    case ConnectionState::Unknown: return "Unknown";
    case ConnectionState::Disconnected: return "Disconnected";
    case ConnectionState::Preparing: return "Preparing";
    case ConnectionState::Connecting: return "Connecting";
    case ConnectionState::Connected: return "Connected";
    case ConnectionState::Disconnecting: return "Disconnecting";
    case ConnectionState::TunnelReconnecting: return "TunnelReconnecting";
    case ConnectionState::Error: return "Error";
    default:
        ;
    }

    return QString();
}

QString VpnProtocol::textConnectionState() const
{
    return textConnectionState(m_connectionState);
}

bool VpnProtocol::connected() const
{
    return m_connectionState == ConnectionState::Connected;
}

bool VpnProtocol::disconnected() const
{
    return m_connectionState == ConnectionState::Disconnected;
}
