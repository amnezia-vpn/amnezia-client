#include "vpnConnection.h"

#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QHostInfo>
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <core/configurators/openVpnConfigurator.h>
#include <core/configurators/wireguardConfigurator.h>

#ifdef AMNEZIA_DESKTOP
    #include "core/utils/ipcClient.h"
    #include <core/protocols/wireGuardProtocol.h>
#endif

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
    #include <QThread>

#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
#endif

#include "core/utils/networkUtilities.h"
#include "core/utils/serverConfigUtils.h"
#include "vpnConnection.h"

using namespace ProtocolUtils;

#ifdef Q_OS_WIN
namespace
{
constexpr int SiteDnsLookupConcurrency = 64;
constexpr int SiteDnsLookupTimeoutMs = 15000;
}

struct VpnConnection::SiteResolutionState
{
    DockerContainer container;
    amnezia::RouteMode routeMode = amnezia::RouteMode::VpnAllSites;
    QJsonObject vpnConfiguration;
    QStringList hostnames;
    QStringList resolvedIpv4Addresses;
    QSet<int> lookupIds;
    int nextHostnameIndex = 0;
    int activeLookups = 0;
    int completedLookups = 0;
    int failedLookups = 0;
    bool finished = false;
};
#endif

VpnConnection::VpnConnection(SecureServersRepository* serversRepository, SecureAppSettingsRepository* appSettingsRepository, QObject *parent)
    : QObject(parent), m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository), m_checkTimer(this)
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    m_checkTimer.setInterval(1000);
    connect(IosController::Instance(), &IosController::connectionStateChanged, this, &VpnConnection::setConnectionState);
    connect(IosController::Instance(), &IosController::bytesChanged, this, &VpnConnection::onBytesChanged);
#endif
}

VpnConnection::~VpnConnection()
{
#ifdef Q_OS_WIN
    cancelSiteDnsResolution();
#endif
}

void VpnConnection::onBytesChanged(quint64 receivedBytes, quint64 sentBytes)
{
    emit bytesChanged(receivedBytes, sentBytes);
}

void VpnConnection::onKillSwitchModeChanged(bool enabled)
{
#ifdef AMNEZIA_DESKTOP
    IpcClient::withInterface([enabled](QSharedPointer<IpcInterfaceReplica> iface){
        QRemoteObjectPendingReply<bool> reply = iface->refreshKillSwitch(enabled);
        if (reply.waitForFinished() && reply.returnValue())
            qDebug() << "VpnConnection::onKillSwitchModeChanged: Killswitch refreshed";
        else
            qWarning() << "VpnConnection::onKillSwitchModeChanged: Failed to execute remote refreshKillSwitch call";
    });
#endif
}

