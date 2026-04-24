#include "secureServersRepository.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>

#include "core/utils/api/apiEnums.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/networkUtilities.h"

using namespace amnezia;

namespace
{
QStringList managedSitesKeys(RouteMode mode)
{
    switch (mode) {
    case RouteMode::VpnAllExceptSites:
        return { QString(configKey::managedSplitTunnelExceptSourceSites),
                 QString(configKey::managedSplitTunnelExceptSites) };
    case RouteMode::VpnOnlyForwardSites:
    case RouteMode::VpnAllSites:
    default:
        return {};
    }
}

QVariantMap normalizedManagedSites(const QJsonValue &value)
{
    QVariantMap sites;
    if (value.isObject()) {
        return value.toObject().toVariantMap();
    }
    if (!value.isArray()) {
        return sites;
    }

    const QJsonArray items = value.toArray();
    for (const QJsonValue &item : items) {
        if (!item.isObject()) {
            continue;
        }
        const QJsonObject siteObject = item.toObject();
        const QString site = siteObject.value("hostname").toString(siteObject.value("url").toString()).trimmed();
        if (!site.isEmpty()) {
            sites.insert(site, siteObject.value("ip").toString());
        }
    }
    return sites;
}

void mergeSitesMap(QVariantMap &target, const QVariantMap &source)
{
    for (auto it = source.constBegin(); it != source.constEnd(); ++it) {
        target.insert(it.key(), it.value());
    }
}

QStringList splitTunnelStoredIps(const QString &value)
{
    QStringList ips;
    const QStringList tokens = value.split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts);
    for (const QString &token : tokens) {
        const QString ip = token.trimmed();
        if (NetworkUtilities::checkIpSubnetFormat(ip) && !ips.contains(ip)) {
            ips.append(ip);
        }
    }
    return ips;
}

QString mergeSplitTunnelIpValues(const QStringList &values)
{
    QStringList ips;
    for (const QString &value : values) {
        const QStringList storedIps = splitTunnelStoredIps(value);
        for (const QString &ip : storedIps) {
            if (!ips.contains(ip)) {
                ips.append(ip);
            }
        }
    }
    return ips.join(QStringLiteral(", "));
}

QVariantMap sourceManagedVpnSites(const QJsonObject &serverConfig, RouteMode mode)
{
    if (mode != RouteMode::VpnAllExceptSites) {
        return {};
    }
    if (serverConfig.contains(configKey::managedSplitTunnelExceptSourceSites)) {
        return normalizedManagedSites(serverConfig.value(configKey::managedSplitTunnelExceptSourceSites));
    }
    if (serverConfig.contains(configKey::managedSplitTunnelExceptSites)) {
        return normalizedManagedSites(serverConfig.value(configKey::managedSplitTunnelExceptSites));
    }
    return normalizedManagedSites(serverConfig.value(configKey::serverExcept));
}

QVariantMap routingManagedVpnSites(const QJsonObject &serverConfig, RouteMode mode)
{
    if (mode != RouteMode::VpnAllExceptSites) {
        return {};
    }

    const QVariantMap sourceSites = sourceManagedVpnSites(serverConfig, mode);
    QVariantMap resolvedSites = normalizedManagedSites(serverConfig.value(configKey::serverExcept));
    const QVariantMap clientResolvedSites =
            normalizedManagedSites(serverConfig.value(configKey::managedSplitTunnelClientResolvedExceptSites));

    if (serverConfig.contains(configKey::managedSplitTunnelExceptSourceSites)
        || serverConfig.contains(configKey::managedSplitTunnelExceptSites)) {
        QVariantMap sites;
        for (auto it = sourceSites.constBegin(); it != sourceSites.constEnd(); ++it) {
            const QString mergedIps = mergeSplitTunnelIpValues({ it.value().toString(),
                                                                 resolvedSites.value(it.key()).toString(),
                                                                 clientResolvedSites.value(it.key()).toString() });
            sites.insert(it.key(), mergedIps);
        }
        return sites;
    }

    QVariantMap sites = sourceSites;
    mergeSitesMap(sites, resolvedSites);
    mergeSitesMap(sites, clientResolvedSites);
    return sites;
}
}

