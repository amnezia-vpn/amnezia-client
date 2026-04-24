#include "connectionController.h"

#include <QDateTime>
#include <QHostAddress>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QRegularExpression>

#include "amneziaApplication.h"
#include "core/configurators/configuratorBase.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/utilities.h"
#include "core/utils/networkUtilities.h"
#include "version.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"
#include "core/models/protocolConfig.h"

using namespace amnezia;
using namespace ProtocolUtils;

namespace
{
constexpr int serverRoutingRulesInitialSyncIntervalMs = 1000;
constexpr int serverRoutingRulesPeriodicSyncIntervalMs = 24 * 60 * 60 * 1000;
constexpr int serverRoutingRulesFastRetryIntervalMs = 15 * 1000;
constexpr int serverRoutingRulesMaxFastRetryCount = 3;
constexpr int serverRoutingRulesClientResolveIntervalSeconds = 24 * 60 * 60;
constexpr int serverRoutingRulesClientResolveInitialDelayMs = 2000;
constexpr int serverRoutingRulesClientResolveJitterMs = 30 * 1000;
constexpr qsizetype serverRoutingRulesMaxPayloadBytes = 4 * 1024 * 1024;
constexpr qsizetype serverRoutingRulesMaxSiteCount = 50000;

QString serverRoutingRulesSyncUrl(const QString &host)
{
    const QString syncHost = host.trimmed().isEmpty()
            ? QString::fromLatin1(protocols::serverRoutingRules::syncHost)
            : host.trimmed();

    return QStringLiteral("http://%1:%2%3")
            .arg(syncHost,
                 QString::number(protocols::serverRoutingRules::syncPort),
                 QString::fromLatin1(protocols::serverRoutingRules::syncPath));
}

QList<QUrl> serverRoutingRulesSyncUrls(const QString &primaryHost)
{
    QStringList hosts;
    const auto addHost = [&hosts](const QString &host) {
        const QString trimmedHost = host.trimmed();
        if (!trimmedHost.isEmpty() && !hosts.contains(trimmedHost)) {
            hosts.append(trimmedHost);
        }
    };

    addHost(primaryHost);
    addHost(QString::fromLatin1(protocols::serverRoutingRules::syncHost));

    QList<QUrl> urls;
    urls.reserve(hosts.size());
    for (const QString &host : hosts) {
        urls.append(QUrl(serverRoutingRulesSyncUrl(host)));
    }
    return urls;
}

bool isServerRoutingRulesSitesValue(const QJsonValue &value)
{
    return value.isObject() || value.isArray();
}

QJsonObject normalizedServerRoutingRulesSites(const QJsonValue &value)
{
    QJsonObject sites;
    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
            sites.insert(it.key().trimmed().toLower(), it.value().toString());
        }
        return sites;
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
        const QString site = siteObject.value("hostname").toString(siteObject.value("url").toString()).trimmed().toLower();
        if (!site.isEmpty()) {
            sites.insert(site, siteObject.value("ip").toString());
        }
    }
    return sites;
}

QJsonObject serverRoutingRulesExceptSites(const QJsonObject &payload)
{
    QJsonValue sitesValue = payload.value(configKey::serverExcept);
    if (!isServerRoutingRulesSitesValue(sitesValue)) {
        sitesValue = payload.value(configKey::managedSplitTunnelExceptSites);
    }
    return normalizedServerRoutingRulesSites(sitesValue);
}

QJsonObject serverRoutingRulesSourceSites(const QJsonObject &payload)
{
    QJsonValue sitesValue = payload.value(configKey::managedSplitTunnelExceptSourceSites);
    if (!isServerRoutingRulesSitesValue(sitesValue)) {
        sitesValue = payload.value(configKey::managedSplitTunnelExceptSites);
    }
    if (!isServerRoutingRulesSitesValue(sitesValue)) {
        sitesValue = payload.value(configKey::serverExcept);
    }
    return normalizedServerRoutingRulesSites(sitesValue);
}

bool hasServerRoutingRulesExceptSites(const QJsonObject &payload)
{
    return isServerRoutingRulesSitesValue(payload.value(configKey::serverExcept))
           || isServerRoutingRulesSitesValue(payload.value(configKey::managedSplitTunnelExceptSourceSites))
           || isServerRoutingRulesSitesValue(payload.value(configKey::managedSplitTunnelExceptSites));
}

