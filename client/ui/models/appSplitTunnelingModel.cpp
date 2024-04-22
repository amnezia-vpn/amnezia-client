#include "appSplitTunnelingModel.h"

#include <QFileInfo>

AppSplitTunnelingModel::AppSplitTunnelingModel(std::shared_ptr<Settings> settings, QObject *parent)
    : QAbstractListModel(parent), m_settings(settings)
{
    m_isSplitTunnelingEnabled = m_settings->getAppsSplitTunnelingEnabled();
    m_currentRouteMode = m_settings->getAppsRouteMode();
    if (m_currentRouteMode == Settings::VpnAllApps) { // for old split tunneling configs
        m_settings->setAppsRouteMode(static_cast<Settings::AppsRouteMode>(Settings::VpnAllExceptApps));
        m_currentRouteMode = Settings::VpnAllExceptApps;
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
    m_settings->setAppsSplitTunnelingEnabled(enabled);
    m_isSplitTunnelingEnabled = enabled;
    emit splitTunnelingToggled();
}

QHash<int, QByteArray> AppSplitTunnelingModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[AppPathRole] = "appPath";
    return roles;
}
