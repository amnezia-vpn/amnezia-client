#include "installedAppsModel.h"

#include "platforms/android/android_controller.h"

InstalledAppsModel::InstalledAppsModel(QObject *parent) : QAbstractListModel(parent)
{
}

int InstalledAppsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_installedApps.size();
}

QVariant InstalledAppsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(rowCount()))
        return QVariant();

    switch (role) {
    case AppNameRole: {
        return m_installedApps.at(index.row()).toObject().value("name");
    }
    case AppIconRole: {
        return m_installedApps.at(index.row()).toObject().value("package").toString();
    }
    case PackageNameRole: {
        return m_installedApps.at(index.row()).toObject().value("package");
    }
    }

    return QVariant();
}

void InstalledAppsModel::selectedStateChanged(const int index, const bool selected)
{
    if (selected) {
        m_selectedAppIndexes.insert(index);
    } else {
        m_selectedAppIndexes.remove(index);
    }
}

QVector<QPair<QString, QString>> InstalledAppsModel::getSelectedAppsInfo()
{
    QVector<QPair<QString, QString>> appsInfo;
    for (const auto i : m_selectedAppIndexes) {
        QString packageName = data(index(i, 0), PackageNameRole).toString();
        QString appName = data(index(i, 0), AppNameRole).toString();
        if (appName.isEmpty()) {
            appName = packageName;
        }

        appsInfo.push_back({appName, packageName});
    }

    return appsInfo;
}

void InstalledAppsModel::updateModel()
{
    beginResetModel();
    m_installedApps = AndroidController::instance()->getAppList();
    endResetModel();
}

QHash<int, QByteArray> InstalledAppsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[AppNameRole] = "appName";
    roles[AppIconRole] = "appIcon";
    roles[PackageNameRole] = "packageName";
    return roles;
}