bool isValidServerRoutingRulesExceptSites(const QJsonObject &sites)
{
    if (sites.size() > serverRoutingRulesMaxSiteCount) {
        return false;
    }

    for (auto it = sites.constBegin(); it != sites.constEnd(); ++it) {
        if (it.key().size() > 1024 || !it.value().isString()) {
            return false;
        }
    }
    return true;
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

QString mergedStoredIps(const QStringList &values)
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

QJsonObject sitesBoundToSource(const QJsonObject &sites, const QJsonObject &sourceSites)
{
    QJsonObject boundedSites;
    for (auto it = sourceSites.constBegin(); it != sourceSites.constEnd(); ++it) {
        if (sites.contains(it.key())) {
            boundedSites.insert(it.key(), sites.value(it.key()));
        }
    }
    return boundedSites;
}

QJsonObject clientResolvedSitesBoundToSource(const QJsonObject &serverConfig, const QJsonObject &sourceSites)
{
    const QJsonObject clientResolvedSites =
            normalizedServerRoutingRulesSites(serverConfig.value(configKey::managedSplitTunnelClientResolvedExceptSites));
    return sitesBoundToSource(clientResolvedSites, sourceSites);
}

bool isManagedResolveDomain(const QString &site)
{
    return !NetworkUtilities::checkIpSubnetFormat(site) && NetworkUtilities::domainRegExp().exactMatch(site);
}

QStringList managedResolveDomains(const QJsonObject &sourceSites)
{
    QStringList domains;
    for (auto it = sourceSites.constBegin(); it != sourceSites.constEnd(); ++it) {
        const QString site = it.key().trimmed().toLower();
        if (isManagedResolveDomain(site) && !domains.contains(site)) {
            domains.append(site);
        }
    }
    return domains;
}

bool shouldRunClientManagedResolve(const QJsonObject &serverConfig, const QJsonObject &sourceSites)
{
    const QStringList domains = managedResolveDomains(sourceSites);
    if (domains.isEmpty()) {
        return false;
    }

    const QJsonObject clientResolvedSites =
            normalizedServerRoutingRulesSites(serverConfig.value(configKey::managedSplitTunnelClientResolvedExceptSites));
    for (const QString &domain : domains) {
        if (!clientResolvedSites.contains(domain)) {
            return true;
        }
    }

    const QDateTime resolvedAt =
            QDateTime::fromString(serverConfig.value(configKey::managedSplitTunnelClientResolvedAt).toString(), Qt::ISODate);
    if (!resolvedAt.isValid()) {
        return true;
    }

    return resolvedAt.toUTC().secsTo(QDateTime::currentDateTimeUtc()) >= serverRoutingRulesClientResolveIntervalSeconds;
}

QStringList hostInfoIpv4Addresses(const QHostInfo &hostInfo)
{
    QStringList ips;
    if (hostInfo.error() != QHostInfo::NoError) {
        return ips;
    }
    for (const QHostAddress &addr : hostInfo.addresses()) {
        if (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol && !ips.contains(addr.toString())) {
            ips.append(addr.toString());
        }
    }
    return ips;
}
}

ConnectionController::ConnectionController(SecureServersRepository* serversRepository,
                                         SecureAppSettingsRepository* appSettingsRepository,
                                         VpnConnection* vpnConnection,
                                         QObject* parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository),
      m_vpnConnection(vpnConnection)
{
    connect(m_vpnConnection, &VpnConnection::connectionStateChanged, this, &ConnectionController::onVpnConnectionStateChanged);
    connect(this, &ConnectionController::openConnectionRequested, m_vpnConnection, &VpnConnection::connectToVpn, Qt::QueuedConnection);
    connect(this, &ConnectionController::closeConnectionRequested, m_vpnConnection, &VpnConnection::disconnectFromVpn, Qt::QueuedConnection);
    connect(this, &ConnectionController::setConnectionStateRequested, m_vpnConnection, &VpnConnection::setConnectionState, Qt::QueuedConnection);
    connect(this, &ConnectionController::killSwitchModeChangedRequested, m_vpnConnection, &VpnConnection::onKillSwitchModeChanged, Qt::QueuedConnection);
#ifdef Q_OS_ANDROID
    connect(this, &ConnectionController::restoreConnectionRequested, m_vpnConnection, &VpnConnection::restoreConnection, Qt::QueuedConnection);
#endif
    m_serverRoutingRulesSyncTimer.setSingleShot(true);
    connect(&m_serverRoutingRulesSyncTimer, &QTimer::timeout, this, &ConnectionController::syncServerRoutingRules);
    m_serverRoutingRulesClientResolveTimer.setSingleShot(true);
    connect(&m_serverRoutingRulesClientResolveTimer, &QTimer::timeout, this, &ConnectionController::startClientManagedSitesResolve);
}

