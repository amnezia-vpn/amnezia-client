#include "vpnTrafficGuard.h"

#include <QDebug>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QPointer>
#include <QStringList>
#include <QTimer>
#include <QJsonObject>

#ifdef AMNEZIA_DESKTOP
    #include "core/utils/ipcClient.h"
#endif

#include "core/utils/networkUtilities.h"

VpnTrafficGuard::VpnTrafficGuard(SecureAppSettingsRepository* appSettings, QObject *parent)
    : QObject(parent), m_appSettingsRepository(appSettings)
{

}

VpnTrafficGuard::~VpnTrafficGuard()
{
}

void VpnTrafficGuard::setConfig(const QJsonObject &config)
{
    m_config = config;
}

bool VpnTrafficGuard::allowEndpoint(const QString &remoteAddress)
{
#ifdef AMNEZIA_DESKTOP
    if (remoteAddress.isEmpty()) {
        return false;
    }
    return IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        QRemoteObjectPendingReply<bool> reply = iface->addKillSwitchAllowedRange(QStringList(remoteAddress));
        return reply.waitForFinished(1000) && reply.returnValue();
    });
#else
    Q_UNUSED(remoteAddress)
    return true;
#endif
}

void VpnTrafficGuard::setupRoutes(const QJsonObject &vpnConfiguration, const QSharedPointer<VpnProtocol> &protocol, const QString &remoteAddress)
{
#ifdef AMNEZIA_DESKTOP
    if (!m_appSettingsRepository) {
        qCritical() << "VpnTrafficGuard::setupRoutes: repositories not initialized";
        return;
    }
    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        iface->resetIpStack();
        auto flushDns = iface->flushDns();
        if (flushDns.waitForFinished() && flushDns.returnValue())
            qDebug() << "VpnTrafficGuard::setupRoutes: Successfully flushed DNS";
        else
            qWarning() << "VpnTrafficGuard::setupRoutes: Failed to flush DNS";

        const QString proto = vpnConfiguration.value(configKey::vpnProto).toString();
        const bool isWgBased = (proto == ProtocolUtils::protoToString(Proto::Awg) ||
                                proto == ProtocolUtils::protoToString(Proto::WireGuard));
        if (!isWgBased) {
            QString dns1 = vpnConfiguration.value(configKey::dns1).toString();
            QString dns2 = vpnConfiguration.value(configKey::dns2).toString();
#ifdef Q_OS_MACOS
            if (!m_appSettingsRepository->isSitesSplitTunnelingEnabled() || m_appSettingsRepository->routeMode() != amnezia::RouteMode::VpnAllExceptSites) {
                iface->routeAddList(protocol->vpnGateway(), QStringList() << dns1 << dns2);
            }
#else
            iface->routeAddList(protocol->vpnGateway(), QStringList() << dns1 << dns2);
#endif
            // TODO: add error code handling for all routeAddList (or rework the code below)
            if (!protocol) {
                qWarning() << "VpnTrafficGuard::setupRoutes: protocol is null";
                return;
            }
            iface->routeAddList(protocol->vpnGateway(), QStringList() << dns1 << dns2);

            if (m_appSettingsRepository->isSitesSplitTunnelingEnabled()) {
                iface->routeDeleteList(protocol->vpnGateway(), QStringList() << "0.0.0.0");
                if (m_appSettingsRepository->routeMode() == amnezia::route_mode_ns::VpnOnlyForwardSites) {
                    QPointer<VpnProtocol> protocolPtr(protocol.data());
                    QTimer::singleShot(1000, protocol.data(),
                                        [this, protocolPtr]() {
                                                if (!protocolPtr) {
                                                    return;
                                                }
                                                addSplitTunnelRoutes(protocolPtr->vpnGateway(), m_appSettingsRepository->routeMode());
                                            });
                } else if (m_appSettingsRepository->routeMode() == amnezia::route_mode_ns::VpnAllExceptSites) {
                    iface->routeAddList(protocol->vpnGateway(), QStringList() << "0.0.0.0/1");
                    iface->routeAddList(protocol->vpnGateway(), QStringList() << "128.0.0.0/1");

                    iface->routeAddList(protocol->routeGateway(), QStringList() << remoteAddress);
#ifdef Q_OS_MACOS
                    iface->routeAddList(protocol->routeGateway(), QStringList() << dns1 << dns2);
#endif
                    addSplitTunnelRoutes(protocol->routeGateway(), m_appSettingsRepository->routeMode());
                }
            }
        }
    });
#endif
}

void VpnTrafficGuard::addSplitTunnelRoutes(const QString &gw, amnezia::RouteMode mode)
{
#ifdef AMNEZIA_DESKTOP
    if (!m_appSettingsRepository) {
        qCritical() << "VpnTrafficGuard::addSplitTunnelRoutes: repositories not initialized";
        return;
    }

    QStringList ips;
    QStringList sites;
    const QVariantMap &m = m_appSettingsRepository->vpnSites(mode);
    for (auto i = m.constBegin(); i != m.constEnd(); ++i) {
        if (NetworkUtilities::checkIpSubnetFormat(i.key())) {
            ips.append(i.key());
        } else {
            if (NetworkUtilities::checkIpSubnetFormat(i.value().toString())) {
                ips.append(i.value().toString());
            }
            sites.append(i.key());
        }
    }
    ips.removeDuplicates();

    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        iface->routeAddList(gw, ips);
    });

    // re-resolve domains
    for (const QString &site : sites) {
        const auto &cbResolv = [this, site, gw, mode, ips](const QHostInfo &hostInfo) {
            for (const QHostAddress &addr : hostInfo.addresses()) {
                if (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
                    const QString &ip = addr.toString();
                    if (!ips.contains(ip)) {
                        IpcClient::withInterface([gw, ip](QSharedPointer<IpcInterfaceReplica> iface) {
                            iface->routeAddList(gw, QStringList() << ip);
                        });
                        m_appSettingsRepository->addVpnSite(mode, site, ip);
                    }
                    IpcClient::withInterface([](QSharedPointer<IpcInterfaceReplica> iface) {
                        auto reply = iface->flushDns();
                        if (!reply.waitForFinished() || !reply.returnValue())
                            qWarning() << "VpnTrafficGuard::addSplitTunnelRoutes: Failed to flush DNS";
                    });
                    break;
                }
            }
        };
        QHostInfo::lookupHost(site, this, cbResolv);
    }
