#include "containers_model.h"

#include "core/controllers/serverController.h"

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
    case DescriptionRole:
        //        return ContainerProps::containerDescriptions().value(container);
    case ConfigRole: {
        m_settings->setContainerConfig(m_currentlyProcessedServerIndex, container, value.toJsonObject());
        m_containers = m_settings->containers(m_currentlyProcessedServerIndex);
        if (m_defaultContainerIndex != DockerContainer::None) {
            break;
        } else if (ContainerProps::containerService(container) == ServiceType::Other) {
            break;
        }
    }
    case ServiceTypeRole:
    //        return ContainerProps::containerService(container);
    case DockerContainerRole:
        //        return container;
    case IsInstalledRole:
    //        return m_settings->containers(m_currentlyProcessedServerIndex).contains(container);
    case IsDefaultRole: { //todo remove
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
    case IsDefaultRole: return container == m_defaultContainerIndex;
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

void ContainersModel::setCurrentlyProcessedServerIndex(const int index)
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

void ContainersModel::setDefaultContainer(int index)
{
    auto container = static_cast<DockerContainer>(index);
    m_settings->setDefaultContainer(m_currentlyProcessedServerIndex, container);
    m_defaultContainerIndex = container;
    emit defaultContainerChanged();
}

int ContainersModel::getCurrentlyProcessedContainerIndex()
{
    return m_currentlyProcessedContainerIndex;
}

QString ContainersModel::getCurrentlyProcessedContainerName()
{
    return ContainerProps::containerHumanNames().value(static_cast<DockerContainer>(m_currentlyProcessedContainerIndex));
}

QJsonObject ContainersModel::getCurrentlyProcessedContainerConfig()
{
    return qvariant_cast<QJsonObject>(data(index(m_currentlyProcessedContainerIndex), ConfigRole));
}

QStringList ContainersModel::getAllInstalledServicesName(const int serverIndex)
{
    QStringList servicesName;
    const auto &containers = m_settings->containers(serverIndex);
    for (const DockerContainer &container : containers.keys()) {
        if (ContainerProps::containerService(container) == ServiceType::Other && m_containers.contains(container)) {
            if (container == DockerContainer::Dns) {
                servicesName.append("DNS");
            } else if (container == DockerContainer::Sftp) {
                servicesName.append("SFTP");
            } else if (container == DockerContainer::TorWebSite) {
                servicesName.append("TOR");
            }
        }
    }
    servicesName.sort();
    return servicesName;
}

ErrorCode ContainersModel::removeAllContainers()
{

    ServerController serverController(m_settings);
    ErrorCode errorCode =
            serverController.removeAllContainers(m_settings->serverCredentials(m_currentlyProcessedServerIndex));

    if (errorCode == ErrorCode::NoError) {
        beginResetModel();

        m_settings->setContainers(m_currentlyProcessedServerIndex, {});
        m_containers = m_settings->containers(m_currentlyProcessedServerIndex);

        setData(index(DockerContainer::None, 0), true, IsDefaultRole);
        endResetModel();
    }
    return errorCode;
}

ErrorCode ContainersModel::removeCurrentlyProcessedContainer()
{
    ServerController serverController(m_settings);
    auto credentials = m_settings->serverCredentials(m_currentlyProcessedServerIndex);
    auto dockerContainer = static_cast<DockerContainer>(m_currentlyProcessedContainerIndex);

    ErrorCode errorCode = serverController.removeContainer(credentials, dockerContainer);

    if (errorCode == ErrorCode::NoError) {
        beginResetModel();
        m_settings->removeContainerConfig(m_currentlyProcessedServerIndex, dockerContainer);
        m_containers = m_settings->containers(m_currentlyProcessedServerIndex);

        if (m_defaultContainerIndex == m_currentlyProcessedContainerIndex) {
            if (m_containers.isEmpty()) {
                setData(index(DockerContainer::None, 0), true, IsDefaultRole);
            } else {
                setData(index(m_containers.begin().key(), 0), true, IsDefaultRole);
            }
        }
        endResetModel();
    }
    return errorCode;
}

void ContainersModel::clearCachedProfiles()
{
    const auto &containers = m_settings->containers(m_currentlyProcessedServerIndex);
    for (DockerContainer container : containers.keys()) {
        m_settings->clearLastConnectionConfig(m_currentlyProcessedServerIndex, container);
    }
}

bool ContainersModel::isAmneziaDnsContainerInstalled()
{
    return m_containers.contains(DockerContainer::Dns);
}

bool ContainersModel::isAmneziaDnsContainerInstalled(const int serverIndex)
{
    QMap<DockerContainer, QJsonObject> containers = m_settings->containers(serverIndex);
    return containers.contains(DockerContainer::Dns);
}

bool ContainersModel::isAnyContainerInstalled()
{
    for (int row=0; row < rowCount(); row++) {
        QModelIndex idx = this->index(row, 0);

        if (this->data(idx, IsInstalledRole).toBool() &&
            this->data(idx, ServiceTypeRole).toInt() == ServiceType::Vpn) {
            return true;
        }
    }

    return false;
}

void ContainersModel::updateContainersConfig()
{
    m_containers = m_settings->containers(m_currentlyProcessedServerIndex);
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
    roles[IsDefaultRole] = "isDefault";
    roles[IsSupportedRole] = "isSupported";
    roles[IsShareableRole] = "isShareable";
    return roles;
}
