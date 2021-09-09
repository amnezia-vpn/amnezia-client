#include "all_containers_model.h"

AllContainersModel::AllContainersModel(QObject *parent) :
    QAbstractListModel(parent)
{

}

int AllContainersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return amnezia::allContainers().size();
}

QHash<int, QByteArray> AllContainersModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[DescRole] = "desc";
    roles[TypeRole] = "is_vpn";
    roles[InstalledRole] = "installed";
    return roles;
}

QVariant AllContainersModel::data(const QModelIndex &index, int role) const
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
    if (role == TypeRole) {
        return isContainerVpnType(c);
    }
    return QVariant();
}

void AllContainersModel::setServerData(const QJsonObject &server)
{
    beginResetModel();
    m_serverData = server;
    endResetModel();
}


