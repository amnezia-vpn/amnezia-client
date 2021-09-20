#include "containers_model.h"

ContainersModel::ContainersModel(QObject *parent) :
    QAbstractListModel(parent)
{

}

int ContainersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ContainerProps::allContainers().size();
}

QHash<int, QByteArray> ContainersModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name_role";
    roles[DescRole] = "desc_role";
    roles[DefaultRole] = "default_role";
    roles[ServiceTypeRole] = "service_type_role";
    roles[IsInstalledRole] = "is_installed_role";
    return roles;
}

QVariant ContainersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
            || index.row() >= ContainerProps::allContainers().size()) {
        return QVariant();
    }

    DockerContainer c = ContainerProps::allContainers().at(index.row());
    if (role == NameRole) {
        return ContainerProps::containerHumanNames().value(c);
    }
    if (role == DescRole) {
        return ContainerProps::containerDescriptions().value(c);
    }
    if (role == DefaultRole) {
        return c == m_settings.defaultContainer(m_selectedServerIndex);
    }
    if (role == ServiceTypeRole) {
        return ContainerProps::containerService(c);
    }
    if (role == IsInstalledRole) {
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


