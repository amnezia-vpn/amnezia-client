#include "vpnTrafficGuard.h"

#include <QDebug>
#include <QEventLoop>
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
#include "core/utils/constants/protocolConstants.h"
#include "core/tunnel.h"
#include "mozilla/localsocketcontroller.h"

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

bool VpnTrafficGuard::allowEndpoint(const QString &remoteAddress, const QString &ifname)
{
#ifdef AMNEZIA_DESKTOP
    if (remoteAddress.isEmpty()) {
        return false;
    }
    if (m_allowedEndpoints.contains(remoteAddress)) {
        return true;
    }
    m_allowedEndpoints.append(remoteAddress);
    return IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        QRemoteObjectPendingReply<bool> reply = iface->addKillSwitchAllowedRange(ifname, QStringList(remoteAddress));
        return reply.waitForFinished(1000) && reply.returnValue();
    });
#else
    Q_UNUSED(remoteAddress)
    Q_UNUSED(ifname)
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
            if (!protocol) {
                qWarning() << "VpnTrafficGuard::setupRoutes: protocol is null";
                return;
            }
            QString dns1 = vpnConfiguration.value(configKey::dns1).toString();
            QString dns2 = vpnConfiguration.value(configKey::dns2).toString();
            const QString xrayIfname = vpnConfiguration.value("ifname").toString();
#ifdef Q_OS_MACOS
            if (!m_appSettingsRepository->isSitesSplitTunnelingEnabled() || m_appSettingsRepository->routeMode() != amnezia::RouteMode::VpnAllExceptSites) {
                iface->routeAddListVia(xrayIfname, protocol->vpnGateway(), QStringList() << dns1 << dns2);
            }
#else
            iface->routeAddListVia(xrayIfname, protocol->vpnGateway(), QStringList() << dns1 << dns2);
#endif

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
                    iface->routeAddListVia(xrayIfname, protocol->vpnGateway(), QStringList() << "0.0.0.0/1" << "128.0.0.0/1");

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

void VpnTrafficGuard::finishFirewallHandover(Tunnel* tunnel)
{
#if defined(AMNEZIA_DESKTOP) && defined(Q_OS_WIN)
    if (!tunnel) return;
    const QString handoverIfname = tunnel->handoverIfname();
    if (handoverIfname.isEmpty() || handoverIfname == tunnel->ifname()) {
        tunnel->clearHandoverIfname();
        return;
    }
    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        iface->disableKillSwitchForTunnel(handoverIfname, QStringList());
    });
    tunnel->clearHandoverIfname();
#else
    Q_UNUSED(tunnel)
#endif
}

