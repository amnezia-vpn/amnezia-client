#include "xrayProtocol.h"

#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/ipcClient.h"
#include "core/utils/networkUtilities.h"
#include "core/utils/serialization/serialization.h"
#include "ipc.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QNetworkProxy>
#include <QTcpSocket>
#include <QtCore/qlogging.h>
#include <QtCore/qobjectdefs.h>
#include <QtCore/qprocess.h>

#include <exception>

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

    m_routeMode = static_cast<amnezia::RouteMode>(configuration.value(amnezia::configKey::splitTunnelType).toInt());
    m_remoteAddress = NetworkUtilities::getIPAddress(m_rawConfig.value(amnezia::configKey::hostName).toString());

    const QString primaryDns = configuration.value(amnezia::configKey::dns1).toString();
    m_dnsServers.push_back(QHostAddress(primaryDns));
    if (primaryDns != amnezia::protocols::dns::amneziaDnsIp) {
        const QString secondaryDns = configuration.value(amnezia::configKey::dns2).toString();
        m_dnsServers.push_back(QHostAddress(secondaryDns));
    }

    QJsonObject xrayConfiguration = configuration.value(ProtocolUtils::key_proto_config_data(Proto::Xray)).toObject();
    if (xrayConfiguration.isEmpty()) {
        xrayConfiguration = configuration.value(ProtocolUtils::key_proto_config_data(Proto::SSXray)).toObject();
    }

    if (xrayConfiguration.isEmpty()) {
        qWarning() << "Xray config wrapper is empty";
        m_xrayConfig = {};
    }

    m_xrayConfig = QJsonDocument::fromJson(xrayConfiguration.value(amnezia::configKey::config).toString().toUtf8()).object();
    if (m_xrayConfig.isEmpty()) {
        qWarning() << "Xray config string is not a valid JSON object";
        m_xrayConfig = {};
    }

    m_serverPort = extractServerPort();
}

int XrayProtocol::extractServerPort() const
{
    const QJsonArray outbounds = m_xrayConfig.value(amnezia::protocols::xray::outbounds).toArray();
    if (outbounds.isEmpty())
        return 0;

    const QJsonObject settings = outbounds.first().toObject().value(amnezia::protocols::xray::settings).toObject();

    QJsonArray servers;
    if (settings.contains(amnezia::protocols::xray::vnext))
        servers = settings.value(amnezia::protocols::xray::vnext).toArray();
    else if (settings.contains(amnezia::protocols::xray::servers))
        servers = settings.value(amnezia::protocols::xray::servers).toArray();

    if (servers.isEmpty())
        return 0;

    return servers.first().toObject().value(amnezia::protocols::xray::port).toInt();
}

XrayProtocol::~XrayProtocol()
{
    qDebug() << "XrayProtocol::~XrayProtocol()";
    XrayProtocol::stop();
}

ErrorCode XrayProtocol::start()
{
    qDebug() << "XrayProtocol::start()";

    m_connectivityProbeStarted = false;

    if (!probeServerReachable()) {
        qCritical() << "XrayProtocol: VPN server" << m_remoteAddress << "is unreachable";
        return ErrorCode::XrayServerUnreachable;
    }

    // Inject SOCKS5 auth into the inbound before starting xray.
    // Re-uses existing credentials if the config already has them (e.g. imported config).
    amnezia::serialization::inbounds::InboundCredentials creds;
    try {
        creds = amnezia::serialization::inbounds::EnsureInboundAuth(m_xrayConfig);
    } catch (const std::exception &e) {
        qCritical() << "EnsureInboundAuth failed:" << e.what();
        return ErrorCode::InternalError;
    }
    m_socksUser = creds.username;
    m_socksPassword = creds.password;
    m_socksPort = creds.port;

    QString xrayConfigStr = QJsonDocument(m_xrayConfig).toJson(QJsonDocument::Compact);
    if (xrayConfigStr.isEmpty()) {
        qCritical() << "Xray config is empty";
        return ErrorCode::XrayExecutableCrashed;
    }

    // Fix fingerprint: old configs may contain "Mozilla/5.0" which xray-core rejects.
    // Replace with the correct default at runtime so stale stored configs still work.
    if (xrayConfigStr.contains("Mozilla/5.0", Qt::CaseInsensitive)) {
        xrayConfigStr.replace("Mozilla/5.0", amnezia::protocols::xray::defaultFingerprint,
                              Qt::CaseInsensitive);
        qDebug() << "XrayProtocol: patched legacy fingerprint to"
                 << amnezia::protocols::xray::defaultFingerprint;
    }

    // Fix inbound listen address: old configs may use "10.33.0.2" which doesn't exist
    // until TUN is created. xray must listen on 127.0.0.1 so tun2socks can connect.
    if (xrayConfigStr.contains(amnezia::protocols::xray::defaultLocalAddr)) {
        xrayConfigStr.replace(amnezia::protocols::xray::defaultLocalAddr,
                              amnezia::protocols::xray::defaultLocalListenAddr);
        qDebug() << "XrayProtocol: patched legacy inbound listen address to 127.0.0.1";
    }

    startTimeoutTimer();

    return IpcClient::withInterface(
            [&](QSharedPointer<IpcInterfaceReplica> iface) {
                auto xrayStart = iface->xrayStart(xrayConfigStr);
                if (!xrayStart.waitForFinished() || !xrayStart.returnValue()) {
                    qCritical() << "Failed to start xray";
                    stopTimeoutTimer();
                    return ErrorCode::XrayExecutableCrashed;
                }
                return startTun2Socks();
            },
            [this]() {
                stopTimeoutTimer();
                return ErrorCode::AmneziaServiceConnectionFailed;
            });
}

