#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QTcpSocket>
#include <QThread>

#include "wireGuardProtocol.h"
#include "core/utils/networkUtilities.h"

#include "mozilla/localsocketcontroller.h"

WireguardProtocol::WireguardProtocol(const QJsonObject &configuration, QObject *parent)
    : VpnProtocol(configuration, parent)
{
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
    m_impl->initialize(nullptr, nullptr);
}

WireguardProtocol::~WireguardProtocol()
{
    WireguardProtocol::stop();
    QThread::msleep(200);
}

void WireguardProtocol::stop()
{
    if (m_stopped) {
        return;
    }
    m_stopped = true;
    stopMzImpl(takeKeepFirewallOnNextStop());
    return;
}

ErrorCode WireguardProtocol::startMzImpl()
{
    const QString protocolName = m_rawConfig.value("protocol").toString();
    const QString configDataKey = protocolName + "_config_data";
    QJsonObject vpnConfigData = m_rawConfig.value(configDataKey).toObject();

    const QString endpointHost = vpnConfigData.value(configKey::hostName).toString();
    const QString endpointIp = NetworkUtilities::getIPAddress(endpointHost);
    if (endpointIp.isEmpty()) {
        qWarning() << "WireguardProtocol: unable to resolve the endpoint host, aborting this attempt";
        recordLastError(ErrorCode::EndpointResolutionError);
        return ErrorCode::EndpointResolutionError;
    }

    // Activate a resolved copy: m_rawConfig must keep the original hostname so
    // a later attempt (e.g. reconnect after wakeup) re-resolves it from scratch
    // instead of reusing a stale — or empty — address.
    QJsonObject rawConfig = m_rawConfig;
    vpnConfigData[configKey::hostName] = endpointIp;
    rawConfig.insert(configDataKey, vpnConfigData);
    if (rawConfig.value(configKey::hostName).toString() == endpointHost) {
        rawConfig[configKey::hostName] = endpointIp;
    }

    m_impl->activate(rawConfig);
    return ErrorCode::NoError;
}

ErrorCode WireguardProtocol::stopMzImpl(bool keepFirewall)
{
    m_impl->deactivate(keepFirewall);
    return ErrorCode::NoError;
}


ErrorCode WireguardProtocol::start()
{
    m_stopped = false;
    return startMzImpl();
}
