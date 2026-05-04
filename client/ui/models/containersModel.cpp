#include "containersModel.h"

#include <QJsonArray>

#include "core/models/protocolConfig.h"

using namespace amnezia;

ContainersModel::ContainersModel(QObject *parent) : QAbstractListModel(parent)
{
}

int ContainersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ContainerUtils::allContainers().size();
}

QVariant ContainersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerUtils::allContainers().size()) {
        return QVariant();
    }

    DockerContainer container = ContainerUtils::allContainers().at(index.row());
    bool isThirdPartyConfig = false;
    if (m_containers.contains(container)) {
        const ContainerConfig& config = m_containers.value(container);
        isThirdPartyConfig = config.protocolConfig.isThirdPartyConfig();
    }

    switch (role) {
    case NameRole: {
        if (container == DockerContainer::Awg && !isThirdPartyConfig) {
            return "AmneziaWG Legacy";
        }
        return ContainerUtils::containerHumanNames().value(container);
    }
    case DescriptionRole: {
        if (container == DockerContainer::Awg && !isThirdPartyConfig) {
            return QObject::tr("AmneziaWG is a special protocol from Amnezia based on WireGuard. "
                           "It provides high connection speed and ensures stable operation even in the most challenging network conditions.");
        }

        return ContainerUtils::containerDescriptions().value(container);
    }
    case DetailedDescriptionRole: return ContainerUtils::containerDetailedDescriptions().value(container);
    case ConfigRole: {
        if (container == DockerContainer::None) {
            return QJsonObject();
        }
        if (m_containers.contains(container)) {
            return m_containers.value(container).toJson();
        }
        return QJsonObject();
    }
    case IsThirdPartyConfigRole: return isThirdPartyConfig;
    case ServiceTypeRole: return ContainerUtils::containerService(container);
    case DockerContainerRole: return container;
    case ContainerStringRole: return ContainerUtils::containerToString(container);
    case IsEasySetupContainerRole: return ContainerUtils::isEasySetupContainer(container);
    case EasySetupHeaderRole: return ContainerUtils::easySetupHeader(container);
    case EasySetupDescriptionRole: return ContainerUtils::easySetupDescription(container);
    case EasySetupOrderRole: return ContainerUtils::easySetupOrder(container);
    case IsInstallationAllowedRole: return ContainersModel::isInstallationAllowed(container);
    case IsInstalledRole: return m_containers.contains(container);
    case IsCurrentlyProcessedRole: return container == static_cast<DockerContainer>(m_processedContainerIndex);
    case IsSupportedRole: return ContainerUtils::isSupportedByCurrentPlatform(container);
    case IsShareableRole: return ContainerUtils::isShareable(container);
    case IsVpnContainerRole: return ContainerUtils::containerService(container) == ServiceType::Vpn;
    case IsServiceContainerRole: return ContainerUtils::containerService(container) == ServiceType::Other;
    case IsIpsecRole: return container == DockerContainer::Ipsec;
    case IsDnsRole: return container == DockerContainer::Dns;
    case IsSftpRole: return container == DockerContainer::Sftp;
    case IsTorWebsiteRole: return container == DockerContainer::TorWebSite;
    case IsSocks5ProxyRole: return container == DockerContainer::Socks5Proxy;
    case IsMtProxyRole: return container == DockerContainer::MtProxy;
    case InstallPageOrderRole: return ContainerUtils::installPageOrder(container);
    }

    return QVariant();
}

QVariant ContainersModel::data(const int index, int role) const
{
    QModelIndex modelIndex = this->index(index);
    return data(modelIndex, role);
}

void ContainersModel::updateModel(const QMap<DockerContainer, ContainerConfig> &containers)
{
    beginResetModel();
    m_containers = containers;
    endResetModel();
}

void ContainersModel::setProcessedContainerIndex(int index)
{
    m_processedContainerIndex = index;
}

QString ContainersModel::getProcessedContainerName()
{
    return ContainerUtils::containerHumanNames().value(static_cast<DockerContainer>(m_processedContainerIndex));
}

QJsonObject ContainersModel::getContainerConfig(const int containerIndex)
{
    return qvariant_cast<QJsonObject>(data(index(containerIndex), ConfigRole));
}

bool ContainersModel::isSupportedByCurrentPlatform(const int containerIndex)
{
    return qvariant_cast<bool>(data(index(containerIndex), IsSupportedRole));
}

bool ContainersModel::isServiceContainer(const int containerIndex)
{
    return qvariant_cast<ServiceType>(data(index(containerIndex), ServiceTypeRole) == ServiceType::Other);
}

bool ContainersModel::hasInstalledServices()
{
    for (const auto &container : m_containers.keys()) {
        if (ContainerUtils::containerService(container) == ServiceType::Other) {
            return true;
        }
    }
    return false;
}

bool ContainersModel::hasInstalledProtocols()
{
    for (const auto &container : m_containers.keys()) {
        if (ContainerUtils::containerService(container) == ServiceType::Vpn) {
            return true;
        }
    }
    return false;
}

bool ContainersModel::isInstallationAllowed(DockerContainer container)
{
    return container != DockerContainer::Awg;
}

void ContainersModel::openContainerSettings(int containerIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    
    // This method will be connected to QML signals to open appropriate settings page
    // The actual navigation will be handled in QML based on container type
    // For now, we emit a signal that QML can listen to
    // In a full implementation, this would directly call PageController or emit a signal
}

QHash<int, QByteArray> ContainersModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[DescriptionRole] = "description";
    roles[DetailedDescriptionRole] = "detailedDescription";
    roles[ServiceTypeRole] = "serviceType";
    roles[DockerContainerRole] = "dockerContainer";
    roles[ContainerStringRole] = "containerString";
    roles[ConfigRole] = "config";
    roles[IsThirdPartyConfigRole] = "isThirdPartyConfig";

    roles[IsEasySetupContainerRole] = "isEasySetupContainer";
    roles[EasySetupHeaderRole] = "easySetupHeader";
    roles[EasySetupDescriptionRole] = "easySetupDescription";
    roles[EasySetupOrderRole] = "easySetupOrder";

    roles[IsInstalledRole] = "isInstalled";
    roles[IsCurrentlyProcessedRole] = "isCurrentlyProcessed";
    roles[IsSupportedRole] = "isSupported";
    roles[IsShareableRole] = "isShareable";
    roles[IsInstallationAllowedRole] = "isInstallationAllowed";
    roles[InstallPageOrderRole] = "installPageOrder";
    
    roles[IsVpnContainerRole] = "isVpnContainer";
    roles[IsServiceContainerRole] = "isServiceContainer";
    roles[IsIpsecRole] = "isIpsec";
    roles[IsDnsRole] = "isDns";
    roles[IsSftpRole] = "isSftp";
    roles[IsTorWebsiteRole] = "isTorWebsite";
    roles[IsSocks5ProxyRole] = "isSocks5Proxy";
    roles[IsMtProxyRole] = "isMtProxy";
    return roles;
}