void XrayProtocol::stop()
{
    qDebug() << "XrayProtocol::stop()";

    stopTimeoutTimer();
    stopLivenessMonitor();

    if (m_tun2socksProcess) {
        m_tun2socksProcess->blockSignals(true);

#ifndef Q_OS_WIN
        m_tun2socksProcess->terminate();
        auto waitForFinished = m_tun2socksProcess->waitForFinished(1000);
        if (!waitForFinished.waitForFinished() || !waitForFinished.returnValue()) {
            qWarning() << "Failed to terminate tun2socks. Killing the process...";
            m_tun2socksProcess->kill();
            m_tun2socksProcess->waitForFinished(1000);
        }
#else
        m_tun2socksProcess->kill();
#endif

        m_tun2socksProcess->close();
        m_tun2socksProcess.reset();
    }

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

    setConnectionState(Vpn::ConnectionState::Disconnected);
}

ErrorCode XrayProtocol::startTun2Socks()
{
    m_tunResourceBusy = false;

    m_tun2socksProcess = IpcClient::CreatePrivilegedProcess();
    if (!m_tun2socksProcess->waitForSource()) {
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    const QString proxyUrl = QString("socks5://%1:%2@127.0.0.1:%3").arg(m_socksUser, m_socksPassword, QString::number(m_socksPort));

    m_tun2socksProcess->setProgram(PermittedProcess::Tun2Socks);
    m_tun2socksProcess->setArguments({ "-device", QString("tun://%1").arg(tunName), "-proxy", proxyUrl });

    connect(
            m_tun2socksProcess.data(), &IpcProcessInterfaceReplica::readyReadStandardError, this, 
            [this]() {
                auto readAllStandardError = m_tun2socksProcess->readAllStandardError();
                if (!readAllStandardError.waitForFinished()) {
                    qWarning() << "Failed to read output from tun2socks";
                    return;
                }

                const QString line = readAllStandardError.returnValue();

                if (!line.contains("[TCP]") && !line.contains("[UDP]"))
                    qDebug() << "[tun2socks]:" << line;

                if (line.contains("resource busy"))
                    m_tunResourceBusy = true;

                if (line.contains("[STACK] tun://") && line.contains("<-> socks5://") && !m_connectivityProbeStarted) {
                    m_connectivityProbeStarted = true;
                    disconnect(m_tun2socksProcess.data(), &IpcProcessInterfaceReplica::readyReadStandardOutput, this, nullptr);
                    disconnect(m_tun2socksProcess.data(), &IpcProcessInterfaceReplica::readyReadStandardError, this, nullptr);

                    runConnectivityProbe([this](bool ok) {
                        if (!ok) {
                            qCritical() << "Xray connectivity probe failed: no traffic flows through the tunnel";
                            stop();
                            setLastError(ErrorCode::XrayConnectivityCheckFailed);
                            return;
                        }

                        if (ErrorCode res = setupRouting(); res != ErrorCode::NoError) {
                            stop();
                            setLastError(res);
                        } else {
                            stopTimeoutTimer();
                            setConnectionState(Vpn::ConnectionState::Connected);
                            startLivenessMonitor();
                        }
                    });
                }
            },
            Qt::QueuedConnection);

    connect(
            m_tun2socksProcess.data(), &IpcProcessInterfaceReplica::finished, this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                const bool resourceBusy = m_tunResourceBusy;

                if (resourceBusy && m_tun2socksRetryCount < maxTun2SocksRetries) {
                    m_tun2socksRetryCount++;
                    qWarning() << QString("Tun2socks: TUN resource busy, retrying (%1/%2) in %3ms...")
                                      .arg(m_tun2socksRetryCount)
                                      .arg(maxTun2SocksRetries)
                                      .arg(tun2socksRetryDelayMs);
                    QTimer::singleShot(tun2socksRetryDelayMs, this, [this]() {
                        if (ErrorCode err = startTun2Socks(); err != ErrorCode::NoError) {
                            stop();
                            setLastError(err);
                        }
                    });
                    return;
                }

                m_tun2socksRetryCount = 0;

                if (exitStatus == QProcess::ExitStatus::CrashExit) {
                    qCritical() << "Tun2socks process crashed!";
                } else {
                    qCritical() << QString("Tun2socks process was closed with %1 exit code").arg(exitCode);
                }
                stop();
                setLastError(ErrorCode::Tun2SockExecutableCrashed);
            },
            Qt::QueuedConnection);

    m_tun2socksProcess->start();
    return ErrorCode::NoError;
}

