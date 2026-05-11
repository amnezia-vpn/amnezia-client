// SPDX-License-Identifier: GPL-3.0-or-later

#include "masterDnsVpnProtocol.h"

#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/ipcClient.h"
#include "core/utils/networkUtilities.h"
#include "ipc.h"

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QtCore/qprocess.h>

namespace {
#ifdef Q_OS_MACOS
constexpr char kTunName[] = "utun24";
#else
constexpr char kTunName[] = "tun3";
#endif
} // namespace

MasterDnsVpnProtocol::MasterDnsVpnProtocol(const QJsonObject &configuration, QObject *parent)
    : VpnProtocol(configuration, parent)
{
    m_vpnGateway = amnezia::protocols::masterDnsVpn::defaultLocalAddr;
    m_vpnLocalAddress = amnezia::protocols::masterDnsVpn::defaultLocalAddr;
    m_routeGateway = NetworkUtilities::getGatewayAndIface().first;

    m_routeMode = static_cast<amnezia::RouteMode>(
            configuration.value(amnezia::configKey::splitTunnelType).toInt());
    m_remoteAddress = NetworkUtilities::getIPAddress(
            m_rawConfig.value(amnezia::configKey::hostName).toString());

    const QString primaryDns = configuration.value(amnezia::configKey::dns1).toString();
    if (!primaryDns.isEmpty()) {
        m_dnsServers.push_back(QHostAddress(primaryDns));
    }
    if (primaryDns != amnezia::protocols::dns::amneziaDnsIp) {
        const QString secondaryDns = configuration.value(amnezia::configKey::dns2).toString();
        if (!secondaryDns.isEmpty()) {
            m_dnsServers.push_back(QHostAddress(secondaryDns));
        }
    }

    // The wrapped JSON the model stores is the full MasterDnsVpnProtocolConfig
    // serialised form; the engine wants just the inner per-session blob.
    // We pass the whole object through — the engine ignores keys it doesn't
    // recognise (additionalConfig is preserved for future use).
    m_engineConfig =
            configuration.value(ProtocolUtils::key_proto_config_data(Proto::MasterDnsVpn))
                    .toObject();
}

MasterDnsVpnProtocol::~MasterDnsVpnProtocol()
{
    qDebug() << "MasterDnsVpnProtocol::~MasterDnsVpnProtocol()";
    MasterDnsVpnProtocol::stop();
}

ErrorCode MasterDnsVpnProtocol::start()
{
    qDebug() << "MasterDnsVpnProtocol::start()";

    if (m_engineConfig.isEmpty()) {
        qCritical() << "MasterDnsVpn config wrapper is empty";
        return ErrorCode::InternalError;
    }

    const QString configJson = QString::fromUtf8(
            QJsonDocument(m_engineConfig).toJson(QJsonDocument::Compact));

    return IpcClient::withInterface(
            [&](QSharedPointer<IpcInterfaceReplica> iface) {
                auto startReply = iface->masterDnsVpnStart(configJson);
                if (!startReply.waitForFinished() || !startReply.returnValue()) {
                    qCritical() << "Failed to start MasterDnsVpn engine in service";
                    return ErrorCode::InternalError;
                }

                auto portReply = iface->masterDnsVpnSocksPort();
                if (!portReply.waitForFinished()) {
                    qCritical() << "Failed to fetch MasterDnsVpn SOCKS5 port";
                    return ErrorCode::InternalError;
                }
                const quint16 socksPort = portReply.returnValue();
                if (socksPort == 0) {
                    qCritical() << "MasterDnsVpn engine did not bind a SOCKS5 port";
                    return ErrorCode::InternalError;
                }

                return startTun2Socks(socksPort);
            },
            []() { return ErrorCode::AmneziaServiceConnectionFailed; });
}

void MasterDnsVpnProtocol::stop()
{
    qDebug() << "MasterDnsVpnProtocol::stop()";

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

        auto deleteTun = iface->deleteTun(kTunName);
        if (!deleteTun.waitForFinished() || !deleteTun.returnValue())
            qWarning() << "Failed to delete tun";

        auto stopReply = iface->masterDnsVpnStop();
        if (!stopReply.waitForFinished() || !stopReply.returnValue())
            qWarning() << "Failed to stop MasterDnsVpn engine in service";
    });

    if (m_tun2socksProcess) {
        m_tun2socksProcess->blockSignals(true);
#ifndef Q_OS_WIN
        m_tun2socksProcess->terminate();
        auto waitForFinished = m_tun2socksProcess->waitForFinished(1000);
        if (!waitForFinished.waitForFinished() || !waitForFinished.returnValue()) {
            qWarning() << "Failed to terminate tun2socks; killing the process";
            m_tun2socksProcess->kill();
        }
#else
        m_tun2socksProcess->kill();
#endif
        m_tun2socksProcess->close();
        m_tun2socksProcess.reset();
    }

    setConnectionState(Vpn::ConnectionState::Disconnected);
}

