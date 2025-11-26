#include "sitesController.h"

SitesController::SitesController(AppSettingsRepository* appSettingsRepository)
    : m_appSettingsRepository(appSettingsRepository)
{
    m_currentRouteMode = m_appSettingsRepository->routeMode();
    if (m_currentRouteMode == RouteMode::VpnAllSites) { // for old split tunneling configs
        m_appSettingsRepository->setRouteMode(RouteMode::VpnOnlyForwardSites);
        m_currentRouteMode = RouteMode::VpnOnlyForwardSites;
    }
    fillSites();
}

bool SitesController::addSite(const QString &hostname, const QString &ip)
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

void SitesController::removeSite(const QString &hostname)
{
    m_appSettingsRepository->removeVpnSite(m_currentRouteMode, hostname);
    for (int i = 0; i < m_sites.size(); i++) {
        if (m_sites[i].first == hostname) {
            m_sites.removeAt(i);
            break;
        }
    }
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

