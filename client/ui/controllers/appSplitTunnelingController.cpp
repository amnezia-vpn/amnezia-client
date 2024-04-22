#include "appSplitTunnelingController.h"

#include <QFileInfo>

#include "core/defs.h"

AppSplitTunnelingController::AppSplitTunnelingController(const std::shared_ptr<Settings> &settings,
                                                         const QSharedPointer<AppSplitTunnelingModel> &appSplitTunnelingModel, QObject *parent)
    : QObject(parent), m_settings(settings), m_appSplitTunnelingModel(appSplitTunnelingModel)
{
}

void AppSplitTunnelingController::addApp(const QString &appPath)
{

    InstalledAppInfo appInfo { "", "", appPath };
    if (!appPath.isEmpty()) {
        QFileInfo fileInfo(appPath);
        appInfo.appName = fileInfo.fileName();
    }

    if (m_appSplitTunnelingModel->addApp(appInfo)) {
        emit finished(tr("Application added: %1").arg(appInfo.appName));

    } else {
        emit errorOccurred(tr("The application has already been added"));
    }
}

void AppSplitTunnelingController::addApps(QVector<QPair<QString, QString>> apps)
{
    for (const auto &app : apps) {
        InstalledAppInfo appInfo { app.first, app.second, "" };

        m_appSplitTunnelingModel->addApp(appInfo);
    }
    emit finished(tr("The selected applications have been added"));
}

void AppSplitTunnelingController::removeApp(const int index)
{
    auto modelIndex = m_appSplitTunnelingModel->index(index);
    auto appPath = m_appSplitTunnelingModel->data(modelIndex, AppSplitTunnelingModel::Roles::AppPathRole).toString();
    m_appSplitTunnelingModel->removeApp(modelIndex);

    QFileInfo fileInfo(appPath);

    emit finished(tr("Application removed: %1").arg(fileInfo.fileName()));
}