void VpnConnection::onConnectionStateChanged(Vpn::ConnectionState state)
{
#ifdef AMNEZIA_DESKTOP
    if (!m_serversRepository || !m_appSettingsRepository) {
        qCritical() << "VpnConnection::onConnectionStateChanged: repositories not initialized";
        return;
    }

    const QString defaultServerId = m_serversRepository->defaultServerId();
    DockerContainer container = DockerContainer::None;
    switch (m_serversRepository->serverKind(defaultServerId)) {
    case serverConfigUtils::ConfigType::SelfHostedAdmin: {
        const auto cfg = m_serversRepository->selfHostedAdminConfig(defaultServerId);
        if (cfg.has_value()) {
            container = cfg->defaultContainer;
        }
        break;
    }
    case serverConfigUtils::ConfigType::SelfHostedUser: {
        const auto cfg = m_serversRepository->selfHostedUserConfig(defaultServerId);
        if (cfg.has_value()) {
            container = cfg->defaultContainer;
        }
        break;
    }
    case serverConfigUtils::ConfigType::Native: {
        const auto cfg = m_serversRepository->nativeConfig(defaultServerId);
        if (cfg.has_value()) {
            container = cfg->defaultContainer;
        }
        break;
    }
    case serverConfigUtils::ConfigType::AmneziaPremiumV2:
    case serverConfigUtils::ConfigType::AmneziaFreeV3:
    case serverConfigUtils::ConfigType::ExternalPremium: {
        const auto cfg = m_serversRepository->apiV2Config(defaultServerId);
        if (cfg.has_value()) {
            container = cfg->defaultContainer;
        }
        break;
    }
    case serverConfigUtils::ConfigType::AmneziaPremiumV1:
    case serverConfigUtils::ConfigType::AmneziaFreeV2:
        break;
    case serverConfigUtils::ConfigType::Invalid:
    default:
        break;
    }

    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        switch (state) {
            case Vpn::ConnectionState::Connected: {
                iface->resetIpStack();

                auto flushDns = iface->flushDns();
                if (flushDns.waitForFinished() && flushDns.returnValue())
                    qDebug() << "VpnConnection::onConnectionStateChanged: Successfully flushed DNS";
                else
                    qWarning() << "VpnConnection::onConnectionStateChanged: Failed to flush DNS";

                if (!ContainerUtils::isAwgContainer(container) && container != DockerContainer::WireGuard) {
                    const auto routeMode = static_cast<amnezia::RouteMode>(
                        m_vpnConfiguration.value(configKey::splitTunnelType).toInt());
                    QString dns1 = m_vpnConfiguration.value(configKey::dns1).toString();
                    QString dns2 = m_vpnConfiguration.value(configKey::dns2).toString();

#ifdef Q_OS_MACOS
                    if (routeMode != amnezia::RouteMode::VpnAllExceptSites) {
                        iface->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << dns1 << dns2);
                    }
#else
                    iface->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << dns1 << dns2);
#endif

                    if (routeMode != amnezia::RouteMode::VpnAllSites) {
                        iface->routeDeleteList(m_vpnProtocol->vpnGateway(), QStringList() << "0.0.0.0");
                        if (routeMode == amnezia::RouteMode::VpnOnlyForwardSites) {
                            QTimer::singleShot(1000, m_vpnProtocol.data(),
                                               [this, routeMode]() { addSitesRoutes(m_vpnProtocol->vpnGateway(), routeMode); });
                        } else if (routeMode == amnezia::RouteMode::VpnAllExceptSites) {
                            iface->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << "0.0.0.0/1");
                            iface->routeAddList(m_vpnProtocol->vpnGateway(), QStringList() << "128.0.0.0/1");

                            iface->routeAddList(m_vpnProtocol->routeGateway(), QStringList() << remoteAddress());
#ifdef Q_OS_MACOS
                            iface->routeAddList(m_vpnProtocol->routeGateway(), QStringList() << dns1 << dns2);
#endif
                            addSitesRoutes(m_vpnProtocol->routeGateway(), routeMode);
                        }
                    }
                }
            } break;
            case Vpn::ConnectionState::Disconnected:
            case Vpn::ConnectionState::Error: {
                auto flushDns = iface->flushDns();
                if (flushDns.waitForFinished() && flushDns.returnValue())
                    qDebug() << "VpnConnection::onConnectionStateChanged: Successfully flushed DNS";
                else
                    qWarning() << "VpnConnection::onConnectionStateChanged: Failed to flush DNS";

                auto clearSavedRoutes = iface->clearSavedRoutes();
                if (clearSavedRoutes.waitForFinished() && clearSavedRoutes.returnValue())
                    qDebug() << "VpnConnection::onConnectionStateChanged: Successfully cleared saved routes";
                else
                    qWarning() << "VpnConnection::onConnectionStateChanged: Failed to clear saved routes";
            } break;
            default:
                break;
        }
    });
#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    if (state == Vpn::ConnectionState::Connected ||
        state == Vpn::ConnectionState::Connecting ||
        state == Vpn::ConnectionState::Reconnecting) {
        m_checkTimer.start();
    } else {
        m_checkTimer.stop();
    }
#endif
}

const QString &VpnConnection::remoteAddress() const
{
    return m_remoteAddress;
}

