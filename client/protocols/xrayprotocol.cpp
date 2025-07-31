#include "xrayprotocol.h"

#include "core/defs.h"
#include "core/networkUtilities.h"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QtCore/qcontainerfwd.h>
#include <QtCore/qdebug.h>
#include <QtCore/qlogging.h>
#include <QtCore/qobject.h>
#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>

#include "core/ipcclient.h"
#include "libxray.h"

XrayProtocol::XrayProtocol(const QJsonObject &configuration, QObject *parent) : VpnProtocol(configuration, parent)
{
    readXrayConfiguration(configuration);
    m_routeGateway = NetworkUtilities::getGatewayAndIface();
    m_vpnGateway = amnezia::protocols::xray::defaultLocalAddr;
    m_vpnLocalAddress = amnezia::protocols::xray::defaultLocalAddr;
    m_t2sProcess = IpcClient::InterfaceTun2Socks();

    amnezia_xray_setloghandler(&XrayProtocol::ctxLogHandler, this);
    amnezia_xray_setsockcallback(&XrayProtocol::ctxSockCallback, this);
}

XrayProtocol::~XrayProtocol()
{
    qDebug() << "XrayProtocol::~XrayProtocol()";
    XrayProtocol::stop();
}

ErrorCode XrayProtocol::start()
{
    qDebug() << "XrayProtocol::start()";

    auto cfg = QJsonDocument(m_xrayConfig).toJson().toStdString();
    if (auto err = amnezia_xray_configure(cfg.data()); err != 0) {
        return ErrorCode::InternalError;
    }

    if (auto err = amnezia_xray_start(); err != 0) {
        return ErrorCode::NotImplementedError;
    }

    setConnectionState(Vpn::ConnectionState::Connecting);

    return startTun2Sock();
}

ErrorCode XrayProtocol::startTun2Sock()
{
    m_t2sProcess->start();

#ifdef Q_OS_WIN
    m_configData.insert("inetAdapterIndex", NetworkUtilities::AdapterIndexTo(QHostAddress(m_remoteAddress)));
#endif

    connect(m_t2sProcess.data(), &IpcProcessTun2SocksReplica::stateChanged, this,
            [&](QProcess::ProcessState newState) { qDebug() << "PrivilegedProcess stateChanged" << newState; });

    connect(m_t2sProcess.data(), &IpcProcessTun2SocksReplica::setConnectionState, this, [&](int vpnState) {
        qDebug() << "PrivilegedProcess setConnectionState " << vpnState;
        if (vpnState == Vpn::ConnectionState::Connected) {
            setConnectionState(Vpn::ConnectionState::Connecting);
            QList<QHostAddress> dnsAddr;

            dnsAddr.push_back(QHostAddress(m_configData.value(config_key::dns1).toString()));
            // We don't use secondary DNS if primary DNS is AmneziaDNS
            if (!m_configData.value(amnezia::config_key::dns1).toString().
                 contains(amnezia::protocols::dns::amneziaDnsIp)) {
                dnsAddr.push_back(QHostAddress(m_configData.value(config_key::dns2).toString()));
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
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
            // killSwitch toggle
            if (QVariant(m_configData.value(config_key::killSwitchOption).toString()).toBool()) {
                m_configData.insert("vpnServer", m_remoteAddress);
                IpcClient::Interface()->enableKillSwitch(m_configData, 0);
            }
#endif
            if (m_routeMode == Settings::RouteMode::VpnAllSites) {
                IpcClient::Interface()->routeAddList(m_vpnGateway, QStringList() << "1.0.0.0/8" << "2.0.0.0/7" << "4.0.0.0/6" << "8.0.0.0/5" << "16.0.0.0/4" << "32.0.0.0/3" << "64.0.0.0/2" << "128.0.0.0/1");
            }
            IpcClient::Interface()->StopRoutingIpv6();
#ifdef Q_OS_WIN
            IpcClient::Interface()->updateResolvers("tun2", dnsAddr);
            QList<QNetworkInterface> netInterfaces = QNetworkInterface::allInterfaces();
            for (int i = 0; i < netInterfaces.size(); i++) {
                for (int j = 0; j < netInterfaces.at(i).addressEntries().size(); j++) {
                    // killSwitch toggle
                    if (m_vpnLocalAddress == netInterfaces.at(i).addressEntries().at(j).ip().toString()) {
                        if (QVariant(m_configData.value(config_key::killSwitchOption).toString()).toBool()) {
                            IpcClient::Interface()->enableKillSwitch(m_configData, netInterfaces.at(i).index());
                        }
                        m_configData.insert("vpnAdapterIndex", netInterfaces.at(i).index());
                        m_configData.insert("vpnGateway", m_vpnGateway);
                        m_configData.insert("vpnServer", m_remoteAddress);
                        IpcClient::Interface()->enablePeerTraffic(m_configData);
                    }
                }
            }
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
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    IpcClient::Interface()->disableKillSwitch();
    IpcClient::Interface()->StartRoutingIpv6();
#endif
    qDebug() << "XrayProtocol::stop()";

    amnezia_xray_stop();

    if (m_t2sProcess) {
        m_t2sProcess->stop();
    }

    setConnectionState(Vpn::ConnectionState::Disconnected);
}

void XrayProtocol::readXrayConfiguration(const QJsonObject &configuration)
{
    m_configData = configuration;
    QJsonObject xrayConfiguration = configuration.value(ProtocolProps::key_proto_config_data(Proto::Xray)).toObject();
    if (xrayConfiguration.isEmpty()) {
        xrayConfiguration = configuration.value(ProtocolProps::key_proto_config_data(Proto::SSXray)).toObject();
    }
    m_xrayConfig = xrayConfiguration;
    m_localPort = QString(amnezia::protocols::xray::defaultLocalProxyPort).toInt();
    m_remoteHost = configuration.value(amnezia::config_key::hostName).toString();
    m_remoteAddress = NetworkUtilities::getIPAddress(m_remoteHost);
    m_routeMode = static_cast<Settings::RouteMode>(configuration.value(amnezia::config_key::splitTunnelType).toInt());
    m_primaryDNS = configuration.value(amnezia::config_key::dns1).toString();
    m_secondaryDNS = configuration.value(amnezia::config_key::dns2).toString();
}

void XrayProtocol::sockCallback(uintptr_t fd)
{
// #ifdef Q_OS_DARWIN
//     const int iface = if_nametoindex("en0");
//     if (iface > 0)
//     {
//         setsockopt(fd, IPPROTO_IP, IP_BOUND_IF, &iface, sizeof(iface));
//         setsockopt(fd, IPPROTO_IPV6, IPV6_BOUND_IF, &iface, sizeof(iface));
//     }
// #endif
}

void XrayProtocol::logHandler(char* str)
{
    
}