void VpnTrafficGuard::applyKillSwitch(Tunnel* tunnel, const QString &gateway, const QString &localAddress)
{
#ifdef AMNEZIA_DESKTOP
    finishFirewallHandover(tunnel);

    QJsonObject updatedConfig = m_config;
    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
#ifdef Q_OS_WIN
        const QString ifname = updatedConfig.value("ifname").toString();
        if (!ifname.isEmpty()) {
            updatedConfig.insert("vpnGateway", gateway);
            updatedConfig.insert("vpnServer", NetworkUtilities::getIPAddress(updatedConfig.value(configKey::hostName).toString()));
            if (QVariant(updatedConfig.value(configKey::killSwitchOption).toString()).toBool()) {
                iface->enableKillSwitch(updatedConfig, 0);
            }
            iface->enablePeerTraffic(updatedConfig);
        } else {
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
        }
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
        if (QVariant(updatedConfig.value(configKey::killSwitchOption).toString()).toBool()) {
            updatedConfig.insert("vpnServer",
                                NetworkUtilities::getIPAddress(updatedConfig.value(amnezia::configKey::hostName).toString()));
            QRemoteObjectPendingReply<bool> reply = iface->enableKillSwitch(updatedConfig, 0);
            //TODO: why it takes so long?
            if (!reply.waitForFinished(1000) || !reply.returnValue()) {
                qWarning() << "VpnTrafficGuard::applyKillSwitch: Failed to enable killswitch";
            } else {
                qDebug() << "VpnTrafficGuard::applyKillSwitch: Successfully enabled killswitch";
            }
        }
#endif
        const QString proto = updatedConfig.value(configKey::vpnProto).toString();
        const bool isXrayBased = (proto == ProtocolUtils::protoToString(Proto::Xray) ||
                                  proto == ProtocolUtils::protoToString(Proto::SSXray));
        if (isXrayBased) {
            if (updatedConfig.value(configKey::splitTunnelType).toInt() == amnezia::route_mode_ns::VpnAllSites) {
                static const QStringList subnets = { "1.0.0.0/8", "2.0.0.0/7", "4.0.0.0/6", "8.0.0.0/5", "16.0.0.0/4", "32.0.0.0/3", "64.0.0.0/2", "128.0.0.0/1" };
                const QString xrayIfname = tunnel ? tunnel->ifname() : QString();
                auto routeAddList = iface->routeAddListVia(xrayIfname, gateway, subnets);
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

void VpnTrafficGuard::flushAll()
{
#ifdef AMNEZIA_DESKTOP
    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        iface->restoreTunnelResolvers();
        QRemoteObjectPendingReply<bool> reply = iface->disableKillSwitch();
        m_allowedEndpoints.clear();
        //TODO: why it takes so long?
        if (!reply.waitForFinished(1000) || !reply.returnValue()) {
            qWarning() << "VpnTrafficGuard::flushAll: Failed to disable killswitch";
        } else {
            qDebug() << "VpnTrafficGuard::flushAll: Successfully disabled killswitch";
        }
        auto flushDns = iface->flushDns();
        if (flushDns.waitForFinished() && flushDns.returnValue())
            qDebug() << "VpnTrafficGuard::flushAll: Successfully flushed DNS";
        else
            qWarning() << "VpnTrafficGuard::flushAll: Failed to flush DNS";

        auto clearSavedRoutes = iface->clearSavedRoutes();
        if (clearSavedRoutes.waitForFinished() && clearSavedRoutes.returnValue())
            qDebug() << "VpnTrafficGuard::flushAll: Successfully cleared saved routes";
        else
            qWarning() << "VpnTrafficGuard::flushAll: Failed to clear saved routes";
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

namespace {
QStringList allowedIpPrefixesFor(const QJsonObject& activateJson)
{
    QStringList prefixes;
    const QJsonArray ranges = activateJson.value("allowedIPAddressRanges").toArray();
    for (const QJsonValue& v : ranges) {
        const QJsonObject r = v.toObject();
        const QString addr = r.value("address").toString();
        if (addr.isEmpty()) continue;
        prefixes.append(QStringLiteral("%1/%2").arg(addr).arg(r.value("range").toInt()));
    }
    return prefixes;
}

QStringList excludedAddressesFor(const QJsonObject& activateJson)
{
    QStringList addrs;
    const QJsonArray excluded = activateJson.value("excludedAddresses").toArray();
    for (const QJsonValue& v : excluded) {
        const QString s = v.toString();
        if (!s.isEmpty()) addrs.append(s);
    }
    return addrs;
}

QStringList resolversFor(const QJsonObject& activateJson)
{
    QStringList dns;
    const QString primary = activateJson.value("primaryDnsServer").toString();
    if (!primary.isEmpty()) dns.append(primary);
    const QString secondary = activateJson.value("secondaryDnsServer").toString();
    if (!secondary.isEmpty()) dns.append(secondary);
    return dns;
}
}

void VpnTrafficGuard::reserve(Tunnel* tunnel)
{
    if (!tunnel) return;
#ifdef AMNEZIA_DESKTOP
    allowEndpoint(tunnel->remoteAddress(), tunnel->ifname());
#else
    Q_UNUSED(tunnel)
#endif
}

void VpnTrafficGuard::release(Tunnel* tunnel)
{
    if (!tunnel) return;
    disconnect(tunnel, nullptr, this, nullptr);
#ifdef AMNEZIA_DESKTOP
    m_allowedEndpoints.removeAll(tunnel->remoteAddress());
    IpcClient::withInterface([this, &tunnel](QSharedPointer<IpcInterfaceReplica> iface) {
        iface->disableKillSwitchForTunnel(tunnel->ifname(), m_allowedEndpoints);
    });
#else
    Q_UNUSED(tunnel)
#endif
}

void VpnTrafficGuard::applyPolicy(Tunnel* tunnel)
{
    if (!tunnel) return;
#ifdef AMNEZIA_DESKTOP
    const QString ifname = tunnel->ifname();

    if (VpnProtocol::isXrayBased(tunnel->container())) {
        const QJsonObject cfg = tunnel->config();
        const QString primary = cfg.value(amnezia::configKey::dns1).toString();
        const QString secondary = cfg.value(amnezia::configKey::dns2).toString();
        QList<QHostAddress> dns;
        if (!primary.isEmpty()) dns.append(QHostAddress(primary));
        if (!secondary.isEmpty() && secondary != primary) dns.append(QHostAddress(secondary));

        IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
            auto updateRes = iface->updateResolvers(ifname, dns);
            if (!updateRes.waitForFinished() || !updateRes.returnValue()) {
                qWarning() << "VpnTrafficGuard::applyPolicy: updateResolvers failed for" << ifname;
            }
#ifdef Q_OS_MAC
            const auto gw = NetworkUtilities::getGatewayAndIface();
            const QString uplinkIface = gw.second.name();
            const QString uplinkGateway = gw.first;
            if (!uplinkIface.isEmpty() && !uplinkGateway.isEmpty()) {
                auto add = iface->xrayAddUplinkRoutes(uplinkIface, uplinkGateway);
                if (!add.waitForFinished() || !add.returnValue()) {
                    qWarning() << "VpnTrafficGuard::applyPolicy: xrayAddUplinkRoutes failed on" << uplinkIface;
                }
            }
#endif
        });
        return;
    }

    const QJsonObject activate = LocalSocketController::buildActivateJson(tunnel->config(), tunnel->ifname());
    const QStringList prefixes = allowedIpPrefixesFor(activate);
    const QStringList excluded = excludedAddressesFor(activate);
    const QStringList dns = resolversFor(activate);
    const QString peer = tunnel->remoteAddress();

    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        if (!peer.isEmpty()) iface->addExclusionRoute(ifname, peer);
        for (const QString& addr : excluded) {
            iface->addExclusionRoute(ifname, addr);
        }
        for (const QString& prefix : prefixes) {
            iface->addAllowedIp(ifname, prefix);
        }
        iface->setTunnelResolvers(ifname, dns);
        iface->flushDns();
    });
#else
    Q_UNUSED(tunnel)
#endif
}

