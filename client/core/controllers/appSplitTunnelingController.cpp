#include "appSplitTunnelingController.h"

#include "logger.h"

namespace {
Logger logger("AppSplitTunneling");

void logApps(const char *where, const QVector<amnezia::InstalledAppInfo> &apps)
{
    logger.debug() << where << "count=" << apps.size();
    for (int i = 0; i < apps.size(); ++i) {
        const auto &app = apps.at(i);
        logger.debug() << where << i
                       << "name=" << app.appName
                       << "bundleId=" << app.packageName
                       << "path=" << app.appPath;
    }
}
} // namespace

AppSplitTunnelingController::AppSplitTunnelingController(SecureAppSettingsRepository* appSettingsRepository)
    : m_appSettingsRepository(appSettingsRepository)
{
    m_currentRouteMode = m_appSettingsRepository->appsRouteMode();
    if (m_currentRouteMode == AppsRouteMode::VpnAllApps) { // for old split tunneling configs
        m_currentRouteMode = AppsRouteMode::VpnAllExceptApps;
        m_apps = m_appSettingsRepository->vpnApps(m_currentRouteMode);
        m_appSettingsRepository->setAppsRouteMode(AppsRouteMode::VpnAllExceptApps);
    } else {
        m_apps = m_appSettingsRepository->vpnApps(m_currentRouteMode);
    }
    logger.debug() << "ctor routeMode=" << static_cast<int>(m_currentRouteMode)
                   << "enabled=" << m_appSettingsRepository->isAppsSplitTunnelingEnabled();
    logApps("ctor", m_apps);
}

bool AppSplitTunnelingController::addApp(const amnezia::InstalledAppInfo &appInfo)
{
    logger.debug() << "addApp name=" << appInfo.appName
                   << "bundleId=" << appInfo.packageName
                   << "path=" << appInfo.appPath
                   << "routeMode=" << static_cast<int>(m_currentRouteMode);
    if (m_apps.contains(appInfo)) {
        logger.debug() << "addApp skipped: already in list";
        return false;
    }

    m_apps.append(appInfo);
    m_appSettingsRepository->setVpnApps(m_currentRouteMode, m_apps);
    logApps("after addApp", m_apps);

    return true;
}

void AppSplitTunnelingController::removeApp(int index)
{
    if (index < 0 || index >= m_apps.size()) {
        return;
    }

    m_apps.removeAt(index);
    m_appSettingsRepository->setVpnApps(m_currentRouteMode, m_apps);
}

void AppSplitTunnelingController::clearAppsList()
{
    m_apps.clear();
    m_appSettingsRepository->setVpnApps(m_currentRouteMode, m_apps);
}

void AppSplitTunnelingController::setRouteMode(AppsRouteMode routeMode)
{
    m_currentRouteMode = routeMode;
    m_apps = m_appSettingsRepository->vpnApps(m_currentRouteMode);
    m_appSettingsRepository->setAppsRouteMode(routeMode);
}

void AppSplitTunnelingController::toggleSplitTunneling(bool enabled)
{
    logger.debug() << "toggleSplitTunneling" << enabled
                   << "routeMode=" << static_cast<int>(m_currentRouteMode);
    logApps("toggleSplitTunneling", m_apps);
    m_appSettingsRepository->setAppsSplitTunnelingEnabled(enabled);
}

AppsRouteMode AppSplitTunnelingController::getRouteMode() const
{
    return m_currentRouteMode;
}

bool AppSplitTunnelingController::isSplitTunnelingEnabled() const
{
    return m_appSettingsRepository->isAppsSplitTunnelingEnabled();
}

QVector<amnezia::InstalledAppInfo> AppSplitTunnelingController::getApps() const
{
    return m_apps;
}

