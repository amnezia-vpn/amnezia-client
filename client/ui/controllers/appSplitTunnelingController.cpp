#include "appSplitTunnelingController.h"

#include <QFileInfo>

AppSplitTunnelingController::AppSplitTunnelingController(const std::shared_ptr<Settings> &settings,
                                                         const QSharedPointer<AppSplitTunnelingModel> &appSplitTunnelingModel, QObject *parent)
    : QObject(parent), m_settings(settings), m_appSplitTunnelingModel(appSplitTunnelingModel)
{
}

void AppSplitTunnelingController::addApp(const QString &appPath)
{
    QFileInfo fileInfo(appPath);
    if (fileInfo.isExecutable()) {
       emit errorOccurred(tr("The selected file is not executable"));
    }

    if (m_appSplitTunnelingModel->addApp(appPath)) {
        emit finished(tr("Application added: %1").arg(fileInfo.fileName()));

    } else {
        emit errorOccurred(tr("The application has already been added"));
    }
}

void AppSplitTunnelingController::removeApp(const int index)
{
    auto modelIndex = m_appSplitTunnelingModel->index(index);
    auto appPath = m_appSplitTunnelingModel->data(modelIndex, AppSplitTunnelingModel::Roles::AppPathRole).toString();
    m_appSplitTunnelingModel->removeApp(modelIndex);

    QFileInfo fileInfo(appPath);

    emit finished(tr("Application removed: %1").arg(fileInfo.fileName()));
}
