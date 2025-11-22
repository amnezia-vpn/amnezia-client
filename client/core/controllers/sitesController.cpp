#include "sitesController.h"

SitesController::SitesController(std::shared_ptr<Settings> settings)
    : m_settings(settings)
{
    m_currentRouteMode = m_settings->routeMode();
    if (m_currentRouteMode == Settings::VpnAllSites) { // for old split tunneling configs
        m_settings->setRouteMode(static_cast<Settings::RouteMode>(Settings::VpnOnlyForwardSites));
        m_currentRouteMode = Settings::VpnOnlyForwardSites;
    }
    fillSites();
}

bool SitesController::addSite(const QString &hostname, const QString &ip)
{
    if (!m_settings->addVpnSite(m_currentRouteMode, hostname, ip)) {
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
        m_settings->removeAllVpnSites(m_currentRouteMode);
    }
    m_settings->addVpnSites(m_currentRouteMode, sites);
    fillSites();
}

void SitesController::removeSite(const QString &hostname)
{
    m_settings->removeVpnSite(m_currentRouteMode, hostname);
    for (int i = 0; i < m_sites.size(); i++) {
        if (m_sites[i].first == hostname) {
            m_sites.removeAt(i);
            break;
        }
    }
}

void SitesController::removeSites()
{
    m_settings->removeAllVpnSites(m_currentRouteMode);
    fillSites();
}

void SitesController::setRouteMode(int routeMode)
{
    m_settings->setRouteMode(static_cast<Settings::RouteMode>(routeMode));
    m_currentRouteMode = m_settings->routeMode();
    fillSites();
}

void SitesController::toggleSplitTunneling(bool enabled)
{
    m_settings->setSitesSplitTunnelingEnabled(enabled);
}

int SitesController::getRouteMode() const
{
    return static_cast<int>(m_currentRouteMode);
}

bool SitesController::isSplitTunnelingEnabled() const
{
    return m_settings->isSitesSplitTunnelingEnabled();
}

QVector<QPair<QString, QString>> SitesController::getCurrentSites() const
{
    return m_sites;
}

void SitesController::fillSites()
{
    m_sites.clear();
    const QVariantMap &sites = m_settings->vpnSites(m_currentRouteMode);
    auto i = sites.constBegin();
    while (i != sites.constEnd()) {
        m_sites.append(qMakePair(i.key(), i.value().toString()));
        ++i;
    }
}