bool ConnectionController::isConnected() const
{
    return m_vpnConnection && m_vpnConnection->connectionState() == Vpn::ConnectionState::Connected;
}

void ConnectionController::setConnectionState(Vpn::ConnectionState state)
{
    if (m_vpnConnection) {
        emit setConnectionStateRequested(state);
    }
}

void ConnectionController::onVpnConnectionStateChanged(Vpn::ConnectionState state)
{
    switch (state) {
    case Vpn::ConnectionState::Connected:
        ++m_serverRoutingRulesSyncGeneration;
        m_isServerRoutingRulesSyncInProgress = false;
        m_serverRoutingRulesSyncPendingRefresh = false;
        m_serverRoutingRulesSyncFastRetryCount = 0;
        cancelClientManagedSitesResolve();
        scheduleServerRoutingRulesSync(serverRoutingRulesInitialSyncIntervalMs);
        break;
    case Vpn::ConnectionState::Disconnected:
        m_serverRoutingRulesSyncTimer.stop();
        ++m_serverRoutingRulesSyncGeneration;
        m_isServerRoutingRulesSyncInProgress = false;
        m_serverRoutingRulesSyncPendingRefresh = false;
        m_serverRoutingRulesSyncFastRetryCount = 0;
        cancelClientManagedSitesResolve();
        break;
    case Vpn::ConnectionState::Error:
    case Vpn::ConnectionState::Unknown:
        m_serverRoutingRulesSyncTimer.stop();
        ++m_serverRoutingRulesSyncGeneration;
        m_isServerRoutingRulesSyncInProgress = false;
        m_serverRoutingRulesSyncPendingRefresh = false;
        cancelClientManagedSitesResolve();
        break;
    default:
        break;
    }

    emit connectionStateChanged(state);
}

ErrorCode ConnectionController::prepareConnection(int serverIndex,
                                                 QJsonObject& vpnConfiguration,
                                                 DockerContainer& container)
{
    if (!isServiceReady()) {
        return ErrorCode::AmneziaServiceNotRunning;
    }

    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);
    container = serverConfigModel.defaultContainer();

    if (!isContainerSupported(container)) {
        return ErrorCode::NotSupportedOnThisPlatform;
    }

    ContainerConfig containerConfigModel = m_serversRepository->containerConfig(serverIndex, container);

    auto dns = serverConfigModel.getDnsPair(m_appSettingsRepository->useAmneziaDns(),
                                            m_appSettingsRepository->primaryDns(),
                                            m_appSettingsRepository->secondaryDns());

    vpnConfiguration = createConnectionConfiguration(serverIndex, dns, serverConfigModel, containerConfigModel, container);

    return ErrorCode::NoError;
}

ErrorCode ConnectionController::openConnection(int serverIndex)
{
    QJsonObject vpnConfiguration;
    DockerContainer container;

    ErrorCode errorCode = prepareConnection(serverIndex, vpnConfiguration, container);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    emit openConnectionRequested(serverIndex, container, vpnConfiguration);
    return ErrorCode::NoError;
}

void ConnectionController::closeConnection()
{
    if (m_vpnConnection) {
        emit closeConnectionRequested();
    }
}

#ifdef Q_OS_ANDROID
void ConnectionController::restoreConnection(Vpn::ConnectionState state, int serverIndex)
{
    if (!m_vpnConnection) {
        return;
    }

    if (serverIndex < 0 || serverIndex >= m_serversRepository->serversCount()) {
        serverIndex = m_serversRepository->defaultServerIndex();
    }
    if (serverIndex < 0 || serverIndex >= m_serversRepository->serversCount()) {
        return;
    }

    QJsonObject vpnConfiguration;
    DockerContainer container = DockerContainer::None;
    if (prepareConnection(serverIndex, vpnConfiguration, container) != ErrorCode::NoError) {
        return;
    }

    emit restoreConnectionRequested(serverIndex, container, vpnConfiguration, state);
}
#endif

void ConnectionController::onKillSwitchModeChanged(bool enabled)
{
    if (m_vpnConnection) {
        emit killSwitchModeChangedRequested(enabled);
    }
}

void ConnectionController::onManagedSplitTunnelingRulesPublished(int serverIndex)
{
    emit serverRoutingRulesChanged(serverIndex);
    if (!isConnected() || !isCurrentConnectionServerIndex(serverIndex)) {
        return;
    }
    scheduleServerRoutingRulesSync(0);
}

ErrorCode ConnectionController::lastConnectionError() const
{
    return m_vpnConnection->lastError();
}

