#include "containers_model.h"

ContainersModel::ContainersModel(std::shared_ptr<Settings> settings, QObject *parent) : m_settings(settings), QAbstractListModel(parent)
{
    setSelectedServerIndex(m_settings->defaultServerIndex());
}

int ContainersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ContainerProps::allContainers().size();
}

bool ContainersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerProps::allContainers().size()) {
        return false;
    }

    if (role == DefaultRole) {
        DockerContainer container = ContainerProps::allContainers().at(index.row());
        m_settings->setDefaultContainer(m_selectedServerIndex, container);
    }

    emit dataChanged(index, index);
    return true;
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
        return c == m_settings->defaultContainer(m_selectedServerIndex);
    }
    if (role == ServiceTypeRole) {
        return ContainerProps::containerService(c);
    }
    if (role == IsInstalledRole) {
        return m_settings->containers(m_selectedServerIndex).contains(c);
    }
    return QVariant();
}

void ContainersModel::setSelectedServerIndex(int index)
{
    beginResetModel();
    m_selectedServerIndex = index;
    endResetModel();
}

void ContainersModel::setCurrentlyInstalledContainerIndex(int index)
{
//    beginResetModel();
    m_currentlyInstalledContainerIndex = createIndex(index, 0);
//    endResetModel();
}

QString ContainersModel::getCurrentlyInstalledContainerName()
{
    return data(m_currentlyInstalledContainerIndex, NameRole).toString();
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
