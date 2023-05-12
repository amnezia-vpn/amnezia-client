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

    if (role == IsDefaultRole) {
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

    DockerContainer container = ContainerProps::allContainers().at(index.row());

    switch (role) {
        case NameRole:
            return ContainerProps::containerHumanNames().value(container);
        case DescRole:
            return ContainerProps::containerDescriptions().value(container);
        case ConfigRole:
            return m_settings->containerConfig(m_selectedServerIndex, container);
        case ServiceTypeRole:
            return ContainerProps::containerService(container);
        case IsInstalledRole:
            return m_settings->containers(m_selectedServerIndex).contains(container);
        case IsDefaultRole:
            return container == m_settings->defaultContainer(m_selectedServerIndex);
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

DockerContainer ContainersModel::getDefaultContainer()
{
    return m_settings->defaultContainer(m_selectedServerIndex);
}

QHash<int, QByteArray> ContainersModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[DescRole] = "description";
    roles[ServiceTypeRole] = "serviceType";
    roles[IsInstalledRole] = "isInstalled";
    roles[IsDefaultRole] = "isDefault";
    return roles;
}