#endif
}

void VpnTrafficGuard::applyFirewall(const QString &gateway, const QString &localAddress)
{
#ifdef AMNEZIA_DESKTOP
    QJsonObject updatedConfig = m_config;
    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
#ifdef Q_OS_WIN
        QList<QNetworkInterface> netInterfaces = QNetworkInterface::allInterfaces();
        for (int i = 0; i < netInterfaces.size(); i++) {
            for (int j=0; j < netInterfaces.at(i).addressEntries().size(); j++)
            {
                if (localAddress == netInterfaces.at(i).addressEntries().at(j).ip().toString()) {
                    updatedConfig.insert("vpnAdapterIndex", netInterfaces.at(i).index());
                    updatedConfig.insert("vpnGateway", gateway);
                    updatedConfig.insert("vpnServer", NetworkUtilities::getIPAddress(updatedConfig.value(configKey::hostName).toString()));
                    if (QVariant(updatedConfig.value(configKey::killSwitchOption).toString()).toBool()) {
                        iface->enableKillSwitch(updatedConfig, netInterfaces.at(i).index());
                    }
                    iface->enablePeerTraffic(updatedConfig);
                }
            }
        }
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
        if (QVariant(updatedConfig.value(configKey::killSwitchOption).toString()).toBool()) {
            updatedConfig.insert("vpnServer",
                                NetworkUtilities::getIPAddress(updatedConfig.value(amnezia::configKey::hostName).toString()));
            QRemoteObjectPendingReply<bool> reply = iface->enableKillSwitch(updatedConfig, 0);
            //TODO: why it takes so long?
            if (!reply.waitForFinished(5000) || !reply.returnValue()) {
                qWarning() << "VpnTrafficGuard::applyFirewall: Failed to enable killswitch";
            } else {
                qDebug() << "VpnTrafficGuard::applyFirewall: Successfully enabled killswitch";
            }
        }
#endif
        const QString proto = updatedConfig.value(configKey::vpnProto).toString();
        const bool isXrayBased = (proto == ProtocolUtils::protoToString(Proto::Xray) ||
                                  proto == ProtocolUtils::protoToString(Proto::SSXray));
        if (isXrayBased) {
            if (updatedConfig.value(configKey::splitTunnelType).toInt() == amnezia::route_mode_ns::VpnAllSites) {
                static const QStringList subnets = { "1.0.0.0/8", "2.0.0.0/7", "4.0.0.0/6", "8.0.0.0/5", "16.0.0.0/4", "32.0.0.0/3", "64.0.0.0/2", "128.0.0.0/1" };
                auto routeAddList = iface->routeAddList(gateway, subnets);
                if (!routeAddList.waitForFinished() || routeAddList.returnValue() != subnets.count()) {
                    qCritical() << "Failed to set routes for TUN";
                }
            }
            auto StopRoutingIpv6 = iface->StopRoutingIpv6();
            if (!StopRoutingIpv6.waitForFinished() || !StopRoutingIpv6.returnValue()) {
                qCritical() << "Failed to disable IPv6 routing";
            } else {
                m_ipv6RoutingStopped = true;
            }
        }
    });
#endif
}

void VpnTrafficGuard::teardown()
{
#ifdef AMNEZIA_DESKTOP
    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        QRemoteObjectPendingReply<bool> reply = iface->disableKillSwitch();
        //TODO: why it takes so long?
        if (!reply.waitForFinished(5000) || !reply.returnValue()) {
            qWarning() << "VpnTrafficGuard::teardown: Failed to disable killswitch";
        } else {
            qDebug() << "VpnTrafficGuard::teardown: Successfully disabled killswitch";
        }
        auto flushDns = iface->flushDns();
        if (flushDns.waitForFinished() && flushDns.returnValue())
            qDebug() << "VpnTrafficGuard::teardown: Successfully flushed DNS";
        else
            qWarning() << "VpnTrafficGuard::teardown: Failed to flush DNS";

        auto clearSavedRoutes = iface->clearSavedRoutes();
        if (clearSavedRoutes.waitForFinished() && clearSavedRoutes.returnValue())
            qDebug() << "VpnTrafficGuard::teardown: Successfully cleared saved routes";
        else
            qWarning() << "VpnTrafficGuard::teardown: Failed to clear saved routes";
        if (m_ipv6RoutingStopped) {
            auto StartRoutingIpv6 = iface->StartRoutingIpv6();
            if (!StartRoutingIpv6.waitForFinished() || !StartRoutingIpv6.returnValue()) {
                qCritical() << "Failed to enable IPv6 routing";
            } else {
                m_ipv6RoutingStopped = false;
            }
        }
    });
#endif
}
