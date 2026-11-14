#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QHostAddress>
#include <QProcess>
#include <QTcpSocket>
#include <QThread>

#include "wireGuardProtocol.h"
#include "core/utils/networkUtilities.h"

#include "mozilla/localsocketcontroller.h"

namespace
{
    constexpr int kStatusPollMsecs = 20 * 1000;

    bool isHostName(const QString &host)
    {
        return !host.isEmpty() && QHostAddress(host).isNull();
    }
}

WireguardProtocol::WireguardProtocol(const QJsonObject &configuration, QObject *parent)
    : VpnProtocol(configuration, parent)
{
    const QString protocolName = m_rawConfig.value("protocol").toString();
    m_hostName = m_rawConfig.value(configKey::hostName).toString();
    m_configHostName = m_rawConfig.value(protocolName + "_config_data").toObject().value(configKey::hostName).toString();
    m_watchEndpoint = isHostName(m_hostName) || isHostName(m_configHostName);

    m_statusTimer.setInterval(kStatusPollMsecs);

    m_impl.reset(new LocalSocketController());
    connect(m_impl.get(), &ControllerImpl::connected, this,
            [this](const QString &pubkey, const QDateTime &connectionTimestamp) {
                setConnectionState(Vpn::ConnectionState::Connected);
            });
    connect(m_impl.get(), &ControllerImpl::statusUpdated, this,
            [this](const QString& serverIpv4Gateway,
                   const QString& deviceIpv4Address, uint64_t txBytes,
                   uint64_t rxBytes) {
                const QString previousGateway = m_vpnGateway;
                const QString previousLocal = m_vpnLocalAddress;

                if (!serverIpv4Gateway.isEmpty()) {
                    m_vpnGateway = serverIpv4Gateway;
                }
                if (!deviceIpv4Address.isEmpty()) {
                    m_vpnLocalAddress = deviceIpv4Address;
                }

                if ((!m_vpnGateway.isEmpty() && m_vpnGateway != previousGateway) ||
                    (!m_vpnLocalAddress.isEmpty() && m_vpnLocalAddress != previousLocal)) {
                    emit tunnelAddressesUpdated(m_vpnGateway, m_vpnLocalAddress);
                }
            });

    connect(m_impl.get(), &ControllerImpl::disconnected, this,
            [this]() { setConnectionState(Vpn::ConnectionState::Disconnected); });
    connect(m_impl.get(), &ControllerImpl::peerStatusUpdated, this, &WireguardProtocol::onPeerStatus);
    connect(&m_statusTimer, &QTimer::timeout, this, [this]() { m_impl->checkStatus(); });
    m_impl->initialize(nullptr, nullptr);
}

WireguardProtocol::~WireguardProtocol()
{
    WireguardProtocol::stop();
    QThread::msleep(200);
}

void WireguardProtocol::stop()
{
    if (guardStop()) {
        return;
    }
    stopMzImpl();
}

ErrorCode WireguardProtocol::startMzImpl()
{
    QString protocolName = m_rawConfig.value("protocol").toString();
    QJsonObject vpnConfigData = m_rawConfig.value(protocolName + "_config_data").toObject();
    vpnConfigData[configKey::hostName] = resolveHostName(m_configHostName);
    m_rawConfig.insert(protocolName + "_config_data", vpnConfigData);
    m_rawConfig[configKey::hostName] = resolveHostName(m_hostName);

    m_endpointWatchdog.tunnelStarted(QDateTime::currentMSecsSinceEpoch());
    m_impl->activate(m_rawConfig);
    if (m_watchEndpoint) {
        m_statusTimer.start();
    }
    return ErrorCode::NoError;
}

ErrorCode WireguardProtocol::stopMzImpl()
{
    m_statusTimer.stop();
    m_impl->deactivate();
    return ErrorCode::NoError;
}

QString WireguardProtocol::resolveHostName(const QString &hostName)
{
    const QString address = NetworkUtilities::getIPAddress(hostName);
    if (!address.isEmpty()) {
        m_lastAddresses.insert(hostName, address);
        return address;
    }
    // DNS can be briefly unavailable right after the previous tunnel went
    // down (e.g. with the kill switch on): keep the address that worked
    // before instead of starting without an endpoint.
    return m_lastAddresses.value(hostName);
}

void WireguardProtocol::onPeerStatus(qint64 lastHandshakeMsecs, quint64 txBytes)
{
    if (!m_watchEndpoint || !m_statusTimer.isActive()) {
        return;
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (!m_endpointWatchdog.observe(now, lastHandshakeMsecs, txBytes)) {
        return;
    }
    m_endpointWatchdog.refreshAttempted(now);
    qInfo() << "WireguardProtocol: no handshake with" << m_hostName
            << "while sending, reconnecting to resolve its address again";
    emit reconnectRequested();
}


ErrorCode WireguardProtocol::start()
{
    resetStopGuard();
    return startMzImpl();
}