void VpnTrafficGuard::revokePolicy(Tunnel* tunnel)
{
    if (!tunnel) return;
#ifdef AMNEZIA_DESKTOP
    const QString ifname = tunnel->ifname();

    if (VpnProtocol::isXrayBased(tunnel->container())) {
        IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
            iface->restoreResolvers();
#ifdef Q_OS_MAC
            const auto gw = NetworkUtilities::getGatewayAndIface();
            const QString uplinkIface = gw.second.name();
            const QString uplinkGateway = gw.first;
            if (!uplinkIface.isEmpty()) {
                iface->xrayRemoveUplinkRoutes(uplinkIface, uplinkGateway);
            }
#endif
        });
        return;
    }

    const QJsonObject activate = LocalSocketController::buildActivateJson(tunnel->config(), tunnel->ifname());
    const QStringList prefixes = allowedIpPrefixesFor(activate);
    const QStringList excluded = excludedAddressesFor(activate);
    const QString peer = tunnel->remoteAddress();

    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        for (const QString& prefix : prefixes) {
            iface->delAllowedIp(ifname, prefix);
        }
        for (const QString& addr : excluded) {
            iface->delExclusionRoute(ifname, addr);
        }
        if (!peer.isEmpty()) iface->delExclusionRoute(ifname, peer);
    });
#else
    Q_UNUSED(tunnel)
#endif
}

void VpnTrafficGuard::bringUp(Tunnel* tunnel)
{
    if (!tunnel) return;
    reserve(tunnel);
    tunnel->prepare();
}