ErrorCode XrayProtocol::setupRouting()
{
    return IpcClient::withInterface(
            [this](QSharedPointer<IpcInterfaceReplica> iface) -> ErrorCode {
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
                for (auto &netInterface : netInterfaces) {
                    for (auto &address : netInterface.addressEntries()) {
                        if (m_vpnLocalAddress == address.ip().toString())
                            vpnAdapterIndex = netInterface.index();
                    }
                }
#else
                static const int vpnAdapterIndex = 0;
#endif
                const bool killSwitchEnabled = QVariant(m_rawConfig.value(configKey::killSwitchOption).toString()).toBool();
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

                if (m_routeMode == amnezia::RouteMode::VpnAllSites) {
                    static const QStringList subnets = { "1.0.0.0/8",  "2.0.0.0/7",  "4.0.0.0/6",  "8.0.0.0/5",
                                                         "16.0.0.0/4", "32.0.0.0/3", "64.0.0.0/2", "128.0.0.0/1" };

                    auto routeAddList = iface->routeAddList(m_vpnGateway, subnets);
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
            []() { return ErrorCode::AmneziaServiceConnectionFailed; });
}

bool XrayProtocol::probeServerReachable()
{
    if (m_remoteAddress.isEmpty() || m_serverPort <= 0) {
        qWarning() << "XrayProtocol: skipping server reachability probe (address/port unknown)";
        return true;
    }

    QTcpSocket sock;
    sock.connectToHost(m_remoteAddress, static_cast<quint16>(m_serverPort));
    const bool ok = sock.waitForConnected(m_serverProbeTimeoutMs);
    if (!ok) {
        qWarning() << "XrayProtocol: server" << m_remoteAddress << ":" << m_serverPort
                   << "unreachable:" << sock.errorString();
    }
    sock.abort();
    return ok;
}

void XrayProtocol::runConnectivityProbe(std::function<void(bool)> onResult)
{
    if (m_remoteAddress.isEmpty() || m_serverPort <= 0) {
        qWarning() << "XrayProtocol: connectivity probe skipped (server address/port unknown)";
        onResult(true);
        return;
    }

    auto *sock = new QTcpSocket(this);

    QNetworkProxy proxy(QNetworkProxy::Socks5Proxy, QStringLiteral("127.0.0.1"),
                        static_cast<quint16>(m_socksPort), m_socksUser, m_socksPassword);
    proxy.setCapabilities(QNetworkProxy::TunnelingCapability | QNetworkProxy::HostNameLookupCapability);
    sock->setProxy(proxy);

    auto *timeout = new QTimer(this);
    timeout->setSingleShot(true);

    auto done = QSharedPointer<bool>::create(false);

    auto finish = [=](bool ok) {
        if (*done)
            return;
        *done = true;
        timeout->stop();
        timeout->deleteLater();
        sock->abort();
        sock->deleteLater();
        onResult(ok);
    };

    connect(sock, &QTcpSocket::connected, this, [=]() { finish(true); });
    connect(sock, &QAbstractSocket::errorOccurred, this, [=](QAbstractSocket::SocketError) { finish(false); });
    connect(timeout, &QTimer::timeout, this, [=]() { finish(false); });

    timeout->start(m_connectivityProbeTimeoutMs);
    sock->connectToHost(m_remoteAddress, static_cast<quint16>(m_serverPort));
}

void XrayProtocol::startLivenessMonitor()
{
    if (!m_livenessTimer) {
        m_livenessTimer = new QTimer(this);
        connect(m_livenessTimer, &QTimer::timeout, this, [this]() {
            if (connectionState() != Vpn::ConnectionState::Connected)
                return;

            runConnectivityProbe([this](bool ok) {
                if (connectionState() != Vpn::ConnectionState::Connected)
                    return;

                if (ok) {
                    m_livenessFailures = 0;
                } else if (++m_livenessFailures >= m_maxLivenessFailures) {
                    qCritical() << "XrayProtocol: liveness check failed" << m_livenessFailures
                                << "times in a row, the tunnel is dead";
                    stop();
                    setLastError(ErrorCode::XrayConnectionLost);
                } else {
                    qWarning() << "XrayProtocol: liveness check failed (" << m_livenessFailures << "/"
                               << m_maxLivenessFailures << ")";
                }
            });
        });
    }

    m_livenessFailures = 0;
    m_livenessTimer->start(m_livenessIntervalMs);
}

void XrayProtocol::stopLivenessMonitor()
{
    if (m_livenessTimer)
        m_livenessTimer->stop();
    m_livenessFailures = 0;
}
