#include "containers_model.h"

#include <QJsonArray>

ContainersModel::ContainersModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ContainersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ContainerProps::allContainers().size();
}

QVariant ContainersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerProps::allContainers().size()) {
        return QVariant();
    }

    DockerContainer container = ContainerProps::allContainers().at(index.row());

    switch (role) {
    case NameRole: return ContainerProps::containerHumanNames().value(container);
    case DescriptionRole: return ContainerProps::containerDescriptions().value(container);
    case DetailedDescriptionRole: return ContainerProps::containerDetailedDescriptions().value(container);
    case ConfigRole: {
        if (container == DockerContainer::None) {
            return QJsonObject();
        }
        return m_containers.value(container);
    }
    case ServiceTypeRole: return ContainerProps::containerService(container);
    case DockerContainerRole: return container;
    case IsEasySetupContainerRole: return ContainerProps::isEasySetupContainer(container);
    case EasySetupHeaderRole: return ContainerProps::easySetupHeader(container);
    case EasySetupDescriptionRole: return ContainerProps::easySetupDescription(container);
    case EasySetupOrderRole: return ContainerProps::easySetupOrder(container);
    case IsInstalledRole: return m_containers.contains(container);
    case IsCurrentlyProcessedRole: return container == static_cast<DockerContainer>(m_currentlyProcessedContainerIndex);
    case IsSupportedRole: return ContainerProps::isSupportedByCurrentPlatform(container);
    case IsShareableRole: return ContainerProps::isShareable(container);
    }

    return QVariant();
}

QVariant ContainersModel::data(const int index, int role) const
{
    QModelIndex modelIndex = this->index(index);
    return data(modelIndex, role);
}

void ContainersModel::updateModel(const QJsonArray &containers)
{
    beginResetModel();
    m_containers.clear();
    for (const QJsonValue &val : containers) {
        m_containers.insert(ContainerProps::containerFromString(val.toObject().value(config_key::container).toString()),
                             val.toObject());
    }
    endResetModel();
}

void ContainersModel::setCurrentlyProcessedContainerIndex(int index)
{
    m_currentlyProcessedContainerIndex = index;
}

int ContainersModel::getCurrentlyProcessedContainerIndex()
{
    return m_currentlyProcessedContainerIndex;
}

QString ContainersModel::getCurrentlyProcessedContainerName()
{
    return ContainerProps::containerHumanNames().value(static_cast<DockerContainer>(m_currentlyProcessedContainerIndex));
}

QJsonObject ContainersModel::getContainerConfig(const int containerIndex)
{
    return qvariant_cast<QJsonObject>(data(index(containerIndex), ConfigRole));
}

QHash<int, QByteArray> ContainersModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[DescriptionRole] = "description";
    roles[DetailedDescriptionRole] = "detailedDescription";
    roles[ServiceTypeRole] = "serviceType";
    roles[DockerContainerRole] = "dockerContainer";
    roles[ConfigRole] = "config";

    roles[IsEasySetupContainerRole] = "isEasySetupContainer";
    roles[EasySetupHeaderRole] = "easySetupHeader";
    roles[EasySetupDescriptionRole] = "easySetupDescription";
    roles[EasySetupOrderRole] = "easySetupOrder";

    roles[IsInstalledRole] = "isInstalled";
    roles[IsCurrentlyProcessedRole] = "isCurrentlyProcessed";
    roles[IsSupportedRole] = "isSupported";
    roles[IsShareableRole] = "isShareable";
    return roles;
}