void VpnTrafficGuard::commit(Tunnel* tunnel)
{
    if (!tunnel) return;

#ifdef Q_OS_WIN
    const QString ipv4 = tunnel->config().value(QStringLiteral("deviceIpv4Address")).toString();
    const QString ipv6 = tunnel->config().value(QStringLiteral("deviceIpv6Address")).toString();
    if (!ipv4.isEmpty() || !ipv6.isEmpty()) {
        IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
            auto ap = iface->applyAdapterAddress(tunnel->ifname(), ipv4, ipv6);
            if (!ap.waitForFinished(15000) || !ap.returnValue()) {
                qWarning() << "VpnTrafficGuard::commit: applyAdapterAddress failed for"
                           << tunnel->ifname();
            }
        });
    }
#endif

    applyPolicy(tunnel);
    connect(tunnel, &Tunnel::activated, this, [this, tunnel] {
        if (auto p = tunnel->protocol()) {
            applyKillSwitch(tunnel, p->vpnGateway(), p->vpnLocalAddress());
        }
    });
#ifdef Q_OS_WIN
    connect(tunnel, &Tunnel::addressesUpdated, this,
            [this, tunnel](const QString& gw, const QString& la) {
        applyKillSwitch(tunnel, gw, la);
    });
#endif
    tunnel->commit();
}

void VpnTrafficGuard::tearDown(Tunnel* tunnel)
{
    if (!tunnel) return;
    revokePolicy(tunnel);
    release(tunnel);
    tunnel->deactivate();
}

void VpnTrafficGuard::swap(Tunnel* from, Tunnel* to)
{
    if (!to) return;
    if (from) {
        to->setHandoverIfname(from->ifname());
    }

#ifdef Q_OS_WIN
    if (from) {
        const QString fromIpv4 = from->config().value(QStringLiteral("deviceIpv4Address")).toString();
        const QString fromIpv6 = from->config().value(QStringLiteral("deviceIpv6Address")).toString();
        if (!fromIpv4.isEmpty() || !fromIpv6.isEmpty()) {
            IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
                auto rm = iface->removeAdapterAddress(from->ifname(), fromIpv4, fromIpv6);
                if (!rm.waitForFinished(2000) || !rm.returnValue()) {
                    qWarning() << "VpnTrafficGuard::swap: removeAdapterAddress failed for"
                               << from->ifname();
                }
            });
        }
    }
#endif

    QEventLoop loop;
    QTimer guard;
    guard.setSingleShot(true);
    const auto activatedConn = connect(to, &Tunnel::activated, &loop, &QEventLoop::quit);
    const auto failedConn = connect(to, &Tunnel::failed, &loop, [&loop](amnezia::ErrorCode) { loop.quit(); });
    const auto timeoutConn = connect(&guard, &QTimer::timeout, &loop, [&loop]() {
        qWarning() << "VpnTrafficGuard::swap: timed out waiting for new tunnel activation";
        loop.quit();
    });

    commit(to);

    guard.start(5000);
    loop.exec();
    guard.stop();

    disconnect(activatedConn);
    disconnect(failedConn);
    disconnect(timeoutConn);

    // Service IPCs are processed in order on a single connection. Wait on a trailing
    // sync request so the killswitch enable from the activation handlers is fully
    // applied before we deactivate the previous tunnel.
    IpcClient::withInterface([](QSharedPointer<IpcInterfaceReplica> iface) {
        auto reply = iface->flushDns();
        if (!reply.waitForFinished(5000) || !reply.returnValue()) {
            qWarning() << "VpnTrafficGuard::swap: trailing sync IPC timed out or failed";
        }
    });

    if (from) {
        m_allowedEndpoints.removeAll(from->remoteAddress());
#ifndef Q_OS_WIN
        IpcClient::withInterface([this](QSharedPointer<IpcInterfaceReplica> iface) {
            iface->resetKillSwitchAllowedRange(m_allowedEndpoints);
        });
#endif
        revokePolicy(from);
        from->deactivate();

#ifdef Q_OS_MAC
        if (VpnProtocol::isXrayBased(to->container())) {
            if (auto p = to->protocol()) {
                applyKillSwitch(to, p->vpnGateway(), p->vpnLocalAddress());
                setupRoutes(to->config(), p, to->remoteAddress());
            }
        }
#endif
    }
}
