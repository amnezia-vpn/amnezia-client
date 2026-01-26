#include "sitesController.h"
#include "vpnConnection.h"
#include "core/utils/networkUtilities.h"

SitesController::SitesController(SecureAppSettingsRepository* appSettingsRepository, VpnConnection* vpnConnection, QObject* parent)
    : QObject(parent),
      m_appSettingsRepository(appSettingsRepository),
      m_vpnConnection(vpnConnection)
{
    m_currentRouteMode = m_appSettingsRepository->routeMode();
    if (m_currentRouteMode == RouteMode::VpnAllSites) { // for old split tunneling configs
        m_appSettingsRepository->setRouteMode(RouteMode::VpnOnlyForwardSites);
        m_currentRouteMode = RouteMode::VpnOnlyForwardSites;
    }
    fillSites();
}

bool SitesController::addSiteInternal(const QString &hostname, const QString &ip)
{
    if (!m_appSettingsRepository->addVpnSite(m_currentRouteMode, hostname, ip)) {
        return false;
    }
    for (int i = 0; i < m_sites.size(); i++) {
        if (m_sites[i].first == hostname && (m_sites[i].second.isEmpty() && !ip.isEmpty())) {
            m_sites[i].second = ip;
            return true;
        } else if (m_sites[i].first == hostname && (m_sites[i].second == ip)) {
            return false;
        }
    }
    m_sites.append(qMakePair(hostname, ip));
    return true;
}

void SitesController::addSites(const QMap<QString, QString> &sites, bool replaceExisting)
{
    if (replaceExisting) {
        m_appSettingsRepository->removeAllVpnSites(m_currentRouteMode);
    }
    m_appSettingsRepository->addVpnSites(m_currentRouteMode, sites);
    fillSites();
}

bool SitesController::addSite(const QString &hostname)
{
    QString normalizedHostname = normalizeHostname(hostname);
    
    if (!validateHostname(normalizedHostname)) {
        return false;
    }
    
    if (NetworkUtilities::ipAddressWithSubnetRegExp().exactMatch(normalizedHostname)) {
        processSite(normalizedHostname, "");
        return true;
    }
    
    if (addSiteInternal(normalizedHostname, "")) {
        QHostInfo::lookupHost(normalizedHostname, this, SLOT(onHostResolved(QHostInfo)));
        return true;
    }
    
    return false;
}

bool SitesController::removeSite(const QString &hostname)
{
    for (int i = 0; i < m_sites.size(); i++) {
        if (m_sites[i].first == hostname) {
            m_appSettingsRepository->removeVpnSite(m_currentRouteMode, hostname);
            m_sites.removeAt(i);
            deleteRoutes(QStringList() << hostname);
            return true;
        }
    }
    return false;
}

void SitesController::removeSites()
{
    m_appSettingsRepository->removeAllVpnSites(m_currentRouteMode);
    fillSites();
}

void SitesController::setRouteMode(RouteMode routeMode)
{
    m_appSettingsRepository->setRouteMode(routeMode);
    m_currentRouteMode = m_appSettingsRepository->routeMode();
    fillSites();
}

void SitesController::toggleSplitTunneling(bool enabled)
{
    m_appSettingsRepository->setSitesSplitTunnelingEnabled(enabled);
}

RouteMode SitesController::getRouteMode() const
{
    return m_currentRouteMode;
}

bool SitesController::isSplitTunnelingEnabled() const
{
    return m_appSettingsRepository->isSitesSplitTunnelingEnabled();
}

QVector<QPair<QString, QString>> SitesController::getCurrentSites() const
{
    return m_sites;
}

void SitesController::fillSites()
{
    QVariantMap sitesMap = m_appSettingsRepository->vpnSites(m_currentRouteMode);
    m_sites.clear();
    for (auto it = sitesMap.begin(); it != sitesMap.end(); ++it) {
        m_sites.append(qMakePair(it.key(), it.value().toString()));
    }
}