void VpnConnection::setRepositories(SecureServersRepository* serversRepository, SecureAppSettingsRepository* appSettingsRepository)
{
    m_serversRepository = serversRepository;
    m_appSettingsRepository = appSettingsRepository;
}

void VpnConnection::addSitesRoutes(const QString &gw, amnezia::RouteMode mode)
{
#ifdef AMNEZIA_DESKTOP
    if (!m_appSettingsRepository) {
        qCritical() << "VpnConnection::addSitesRoutes: repositories not initialized";
        return;
    }

    QStringList ips;
    QStringList sites;
    const QVariantMap &m = m_appSettingsRepository->vpnSites(mode);
    for (auto i = m.constBegin(); i != m.constEnd(); ++i) {
        if (NetworkUtilities::checkIpSubnetFormat(i.key())) {
            ips.append(i.key());
        } else {
#ifndef Q_OS_WIN
            if (NetworkUtilities::checkIpSubnetFormat(i.value().toString())) {
                ips.append(i.value().toString());
            }
#endif
            sites.append(i.key());
        }
    }
    ips.removeDuplicates();

    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        iface->routeAddList(gw, ips);
    });

    // re-resolve domains
    for (const QString &site : sites) {
        const auto cbResolv = [this, site, gw, mode, ips](const QHostInfo &hostInfo) {
            if (hostInfo.error() != QHostInfo::NoError) {
                qWarning() << "VpnConnection::addSitesRoutes: failed to resolve" << site
                           << hostInfo.errorString();
                return;
            }

            QStringList ipv4Addresses;
            for (const QHostAddress &addr : hostInfo.addresses()) {
                if (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
                    ipv4Addresses.append(addr.toString());
                }
            }
            ipv4Addresses.removeDuplicates();

            if (ipv4Addresses.isEmpty()) {
                qWarning() << "VpnConnection::addSitesRoutes: no IPv4 address for" << site;
                return;
            }

            QStringList addressesForRoutes = ipv4Addresses;
#ifndef Q_OS_WIN
            if (addressesForRoutes.size() > 1) {
                addressesForRoutes = { addressesForRoutes.constFirst() };
            }
#endif

            QStringList addressesToAdd;
            for (const QString &address : addressesForRoutes) {
                if (!ips.contains(address)) {
                    addressesToAdd.append(address);
                }
            }
            addressesToAdd.removeDuplicates();

            if (!addressesToAdd.isEmpty()) {
                IpcClient::withInterface([gw, addressesToAdd](QSharedPointer<IpcInterfaceReplica> iface) {
                    iface->routeAddList(gw, addressesToAdd);
                });

                // Keep the existing single-IP settings format for JSON compatibility.
                m_appSettingsRepository->addVpnSite(mode, site, addressesToAdd.constFirst());
                IpcClient::withInterface([](QSharedPointer<IpcInterfaceReplica> iface) {
                    auto reply = iface->flushDns();
                    if (!reply.waitForFinished() || !reply.returnValue())
                        qWarning() << "VpnConnection::addSitesRoutes: Failed to flush DNS";
                });
            }
        };
        QHostInfo::lookupHost(site, this, cbResolv);
    }
#endif
}

QSharedPointer<VpnProtocol> VpnConnection::vpnProtocol() const
{
    return m_vpnProtocol;
}

void VpnConnection::disconnectSlots()
{
    if (m_vpnProtocol) {
        m_vpnProtocol->disconnect();
    }
}

ErrorCode VpnConnection::lastError() const
{
#ifdef Q_OS_ANDROID
    return ErrorCode::AndroidError;
#endif

    if (m_vpnProtocol.isNull()) {
        return ErrorCode::InternalError;
    }

    return m_vpnProtocol.data()->lastError();
}

Vpn::ConnectionState VpnConnection::connectionState() const
{
    return m_connectionState;
}

