#include "appSplitTunnelingModel.h"

AppSplitTunnelingModel::AppSplitTunnelingModel(QObject *parent) : QAbstractListModel(parent)
{
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
        const auto &app = m_apps.at(index.row());
        return app.appPath;
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

void AppSplitTunnelingModel::clearAppsList()
{
    beginResetModel();
    m_apps.clear();
    m_settings->setVpnApps(m_currentRouteMode, m_apps);
    endResetModel();
}

int AppSplitTunnelingModel::getRouteMode()
{
    return m_currentRouteMode;
}

void AppSplitTunnelingModel::updateModel(const QVector<amnezia::InstalledAppInfo> &apps)
{
    beginResetModel();
    m_apps = apps;
    endResetModel();
}

QHash<int, QByteArray> AppSplitTunnelingModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[AppPathRole] = "appPath";
    return roles;
}