QJsonObject ConnectionController::createConnectionConfiguration(int serverIndex,
                                                              const QPair<QString, QString> &dns,
                                                              const ServerConfig &serverConfig,
                                                              const ContainerConfig &containerConfig,
                                                              DockerContainer container)
{
    QJsonObject vpnConfiguration {};

    if (ContainerUtils::containerService(container) == ServiceType::Other) {
        return vpnConfiguration;
    }

    Proto proto = ContainerUtils::defaultProtocol(container);

    const RouteMode routeMode = m_serversRepository->effectiveSiteRouteMode(
            serverIndex, m_appSettingsRepository->isSitesSplitTunnelingEnabled(), m_appSettingsRepository->routeMode());

    ConnectionSettings connectionSettings = {
        { dns.first, dns.second },
        serverConfig.isApiConfig(),
        {
            routeMode != RouteMode::VpnAllSites,
            routeMode
        }
    };

    auto configurator = ConfiguratorBase::create(proto, nullptr);
    ProtocolConfig processedConfig = configurator->processConfigWithLocalSettings(connectionSettings,
                                                                                  containerConfig.protocolConfig);

    QJsonObject vpnConfigData = processedConfig.getClientConfigJson();
    if (ContainerUtils::isAwgContainer(container) || container == DockerContainer::WireGuard) {
        if (vpnConfigData[configKey::mtu].toString().isEmpty()) {
            vpnConfigData[configKey::mtu] =
                    ContainerUtils::isAwgContainer(container) ? protocols::awg::defaultMtu :
                    protocols::wireguard::defaultMtu;
        }
    }

    vpnConfiguration.insert(ProtocolUtils::key_proto_config_data(proto), vpnConfigData);
    vpnConfiguration[configKey::vpnProto] = ProtocolUtils::protoToString(proto);

    vpnConfiguration[configKey::dns1] = dns.first;
    vpnConfiguration[configKey::dns2] = dns.second;

    vpnConfiguration[configKey::hostName] = serverConfig.hostName();
    vpnConfiguration[configKey::description] = serverConfig.description();
    vpnConfiguration[configKey::serverIndex] = serverIndex;

    vpnConfiguration[configKey::configVersion] = serverConfig.configVersion();

    const QJsonObject serverJson = m_serversRepository->serverJson(serverIndex);
    const QString syncHost = serverJson.value(configKey::serverRoutingRulesSyncHost).toString().trimmed();
    if (!syncHost.isEmpty()) {
        vpnConfiguration[configKey::serverRoutingRulesSyncHost] = syncHost;
    }

    return vpnConfiguration;
}

void ConnectionController::scheduleServerRoutingRulesSync(int intervalMs)
{
    if (!isConnected()) {
        return;
    }
    m_serverRoutingRulesSyncTimer.start(intervalMs);
}

void ConnectionController::scheduleNextServerRoutingRulesSync(bool success)
{
    if (success) {
        m_serverRoutingRulesSyncFastRetryCount = 0;
        scheduleServerRoutingRulesSync(serverRoutingRulesPeriodicSyncIntervalMs);
        return;
    }

    if (m_serverRoutingRulesSyncFastRetryCount < serverRoutingRulesMaxFastRetryCount) {
        ++m_serverRoutingRulesSyncFastRetryCount;
        scheduleServerRoutingRulesSync(serverRoutingRulesFastRetryIntervalMs);
        return;
    }

    scheduleServerRoutingRulesSync(serverRoutingRulesPeriodicSyncIntervalMs);
}

void ConnectionController::finishServerRoutingRulesSync(bool success)
{
    m_isServerRoutingRulesSyncInProgress = false;
    if (m_serverRoutingRulesSyncPendingRefresh && isConnected()) {
        m_serverRoutingRulesSyncPendingRefresh = false;
        scheduleServerRoutingRulesSync(0);
        return;
    }
    m_serverRoutingRulesSyncPendingRefresh = false;
    scheduleNextServerRoutingRulesSync(success);
}

QStringList ConnectionController::effectiveSplitTunnelIpsForSync(int serverIndex, RouteMode routeMode) const
{
    if (routeMode == RouteMode::VpnAllSites) {
        return {};
    }

    QStringList ips;
    const auto appendSites = [&ips](const QVariantMap &sites) {
        for (auto it = sites.constBegin(); it != sites.constEnd(); ++it) {
            if (NetworkUtilities::checkIpSubnetFormat(it.key())) {
                ips.append(it.key());
            }
            ips.append(splitTunnelStoredIps(it.value().toString()));
        }
    };

    appendSites(m_appSettingsRepository->vpnSites(routeMode));
    appendSites(m_serversRepository->managedVpnSitesForRouting(serverIndex, routeMode));
    ips.removeDuplicates();
    ips.sort();
    return ips;
}