void VpnConnection::connectToVpn(const QString &serverId, DockerContainer container, const QJsonObject &vpnConfiguration)
{
    if (!m_appSettingsRepository || !m_serversRepository) {
        qCritical() << "VpnConnection::connectToVpn: repositories not initialized";
        setConnectionState(Vpn::ConnectionState::Error);
        return;
    }

    qDebug() << QString("Trying to connect to VPN, server id is %1, container is %2, route mode is")
                        .arg(serverId)
                        .arg(ContainerUtils::containerToString(container))
             << m_appSettingsRepository->routeMode();

    m_remoteAddress = NetworkUtilities::getIPAddress(vpnConfiguration.value(configKey::hostName).toString());
    setConnectionState(Vpn::ConnectionState::Connecting);

#ifdef Q_OS_WIN
    cancelSiteDnsResolution();
    if (m_appSettingsRepository->isSitesSplitTunnelingEnabled()
        && m_appSettingsRepository->routeMode() != amnezia::RouteMode::VpnAllSites) {
        resolveSiteAddressesAndConnect(container, vpnConfiguration);
        return;
    }
#endif

    continueConnectToVpn(container, vpnConfiguration, {}, false);
}

#ifdef Q_OS_WIN
void VpnConnection::resolveSiteAddressesAndConnect(DockerContainer container, const QJsonObject &vpnConfiguration)
{
    auto state = QSharedPointer<SiteResolutionState>::create();
    state->container = container;
    state->vpnConfiguration = vpnConfiguration;

    state->routeMode = m_appSettingsRepository->routeMode();
    const QVariantMap sites = m_appSettingsRepository->vpnSites(state->routeMode);
    for (auto it = sites.constBegin(); it != sites.constEnd(); ++it) {
        if (NetworkUtilities::checkIpSubnetFormat(it.key())) {
            state->resolvedIpv4Addresses.append(it.key());
        } else {
            state->hostnames.append(it.key());
        }
    }
    state->hostnames.removeDuplicates();
    state->resolvedIpv4Addresses.removeDuplicates();

    qDebug() << "VpnConnection: resolving site split tunneling hostnames"
             << "routeMode=" << state->routeMode
             << "hostnameCount=" << state->hostnames.size()
             << "explicitAddressCount=" << state->resolvedIpv4Addresses.size();

    m_siteResolutionState = state;

    QTimer::singleShot(SiteDnsLookupTimeoutMs, this, [this, state]() {
        if (m_siteResolutionState == state && !state->finished) {
            finishSiteDnsResolution(state, true);
        }
    });

    startSiteDnsLookups(state);
}

void VpnConnection::startSiteDnsLookups(const QSharedPointer<SiteResolutionState> &state)
{
    if (m_siteResolutionState != state || state->finished) {
        return;
    }

    while (state->activeLookups < SiteDnsLookupConcurrency
           && state->nextHostnameIndex < state->hostnames.size()) {
        const QString hostname = state->hostnames.at(state->nextHostnameIndex++);
        state->activeLookups++;

        const int lookupId = QHostInfo::lookupHost(hostname, this, [this, state](const QHostInfo &hostInfo) {
            if (m_siteResolutionState != state || state->finished) {
                return;
            }

            state->lookupIds.remove(hostInfo.lookupId());
            state->activeLookups--;
            state->completedLookups++;

            QStringList ipv4Addresses;
            for (const QHostAddress &address : hostInfo.addresses()) {
                if (address.protocol() == QAbstractSocket::IPv4Protocol) {
                    ipv4Addresses.append(address.toString());
                }
            }
            ipv4Addresses.removeDuplicates();

            if (hostInfo.error() != QHostInfo::NoError || ipv4Addresses.isEmpty()) {
                state->failedLookups++;
            } else {
                state->resolvedIpv4Addresses.append(ipv4Addresses);
            }

            startSiteDnsLookups(state);
        });
        state->lookupIds.insert(lookupId);
    }

    if (state->activeLookups == 0 && state->nextHostnameIndex >= state->hostnames.size()) {
        finishSiteDnsResolution(state, false);
    }
}

