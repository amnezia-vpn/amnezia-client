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

ErrorCode XrayProtocol::setupRouting() {
    return IpcClient::withInterface([this](QSharedPointer<IpcInterfaceReplica> iface) -> ErrorCode {
        QList<QHostAddress> dnsAddr;

        dnsAddr.push_back(QHostAddress(m_primaryDNS));
        // We don't use secondary DNS if primary DNS is AmneziaDNS
        if (!m_primaryDNS.contains(amnezia::protocols::dns::amneziaDnsIp)) {
            dnsAddr.push_back(QHostAddress(m_secondaryDNS));
        }

#ifdef AMNEZIA_DESKTOP
    #ifdef Q_OS_MACOS
        const QString tunName = "utun22";
    #else
        const QString tunName = "tun2";
    #endif
        auto createTun = iface->createTun(tunName, amnezia::protocols::xray::defaultLocalAddr);
        if (!createTun.waitForFinished(1000) || !createTun.returnValue()) {
            qWarning() << "Failed to assign IP address for TUN";
            return ErrorCode::InternalError;
        }

        auto updateResolvers = iface->updateResolvers(tunName, dnsAddr);
        if (!updateResolvers.waitForFinished(1000) || !updateResolvers.returnValue()) {
            qWarning() << "Failed to set DNS resolvers for TUN";
            return ErrorCode::InternalError;
        }
#endif

        if (m_routeMode == Settings::RouteMode::VpnAllSites) {
            static const QStringList subnets = { "1.0.0.0/8", "2.0.0.0/7", "4.0.0.0/6", "8.0.0.0/5", "16.0.0.0/4", "32.0.0.0/3", "64.0.0.0/2", "128.0.0.0/1" };

            auto routeAddList =  iface->routeAddList(m_vpnGateway, subnets);
            if (!routeAddList.waitForFinished(1000) || routeAddList.returnValue() != subnets.count()) {
                qWarning() << "Failed to set routes for TUN";
                return ErrorCode::InternalError;
            }
        }

        auto StopRoutingIpv6 = iface->StopRoutingIpv6();
        if (!StopRoutingIpv6.waitForFinished(1000) || !StopRoutingIpv6.returnValue()) {
            qWarning() << "Failed to disable IPv6 routing";
            return ErrorCode::InternalError;
        }

#ifdef Q_OS_WIN
        auto enablePeerTraffic = iface->enablePeerTraffic(m_xrayConfig);
        if (!enablePeerTraffic.waitForFinished(5000) || !enablePeerTraffic.returnValue()) {
            qWarning() << "Failed to enable peer traffic";
            return ErrorCode::InternalError;
        }
#endif
        return ErrorCode::NoError;
    },
    [] () {
        return ErrorCode::AmneziaServiceConnectionFailed;
    });
}

ErrorCode XrayProtocol::startTun2Sock()
{
    m_t2sProcess->start();

    connect(m_t2sProcess.data(), &IpcProcessTun2SocksReplica::stateChanged, this,
            [&](QProcess::ProcessState newState) { qDebug() << "PrivilegedProcess stateChanged" << newState; });

    connect(m_t2sProcess.data(), &IpcProcessTun2SocksReplica::setConnectionState, this, [&](int vpnState) {
        QMetaObject::invokeMethod(this, [this, vpnState]() {
            qDebug() << "PrivilegedProcess setConnectionState " << vpnState;

            if (vpnState == Vpn::ConnectionState::Connected) {
                setConnectionState(Vpn::ConnectionState::Connecting);

                if (ErrorCode res = setupRouting(); res != ErrorCode::NoError) {
                    stop();
                    setLastError(res);
                } else
                    setConnectionState(Vpn::ConnectionState::Connected);
            }

            if (vpnState == Vpn::ConnectionState::Disconnected)
                stop();

        }, Qt::QueuedConnection);
    });

    return ErrorCode::NoError;
}

void XrayProtocol::stop()
{
    qDebug() << "XrayProtocol::stop()";

    IpcClient::withInterface([](QSharedPointer<IpcInterfaceReplica> iface) {
#ifdef AMNEZIA_DESKTOP
        auto StartRoutingIpv6 = iface->StartRoutingIpv6();
        if (!StartRoutingIpv6.waitForFinished(1000) || !StartRoutingIpv6.returnValue()) {
            qWarning() << "XrayProtocol::stop(): Failed to start routing ipv6";
        }

        auto restoreResolvers = iface->restoreResolvers();
        if (!restoreResolvers.waitForFinished(1000) || !restoreResolvers.returnValue()) {
            qWarning() << "XrayProtocol::stop(): Failed to restore resolvers";
        }

    #if !defined(Q_OS_MACOS)
        auto deleteTun = iface->deleteTun("tun2");
        if (!deleteTun.waitForFinished(1000) || !deleteTun.returnValue()) {
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