int ConnectionController::currentConnectionServerIndex() const
{
    return m_vpnConnection ? m_vpnConnection->serverIndex() : -1;
}

bool ConnectionController::isCurrentConnectionServerIndex(int serverIndex) const
{
    return serverIndex >= 0 && currentConnectionServerIndex() == serverIndex;
}

void ConnectionController::syncServerRoutingRules()
{
    if (!isConnected()) {
        return;
    }
    if (m_isServerRoutingRulesSyncInProgress) {
        m_serverRoutingRulesSyncPendingRefresh = true;
        return;
    }

    const int serverIndex = currentConnectionServerIndex();
    if (serverIndex < 0 || serverIndex >= m_serversRepository->serversCount()) {
        scheduleNextServerRoutingRulesSync(false);
        return;
    }

    const RouteMode oldRouteMode = m_serversRepository->effectiveSiteRouteMode(
            serverIndex, m_appSettingsRepository->isSitesSplitTunnelingEnabled(), m_appSettingsRepository->routeMode());
    const QStringList oldSplitTunnelIps = effectiveSplitTunnelIpsForSync(serverIndex, oldRouteMode);
    const QList<QUrl> syncUrls = serverRoutingRulesSyncUrls(m_vpnConnection->serverRoutingRulesSyncHost());
    if (syncUrls.isEmpty()) {
        scheduleNextServerRoutingRulesSync(false);
        return;
    }

    m_isServerRoutingRulesSyncInProgress = true;
    const int syncGeneration = ++m_serverRoutingRulesSyncGeneration;
    syncServerRoutingRulesFromUrls(syncUrls, 0, serverIndex, oldRouteMode, oldSplitTunnelIps, syncGeneration);
}

void ConnectionController::syncServerRoutingRulesFromUrls(const QList<QUrl> &syncUrls, int urlIndex, int serverIndex,
                                                         RouteMode oldRouteMode,
                                                         const QStringList &oldSplitTunnelIps,
                                                         int syncGeneration)
{
    if (syncGeneration != m_serverRoutingRulesSyncGeneration) {
        return;
    }
    if (!isConnected() || !isCurrentConnectionServerIndex(serverIndex)) {
        m_isServerRoutingRulesSyncInProgress = false;
        return;
    }
    if (urlIndex < 0 || urlIndex >= syncUrls.size()) {
        finishServerRoutingRulesSync(false);
        return;
    }

    const QUrl syncUrl = syncUrls.at(urlIndex);
    qInfo() << "ConnectionController: syncing server routing rules from" << syncUrl;

    QNetworkRequest request { syncUrl };
    request.setTransferTimeout(4000);

    QNetworkReply *reply = amnApp->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, syncUrls, urlIndex, serverIndex, oldRouteMode, oldSplitTunnelIps, syncGeneration]() {
                reply->deleteLater();

                if (syncGeneration != m_serverRoutingRulesSyncGeneration) {
                    return;
                }
                if (!isConnected() || !isCurrentConnectionServerIndex(serverIndex)) {
                    m_isServerRoutingRulesSyncInProgress = false;
                    return;
                }

                if (reply->error() != QNetworkReply::NoError) {
                    qWarning() << "ConnectionController: failed to sync server routing rules from" << syncUrls.at(urlIndex)
                               << reply->errorString();
                    syncServerRoutingRulesFromUrls(syncUrls, urlIndex + 1, serverIndex, oldRouteMode, oldSplitTunnelIps,
                                                   syncGeneration);
                    return;
                }

                const QVariant contentLength = reply->header(QNetworkRequest::ContentLengthHeader);
                if (contentLength.isValid() && contentLength.toLongLong() > serverRoutingRulesMaxPayloadBytes) {
                    qWarning() << "ConnectionController: server routing rules payload is too large, content length"
                               << contentLength.toLongLong();
                    finishServerRoutingRulesSync(false);
                    return;
                }

                const QByteArray payload = reply->readAll();
                const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                if (payload.size() > serverRoutingRulesMaxPayloadBytes) {
                    qWarning() << "ConnectionController: server routing rules payload is too large, bytes" << payload.size();
                    finishServerRoutingRulesSync(false);
                    return;
                }
                if (statusCode < 200 || statusCode >= 300) {
                    qWarning() << "ConnectionController: unexpected server routing rules http status" << statusCode
                               << "from" << syncUrls.at(urlIndex);
                    syncServerRoutingRulesFromUrls(syncUrls, urlIndex + 1, serverIndex, oldRouteMode, oldSplitTunnelIps,
                                                   syncGeneration);
                    return;
                }

                const QJsonDocument document = QJsonDocument::fromJson(payload);
                if (!document.isObject()) {
                    qWarning() << "ConnectionController: invalid server routing rules payload";
                    finishServerRoutingRulesSync(false);
                    return;
                }

                const QJsonObject payloadObject = document.object();
                if (!hasServerRoutingRulesExceptSites(payloadObject)) {
                    qWarning() << "ConnectionController: server routing rules payload does not contain managed site list";
                    finishServerRoutingRulesSync(false);
                    return;
                }
                if (!isValidServerRoutingRulesExceptSites(serverRoutingRulesExceptSites(payloadObject))
                    || !isValidServerRoutingRulesExceptSites(serverRoutingRulesSourceSites(payloadObject))) {
                    qWarning() << "ConnectionController: invalid server routing rules managed site list";
                    finishServerRoutingRulesSync(false);
                    return;
                }

                if (!applyServerRoutingRulesPayload(serverIndex, payloadObject)) {
                    scheduleClientManagedSitesResolve(serverIndex);
                    finishServerRoutingRulesSync(true);
                    return;
                }
                scheduleClientManagedSitesResolve(serverIndex);

                const RouteMode newRouteMode = m_serversRepository->effectiveSiteRouteMode(
                        serverIndex, m_appSettingsRepository->isSitesSplitTunnelingEnabled(), m_appSettingsRepository->routeMode());
                const QStringList newSplitTunnelIps = effectiveSplitTunnelIpsForSync(serverIndex, newRouteMode);
                if (newRouteMode != oldRouteMode || newSplitTunnelIps != oldSplitTunnelIps) {
                    qInfo() << "ConnectionController: server routing rules changed active routes, reconnecting VPN";
                    m_isServerRoutingRulesSyncInProgress = false;
                    m_serverRoutingRulesSyncPendingRefresh = false;
                    QTimer::singleShot(0, m_vpnConnection, &VpnConnection::reconnectToVpn);
                    return;
                }

                finishServerRoutingRulesSync(true);
            });
}

