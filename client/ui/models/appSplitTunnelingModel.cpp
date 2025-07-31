#include "appSplitTunnelingModel.h"
#include "core/controllers/splitTunnelingController.h"

#include <QFileInfo>

AppSplitTunnelingModel::AppSplitTunnelingModel(QSharedPointer<SplitTunnelingController> splitTunnelingController,
                                               QObject *parent)
    : QAbstractListModel(parent), 
      m_splitTunnelingController(splitTunnelingController)
{
    // Connect to controller signals
    connect(m_splitTunnelingController.data(), &SplitTunnelingController::appAdded,
            this, &AppSplitTunnelingModel::onAppAdded);
    connect(m_splitTunnelingController.data(), &SplitTunnelingController::appRemoved,
            this, &AppSplitTunnelingModel::onAppRemoved);
    connect(m_splitTunnelingController.data(), &SplitTunnelingController::appsRouteModelChanged,
            this, &AppSplitTunnelingModel::onAppsRouteModelChanged);
    connect(m_splitTunnelingController.data(), &SplitTunnelingController::appsSplitTunnelingToggled,
            this, &AppSplitTunnelingModel::onAppsSplitTunnelingToggled);

    // Initialize data
    refreshData();
}

int AppSplitTunnelingModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_apps.size();
}

QVariant AppSplitTunnelingModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(rowCount()))
        return QVariant();

    switch (role) {
    case AppPathRole: {
        return m_apps.at(index.row()).appName;
    }
    default: {
        return true;
    }
    }

    return QVariant();
}

bool AppSplitTunnelingModel::addApp(const InstalledAppInfo &appInfo)
{
    return m_splitTunnelingController->addApp(appInfo);
}

void AppSplitTunnelingModel::removeApp(QModelIndex index)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_apps.size()) {
        return;
    }
    
    const InstalledAppInfo &appInfo = m_apps.at(index.row());
    m_splitTunnelingController->removeApp(appInfo);
}

int AppSplitTunnelingModel::getRouteMode()
{
    return static_cast<int>(m_splitTunnelingController->getAppsRouteMode());
}

void AppSplitTunnelingModel::setRouteMode(int routeMode)
{
    m_splitTunnelingController->setAppsRouteMode(static_cast<Settings::AppsRouteMode>(routeMode));
}

bool AppSplitTunnelingModel::isSplitTunnelingEnabled()
{
    return m_splitTunnelingController->isAppsSplitTunnelingEnabled();
}

void AppSplitTunnelingModel::toggleSplitTunneling(bool enabled)
{
    m_splitTunnelingController->setAppsSplitTunnelingEnabled(enabled);
}

QHash<int, QByteArray> AppSplitTunnelingModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[AppPathRole] = "appPath";
    return roles;
}

void AppSplitTunnelingModel::onAppAdded(const InstalledAppInfo &appInfo)
{
    Q_UNUSED(appInfo)
    beginResetModel();
    refreshData();
    endResetModel();
}

void AppSplitTunnelingModel::onAppRemoved(const InstalledAppInfo &appInfo)
{
    Q_UNUSED(appInfo)
    beginResetModel();
    refreshData();
    endResetModel();
}

void AppSplitTunnelingModel::onAppsRouteModelChanged()
{
    beginResetModel();
    refreshData();
    endResetModel();
    emit routeModeChanged();
}

void AppSplitTunnelingModel::onAppsSplitTunnelingToggled()
{
    emit splitTunnelingToggled();
}

void AppSplitTunnelingModel::refreshData()
{
    m_apps = m_splitTunnelingController->getApps(m_splitTunnelingController->getAppsRouteMode());
}
