#include "appSplitTunnelingUiController.h"

#include <QFileInfo>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "logger.h"

#if defined(Q_OS_MACOS) && !defined(MACOS_NE)
    #include "platforms/macos/splittunnel/macosAppInfo.h"
#endif

namespace {
Logger logger("AppSplitTunnelingUi");
}

AppSplitTunnelingUiController::AppSplitTunnelingUiController(AppSplitTunnelingController* appSplitTunnelingController,
                                                              AppSplitTunnelingModel* appSplitTunnelingModel,
                                                              QObject *parent)
    : QObject(parent),
      m_appSplitTunnelingController(appSplitTunnelingController),
      m_appSplitTunnelingModel(appSplitTunnelingModel)
{
    m_appSplitTunnelingModel->updateModel(m_appSplitTunnelingController->getApps());
}

void AppSplitTunnelingUiController::addApp(const QString &appPath)
{
    logger.debug() << "addApp path=" << appPath;
    amnezia::InstalledAppInfo appInfo { "", "", appPath };
    if (!appPath.isEmpty()) {
        QFileInfo fileInfo(appPath);
        appInfo.appName = fileInfo.fileName();
    }

#if defined(Q_OS_MACOS) && !defined(MACOS_NE)
    const bool filled = MacOSAppInfo::fillFromPath(appPath, appInfo);
    logger.debug() << "fillFromPath ok=" << filled
                   << "name=" << appInfo.appName
                   << "bundleId=" << appInfo.packageName
                   << "path=" << appInfo.appPath;
#endif

    if (m_appSplitTunnelingController->addApp(appInfo)) {
        emit finished(tr("Application added: %1").arg(appInfo.appName));
    } else {
        emit errorOccurred(tr("The application has already been added"));
    }
}

void AppSplitTunnelingUiController::addApps(QVector<QPair<QString, QString>> apps)
{
    for (const auto &app : apps) {
        amnezia::InstalledAppInfo appInfo { app.first, app.second, "" };
        m_appSplitTunnelingController->addApp(appInfo);
    }
    emit finished(tr("The selected applications have been added"));
}

void AppSplitTunnelingUiController::removeApp(const int index)
{
    auto modelIndex = m_appSplitTunnelingModel->index(index);
    auto appPath = m_appSplitTunnelingModel->data(modelIndex, AppSplitTunnelingModel::Roles::AppPathRole).toString();
    logger.info() << "removeApp index=" << index << "path=" << appPath;
    m_appSplitTunnelingController->removeApp(index);

    QFileInfo fileInfo(appPath);
    emit finished(tr("Application removed: %1").arg(fileInfo.fileName()));
}

void AppSplitTunnelingUiController::toggleSplitTunneling(bool enabled)
{
    logger.debug() << "toggleSplitTunneling" << enabled
                   << "routeMode=" << static_cast<int>(m_appSplitTunnelingController->getRouteMode())
                   << "apps=" << m_appSplitTunnelingController->getApps().size();
#if defined(Q_OS_MACOS) && !defined(MACOS_NE)
    // Only the exclude mode is implemented on macOS; pin it before the feature
    // flag flips, so the reconcile triggered by the flag already sees it.
    if (enabled && m_appSplitTunnelingController->getRouteMode() != amnezia::AppsRouteMode::VpnAllExceptApps) {
        logger.info() << "macOS supports exclude mode only, forcing route mode from"
                      << static_cast<int>(m_appSplitTunnelingController->getRouteMode())
                      << "to VpnAllExceptApps";
        m_appSplitTunnelingController->setRouteMode(amnezia::AppsRouteMode::VpnAllExceptApps);
        emit routeModeChanged();
    }
#endif
    // The system extension is activated / torn down by CoreSignalHandlers, which
    // listens on SecureAppSettingsRepository::appsSplitTunnelingEnabledChanged.
    m_appSplitTunnelingController->toggleSplitTunneling(enabled);
    emit isSplitTunnelingEnabledChanged();
}

void AppSplitTunnelingUiController::setRouteMode(int routeMode)
{
    logger.info() << "setRouteMode" << routeMode
                  << "(was" << static_cast<int>(m_appSplitTunnelingController->getRouteMode()) << ")";
    m_appSplitTunnelingController->setRouteMode(static_cast<amnezia::AppsRouteMode>(routeMode));
    emit routeModeChanged();
}

int AppSplitTunnelingUiController::getRouteMode() const
{
    return static_cast<int>(m_appSplitTunnelingController->getRouteMode());
}

bool AppSplitTunnelingUiController::isSplitTunnelingEnabled() const
{
    return m_appSplitTunnelingController->isSplitTunnelingEnabled();
}

void AppSplitTunnelingUiController::updateModel()
{
    const auto apps = m_appSplitTunnelingController->getApps();
    logger.debug() << "updateModel apps=" << apps.size();
    m_appSplitTunnelingModel->updateModel(apps);
}

#if defined(Q_OS_MACOS) && !defined(MACOS_NE)
QString AppSplitTunnelingUiController::pickMacosApp()
{
    return MacOSAppInfo::pickApplication();
}
#endif


