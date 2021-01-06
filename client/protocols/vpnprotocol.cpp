#include <QDebug>
#include <QTimer>

#include "communicator.h"
#include "vpnprotocol.h"

Communicator* VpnProtocol::m_communicator = nullptr;

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

void VpnProtocol::initializeCommunicator(QObject* parent)
{
    if (!m_communicator) {
        m_communicator = new Communicator(parent);
    }
}

Communicator* VpnProtocol::communicator()
{
    return m_communicator;
}

void VpnProtocol::setLastError(ErrorCode lastError)
{
    m_lastError = lastError;
    qCritical().noquote() << m_lastError;
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
