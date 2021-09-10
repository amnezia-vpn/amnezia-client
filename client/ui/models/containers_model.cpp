#include "containers_model.h"

ContainersModel::ContainersModel(QObject *parent) :
    QAbstractListModel(parent)
{

}

int ContainersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return amnezia::allContainers().size();
}

QHash<int, QByteArray> ContainersModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name_role";
    roles[DescRole] = "desc_role";
    roles[isVpnTypeRole] = "is_vpn_role";
    roles[isOtherTypeRole] = "is_other_role";
    roles[isInstalledRole] = "is_installed_role";
    return roles;
}

QVariant ContainersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
            || index.row() >= amnezia::allContainers().size()) {
        return QVariant();
    }

    DockerContainer c = amnezia::allContainers().at(index.row());
    if (role == NameRole) {
        return containerHumanNames().value(c);
    }
    if (role == DescRole) {
        return containerDescriptions().value(c);
    }
    if (role == isVpnTypeRole) {
        return isContainerVpnType(c);
    }
//    if (role == isOtherTypeRole) {
//        return isContainerVpnType(c)
//    }
    if (role == isInstalledRole) {
        return m_settings.containers(m_selectedServerIndex).contains(c);
    }
    return QVariant();
}

void ContainersModel::setSelectedServerIndex(int index)
{
    beginResetModel();
    m_selectedServerIndex = index;
    endResetModel();
}


