#include "appSplitTunnelingController.h"

AppSplitTunnelingController::AppSplitTunnelingController(std::shared_ptr<Settings> settings)
    : m_settings(settings)
{
    m_currentRouteMode = m_settings->getAppsRouteMode();
    if (m_currentRouteMode == Settings::VpnAllApps) { // for old split tunneling configs
        m_settings->setAppsRouteMode(static_cast<Settings::AppsRouteMode>(Settings::VpnAllExceptApps));
        m_currentRouteMode = Settings::VpnAllExceptApps;
    }
    m_apps = m_settings->getVpnApps(m_currentRouteMode);
}

bool AppSplitTunnelingController::addApp(const amnezia::InstalledAppInfo &appInfo)
{
    if (m_apps.contains(appInfo)) {
        return false;
    }

    m_apps.append(appInfo);
    m_settings->setVpnApps(m_currentRouteMode, m_apps);

    return true;
}

void AppSplitTunnelingController::removeApp(int index)
{
    if (index < 0 || index >= m_apps.size()) {
        return;
    }

    m_apps.removeAt(index);
    m_settings->setVpnApps(m_currentRouteMode, m_apps);
}

void AppSplitTunnelingController::clearAppsList()
{
    m_apps.clear();
    m_settings->setVpnApps(m_currentRouteMode, m_apps);
}

void AppSplitTunnelingController::setRouteMode(int routeMode)
{
    m_settings->setAppsRouteMode(static_cast<Settings::AppsRouteMode>(routeMode));
    m_currentRouteMode = m_settings->getAppsRouteMode();
    m_apps = m_settings->getVpnApps(m_currentRouteMode);
}

void AppSplitTunnelingController::toggleSplitTunneling(bool enabled)
{
    m_settings->setAppsSplitTunnelingEnabled(enabled);
}

int AppSplitTunnelingController::getRouteMode() const
{
    return static_cast<int>(m_currentRouteMode);
}

bool AppSplitTunnelingController::isSplitTunnelingEnabled() const
{
    return m_settings->isAppsSplitTunnelingEnabled();
}

QVector<amnezia::InstalledAppInfo> AppSplitTunnelingController::getApps() const
{
    return m_apps;
}

