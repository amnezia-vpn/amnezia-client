#include "xrayprotocol.h"

#include "core/ipcclient.h"
#include "core/privileged_process.h"
#include "ipc.h"
#include "utilities.h"
#include "core/networkUtilities.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QJsonDocument>
#include <QtCore/qlogging.h>
#include <QtCore/qobjectdefs.h>
#include <QtCore/qprocess.h>

#ifdef Q_OS_MACOS
static const QString tunName = "utun22";
#else
static const QString tunName = "tun2";
#endif

XrayProtocol::XrayProtocol(const QJsonObject &configuration, QObject *parent) : VpnProtocol(configuration, parent)
{
    readXrayConfiguration(configuration);
    m_routeGateway = NetworkUtilities::getGatewayAndIface().first;
    m_vpnGateway = amnezia::protocols::xray::defaultLocalAddr;
    m_vpnLocalAddress = amnezia::protocols::xray::defaultLocalAddr;
}

XrayProtocol::~XrayProtocol()
{
    qDebug() << "XrayProtocol::~XrayProtocol()";
    XrayProtocol::stop();
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

ErrorCode XrayProtocol::start()
{
    qDebug() << "XrayProtocol::start()";
    setConnectionState(Vpn::ConnectionState::Connecting);

    const ErrorCode err = IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        auto xrayStart = iface->xrayStart(QJsonDocument(m_xrayConfig).toJson());
        if (!xrayStart.waitForFinished() || !xrayStart.returnValue()) {
            qCritical() << "Failed to start xray";
            return ErrorCode::XrayExecutableCrashed;
        }
        return ErrorCode::NoError;
    }, [] () {
        return ErrorCode::AmneziaServiceConnectionFailed;
    });
    if (err != ErrorCode::NoError)
        return err;

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
        const QString remoteAddress = NetworkUtilities::getIPAddress(m_rawConfig.value(amnezia::config_key::hostName).toString());
    #ifdef Q_OS_WIN
        const int inetAdapterIndex = NetworkUtilities::AdapterIndexTo(QHostAddress(remoteAddress));
    #endif
        auto createTun = iface->createTun(tunName, amnezia::protocols::xray::defaultLocalAddr);
        if (!createTun.waitForFinished() || !createTun.returnValue()) {
            qCritical() << "Failed to assign IP address for TUN";
            return ErrorCode::InternalError;
        }

        auto updateResolvers = iface->updateResolvers(tunName, dnsAddr);
        if (!updateResolvers.waitForFinished() || !updateResolvers.returnValue()) {
            qCritical() << "Failed to set DNS resolvers for TUN";
            return ErrorCode::InternalError;
        }

    #ifdef Q_OS_WIN
        int vpnAdapterIndex = -1;
        QList<QNetworkInterface> netInterfaces = QNetworkInterface::allInterfaces();
        for (auto& netInterface : netInterfaces) {
            for (auto& address : netInterface.addressEntries()) {
                if (m_vpnLocalAddress == address.ip().toString())
                    vpnAdapterIndex = netInterface.index();
            }
        }
    #else
        static const int vpnAdapterIndex = 0;
    #endif
        const bool killSwitchEnabled = QVariant(m_rawConfig.value(config_key::killSwitchOption).toString()).toBool();
        if (killSwitchEnabled) {
            if (vpnAdapterIndex != -1) {
                QJsonObject config = m_rawConfig;
                config.insert("vpnServer", remoteAddress);

                auto enableKillSwitch = IpcClient::Interface()->enableKillSwitch(config, vpnAdapterIndex);
                if (!enableKillSwitch.waitForFinished() || !enableKillSwitch.returnValue()) {
                    qCritical() << "Failed to enable killswitch";
                    return ErrorCode::InternalError;
                }
            } else
                qWarning() << "Failed to get vpnAdapterIndex. Killswitch disabled";
        }
#endif

        if (m_routeMode == Settings::RouteMode::VpnAllSites) {
            static const QStringList subnets = { "1.0.0.0/8", "2.0.0.0/7", "4.0.0.0/6", "8.0.0.0/5", "16.0.0.0/4", "32.0.0.0/3", "64.0.0.0/2", "128.0.0.0/1" };

            auto routeAddList =  iface->routeAddList(m_vpnGateway, subnets);
            if (!routeAddList.waitForFinished() || routeAddList.returnValue() != subnets.count()) {
                qCritical() << "Failed to set routes for TUN";
                return ErrorCode::InternalError;
            }
        }

        auto StopRoutingIpv6 = iface->StopRoutingIpv6();
        if (!StopRoutingIpv6.waitForFinished() || !StopRoutingIpv6.returnValue()) {
            qCritical() << "Failed to disable IPv6 routing";
            return ErrorCode::InternalError;
        }