void VpnConnection::finishSiteDnsResolution(const QSharedPointer<SiteResolutionState> &state, bool timedOut)
{
    if (m_siteResolutionState != state || state->finished) {
        return;
    }

    state->finished = true;
    const QSet<int> lookupIds = state->lookupIds;
    for (int lookupId : lookupIds) {
        QHostInfo::abortHostLookup(lookupId);
    }
    state->lookupIds.clear();
    state->resolvedIpv4Addresses.removeDuplicates();

    qDebug() << "VpnConnection: site split DNS resolution finished"
             << "timedOut=" << timedOut
             << "requestedHostnames=" << state->hostnames.size()
             << "completedLookups=" << state->completedLookups
             << "failedLookups=" << state->failedLookups
             << "resolvedAddressCount=" << state->resolvedIpv4Addresses.size();

    const DockerContainer container = state->container;
    const QJsonObject vpnConfiguration = state->vpnConfiguration;
    const QStringList resolvedSiteAddresses = state->resolvedIpv4Addresses;
    m_siteResolutionState.clear();

    const bool siteSplitTunnelingEnabled = m_appSettingsRepository->isSitesSplitTunnelingEnabled();
    const auto currentRouteMode = m_appSettingsRepository->routeMode();
    if (!siteSplitTunnelingEnabled || currentRouteMode == amnezia::RouteMode::VpnAllSites) {
        qDebug() << "VpnConnection: site split tunneling disabled or switched to full tunnel during DNS resolution";
        continueConnectToVpn(container, vpnConfiguration, {}, false);
        return;
    }

    if (currentRouteMode != state->routeMode) {
        qDebug() << "VpnConnection: route mode changed during DNS resolution; restarting resolution";
        resolveSiteAddressesAndConnect(container, vpnConfiguration);
        return;
    }

    continueConnectToVpn(container, vpnConfiguration, resolvedSiteAddresses, true);
}

void VpnConnection::cancelSiteDnsResolution()
{
    if (m_siteResolutionState.isNull()) {
        return;
    }

    const auto state = m_siteResolutionState;
    state->finished = true;
    const QSet<int> lookupIds = state->lookupIds;
    for (int lookupId : lookupIds) {
        QHostInfo::abortHostLookup(lookupId);
    }
    state->lookupIds.clear();
    m_siteResolutionState.clear();
}
#endif

void VpnConnection::continueConnectToVpn(DockerContainer container, const QJsonObject &vpnConfiguration,
                                         const QStringList &resolvedSiteAddresses,
                                         bool useResolvedSiteAddresses)
{
    m_vpnConfiguration = vpnConfiguration;

#ifdef AMNEZIA_DESKTOP
    if (m_vpnProtocol) {
        disconnect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
        m_vpnProtocol->stop();
        m_vpnProtocol.reset();
    }
    appendKillSwitchConfig();
#endif

    if (!appendSplitTunnelingConfig(resolvedSiteAddresses, useResolvedSiteAddresses)) {
        qCritical() << "VpnConnection::connectToVpn: site split tunneling has no valid IPv4 addresses";
        setConnectionState(Vpn::ConnectionState::Error);
        emit vpnProtocolError(ErrorCode::InternalError);
        return;
    }

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    m_vpnProtocol.reset(VpnProtocol::factory(container, m_vpnConfiguration));
    if (!m_vpnProtocol) {
        setConnectionState(Vpn::ConnectionState::Error);
        return;
    }
    m_vpnProtocol->prepare();
#elif defined Q_OS_ANDROID
    androidVpnProtocol = createDefaultAndroidVpnProtocol();
    createAndroidConnections();

    m_vpnProtocol.reset(androidVpnProtocol);
#elif defined Q_OS_IOS || defined(MACOS_NE)
    Proto proto = ContainerUtils::defaultProtocol(container);
    IosController::Instance()->connectVpn(proto, m_vpnConfiguration);
    connect(&m_checkTimer, &QTimer::timeout, IosController::Instance(), &IosController::checkStatus);
    return;
#endif

    createProtocolConnections();

    if (ErrorCode err = m_vpnProtocol->start(); err != ErrorCode::NoError) {
        setConnectionState(Vpn::ConnectionState::Error);
        emit vpnProtocolError(err);
    }
}

