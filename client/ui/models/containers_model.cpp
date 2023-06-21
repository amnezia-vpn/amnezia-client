#include "containers_model.h"

#include "core/servercontroller.h"

ContainersModel::ContainersModel(std::shared_ptr<Settings> settings, QObject *parent)
    : m_settings(settings), QAbstractListModel(parent)
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
    case ConfigRole: {
        m_settings->setContainerConfig(m_currentlyProcessedServerIndex, container, value.toJsonObject());
        m_containers = m_settings->containers(m_currentlyProcessedServerIndex);
    }
    case ServiceTypeRole:
    //        return ContainerProps::containerService(container);
    case DockerContainerRole:
        //        return container;
    case IsInstalledRole:
    //        return m_settings->containers(m_currentlyProcessedServerIndex).contains(container);
    case IsDefaultRole: {
        m_settings->setDefaultContainer(m_currentlyProcessedServerIndex, container);
        m_defaultContainerIndex = container;
        emit defaultContainerChanged();
    }
    }

    emit dataChanged(index, index);
    return true;
}

QVariant ContainersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerProps::allContainers().size()) {
        return QVariant();
    }

    DockerContainer container = ContainerProps::allContainers().at(index.row());

    switch (role) {
    case NameRole: return ContainerProps::containerHumanNames().value(container);
    case DescRole: return ContainerProps::containerDescriptions().value(container);
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
    case IsInstalledRole: return m_containers.contains(container);
    case IsCurrentlyProcessedRole: return container == static_cast<DockerContainer>(m_currentlyProcessedContainerIndex);
    case IsDefaultRole: return container == m_defaultContainerIndex;
    case IsSupportedRole: return ContainerProps::isSupportedByCurrentPlatform(container);
    }

    return QVariant();
}

void ContainersModel::setCurrentlyProcessedServerIndex(int index)
{
    beginResetModel();
    m_currentlyProcessedServerIndex = index;
    m_containers = m_settings->containers(m_currentlyProcessedServerIndex);
    m_defaultContainerIndex = m_settings->defaultContainer(m_currentlyProcessedServerIndex);
    endResetModel();
    emit defaultContainerChanged();
}

void ContainersModel::setCurrentlyProcessedContainerIndex(int index)
{
    m_currentlyProcessedContainerIndex = index;
}

DockerContainer ContainersModel::getDefaultContainer()
{
    return m_defaultContainerIndex;
}

QString ContainersModel::getDefaultContainerName()
{
    return ContainerProps::containerHumanNames().value(m_defaultContainerIndex);
}

int ContainersModel::getCurrentlyProcessedContainerIndex()
{
    return m_currentlyProcessedContainerIndex;
}

void ContainersModel::removeAllContainers()
{

    ServerController serverController(m_settings);
    auto errorCode = serverController.removeAllContainers(m_settings->serverCredentials(m_currentlyProcessedServerIndex));

    if (errorCode == ErrorCode::NoError) {
        beginResetModel();
        m_settings->setContainers(m_currentlyProcessedServerIndex, {});
        m_settings->setDefaultContainer(m_currentlyProcessedServerIndex, DockerContainer::None);
        endResetModel();
    }

    // todo process errors
}

void ContainersModel::clearCachedProfiles()
{
    const auto &containers = m_settings->containers(m_currentlyProcessedServerIndex);
    for (DockerContainer container : containers.keys()) {
        m_settings->clearLastConnectionConfig(m_currentlyProcessedServerIndex, container);
    }
}

QHash<int, QByteArray> ContainersModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[DescRole] = "description";
    roles[ServiceTypeRole] = "serviceType";
    roles[DockerContainerRole] = "dockerContainer";

    roles[IsEasySetupContainerRole] = "isEasySetupContainer";
    roles[EasySetupHeaderRole] = "easySetupHeader";
    roles[EasySetupDescriptionRole] = "easySetupDescription";

    roles[IsInstalledRole] = "isInstalled";
    roles[IsCurrentlyProcessedRole] = "isCurrentlyProcessed";
    roles[IsDefaultRole] = "isDefault";
    roles[IsSupportedRole] = "isSupported";
    return roles;
}
