#include "appSplitUIController.h"

#include <QFileInfo>

#include "core/defs.h"

AppSplitUIController::AppSplitUIController(QSharedPointer<SplitTunnelingController> splitTunnelingController,
                                           const QSharedPointer<AppSplitTunnelingModel> &appSplitTunnelingModel, 
                                           QObject *parent)
    : QObject(parent), m_splitTunnelingController(splitTunnelingController), m_appSplitTunnelingModel(appSplitTunnelingModel)
{
}

void AppSplitUIController::addApp(const QString &appPath)
{
    if (appPath.isEmpty()) {
        return;
    }

    InstalledAppInfo appInfo { "", "", appPath };
    if (m_splitTunnelingController->addApp(appInfo)) {
        emit finished(tr("Application added"));
    } else {
        emit errorOccurred(tr("The application has already been added"));
    }
}

void AppSplitUIController::addApps(QVector<QPair<QString, QString>> apps)
{
    bool success = false;
    for (const auto &app : apps) {
        InstalledAppInfo appInfo { app.first, app.second, "" };
        if (m_splitTunnelingController->addApp(appInfo)) {
            success = true;
        }
    }
    
    if (success) {
        emit finished(tr("The selected applications have been added"));
    } else {
        emit errorOccurred(tr("Failed to add applications"));
    }
}

void AppSplitUIController::removeApp(const int index)
{
    auto modelIndex = m_appSplitTunnelingModel->index(index);
    auto appPath = m_appSplitTunnelingModel->data(modelIndex, AppSplitTunnelingModel::Roles::AppPathRole).toString();
    auto appName = m_appSplitTunnelingModel->data(modelIndex, AppSplitTunnelingModel::Roles::AppNameRole).toString();
    
    InstalledAppInfo appInfo { appName, "", appPath };
    
    if (m_splitTunnelingController->removeApp(appInfo)) {
        QFileInfo fileInfo(appPath);
        emit finished(tr("Application removed: %1").arg(fileInfo.fileName()));
    } else {
        emit errorOccurred(tr("Failed to remove application"));
    }
}
