#include <QDebug>
#include <QTimer>

#include "vpnprotocol.h"
#include "core/errorstrings.h"
#include "protocols/protocols_defs.h"

VpnProtocol::VpnProtocol(const QJsonObject &configuration, QObject* parent)
    : QObject(parent),
      m_connectionState(ConnectionState::Unknown),
      m_rawConfig(configuration),
      m_timeoutTimer(new QTimer(this)),
      m_receivedBytes(0),
      m_sentBytes(0)
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &VpnProtocol::onTimeout);
}

void VpnProtocol::setLastError(ErrorCode lastError)
{
    m_lastError = lastError;
    if (lastError){
        setConnectionState(ConnectionState::Error);
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
    qDebug() << "VpnProtocol::setConnectionState" << textConnectionState(state);

    if (m_connectionState == state) {
        return;
    }
    if (m_connectionState == ConnectionState::Disconnected && state == ConnectionState::Disconnecting) {
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
    case ConnectionState::Reconnecting: return tr("Reconnecting...");
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

bool VpnProtocol::isConnected() const
{
    return m_connectionState == ConnectionState::Connected;
}

bool VpnProtocol::isDisconnected() const
{
    return m_connectionState == ConnectionState::Disconnected;
}