ErrorCode MasterDnsVpnProtocol::startTun2Socks(quint16 socksPort)
{
    m_tun2socksProcess = IpcClient::CreatePrivilegedProcess();
    if (!m_tun2socksProcess->waitForSource()) {
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    const QString proxyUrl = QStringLiteral("socks5://127.0.0.1:%1").arg(socksPort);

    m_tun2socksProcess->setProgram(amnezia::PermittedProcess::Tun2Socks);
    m_tun2socksProcess->setArguments({ QStringLiteral("-device"),
                                       QStringLiteral("tun://%1").arg(kTunName),
                                       QStringLiteral("-proxy"), proxyUrl });

    connect(m_tun2socksProcess.data(), &IpcProcessInterfaceReplica::readyReadStandardError, this,
            [this]() {
                auto readAllStandardError = m_tun2socksProcess->readAllStandardError();
                if (!readAllStandardError.waitForFinished()) {
                    return;
                }
                const QString line = readAllStandardError.returnValue();
                if (!line.contains("[TCP]") && !line.contains("[UDP]")) {
                    qDebug() << "[tun2socks]:" << line;
                }
                if (line.contains("[STACK] tun://") && line.contains("<-> socks5://")) {
                    disconnect(m_tun2socksProcess.data(),
                               &IpcProcessInterfaceReplica::readyReadStandardOutput, this, nullptr);

                    if (ErrorCode res = setupRouting(); res != ErrorCode::NoError) {
                        stop();
                        setLastError(res);
                    } else {
                        setConnectionState(Vpn::ConnectionState::Connected);
                    }
                }
            },
            Qt::QueuedConnection);

    connect(m_tun2socksProcess.data(), &IpcProcessInterfaceReplica::finished, this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitStatus == QProcess::ExitStatus::CrashExit) {
                    qCritical() << "Tun2socks crashed (mdnsvpn flow)";
                } else {
                    qCritical() << "Tun2socks exited with" << exitCode << "(mdnsvpn flow)";
                }
                stop();
                setLastError(ErrorCode::Tun2SockExecutableCrashed);
            },
            Qt::QueuedConnection);

    m_tun2socksProcess->start();
    return ErrorCode::NoError;
}

ErrorCode MasterDnsVpnProtocol::setupRouting()
{
    return IpcClient::withInterface(
            [this](QSharedPointer<IpcInterfaceReplica> iface) -> ErrorCode {
#ifdef Q_OS_WIN
                const int inetAdapterIndex =
                        NetworkUtilities::AdapterIndexTo(QHostAddress(m_remoteAddress));
#endif
                auto createTun = iface->createTun(kTunName,
                                                  amnezia::protocols::masterDnsVpn::defaultLocalAddr);
                if (!createTun.waitForFinished() || !createTun.returnValue()) {
                    qCritical() << "Failed to assign IP address for TUN";
                    return ErrorCode::InternalError;
                }

                auto updateResolvers = iface->updateResolvers(kTunName, m_dnsServers);
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
                const bool killSwitchEnabled = QVariant(
                        m_rawConfig.value(amnezia::configKey::killSwitchOption).toString())
                        .toBool();
                if (killSwitchEnabled) {
                    if (vpnAdapterIndex != -1) {
                        QJsonObject config = m_rawConfig;
                        config.insert("vpnServer", m_remoteAddress);
                        auto enableKillSwitch =
                                IpcClient::Interface()->enableKillSwitch(config, vpnAdapterIndex);
                        if (!enableKillSwitch.waitForFinished() || !enableKillSwitch.returnValue()) {
                            qCritical() << "Failed to enable killswitch";
                            return ErrorCode::InternalError;
                        }
                    } else {
                        qWarning() << "Failed to get vpnAdapterIndex. Killswitch disabled";
                    }
                }

                if (m_routeMode == amnezia::RouteMode::VpnAllSites) {
                    static const QStringList subnets = { "1.0.0.0/8",  "2.0.0.0/7",  "4.0.0.0/6",
                                                         "8.0.0.0/5",  "16.0.0.0/4", "32.0.0.0/3",
                                                         "64.0.0.0/2", "128.0.0.0/1" };
                    auto routeAddList = iface->routeAddList(m_vpnGateway, subnets);
                    if (!routeAddList.waitForFinished()
                        || routeAddList.returnValue() != subnets.count()) {
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
                } else {
                    qWarning() << "Failed to get adapter indexes. Split-tunneling disabled";
                }
#endif
                return ErrorCode::NoError;
            },
            []() { return ErrorCode::AmneziaServiceConnectionFailed; });
}