void VpnConnection::createProtocolConnections()
{
    connect(m_vpnProtocol.data(), &VpnProtocol::protocolError, this, &VpnConnection::vpnProtocolError);
    connect(m_vpnProtocol.data(), &VpnProtocol::connectionStateChanged, this, &VpnConnection::setConnectionState);
    connect(m_vpnProtocol.data(), SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));

#ifdef AMNEZIA_DESKTOP
    IpcClient::withInterface([this](QSharedPointer<IpcInterfaceReplica> rep) {
        connect(rep.data(), &IpcInterfaceReplica::networkChanged, this, &VpnConnection::reconnectToVpn, Qt::QueuedConnection);
        connect(rep.data(), &IpcInterfaceReplica::wakeup, this, &VpnConnection::reconnectToVpn, Qt::QueuedConnection);
    });
#endif
}

void VpnConnection::appendKillSwitchConfig()
{
    if (!m_appSettingsRepository) {
        qCritical() << "VpnConnection::appendKillSwitchConfig: repositories not initialized";
        return;
    }

    m_vpnConfiguration.insert(configKey::killSwitchOption, QVariant(m_appSettingsRepository->isKillSwitchEnabled()).toString());
    m_vpnConfiguration.insert(configKey::allowedDnsServers, QVariant(m_appSettingsRepository->getAllowedDnsServers()).toJsonValue());
}

