#include "xrayprotocol.h"

#include "core/ipcclient.h"
#include "utilities.h"
#include "core/networkUtilities.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QJsonDocument>

XrayProtocol::XrayProtocol(const QJsonObject &configuration, QObject *parent) : VpnProtocol(configuration, parent)
{
    readXrayConfiguration(configuration);
    m_routeGateway = NetworkUtilities::getGatewayAndIface().first;
    m_vpnGateway = amnezia::protocols::xray::defaultLocalAddr;
    m_vpnLocalAddress = amnezia::protocols::xray::defaultLocalAddr;
    m_t2sProcess = IpcClient::InterfaceTun2Socks();
}

XrayProtocol::~XrayProtocol()
{
    qDebug() << "XrayProtocol::~XrayProtocol()";
    XrayProtocol::stop();
}

ErrorCode XrayProtocol::start()
{
    qDebug() << "XrayProtocol::start()";

    IpcClient::Interface()->xrayStart(QJsonDocument(m_xrayConfig).toJson());

    setConnectionState(Vpn::ConnectionState::Connecting);
    return startTun2Sock();
}

ErrorCode XrayProtocol::startTun2Sock()
{
    m_t2sProcess->start();

    connect(m_t2sProcess.data(), &IpcProcessTun2SocksReplica::stateChanged, this,
            [&](QProcess::ProcessState newState) { qDebug() << "PrivilegedProcess stateChanged" << newState; });

    connect(m_t2sProcess.data(), &IpcProcessTun2SocksReplica::setConnectionState, this, [&](int vpnState) {
        qDebug() << "PrivilegedProcess setConnectionState " << vpnState;
        if (vpnState == Vpn::ConnectionState::Connected) {
            setConnectionState(Vpn::ConnectionState::Connecting);
            QList<QHostAddress> dnsAddr;

            dnsAddr.push_back(QHostAddress(m_primaryDNS));
            // We don't use secondary DNS if primary DNS is AmneziaDNS
            if (!m_primaryDNS.contains(amnezia::protocols::dns::amneziaDnsIp)) {
                dnsAddr.push_back(QHostAddress(m_secondaryDNS));
            }
#ifdef Q_OS_WIN
            QThread::msleep(8000);
#endif
#ifdef Q_OS_MACOS
            QThread::msleep(5000);
            IpcClient::Interface()->createTun("utun22", amnezia::protocols::xray::defaultLocalAddr);
            IpcClient::Interface()->updateResolvers("utun22", dnsAddr);
#endif
#ifdef Q_OS_LINUX
            QThread::msleep(1000);
            IpcClient::Interface()->createTun("tun2", amnezia::protocols::xray::defaultLocalAddr);
            IpcClient::Interface()->updateResolvers("tun2", dnsAddr);
#endif
            if (m_routeMode == Settings::RouteMode::VpnAllSites) {
                IpcClient::Interface()->routeAddList(m_vpnGateway, QStringList() << "1.0.0.0/8" << "2.0.0.0/7" << "4.0.0.0/6" << "8.0.0.0/5" << "16.0.0.0/4" << "32.0.0.0/3" << "64.0.0.0/2" << "128.0.0.0/1");
            }
            IpcClient::Interface()->StopRoutingIpv6();
#ifdef Q_OS_WIN
            IpcClient::Interface()->updateResolvers("tun2", dnsAddr);
#endif
            setConnectionState(Vpn::ConnectionState::Connected);
        }
#if !defined(Q_OS_MACOS)
        if (vpnState == Vpn::ConnectionState::Disconnected) {
            setConnectionState(Vpn::ConnectionState::Disconnected);
            IpcClient::Interface()->deleteTun("tun2");
            IpcClient::Interface()->StartRoutingIpv6();
            IpcClient::Interface()->clearSavedRoutes();
        }
#endif
    });

    return ErrorCode::NoError;
}

void XrayProtocol::stop()
{
#ifdef AMNEZIA_DESKTOP
    QRemoteObjectPendingReply<bool> StartRoutingIpv6Resp = IpcClient::Interface()->StartRoutingIpv6();
    StartRoutingIpv6Resp.waitForFinished(1000);
    QRemoteObjectPendingReply<bool> restoreResolvers = IpcClient::Interface()->restoreResolvers();
    restoreResolvers.waitForFinished(1000);
#if !defined(Q_OS_MACOS)
    QRemoteObjectPendingReply<bool> deleteTunResp = IpcClient::Interface()->deleteTun("tun2");
    deleteTunResp.waitForFinished(1000);
#endif
#endif
    qDebug() << "XrayProtocol::stop()";

    IpcClient::Interface()->xrayStop();

    if (m_t2sProcess) {
        m_t2sProcess->stop();
        QThread::msleep(200);
    }

    setConnectionState(Vpn::ConnectionState::Disconnected);
}

void XrayProtocol::readXrayConfiguration(const QJsonObject &configuration)
{
    QJsonObject xrayConfiguration = configuration.value(ProtocolProps::key_proto_config_data(Proto::Xray)).toObject();
    if (xrayConfiguration.isEmpty()) {
        xrayConfiguration = configuration.value(ProtocolProps::key_proto_config_data(Proto::SSXray)).toObject();
    }
    m_xrayConfig = xrayConfiguration;
    m_routeMode = static_cast<Settings::RouteMode>(configuration.value(amnezia::config_key::splitTunnelType).toInt());
    m_primaryDNS = configuration.value(amnezia::config_key::dns1).toString();
    m_secondaryDNS = configuration.value(amnezia::config_key::dns2).toString();
}