bool ConnectionController::applyServerRoutingRulesPayload(int serverIndex, const QJsonObject &payload)
{
    if (serverIndex < 0 || serverIndex >= m_serversRepository->serversCount()) {
        return false;
    }
    if (!hasServerRoutingRulesExceptSites(payload)) {
        return false;
    }

    QJsonObject exceptSites = serverRoutingRulesExceptSites(payload);
    QJsonObject managedExceptSites = serverRoutingRulesSourceSites(payload);
    bool forceEnabled = payload.value(configKey::managedSplitTunnelForceEnabled).toBool(false);

    QJsonObject serverConfig = m_serversRepository->serverJson(serverIndex);
    const ServerCredentials credentials = m_serversRepository->serverCredentials(serverIndex);
    if (!credentials.userName.isEmpty() && !credentials.secretData.isEmpty()) {
        QJsonValue sourceSitesValue = serverConfig.value(configKey::managedSplitTunnelExceptSourceSites);
        if (!isServerRoutingRulesSitesValue(sourceSitesValue)) {
            sourceSitesValue = isServerRoutingRulesSitesValue(serverConfig.value(configKey::managedSplitTunnelExceptSites))
                    ? serverConfig.value(configKey::managedSplitTunnelExceptSites)
                    : serverConfig.value(configKey::serverExcept);
        }
        if (isServerRoutingRulesSitesValue(sourceSitesValue)) {
            managedExceptSites = normalizedServerRoutingRulesSites(sourceSitesValue);
        }
        forceEnabled = serverConfig.value(configKey::managedSplitTunnelForceEnabled).toBool(false);
    }

    exceptSites = sitesBoundToSource(exceptSites, managedExceptSites);
    const QJsonObject clientResolvedSites = clientResolvedSitesBoundToSource(serverConfig, managedExceptSites);

    bool changed = false;
    if (serverConfig.value(configKey::serverExcept).toObject() != exceptSites) {
        serverConfig.insert(configKey::serverExcept, exceptSites);
        changed = true;
    }
    if (serverConfig.value(configKey::managedSplitTunnelExceptSourceSites).toObject() != managedExceptSites) {
        serverConfig.insert(configKey::managedSplitTunnelExceptSourceSites, managedExceptSites);
        changed = true;
    }
    if (serverConfig.value(configKey::managedSplitTunnelExceptSites).toObject() != managedExceptSites) {
        serverConfig.insert(configKey::managedSplitTunnelExceptSites, managedExceptSites);
        changed = true;
    }

    const bool currentForceEnabled = serverConfig.value(configKey::managedSplitTunnelForceEnabled).toBool(false);
    if (currentForceEnabled != forceEnabled) {
        if (forceEnabled) {
            serverConfig.insert(configKey::managedSplitTunnelForceEnabled, true);
        } else {
            serverConfig.remove(configKey::managedSplitTunnelForceEnabled);
        }
        changed = true;
    }

    if (serverConfig.value(configKey::managedSplitTunnelClientResolvedExceptSites).toObject() != clientResolvedSites) {
        if (clientResolvedSites.isEmpty()) {
            serverConfig.remove(configKey::managedSplitTunnelClientResolvedExceptSites);
            serverConfig.remove(configKey::managedSplitTunnelClientResolvedAt);
        } else {
            serverConfig.insert(configKey::managedSplitTunnelClientResolvedExceptSites, clientResolvedSites);
        }
        changed = true;
    } else if (clientResolvedSites.isEmpty() && serverConfig.contains(configKey::managedSplitTunnelClientResolvedAt)) {
        serverConfig.remove(configKey::managedSplitTunnelClientResolvedAt);
        changed = true;
    }

    if (changed) {
        m_serversRepository->editServerJson(serverIndex, serverConfig);
        qInfo() << "ConnectionController: server routing rules synced for server" << serverIndex
                << "source sites" << managedExceptSites.size() << "resolved sites" << exceptSites.size()
                << "force" << forceEnabled;
        emit serverRoutingRulesChanged(serverIndex);
    }
    return changed;
}

