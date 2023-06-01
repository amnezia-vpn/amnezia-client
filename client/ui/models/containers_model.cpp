#include "containers_model.h"

#include "core/servercontroller.h"

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
        emit defaultContainerChanged();
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
        case IsEasySetupContainerRole:
            return ContainerProps::isEasySetupContainer(container);
        case EasySetupHeaderRole:
            return ContainerProps::easySetupHeader(container);
        case EasySetupDescriptionRole:
            return ContainerProps::easySetupDescription(container);
        case IsInstalledRole:
            return m_settings->containers(m_currentlyProcessedServerIndex).contains(container);
        case IsCurrentlyInstalledRole:
            return container == static_cast<DockerContainer>(m_currentlyInstalledContainerIndex);
        case IsDefaultRole:
            return container == m_settings->defaultContainer(m_currentlyProcessedServerIndex);
        case IsSupportedRole:
            return ContainerProps::isSupportedByCurrentPlatform(container);
    }

    return QVariant();
}

void ContainersModel::setCurrentlyProcessedServerIndex(int index)
{
    beginResetModel();
    m_currentlyProcessedServerIndex = index;
    endResetModel();
    emit defaultContainerChanged();
}

void ContainersModel::setCurrentlyInstalledContainerIndex(int index)
{
    m_currentlyInstalledContainerIndex = index;
}

DockerContainer ContainersModel::getDefaultContainer()
{
    return m_settings->defaultContainer(m_currentlyProcessedServerIndex);
}

QString ContainersModel::getDefaultContainerName()
{
    return ContainerProps::containerHumanNames().value(getDefaultContainer());
}

int ContainersModel::getCurrentlyInstalledContainerIndex()
{
    return m_currentlyInstalledContainerIndex;
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

    //todo process errors
}

void ContainersModel::clearCachedProfiles()
{
    const auto &containers = m_settings->containers(m_currentlyProcessedServerIndex);
    for (DockerContainer container : containers.keys()) {
        m_settings->clearLastConnectionConfig(m_currentlyProcessedServerIndex, container);
    }
}

QHash<int, QByteArray> ContainersModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[DescRole] = "description";
    roles[ServiceTypeRole] = "serviceType";
    roles[DockerContainerRole] = "dockerContainer";

    roles[IsEasySetupContainerRole] = "isEasySetupContainer";
    roles[EasySetupHeaderRole] = "easySetupHeader";
    roles[EasySetupDescriptionRole] = "easySetupDescription";

    roles[IsInstalledRole] = "isInstalled";
    roles[IsCurrentlyInstalledRole] = "isCurrentlyInstalled";
    roles[IsDefaultRole] = "isDefault";
    roles[IsSupportedRole] = "isSupported";
    return roles;
}
