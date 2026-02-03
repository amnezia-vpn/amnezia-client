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

    const ErrorCode err = IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        iface->xrayStart(QJsonDocument(m_xrayConfig).toJson());
        return ErrorCode::NoError;
    }, [] () {
        return ErrorCode::AmneziaServiceConnectionFailed;
    });
    if (err != ErrorCode::NoError)
        return err;

    setConnectionState(Vpn::ConnectionState::Connecting);
    return startTun2Sock();
}

void XrayProtocol::tun2socksConnectionStateChanged(int vpnState) {
    qDebug() << "PrivilegedProcess setConnectionState " << vpnState;
    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        if (vpnState == Vpn::ConnectionState::Connected) {
            setConnectionState(Vpn::ConnectionState::Connecting);
            QList<QHostAddress> dnsAddr;

            dnsAddr.push_back(QHostAddress(m_primaryDNS));
            // We don't use secondary DNS if primary DNS is AmneziaDNS
            if (!m_primaryDNS.contains(amnezia::protocols::dns::amneziaDnsIp)) {
                dnsAddr.push_back(QHostAddress(m_secondaryDNS));
            }

            QRemoteObjectPendingReply<bool> res;

#ifdef AMNEZIA_DESKTOP
#ifdef Q_OS_MACOS
            const QString tunName = "utun22";
#else
            const QString tunName = "tun2";
#endif
            res = iface->createTun(tunName, amnezia::protocols::xray::defaultLocalAddr);
            if (!res.waitForFinished(10000)) {
                qDebug() << "Failed to assign IP address for TUN";
            }

            res = iface->updateResolvers(tunName, dnsAddr);
            if (!res.waitForFinished(1000)) {
                qDebug() << "Failed to set DNS resolvers for TUN";
            }
#endif

            if (m_routeMode == Settings::RouteMode::VpnAllSites) {
                iface->routeAddList(m_vpnGateway, QStringList() << "1.0.0.0/8" << "2.0.0.0/7" << "4.0.0.0/6" << "8.0.0.0/5" << "16.0.0.0/4" << "32.0.0.0/3" << "64.0.0.0/2" << "128.0.0.0/1");
            }

            res = iface->StopRoutingIpv6();
            if (!res.waitForFinished(1000)) {
                qDebug() << "Failed to stop routing IPv6";
            }

#ifdef Q_OS_WIN
            res = iface->enablePeerTraffic(m_xrayConfig);
            if (!res.waitForFinished()) {
                qDebug() << "Failed to enable peer traffic";
            }
#endif
            setConnectionState(Vpn::ConnectionState::Connected);
        }
#if !defined(Q_OS_MACOS)
        if (vpnState == Vpn::ConnectionState::Disconnected) {
            setConnectionState(Vpn::ConnectionState::Disconnected);
            iface->deleteTun("tun2");
            iface->StartRoutingIpv6();
            iface->clearSavedRoutes();
        }
#endif
    });
}

ErrorCode XrayProtocol::startTun2Sock()
{
    m_t2sProcess->start();

    connect(m_t2sProcess.data(), &IpcProcessTun2SocksReplica::stateChanged, this,
            [&](QProcess::ProcessState newState) { qDebug() << "PrivilegedProcess stateChanged" << newState; });

    connect(m_t2sProcess.data(), &IpcProcessTun2SocksReplica::setConnectionState, this, [&](int vpnState) {
        QMetaObject::invokeMethod(this, [this, vpnState]() {
            QThread::msleep(5000);
            tun2socksConnectionStateChanged(vpnState);
        }, Qt::QueuedConnection);
    });

    return ErrorCode::NoError;
}

void XrayProtocol::stop()
{
    qDebug() << "XrayProtocol::stop()";

    IpcClient::withInterface([](QSharedPointer<IpcInterfaceReplica> iface) {
#ifdef AMNEZIA_DESKTOP
        QRemoteObjectPendingReply<bool> StartRoutingIpv6Resp = iface->StartRoutingIpv6();
        if (!StartRoutingIpv6Resp.waitForFinished(1000)) {
            qWarning() << "XrayProtocol::stop(): Failed to start routing ipv6";
        }

        QRemoteObjectPendingReply<bool> restoreResolvers = iface->restoreResolvers();
        if (!restoreResolvers.waitForFinished(1000)) {
            qWarning() << "XrayProtocol::stop(): Failed to restore resolvers";
        }

    #if !defined(Q_OS_MACOS)
        QRemoteObjectPendingReply<bool> deleteTunResp = iface->deleteTun("tun2");
        if (!deleteTunResp.waitForFinished(1000)) {
            qWarning() << "XrayProtocol::stop(): Failed to delete tun";
        }
    #endif
#endif
        iface->xrayStop();
    });

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
