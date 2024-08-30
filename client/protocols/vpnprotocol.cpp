#include <QDebug>
#include <QTimer>

#include "core/errorstrings.h"
#include "vpnprotocol.h"

#if defined(Q_OS_WINDOWS) || defined(Q_OS_MACX) || (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID))
    #include "openvpnovercloakprotocol.h"
    #include "openvpnprotocol.h"
    #include "shadowsocksvpnprotocol.h"
    #include "wireguardprotocol.h"
    #include "xrayprotocol.h"
#endif

#ifdef Q_OS_WINDOWS
    #include "ikev2_vpn_protocol_windows.h"
#endif

#ifdef Q_OS_LINUX
#include "ikev2_vpn_protocol_linux.h"
#endif

VpnProtocol::VpnProtocol(const QJsonObject &configuration, QObject *parent)
    : QObject(parent),
      m_connectionState(Vpn::ConnectionState::Unknown),
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

VpnProtocol *VpnProtocol::factory(DockerContainer container, const QJsonObject &configuration)
{
    switch (container) {
#if defined(Q_OS_WINDOWS) || defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    case DockerContainer::Ipsec: return new Ikev2Protocol(configuration);
#endif
#if defined(Q_OS_WINDOWS) || defined(Q_OS_MACX) || (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID))
    case DockerContainer::OpenVpn: return new OpenVpnProtocol(configuration);
    case DockerContainer::Cloak: return new OpenVpnOverCloakProtocol(configuration);
    case DockerContainer::ShadowSocks: return new ShadowSocksVpnProtocol(configuration);
    case DockerContainer::WireGuard: return new WireguardProtocol(configuration);
    case DockerContainer::Awg: return new WireguardProtocol(configuration);
    case DockerContainer::Xray: return new XrayProtocol(configuration);
    case DockerContainer::SSXray: return new XrayProtocol(configuration);
#endif
    default: return nullptr;
    }
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
