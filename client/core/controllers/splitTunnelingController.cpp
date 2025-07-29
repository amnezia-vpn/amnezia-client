#include "splitTunnelingController.h"
#include "settings.h"

SplitTunnelingController::SplitTunnelingController(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
}

// Apps split tunneling implementation
bool SplitTunnelingController::addApp(const InstalledAppInfo &appInfo)
{
    auto currentApps = m_settings->getVpnApps(getAppsRouteMode());
    
    if (currentApps.contains(appInfo)) {
        return false;
    }
    
    currentApps.append(appInfo);
    m_settings->setVpnApps(getAppsRouteMode(), currentApps);
    
    emit appAdded(appInfo);
    return true;
}

bool SplitTunnelingController::removeApp(const InstalledAppInfo &appInfo)
{
    auto currentApps = m_settings->getVpnApps(getAppsRouteMode());
    
    if (!currentApps.contains(appInfo)) {
        return false;
    }
    
    currentApps.removeAll(appInfo);
    m_settings->setVpnApps(getAppsRouteMode(), currentApps);
    
    emit appRemoved(appInfo);
    return true;
}

QVector<InstalledAppInfo> SplitTunnelingController::getApps(Settings::AppsRouteMode routeMode) const
{
    return m_settings->getVpnApps(routeMode);
}

Settings::AppsRouteMode SplitTunnelingController::getAppsRouteMode() const
{
    return m_settings->getAppsRouteMode();
}

void SplitTunnelingController::setAppsRouteMode(Settings::AppsRouteMode routeMode)
{
    m_settings->setAppsRouteMode(routeMode);
    emit appsRouteModelChanged();
}

bool SplitTunnelingController::isAppsSplitTunnelingEnabled() const
{
    return m_settings->isAppsSplitTunnelingEnabled();
}

void SplitTunnelingController::setAppsSplitTunnelingEnabled(bool enabled)
{
    m_settings->setAppsSplitTunnelingEnabled(enabled);
    emit appsSplitTunnelingToggled();
}

// Sites split tunneling implementation
bool SplitTunnelingController::addSite(const QString &hostname, const QString &ip)
{
    if (!m_settings->addVpnSite(getSitesRouteMode(), hostname, ip)) {
        return false;
    }
    
    emit siteAdded(hostname, ip);
    return true;
}

bool SplitTunnelingController::addSites(const QMap<QString, QString> &sites, bool replaceExisting)
{
    if (replaceExisting) {
        m_settings->removeAllVpnSites(getSitesRouteMode());
    }
    
    m_settings->addVpnSites(getSitesRouteMode(), sites);
    
    // Emit signals for each added site
    auto i = sites.constBegin();
    while (i != sites.constEnd()) {
        emit siteAdded(i.key(), i.value());
        ++i;
    }
    
    return true;
}

bool SplitTunnelingController::removeSite(const QString &hostname)
{
    if (!m_settings->removeVpnSite(getSitesRouteMode(), hostname)) {
        return false;
    }
    
    emit siteRemoved(hostname);
    return true;
}

QVector<QPair<QString, QString>> SplitTunnelingController::getSites(Settings::RouteMode routeMode) const
{
    QVector<QPair<QString, QString>> sites;
    const QVariantMap &sitesMap = m_settings->vpnSites(routeMode);
    
    auto i = sitesMap.constBegin();
    while (i != sitesMap.constEnd()) {
        sites.append(qMakePair(i.key(), i.value().toString()));
        ++i;
    }
    
    return sites;
}

Settings::RouteMode SplitTunnelingController::getSitesRouteMode() const
{
    return m_settings->routeMode();
}

void SplitTunnelingController::setSitesRouteMode(Settings::RouteMode routeMode)
{
    m_settings->setRouteMode(routeMode);
    emit sitesRouteModelChanged();
}

bool SplitTunnelingController::isSitesSplitTunnelingEnabled() const
{
    return m_settings->isSitesSplitTunnelingEnabled();
}

void SplitTunnelingController::setSitesSplitTunnelingEnabled(bool enabled)
{
    m_settings->setSitesSplitTunnelingEnabled(enabled);
    emit sitesSplitTunnelingToggled();
} 