void ConnectionController::cancelClientManagedSitesResolve()
{
    m_serverRoutingRulesClientResolveTimer.stop();
    m_isClientManagedSitesResolveInProgress = false;
    m_clientManagedSitesResolveServerIndex = -1;
    m_clientManagedSitesResolveQueue.clear();
    m_clientManagedSitesResolvedCache = {};
}

void ConnectionController::scheduleClientManagedSitesResolve(int serverIndex)
{
    if (!isConnected() || serverIndex < 0 || serverIndex >= m_serversRepository->serversCount()) {
        return;
    }

    const QJsonObject serverConfig = m_serversRepository->serverJson(serverIndex);
    const QJsonObject sourceSites = serverRoutingRulesSourceSites(serverConfig);
    if (!shouldRunClientManagedResolve(serverConfig, sourceSites)) {
        return;
    }

    m_clientManagedSitesResolveServerIndex = serverIndex;
    const int jitterMs = QRandomGenerator::global()->bounded(serverRoutingRulesClientResolveJitterMs + 1);
    const int delayMs = serverRoutingRulesClientResolveInitialDelayMs + jitterMs;
    qInfo() << "ConnectionController: scheduled client-side managed site resolve in" << delayMs << "ms for server" << serverIndex;
    m_serverRoutingRulesClientResolveTimer.start(delayMs);
}

void ConnectionController::startClientManagedSitesResolve()
{
    if (!isConnected() || m_isClientManagedSitesResolveInProgress) {
        return;
    }

    const int serverIndex = m_clientManagedSitesResolveServerIndex;
    if (serverIndex < 0 || serverIndex >= m_serversRepository->serversCount()) {
        return;
    }
    if (!isCurrentConnectionServerIndex(serverIndex)) {
        cancelClientManagedSitesResolve();
        return;
    }

    const QJsonObject serverConfig = m_serversRepository->serverJson(serverIndex);
    const QJsonObject sourceSites = serverRoutingRulesSourceSites(serverConfig);
    m_clientManagedSitesResolveQueue = managedResolveDomains(sourceSites);
    if (m_clientManagedSitesResolveQueue.isEmpty()) {
        return;
    }

    m_clientManagedSitesResolvedCache = clientResolvedSitesBoundToSource(serverConfig, sourceSites);
    m_clientManagedSitesResolveOldRouteMode = m_serversRepository->effectiveSiteRouteMode(
            serverIndex, m_appSettingsRepository->isSitesSplitTunnelingEnabled(), m_appSettingsRepository->routeMode());
    m_clientManagedSitesResolveOldSplitTunnelIps =
            effectiveSplitTunnelIpsForSync(serverIndex, m_clientManagedSitesResolveOldRouteMode);
    m_isClientManagedSitesResolveInProgress = true;
    qInfo() << "ConnectionController: starting client-side managed site resolve for server" << serverIndex
            << "domains" << m_clientManagedSitesResolveQueue.size();
    resolveNextClientManagedSite();
}

