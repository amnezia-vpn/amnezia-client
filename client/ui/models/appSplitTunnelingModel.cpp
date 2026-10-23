#include "appSplitTunnelingModel.h"

AppSplitTunnelingModel::AppSplitTunnelingModel(QObject *parent)
    : QAbstractListModel(parent)
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
            return m_apps.at(index.row()).appName;
        }
        default: {
            return true;
        }
    }

    return QVariant();
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