bool VpnConnection::appendSplitTunnelingConfig(const QStringList &resolvedSiteAddresses, bool useResolvedSiteAddresses)
{
    if (!m_appSettingsRepository) {
        qCritical() << "VpnConnection::appendSplitTunnelingConfig: repositories not initialized";
        return false;
    }

    bool allowSiteBasedSplitTunneling = true;

    // this block is for old native configs and for old self-hosted configs
    auto protocolName = m_vpnConfiguration.value(configKey::vpnProto).toString();
    if (protocolName == ProtocolUtils::protoToString(Proto::Awg) || protocolName == ProtocolUtils::protoToString(Proto::WireGuard)) {
        allowSiteBasedSplitTunneling = false;
        auto configData = m_vpnConfiguration.value(protocolName + "_config_data").toObject();
        if (configData.value(configKey::allowedIps).isString()) {
            QJsonArray allowedIpsJsonArray = QJsonArray::fromStringList(configData.value(configKey::allowedIps).toString().split(", "));
            configData.insert(configKey::allowedIps, allowedIpsJsonArray);
            m_vpnConfiguration.insert(protocolName + "_config_data", configData);
        } else if (configData.value(configKey::allowedIps).isUndefined()) {
            auto nativeConfig = configData.value(configKey::config).toString();
            auto nativeConfigLines = nativeConfig.split("\n");
            for (auto &line : nativeConfigLines) {
                if (line.contains("AllowedIPs")) {
                    auto allowedIpsString = line.split(" = ");
                    if (allowedIpsString.size() < 1) {
                        break;
                    }
                    QJsonArray allowedIpsJsonArray = QJsonArray::fromStringList(allowedIpsString.at(1).split(", "));
                    configData.insert(configKey::allowedIps, allowedIpsJsonArray);
                    m_vpnConfiguration.insert(protocolName + "_config_data", configData);
                    break;
                }
            }
        }

        if (configData.value(configKey::persistentKeepAlive).isUndefined()) {
            auto nativeConfig = configData.value(configKey::config).toString();
            auto nativeConfigLines = nativeConfig.split("\n");
            for (auto &line : nativeConfigLines) {
                if (line.contains("PersistentKeepalive")) {
                    auto persistentKeepaliveString = line.split(" = ");
                    if (persistentKeepaliveString.size() > 1) {
                        configData.insert(configKey::persistentKeepAlive, persistentKeepaliveString.at(1));
                        m_vpnConfiguration.insert(protocolName + "_config_data", configData);
                    }
                    break;
                }
            }
        }

        QJsonArray allowedIpsJsonArray = configData.value(configKey::allowedIps).toArray();
        if (allowedIpsJsonArray.contains("0.0.0.0/0") && allowedIpsJsonArray.contains("::/0")) {
            allowSiteBasedSplitTunneling = true;
        }
    }

    amnezia::RouteMode routeMode = amnezia::RouteMode::VpnAllSites;
    QJsonArray sitesJsonArray;
    const bool siteSplitTunnelingEnabled = m_appSettingsRepository->isSitesSplitTunnelingEnabled();
    const auto configuredRouteMode = m_appSettingsRepository->routeMode();
    const bool siteSplitTunnelingActive =
        siteSplitTunnelingEnabled && configuredRouteMode != amnezia::RouteMode::VpnAllSites;
    if (siteSplitTunnelingActive) {
        routeMode = configuredRouteMode;

#ifdef Q_OS_WIN
        if (!allowSiteBasedSplitTunneling) {
            qCritical() << "VpnConnection: site split tunneling is not supported by this VPN configuration";
            return false;
        }
#endif

        if (allowSiteBasedSplitTunneling) {
            QStringList sites;
            const QVariantMap &m = m_appSettingsRepository->vpnSites(routeMode);
            for (auto i = m.constBegin(); i != m.constEnd(); ++i) {
                if (!useResolvedSiteAddresses) {
                    if (NetworkUtilities::checkIpSubnetFormat(i.key())) {
                        sites.append(i.key());
                    } else if (NetworkUtilities::checkIpSubnetFormat(i.value().toString())) {
                        sites.append(i.value().toString());
                    }
                }
            }
            if (useResolvedSiteAddresses) {
                sites = resolvedSiteAddresses;
            }
            sites.removeDuplicates();
            for (const auto &site : sites) {
                sitesJsonArray.append(site);
            }

            if (sitesJsonArray.isEmpty()) {
#ifdef Q_OS_WIN
                qCritical() << "VpnConnection: no valid site addresses; refusing to fall back to VpnAllSites";
                return false;
#else
                routeMode = amnezia::RouteMode::VpnAllSites;
#endif
            } else if (routeMode == amnezia::RouteMode::VpnOnlyForwardSites) {
                // Allow traffic to Amnezia DNS
#ifdef Q_OS_WIN
                const QString dns1 = m_vpnConfiguration.value(configKey::dns1).toString();
                const QString dns2 = m_vpnConfiguration.value(configKey::dns2).toString();
                if (NetworkUtilities::checkIpSubnetFormat(dns1)) {
                    sitesJsonArray.append(dns1);
                }
                if (NetworkUtilities::checkIpSubnetFormat(dns2)) {
                    sitesJsonArray.append(dns2);
                }
#else
                sitesJsonArray.append(m_vpnConfiguration.value(configKey::dns1).toString());
                sitesJsonArray.append(m_vpnConfiguration.value(configKey::dns2).toString());
#endif
            }
        }
    }

    m_vpnConfiguration.insert(configKey::splitTunnelType, routeMode);
    m_vpnConfiguration.insert(configKey::splitTunnelSites, sitesJsonArray);

    amnezia::AppsRouteMode appsRouteMode = amnezia::AppsRouteMode::VpnAllApps;
    QJsonArray appsJsonArray;
    if (m_appSettingsRepository->isAppsSplitTunnelingEnabled()) {
        appsRouteMode = m_appSettingsRepository->appsRouteMode();

        auto apps = m_appSettingsRepository->vpnApps(appsRouteMode);
        for (const auto &app : apps) {
            appsJsonArray.append(app.appPath.isEmpty() ? app.packageName : app.appPath);
        }

        if (appsJsonArray.isEmpty()) {
            appsRouteMode = amnezia::AppsRouteMode::VpnAllApps;
        }
    }

    m_vpnConfiguration.insert(configKey::appSplitTunnelType, appsRouteMode);
    m_vpnConfiguration.insert(configKey::splitTunnelApps, appsJsonArray);

    qDebug() << QString("Site split tunneling is %1, route mode is %2")
                        .arg(siteSplitTunnelingActive ? "enabled" : "disabled")
                        .arg(routeMode);
    qDebug() << QString("App split tunneling is %1, route mode is %2")
                        .arg(m_appSettingsRepository->isAppsSplitTunnelingEnabled() ? "enabled" : "disabled")
                        .arg(appsRouteMode);

    return true;
}