SecureServersRepository::SecureServersRepository(SecureQSettings* settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
    QJsonArray arr = QJsonDocument::fromJson(value("Servers/serversList").toByteArray()).array();
    for (const QJsonValue &val : arr) {
        m_servers.append(ServerConfig::fromJson(val.toObject()));
    }
    m_defaultServerIndex = value("Servers/defaultServerIndex", 0).toInt();
}

QVariant SecureServersRepository::value(const QString &key, const QVariant &defaultValue) const
{
    return m_settings->value(key, defaultValue);
}

void SecureServersRepository::setValue(const QString &key, const QVariant &value)
{
    m_settings->setValue(key, value);
}

void SecureServersRepository::syncToStorage()
{
    QJsonArray arr;
    for (const ServerConfig &cfg : m_servers) {
        arr.append(cfg.toJson());
    }
    setValue("Servers/serversList", QJsonDocument(arr).toJson());
}

void SecureServersRepository::invalidateCache()
{
    m_servers.clear();
    QJsonArray arr = QJsonDocument::fromJson(value("Servers/serversList").toByteArray()).array();
    for (const QJsonValue &val : arr) {
        m_servers.append(ServerConfig::fromJson(val.toObject()));
    }
    m_defaultServerIndex = value("Servers/defaultServerIndex", 0).toInt();
}

void SecureServersRepository::setServersArray(const QJsonArray &servers)
{
    m_servers.clear();
    for (const QJsonValue &val : servers) {
        m_servers.append(ServerConfig::fromJson(val.toObject()));
    }
    syncToStorage();
}

void SecureServersRepository::addServer(const ServerConfig &server)
{
    m_servers.append(server);
    syncToStorage();
    emit serverAdded(server);
}

void SecureServersRepository::editServer(int index, const ServerConfig &server)
{
    if (index < 0 || index >= m_servers.size()) {
        return;
    }
    m_servers.replace(index, server);
    syncToStorage();
    emit serverEdited(index, server);
}

void SecureServersRepository::removeServer(int index)
{
    if (index < 0 || index >= m_servers.size()) {
        return;
    }
    int defaultIndex = m_defaultServerIndex;
    m_servers.removeAt(index);

    if (defaultIndex == index) {
        setDefaultServer(0);
    } else if (defaultIndex > index) {
        setDefaultServer(defaultIndex - 1);
    }

    if (m_servers.isEmpty()) {
        setDefaultServer(0);
    }

    syncToStorage();
    emit serverRemoved(index);
}

ServerConfig SecureServersRepository::server(int index) const
{
    if (index < 0 || index >= m_servers.size()) {
        return SelfHostedServerConfig{};
    }
    return m_servers.at(index);
}

QVector<ServerConfig> SecureServersRepository::servers() const
{
    return m_servers;
}

int SecureServersRepository::serversCount() const
{
    return m_servers.size();
}

int SecureServersRepository::defaultServerIndex() const
{
    return m_defaultServerIndex;
}

void SecureServersRepository::setDefaultServer(int index)
{
    if (index < 0) {
        return;
    }
    if (m_servers.size() > 0 && index >= m_servers.size()) {
        return;
    }
    if (m_servers.isEmpty() && index != 0) {
        return;
    }
    if (m_defaultServerIndex == index) {
        return;
    }
    m_defaultServerIndex = index;
    setValue("Servers/defaultServerIndex", index);
    emit defaultServerChanged(index);
}

void SecureServersRepository::setDefaultContainer(int serverIndex, DockerContainer container)
{
    ServerConfig config = server(serverIndex);
    config.visit([container](auto& arg) {
        arg.defaultContainer = container;
    });
    editServer(serverIndex, config);
}

ContainerConfig SecureServersRepository::containerConfig(int serverIndex, DockerContainer container) const
{
    ServerConfig config = server(serverIndex);
    return config.containerConfig(container);
}

