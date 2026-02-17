#include "xrayprotocol.h"

#include "core/ipcclient.h"
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
    m_vpnGateway = amnezia::protocols::xray::defaultLocalAddr;
    m_vpnLocalAddress = amnezia::protocols::xray::defaultLocalAddr;
    m_routeGateway = NetworkUtilities::getGatewayAndIface().first;

    m_routeMode = static_cast<Settings::RouteMode>(configuration.value(amnezia::config_key::splitTunnelType).toInt());
    m_remoteAddress = NetworkUtilities::getIPAddress(m_rawConfig.value(amnezia::config_key::hostName).toString());

    const QString primaryDns = configuration.value(amnezia::config_key::dns1).toString();
    m_dnsServers.push_back(QHostAddress(primaryDns));
    if (primaryDns != amnezia::protocols::dns::amneziaDnsIp) {
        const QString secondaryDns = configuration.value(amnezia::config_key::dns2).toString();
        m_dnsServers.push_back(QHostAddress(secondaryDns));
    }

    QJsonObject xrayConfiguration = configuration.value(ProtocolProps::key_proto_config_data(Proto::Xray)).toObject();
    if (xrayConfiguration.isEmpty()) {
        xrayConfiguration = configuration.value(ProtocolProps::key_proto_config_data(Proto::SSXray)).toObject();
    }
    m_xrayConfig = xrayConfiguration;
}

XrayProtocol::~XrayProtocol()
{
    qDebug() << "XrayProtocol::~XrayProtocol()";
    XrayProtocol::stop();
}

ErrorCode XrayProtocol::start()
{
    qDebug() << "XrayProtocol::start()";
    setConnectionState(Vpn::ConnectionState::Connecting);

    return IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        auto xrayStart = iface->xrayStart(QJsonDocument(m_xrayConfig).toJson());
        if (!xrayStart.waitForFinished() || !xrayStart.returnValue()) {
            qCritical() << "Failed to start xray";
            return ErrorCode::XrayExecutableCrashed;
        }
        return startTun2Socks();
    }, [] () {
        return ErrorCode::AmneziaServiceConnectionFailed;
    });
}

void XrayProtocol::stop()
{
    qDebug() << "XrayProtocol::stop()";
    setConnectionState(Vpn::ConnectionState::Disconnecting);

    IpcClient::withInterface([](QSharedPointer<IpcInterfaceReplica> iface) {
        auto disableKillSwitch = iface->disableKillSwitch();
        if (!disableKillSwitch.waitForFinished() || !disableKillSwitch.returnValue())
            qWarning() << "Failed to disable killswitch";

        auto StartRoutingIpv6 = iface->StartRoutingIpv6();
        if (!StartRoutingIpv6.waitForFinished() || !StartRoutingIpv6.returnValue())
            qWarning() << "Failed to start routing ipv6";

        auto restoreResolvers = iface->restoreResolvers();
        if (!restoreResolvers.waitForFinished() || !restoreResolvers.returnValue())
            qWarning() << "Failed to restore resolvers";

        auto deleteTun = iface->deleteTun(tunName);
        if (!deleteTun.waitForFinished() || !deleteTun.returnValue())
            qWarning() << "Failed to delete tun";

        auto xrayStop = iface->xrayStop();
        if (!xrayStop.waitForFinished() || !xrayStop.returnValue())
            qWarning() << "Failed to stop xray";
    });

    if (m_tun2socksProcess) {
        m_tun2socksProcess->blockSignals(true);

#ifndef Q_OS_WIN
        m_tun2socksProcess->terminate();
        auto waitForFinished = m_tun2socksProcess->waitForFinished(1000);
        if (!waitForFinished.waitForFinished() || !waitForFinished.returnValue()) {
            qWarning() << "Failed to terminate tun2socks. Killing the process...";
            m_tun2socksProcess->kill();
        }
#else
        // terminate does not do anything useful on Windows
        // so just kill the process
        m_tun2socksProcess->kill();
#endif

        m_tun2socksProcess->close();
        m_tun2socksProcess.reset();
    }

    setConnectionState(Vpn::ConnectionState::Disconnected);
}

ErrorCode XrayProtocol::startTun2Socks()
{
    m_tun2socksProcess = IpcClient::CreatePrivilegedProcess();
    if (!m_tun2socksProcess->waitForSource()) {
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_tun2socksProcess->setProgram(PermittedProcess::Tun2Socks);
    m_tun2socksProcess->setArguments({"-device", QString("tun://%1").arg(tunName), "-proxy", "socks5://127.0.0.1:10808" });

    connect(m_tun2socksProcess.data(), &IpcProcessInterfaceReplica::readyReadStandardOutput, this, [this]() {
        auto readAllStandardOutput = m_tun2socksProcess->readAllStandardOutput();
        if (!readAllStandardOutput.waitForFinished()) {
            qWarning() << "Failed to read output from tun2socks";
            return;
        }

        const QString line = readAllStandardOutput.returnValue();

        if (!line.contains("[TCP]") && !line.contains("[UDP]"))
            qDebug() << "[tun2socks]:" << line;
        
        if (line.contains("[STACK] tun://") && line.contains("<-> socks5://127.0.0.1")) {
            disconnect(m_tun2socksProcess.data(), &IpcProcessInterfaceReplica::readyReadStandardOutput, this, nullptr);

            if (ErrorCode res = setupRouting(); res != ErrorCode::NoError) {
                stop();
                setLastError(res);
            } else {
                setConnectionState(Vpn::ConnectionState::Connected);
            }
        }
    }, Qt::QueuedConnection);

    connect(m_tun2socksProcess.data(), &IpcProcessInterfaceReplica::finished, this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus == QProcess::ExitStatus::CrashExit) {
            qCritical() << "Tun2socks process crashed!";
        } else {
            qCritical() << QString("Tun2socks process was closed with %1 exit code").arg(exitCode);
        }
        stop();
        setLastError(ErrorCode::Tun2SockExecutableCrashed);
    }, Qt::QueuedConnection);

    m_tun2socksProcess->start();
    return ErrorCode::NoError;
}

ErrorCode XrayProtocol::setupRouting() {
    return IpcClient::withInterface([this](QSharedPointer<IpcInterfaceReplica> iface) -> ErrorCode {
#ifdef Q_OS_WIN
        const int inetAdapterIndex = NetworkUtilities::AdapterIndexTo(QHostAddress(m_remoteAddress));
#endif
        auto createTun = iface->createTun(tunName, amnezia::protocols::xray::defaultLocalAddr);
        if (!createTun.waitForFinished() || !createTun.returnValue()) {
            qCritical() << "Failed to assign IP address for TUN";
            return ErrorCode::InternalError;
        }

        auto updateResolvers = iface->updateResolvers(tunName, m_dnsServers);
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
                config.insert("vpnServer", m_remoteAddress);

                auto enableKillSwitch = IpcClient::Interface()->enableKillSwitch(config, vpnAdapterIndex);
                if (!enableKillSwitch.waitForFinished() || !enableKillSwitch.returnValue()) {
                    qCritical() << "Failed to enable killswitch";
                    return ErrorCode::InternalError;
                }
            } else
                qWarning() << "Failed to get vpnAdapterIndex. Killswitch disabled";
        }

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
            config.insert("vpnServer", m_remoteAddress);

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