void SitesController::addRoutes(const QStringList& ips)
{
    if (m_vpnConnection) {
        QMetaObject::invokeMethod(m_vpnConnection, "addRoutes", Qt::QueuedConnection, Q_ARG(QStringList, ips));
    }
}

void SitesController::deleteRoutes(const QStringList& ips)
{
    if (m_vpnConnection) {
        QMetaObject::invokeMethod(m_vpnConnection, "deleteRoutes", Qt::QueuedConnection, Q_ARG(QStringList, ips));
    }
}

QString SitesController::normalizeHostname(const QString &hostname) const
{
    QString normalized = hostname;
    normalized.replace("https://", "");
    normalized.replace("http://", "");
    normalized.replace("ftp://", "");
    normalized = normalized.split("/", Qt::SkipEmptyParts).first();
    return normalized;
}

bool SitesController::validateHostname(const QString &hostname) const
{
    if (hostname.isEmpty()) {
        return false;
    }
    if (!hostname.contains(".") && !NetworkUtilities::ipAddressWithSubnetRegExp().exactMatch(hostname)) {
        return false;
    }
    return true;
}


void SitesController::onHostResolved(const QHostInfo &hostInfo)
{
    const QList<QHostAddress> &addresses = hostInfo.addresses();
    QString hostname = hostInfo.hostName();
    
    for (const QHostAddress &addr : addresses) {
        if (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
            processSiteAfterResolve(hostname, addr.toString());
            break;
        }
    }
}

void SitesController::processSiteAfterResolve(const QString &hostname, const QString &ip)
{
    for (int i = 0; i < m_sites.size(); i++) {
        if (m_sites[i].first == hostname && m_sites[i].second.isEmpty()) {
            m_sites[i].second = ip;
            m_appSettingsRepository->addVpnSite(m_currentRouteMode, hostname, ip);
            if (!ip.isEmpty()) {
                addRoutes(QStringList() << ip);
            }
            break;
        }
    }
}

void SitesController::processSite(const QString &hostname, const QString &ip)
{
    if (addSiteInternal(hostname, ip)) {
        if (!ip.isEmpty()) {
            addRoutes(QStringList() << ip);
        } else if (NetworkUtilities::ipAddressWithSubnetRegExp().exactMatch(hostname)) {
            addRoutes(QStringList() << hostname);
        }
    }
}

bool SitesController::importSitesFromJson(const QByteArray& jsonData, bool replaceExisting, QString &errorMessage)
{
    QJsonParseError parseError;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        errorMessage = tr("Failed to parse JSON data: %1").arg(parseError.errorString());
        return false;
    }
    
    if (!jsonDocument.isArray()) {
        errorMessage = tr("The JSON data is not an array");
        return false;
    }
    
    QJsonArray jsonArray = jsonDocument.array();
    QMap<QString, QString> sites;
    QStringList ips;
    
    for (auto jsonValue : jsonArray) {
        QJsonObject jsonObject = jsonValue.toObject();
        QString hostname = jsonObject.value("hostname").toString("");
        QString ip = jsonObject.value("ip").toString("");
        
        QString normalizedHostname = normalizeHostname(hostname);
        
        if (!validateHostname(normalizedHostname)) {
            qDebug() << normalizedHostname << " not look like ip adress or domain name";
            continue;
        }
        
        if (ip.isEmpty()) {
            ips.append(normalizedHostname);
        } else {
            ips.append(ip);
        }
        sites.insert(normalizedHostname, ip);
    }
    
    addSites(sites, replaceExisting);
    addRoutes(ips);
    
    return true;
}

QByteArray SitesController::exportSitesToJson() const
{
    QVector<QPair<QString, QString>> sites = getCurrentSites();
    QJsonArray jsonArray;
    
    for (const auto &site : sites) {
        QJsonObject jsonObject;
        jsonObject["hostname"] = site.first;
        jsonObject["ip"] = site.second;
        jsonArray.append(jsonObject);
    }
    
    QJsonDocument jsonDocument(jsonArray);
    return jsonDocument.toJson();
}