void ConnectionController::resolveNextClientManagedSite()
{
    if (!isConnected() || !m_isClientManagedSitesResolveInProgress) {
        m_isClientManagedSitesResolveInProgress = false;
        m_clientManagedSitesResolveQueue.clear();
        return;
    }

    if (m_clientManagedSitesResolveQueue.isEmpty()) {
        finishClientManagedSitesResolve();
        return;
    }

    const int serverIndex = m_clientManagedSitesResolveServerIndex;
    const QString domain = m_clientManagedSitesResolveQueue.takeFirst();
    QHostInfo::lookupHost(domain, this, [this, serverIndex, domain](const QHostInfo &hostInfo) {
        if (!isConnected() || !m_isClientManagedSitesResolveInProgress) {
            return;
        }
        if (serverIndex != m_clientManagedSitesResolveServerIndex || !isCurrentConnectionServerIndex(serverIndex)) {
            cancelClientManagedSitesResolve();
            return;
        }

        const QStringList resolvedIps = hostInfoIpv4Addresses(hostInfo);
        if (resolvedIps.isEmpty()) {
            qDebug() << "ConnectionController: client-side managed site resolve produced no IPv4 addresses for" << domain;
            resolveNextClientManagedSite();
            return;
        }

        const QString mergedIps =
                mergedStoredIps({ m_clientManagedSitesResolvedCache.value(domain).toString(),
                                  resolvedIps.join(QStringLiteral(", ")) });
        if (!mergedIps.isEmpty()) {
            m_clientManagedSitesResolvedCache.insert(domain, mergedIps);
        }
        qDebug() << "ConnectionController: client-side managed site resolved" << domain << resolvedIps;
        resolveNextClientManagedSite();
    });
}

void ConnectionController::finishClientManagedSitesResolve()
{
    const int serverIndex = m_clientManagedSitesResolveServerIndex;
    m_isClientManagedSitesResolveInProgress = false;
    m_clientManagedSitesResolveQueue.clear();

    if (!isConnected() || serverIndex < 0 || serverIndex >= m_serversRepository->serversCount()) {
        return;
    }
    if (!isCurrentConnectionServerIndex(serverIndex)) {
        cancelClientManagedSitesResolve();
        return;
    }

    QJsonObject serverConfig = m_serversRepository->serverJson(serverIndex);
    const QJsonObject sourceSites = serverRoutingRulesSourceSites(serverConfig);
    const QJsonObject resolvedCache = sitesBoundToSource(m_clientManagedSitesResolvedCache, sourceSites);

    bool changed = false;
    if (serverConfig.value(configKey::managedSplitTunnelClientResolvedExceptSites).toObject() != resolvedCache) {
        if (resolvedCache.isEmpty()) {
            serverConfig.remove(configKey::managedSplitTunnelClientResolvedExceptSites);
        } else {
            serverConfig.insert(configKey::managedSplitTunnelClientResolvedExceptSites, resolvedCache);
        }
        changed = true;
    }

    const QString resolvedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (serverConfig.value(configKey::managedSplitTunnelClientResolvedAt).toString() != resolvedAt) {
        serverConfig.insert(configKey::managedSplitTunnelClientResolvedAt, resolvedAt);
        changed = true;
    }

    if (changed) {
        m_serversRepository->editServerJson(serverIndex, serverConfig);
        emit serverRoutingRulesChanged(serverIndex);
    }

    const RouteMode newRouteMode = m_serversRepository->effectiveSiteRouteMode(
            serverIndex, m_appSettingsRepository->isSitesSplitTunnelingEnabled(), m_appSettingsRepository->routeMode());
    const QStringList newSplitTunnelIps = effectiveSplitTunnelIpsForSync(serverIndex, newRouteMode);
    if (newRouteMode != m_clientManagedSitesResolveOldRouteMode
        || newSplitTunnelIps != m_clientManagedSitesResolveOldSplitTunnelIps) {
        qInfo() << "ConnectionController: client-side managed site resolve changed active routes, reconnecting VPN";
        QTimer::singleShot(0, m_vpnConnection, &VpnConnection::reconnectToVpn);
    }
}

bool ConnectionController::isServiceReady() const
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    return Utils::processIsRunning(Utils::executable(SERVICE_NAME, false), true);
#else
    return true;
#endif
}

bool ConnectionController::isContainerSupported(DockerContainer container) const
{
    return ContainerUtils::isSupportedByCurrentPlatform(container);
}