void SecureServersRepository::setContainerConfig(int serverIndex, DockerContainer container, const ContainerConfig &config)
{
    ServerConfig serverConfig = server(serverIndex);
    serverConfig.visit([container, &config](auto& arg) {
        arg.containers[container] = config;
    });
    editServer(serverIndex, serverConfig);
}

void SecureServersRepository::clearLastConnectionConfig(int serverIndex, DockerContainer container)
{
    ServerConfig serverConfig = server(serverIndex);
    ContainerConfig containerCfg = serverConfig.containerConfig(container);
    
    containerCfg.protocolConfig.clearClientConfig();
    
    setContainerConfig(serverIndex, container, containerCfg);
}

QJsonObject SecureServersRepository::serverJson(int index) const
{
    if (index < 0 || index >= m_servers.size()) {
        return {};
    }
    return m_servers.at(index).toJson();
}

void SecureServersRepository::editServerJson(int index, const QJsonObject &serverJson)
{
    editServer(index, ServerConfig::fromJson(serverJson));
}

QVariantMap SecureServersRepository::managedVpnSites(int serverIndex, RouteMode mode) const
{
    if (serverIndex < 0 || serverIndex >= m_servers.size()) {
        return {};
    }
    return sourceManagedVpnSites(serverJson(serverIndex), mode);
}

QVariantMap SecureServersRepository::managedVpnSitesForRouting(int serverIndex, RouteMode mode) const
{
    if (serverIndex < 0 || serverIndex >= m_servers.size()) {
        return {};
    }
    return routingManagedVpnSites(serverJson(serverIndex), mode);
}

void SecureServersRepository::setManagedVpnSites(int serverIndex, RouteMode mode, const QVariantMap &sites)
{
    if (serverIndex < 0 || serverIndex >= m_servers.size()) {
        return;
    }

    const QStringList keys = managedSitesKeys(mode);
    if (keys.isEmpty()) {
        return;
    }

    QJsonObject config = serverJson(serverIndex);
    const QJsonObject jsonSites = QJsonObject::fromVariantMap(sites);
    for (const QString &key : keys) {
        config.insert(key, jsonSites);
    }
    config.remove(configKey::serverExcept);
    config.remove(configKey::managedSplitTunnelClientResolvedExceptSites);
    config.remove(configKey::managedSplitTunnelClientResolvedAt);
    editServerJson(serverIndex, config);
}

bool SecureServersRepository::addManagedVpnSite(int serverIndex, RouteMode mode, const QString &site, const QString &ip)
{
    QVariantMap sites = managedVpnSites(serverIndex, mode);
    if (sites.contains(site) && ip.isEmpty()) {
        return false;
    }

    sites.insert(site, ip);
    setManagedVpnSites(serverIndex, mode, sites);
    return true;
}

void SecureServersRepository::addManagedVpnSites(int serverIndex, RouteMode mode, const QMap<QString, QString> &sites)
{
    QVariantMap allSites = managedVpnSites(serverIndex, mode);
    for (auto it = sites.constBegin(); it != sites.constEnd(); ++it) {
        if (allSites.contains(it.key()) && allSites.value(it.key()) == it.value()) {
            continue;
        }
        allSites.insert(it.key(), it.value());
    }

    setManagedVpnSites(serverIndex, mode, allSites);
}

void SecureServersRepository::removeManagedVpnSite(int serverIndex, RouteMode mode, const QString &site)
{
    QVariantMap sites = managedVpnSites(serverIndex, mode);
    if (!sites.contains(site)) {
        return;
    }

    sites.remove(site);
    setManagedVpnSites(serverIndex, mode, sites);
}

void SecureServersRepository::removeAllManagedVpnSites(int serverIndex, RouteMode mode)
{
    setManagedVpnSites(serverIndex, mode, {});
}

bool SecureServersRepository::isManagedSplitTunnelingForceEnabled(int serverIndex) const
{
    if (serverIndex < 0 || serverIndex >= m_servers.size()) {
        return false;
    }
    return serverJson(serverIndex).value(configKey::managedSplitTunnelForceEnabled).toBool(false);
}