#ifdef Q_OS_ANDROID
void VpnConnection::restoreConnection()
{
    createAndroidConnections();

    m_vpnProtocol.reset(androidVpnProtocol);

    createProtocolConnections();
}

void VpnConnection::createAndroidConnections()
{
    androidVpnProtocol = createDefaultAndroidVpnProtocol();

    connect(AndroidController::instance(), &AndroidController::connectionStateChanged, androidVpnProtocol,
            &AndroidVpnProtocol::setConnectionState);
    connect(AndroidController::instance(), &AndroidController::statisticsUpdated, androidVpnProtocol, &AndroidVpnProtocol::setBytesChanged);
}

AndroidVpnProtocol *VpnConnection::createDefaultAndroidVpnProtocol()
{
    return new AndroidVpnProtocol(m_vpnConfiguration);
}
#endif

QString VpnConnection::bytesPerSecToText(quint64 bytes)
{
    double mbps = bytes * 8 / 1e6;
    return QString("%1 %2").arg(QString::number(mbps, 'f', 2)).arg(tr("Mbps")); // Mbit/s
}

void VpnConnection::reconnectToVpn() {
    if (m_vpnProtocol.isNull())
        return;

    if (m_connectionState != Vpn::ConnectionState::Connected) {
        qWarning() << QString("Reconnect triggered on %1 during inappropriate state: %2; ignoring slot")
                              .arg(QMetaEnum::fromType<Vpn::ConnectionState>().valueToKey(m_connectionState));
        return;
    }

    qDebug() << "Reconnect triggered. Reconnecting to the server";

    setConnectionState(Vpn::ConnectionState::Reconnecting);

    m_vpnProtocol->stop();
    if (ErrorCode err = m_vpnProtocol->start(); err != ErrorCode::NoError) {
        setConnectionState(Vpn::ConnectionState::Error);
        emit vpnProtocolError(err);
    }
}

void VpnConnection::disconnectFromVpn()
{
#ifdef Q_OS_WIN
    cancelSiteDnsResolution();
#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    // iOS/macOS NE use IosController directly; m_vpnProtocol is not set there.
    IosController::Instance()->disconnectVpn();
    disconnect(&m_checkTimer, &QTimer::timeout, IosController::Instance(), &IosController::checkStatus);
#endif

    if (m_vpnProtocol.isNull()) {
        setConnectionState(Vpn::ConnectionState::Disconnected);
        return;
    }

    setConnectionState(Vpn::ConnectionState::Disconnecting);

#ifdef Q_OS_ANDROID
    auto *const connection = new QMetaObject::Connection;
    *connection = connect(AndroidController::instance(), &AndroidController::vpnStateChanged, this,
                          [this, connection](AndroidController::ConnectionState state) {
                              if (state == AndroidController::ConnectionState::DISCONNECTED) {
                                  setConnectionState(Vpn::ConnectionState::Disconnected);
                                  disconnect(*connection);
                                  delete connection;
                              }
                          });
#endif

    m_vpnProtocol->stop();

#if !defined(Q_OS_ANDROID) && !defined(AMNEZIA_DESKTOP)
    m_vpnProtocol->deleteLater();
#endif

    m_vpnProtocol = nullptr;
}

void VpnConnection::setConnectionState(Vpn::ConnectionState state) {
    onConnectionStateChanged(state);

    if (state == Vpn::Disconnected && m_connectionState == Vpn::Reconnecting)
        return;

    m_connectionState = state;
    emit connectionStateChanged(state);
}