#ifdef Q_OS_WIN
        if (inetAdapterIndex != -1 && vpnAdapterIndex != -1) {
            QJsonObject config = m_rawConfig;
            config.insert("inetAdapterIndex", inetAdapterIndex);
            config.insert("vpnAdapterIndex", vpnAdapterIndex);
            config.insert("vpnGateway", m_vpnGateway);
            config.insert("vpnServer", remoteAddress);

            auto enablePeerTraffic = iface->enablePeerTraffic(config);
            if (!enablePeerTraffic.waitForFinished() || !enablePeerTraffic.returnValue()) {
                qCritical() << "Failed to enable peer traffic";
                return ErrorCode::InternalError;
            }
        } else
            qWarning() << "Failed to get adapter indexes. Split-tunneling disabled";
#endif
        return ErrorCode::NoError;
    },
    [] () {
        return ErrorCode::AmneziaServiceConnectionFailed;
    });
}

ErrorCode XrayProtocol::startTun2Sock()
{
    m_tunProcess = IpcClient::CreatePrivilegedProcess();
    if (!m_tunProcess->waitForSource()) {
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_tunProcess->setProgram(PermittedProcess::Tun2Socks);
    m_tunProcess->setArguments({"-device", tunName, "-proxy", "socks5://127.0.0.1:10808" });

    connect(m_tunProcess.data(), &PrivilegedProcess::readyReadStandardOutput, this, [this]() {
        auto readAllStandardOutput = m_tunProcess->readAllStandardOutput();
        if (!readAllStandardOutput.waitForFinished()) {
            qWarning() << "Failed to read output from tun2socks";
            return;
        }

        const QString line = readAllStandardOutput.returnValue();
        if (line.contains("[STACK] tun://") && line.contains("<-> socks5://127.0.0.1")) {
            if (ErrorCode res = setupRouting(); res != ErrorCode::NoError) {
                stop();
                setLastError(res);
            }
            else
                setConnectionState(Vpn::ConnectionState::Connected);
        }
    }, Qt::QueuedConnection);

    connect(m_tunProcess.data(), &PrivilegedProcess::finished, this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus == QProcess::ExitStatus::CrashExit) {
            qCritical() << "Tun2socks process crashed!";
        } else {
            qCritical() << QString("Tun2socks process was closed with %1 exit code").arg(exitCode);
        }
        stop();
        setLastError(ErrorCode::Tun2SockExecutableCrashed);
    }, Qt::QueuedConnection);

    m_tunProcess->start();
    return ErrorCode::NoError;
}

void XrayProtocol::stop()
{
    qDebug() << "XrayProtocol::stop()";
    setConnectionState(Vpn::ConnectionState::Disconnecting);

#ifdef AMNEZIA_DESKTOP
    IpcClient::withInterface([](QSharedPointer<IpcInterfaceReplica> iface) {
        auto disableKillSwitch = iface->disableKillSwitch();
        if (!disableKillSwitch.waitForFinished() || !disableKillSwitch.returnValue()) {
            qWarning() << "Failed to disable killswitch";
        }

        auto StartRoutingIpv6 = iface->StartRoutingIpv6();
        if (!StartRoutingIpv6.waitForFinished() || !StartRoutingIpv6.returnValue()) {
            qWarning() << "Failed to start routing ipv6";
        }

        auto restoreResolvers = iface->restoreResolvers();
        if (!restoreResolvers.waitForFinished() || !restoreResolvers.returnValue()) {
            qWarning() << "Failed to restore resolvers";
        }

        auto deleteTun = iface->deleteTun(tunName);
        if (!deleteTun.waitForFinished() || !deleteTun.returnValue()) {
            qWarning() << "Failed to delete tun";
        }

        auto xrayStop = iface->xrayStop();
        if (!xrayStop.waitForFinished() || !xrayStop.returnValue()) {
            qWarning() << "Failed to stop xray";
        }
    });


    if (m_tunProcess) {
        m_tunProcess->blockSignals(true);
        m_tunProcess->close();
    }

    setConnectionState(Vpn::ConnectionState::Disconnected);

#endif
}