void SecureServersRepository::setManagedSplitTunnelingForceEnabled(int serverIndex, bool enabled)
{
    if (serverIndex < 0 || serverIndex >= m_servers.size()) {
        return;
    }

    QJsonObject config = serverJson(serverIndex);
    if (enabled) {
        config.insert(configKey::managedSplitTunnelForceEnabled, true);
    } else {
        config.remove(configKey::managedSplitTunnelForceEnabled);
    }
    editServerJson(serverIndex, config);
}

RouteMode SecureServersRepository::effectiveSiteRouteMode(int serverIndex, bool localSplitEnabled, RouteMode localRouteMode) const
{
    if (localSplitEnabled) {
        const RouteMode currentMode = localRouteMode == RouteMode::VpnAllSites
                ? RouteMode::VpnOnlyForwardSites
                : localRouteMode;
        if (currentMode == RouteMode::VpnOnlyForwardSites || currentMode == RouteMode::VpnAllExceptSites) {
            return currentMode;
        }
        return RouteMode::VpnAllSites;
    }

    if (isManagedSplitTunnelingForceEnabled(serverIndex)
        && !managedVpnSitesForRouting(serverIndex, RouteMode::VpnAllExceptSites).isEmpty()) {
        return RouteMode::VpnAllExceptSites;
    }

    return RouteMode::VpnAllSites;
}

ServerCredentials SecureServersRepository::serverCredentials(int index) const
{
    ServerConfig config = server(index);
    
    if (config.isSelfHosted()) {
        const SelfHostedServerConfig* selfHosted = config.as<SelfHostedServerConfig>();
        if (!selfHosted) return ServerCredentials();
        auto creds = selfHosted->credentials();
        if (creds.has_value()) {
            return creds.value();
        }
    }
    
    return ServerCredentials{};
}

bool SecureServersRepository::hasServerWithVpnKey(const QString &vpnKey) const
{
    QString normalizedInput = vpnKey.trimmed();
    if (normalizedInput.startsWith(QStringLiteral("vpn://"), Qt::CaseInsensitive)) {
        normalizedInput = normalizedInput.mid(QStringLiteral("vpn://").size());
    }
    if (normalizedInput.isEmpty()) {
        return false;
    }

    QVector<ServerConfig> serversList = servers();
    for (const ServerConfig& serverConfig : serversList) {
        if (serverConfig.isApiV1()) {
            const ApiV1ServerConfig* apiV1 = serverConfig.as<ApiV1ServerConfig>();
            if (!apiV1) continue;
            QString storedKey = apiV1->vpnKey();
            if (storedKey.isEmpty()) {
                continue;
            }
            QString normalizedStored = storedKey.trimmed();
            if (normalizedStored.startsWith(QStringLiteral("vpn://"), Qt::CaseInsensitive)) {
                normalizedStored = normalizedStored.mid(QStringLiteral("vpn://").size());
            }
            if (normalizedInput == normalizedStored) {
                return true;
            }
        } else if (serverConfig.isApiV2()) {
            const ApiV2ServerConfig* apiV2 = serverConfig.as<ApiV2ServerConfig>();
            if (!apiV2) continue;
            QString storedKey = apiV2->vpnKey();
            if (storedKey.isEmpty()) {
                continue;
            }
            QString normalizedStored = storedKey.trimmed();
            if (normalizedStored.startsWith(QStringLiteral("vpn://"), Qt::CaseInsensitive)) {
                normalizedStored = normalizedStored.mid(QStringLiteral("vpn://").size());
            }
            if (normalizedInput == normalizedStored) {
                return true;
            }
        }
    }
    return false;
}

bool SecureServersRepository::hasServerWithCrc(quint16 crc) const
{
    for (const ServerConfig& serverConfig : m_servers) {
        if (static_cast<quint16>(serverConfig.crc()) == crc) {
            return true;
        }
    }
    return false;
}
