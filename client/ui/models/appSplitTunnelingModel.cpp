#include "appSplitTunnelingModel.h"

#include <QFileInfo>

AppSplitTunnelingModel::AppSplitTunnelingModel(std::shared_ptr<Settings> settings, QObject *parent)
    : QAbstractListModel(parent), m_settings(settings)
{
    auto routeMode = m_settings->getAppsRouteMode();
    if (routeMode == Settings::AppsRouteMode::VpnAllApps) {
        m_isSplitTunnelingEnabled = false;
        m_currentRouteMode = Settings::AppsRouteMode::VpnAllExceptApps;
    } else {
        m_isSplitTunnelingEnabled = true;
        m_currentRouteMode = routeMode;
    }
    m_apps = m_settings->getVpnApps(m_currentRouteMode);
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
    if (m_apps.contains(appInfo)) {
        return false;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_apps.append(appInfo);
    m_settings->setVpnApps(m_currentRouteMode, m_apps);
    endInsertRows();

    qDebug() << "app added " << appInfo.appName;
    return true;
}

void AppSplitTunnelingModel::removeApp(QModelIndex index)
{
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    m_apps.removeAt(index.row());
    m_settings->setVpnApps(m_currentRouteMode, m_apps);
    endRemoveRows();
}

int AppSplitTunnelingModel::getRouteMode()
{
    return m_currentRouteMode;
}

void AppSplitTunnelingModel::setRouteMode(int routeMode)
{
    beginResetModel();
    m_settings->setAppsRouteMode(static_cast<Settings::AppsRouteMode>(routeMode));
    m_currentRouteMode = m_settings->getAppsRouteMode();
    m_apps = m_settings->getVpnApps(m_currentRouteMode);
    endResetModel();
    emit routeModeChanged();
}

bool AppSplitTunnelingModel::isSplitTunnelingEnabled()
{
    return m_isSplitTunnelingEnabled;
}

void AppSplitTunnelingModel::toggleSplitTunneling(bool enabled)
{
    if (enabled) {
        setRouteMode(m_currentRouteMode);
    } else {
        m_settings->setAppsRouteMode(Settings::AppsRouteMode::VpnAllApps);
    }
    m_isSplitTunnelingEnabled = enabled;
    emit splitTunnelingToggled();
}

QHash<int, QByteArray> AppSplitTunnelingModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[AppPathRole] = "appPath";
    return roles;
}
