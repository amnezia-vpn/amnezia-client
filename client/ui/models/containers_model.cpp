#include "containers_model.h"

ContainersModel::ContainersModel(std::shared_ptr<Settings> settings, QObject *parent) : m_settings(settings), QAbstractListModel(parent)
{
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

    DockerContainer container = ContainerProps::allContainers().at(index.row());

    switch (role) {
    case NameRole:
//        return ContainerProps::containerHumanNames().value(container);
    case DescRole:
//        return ContainerProps::containerDescriptions().value(container);
    case ConfigRole:
        m_settings->setContainerConfig(m_currentlyProcessedServerIndex, container, value.toJsonObject());
    case ServiceTypeRole:
//        return ContainerProps::containerService(container);
    case DockerContainerRole:
//        return container;
    case IsInstalledRole:
//        return m_settings->containers(m_currentlyProcessedServerIndex).contains(container);
    case IsDefaultRole:
        m_settings->setDefaultContainer(m_currentlyProcessedServerIndex, container);
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
            return m_settings->containerConfig(m_currentlyProcessedServerIndex, container);
        case ServiceTypeRole:
            return ContainerProps::containerService(container);
        case DockerContainerRole:
            return container;
        case IsInstalledRole:
            return m_settings->containers(m_currentlyProcessedServerIndex).contains(container);
        case IsCurrentlyInstalled:
            return container == static_cast<DockerContainer>(m_currentlyInstalledContainerIndex);
        case IsDefaultRole:
            return container == m_settings->defaultContainer(m_currentlyProcessedServerIndex);
    }

    return QVariant();
}

void ContainersModel::setCurrentlyProcessedServerIndex(int index)
{
    beginResetModel();
    m_currentlyProcessedServerIndex = index;
    endResetModel();
}

void ContainersModel::setCurrentlyInstalledContainerIndex(int index)
{
    m_currentlyInstalledContainerIndex = index;
}

DockerContainer ContainersModel::getDefaultContainer()
{
    return m_settings->defaultContainer(m_currentlyProcessedServerIndex);
}

int ContainersModel::getCurrentlyInstalledContainerIndex()
{
    return m_currentlyInstalledContainerIndex;
}

QHash<int, QByteArray> ContainersModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[DescRole] = "description";
    roles[ServiceTypeRole] = "serviceType";
    roles[DockerContainerRole] = "dockerContainer";
    roles[IsInstalledRole] = "isInstalled";
    roles[IsCurrentlyInstalled] = "isCurrentlyInstalled";
    roles[IsDefaultRole] = "isDefault";
    return roles;
}
