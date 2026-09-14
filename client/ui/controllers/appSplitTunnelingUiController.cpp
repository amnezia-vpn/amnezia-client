#include "appSplitTunnelingUiController.h"

#include <QFileInfo>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "logger.h"

#if defined(Q_OS_MACOS) && !defined(MACOS_NE)
    #include "platforms/macos/splittunnel/macosAppInfo.h"
    #include "platforms/macos/splittunnel/macosSplitTunnelManager.h"
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
    m_appSplitTunnelingController->removeApp(index);

    QFileInfo fileInfo(appPath);
    emit finished(tr("Application removed: %1").arg(fileInfo.fileName()));
}

void AppSplitTunnelingUiController::toggleSplitTunneling(bool enabled)
{
    logger.debug() << "toggleSplitTunneling" << enabled
                   << "routeMode=" << static_cast<int>(m_appSplitTunnelingController->getRouteMode())
                   << "apps=" << m_appSplitTunnelingController->getApps().size();
    m_appSplitTunnelingController->toggleSplitTunneling(enabled);
#if defined(Q_OS_MACOS) && !defined(MACOS_NE)
    if (enabled) {
        if (m_appSplitTunnelingController->getRouteMode() != amnezia::AppsRouteMode::VpnAllExceptApps) {
            m_appSplitTunnelingController->setRouteMode(amnezia::AppsRouteMode::VpnAllExceptApps);
            emit routeModeChanged();
        }
        logger.debug() << "toggle on: activateExtension";
        MacOSSplitTunnelManager::instance()->activateExtension();
    }
#endif
    emit isSplitTunnelingEnabledChanged();
}

void AppSplitTunnelingUiController::setRouteMode(int routeMode)
{
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
    m_appSplitTunnelingModel->updateModel(m_appSplitTunnelingController->getApps());
}

#if defined(Q_OS_MACOS) && !defined(MACOS_NE)
QString AppSplitTunnelingUiController::pickMacosApp()
{
    return MacOSAppInfo::pickApplication();
}
#endif


