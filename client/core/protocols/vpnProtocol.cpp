#include <QDebug>
#include <QTimer>

#include "core/utils/errorStrings.h"
#include "vpnProtocol.h"

VpnProtocol::VpnProtocol(const QJsonObject &configuration, QObject *parent)
    : QObject(parent),
      m_connectionState(Vpn::ConnectionState::Unknown),
      m_rawConfig(configuration),
      m_timeoutTimer(new QTimer(this)),
      m_lastError(ErrorCode::NoError),
      m_receivedBytes(0),
      m_sentBytes(0)
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &VpnProtocol::onTimeout);
}

bool VpnProtocol::guardStop()
{
    if (m_stopped) {
        return true;
    }
    m_stopped = true;
    return false;
}

void VpnProtocol::resetStopGuard()
{
    m_stopped = false;
}

void VpnProtocol::setLastError(ErrorCode lastError)
{
    m_lastError = lastError;
    if (lastError) {
        setConnectionState(Vpn::ConnectionState::Error);
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

Vpn::ConnectionState VpnProtocol::connectionState() const
{
    return m_connectionState;
}

void VpnProtocol::setBytesChanged(quint64 receivedBytes, quint64 sentBytes)
{
    quint64 rxDiff = receivedBytes - m_receivedBytes;
    quint64 txDiff = sentBytes - m_sentBytes;

    emit bytesChanged(rxDiff, txDiff);

    m_receivedBytes = receivedBytes;
    m_sentBytes = sentBytes;
}

void VpnProtocol::setConnectionState(Vpn::ConnectionState state)
{
    qDebug() << "VpnProtocol::setConnectionState" << textConnectionState(state);

    if (m_connectionState == state) {
        return;
    }
    if (m_connectionState == Vpn::ConnectionState::Disconnected && state == Vpn::ConnectionState::Disconnecting) {
        return;
    }

    m_connectionState = state;
    if (m_connectionState == Vpn::ConnectionState::Disconnected) {
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

QString VpnProtocol::vpnLocalAddress() const
{
    return m_vpnLocalAddress;
}

QString VpnProtocol::routeGateway() const
{
    return m_routeGateway;
}

QString VpnProtocol::textConnectionState(Vpn::ConnectionState connectionState)
{
    switch (connectionState) {
    case Vpn::ConnectionState::Unknown: return tr("Unknown");
    case Vpn::ConnectionState::Disconnected: return tr("Disconnected");
    case Vpn::ConnectionState::Preparing: return tr("Preparing");
    case Vpn::ConnectionState::Connecting: return tr("Connecting...");
    case Vpn::ConnectionState::Connected: return tr("Connected");
    case Vpn::ConnectionState::Disconnecting: return tr("Disconnecting...");
    case Vpn::ConnectionState::Reconnecting: return tr("Reconnecting...");
    case Vpn::ConnectionState::Error: return tr("Error");
    default:;
    }

    return QString();
}

QString VpnProtocol::textConnectionState() const
{
    return textConnectionState(m_connectionState);
}

bool VpnProtocol::isConnected() const
{
    return m_connectionState == Vpn::ConnectionState::Connected;
}

bool VpnProtocol::isDisconnected() const
{
    return m_connectionState == Vpn::ConnectionState::Disconnected;
}
