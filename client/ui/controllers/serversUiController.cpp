#include "serversUiController.h"

#include "core/utils/api/apiUtils.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocolConfig.h"
#include "core/models/containerConfig.h"

using namespace amnezia;

namespace {
int rowForServerId(const QVector<ServerDescription> &list, const QString &serverId)
{
    if (serverId.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i).serverId == serverId) {
            return i;
        }
    }
    return -1;
}

bool descriptionsHaveGatewayServers(const QVector<ServerDescription> &list)
{
    for (const auto &d : list) {
        if (d.isServerFromGatewayApi) {
            return true;
        }
    }
    return false;
}
} // namespace
ServersUiController::ServersUiController(ServersController* serversController,
                                         SettingsController* settingsController,
                                         ServersModel* serversModel,
                                         ContainersModel* containersModel,
                                         ContainersModel* defaultServerContainersModel,
                                         QObject *parent)
    : QObject(parent),
      m_serversController(serversController),
      m_settingsController(settingsController),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_defaultServerContainersModel(defaultServerContainersModel)
{
}

void ServersUiController::removeServer(const QString &serverId)
{
    if (serverId.isEmpty()) {
        return;
    }
    m_serversController->removeServer(serverId);
    updateModel();
}

void ServersUiController::removeServerAtIndex(int index)
{
    const QString serverId = getServerId(index);
    if (!serverId.isEmpty()) {
        removeServer(serverId);
    }
}

void ServersUiController::setDefaultServerAtIndex(int index)
{
    const QString serverId = getServerId(index);
    if (!serverId.isEmpty()) {
        setDefaultServer(serverId);
    }
}

void ServersUiController::setDefaultContainerAtIndex(int index, int containerIndex)
{
    const QString serverId = getServerId(index);
    if (!serverId.isEmpty()) {
        setDefaultContainer(serverId, containerIndex);
    }
}

void ServersUiController::editServerName(const QString &serverId, const QString &name)
{
    if (serverId.isEmpty()) {
        return;
    }

    if (!m_serversController->renameServer(serverId, name)) {
        emit errorOccurred(tr("Legacy API v1 configs are no longer supported. Remove this server to continue."));
        emit finished(tr("Use the remove action to delete this legacy config."));
        return;
    }
    updateModel();
}

void ServersUiController::setDefaultServer(const QString &serverId)
{
    if (serverId.isEmpty()) {
        return;
    }
    m_serversController->setDefaultServer(serverId);
    updateModel();
    emit defaultServerIdChanged(serverId);
}

void ServersUiController::setDefaultContainer(const QString &serverId, int containerIndex)
{
    if (serverId.isEmpty()) {
        return;
    }
    auto container = static_cast<DockerContainer>(containerIndex);
    m_serversController->setDefaultContainer(serverId, container);
    updateModel();
}

void ServersUiController::toggleAmneziaDns(bool enabled)
{
    m_settingsController->toggleAmneziaDns(enabled);
    updateModel();
}

void ServersUiController::onDefaultServerChanged(const QString &/*defaultServerId*/)
{
    updateModel();
    setProcessedServerId(m_serversController->getDefaultServerId());
    updateDefaultServerContainersModel();
    emit defaultServerIdChanged(m_serversController->getDefaultServerId());
}

void ServersUiController::updateModel()
{
    QVector<ServerDescription> descriptions =
        m_serversController->buildServerDescriptions(m_settingsController->isAmneziaDnsEnabled());

    const QString defaultServerId = m_serversController->getDefaultServerId();
    const bool hadServersFromGatewayBefore = descriptionsHaveGatewayServers(m_orderedServerDescriptions);
    const bool hasServersFromGatewayNow = descriptionsHaveGatewayServers(descriptions);
    const int listCount = descriptions.size();
    const int defaultRowInDescriptions = rowForServerId(descriptions, defaultServerId);

    m_orderedServerDescriptions = descriptions;

    if (listCount == 0) {
        setProcessedServerId(QString());
    } else if (m_processedServerIndex >= listCount) {
        setProcessedServerId(defaultServerId);
    } else if (!m_processedServerId.isEmpty()) {
        const int row = rowForServerId(m_orderedServerDescriptions, m_processedServerId);
        if (row < 0) {
            setProcessedServerId(defaultServerId);
        } else {
            setProcessedServerId(m_processedServerId);
        }
    } else if (defaultRowInDescriptions >= 0) {
        setProcessedServerId(defaultServerId);
    }

    m_serversModel->updateModel(m_orderedServerDescriptions, defaultRowInDescriptions);

    updateContainersModel();
    updateDefaultServerContainersModel();

    if (hadServersFromGatewayBefore != hasServersFromGatewayNow) {
        emit hasServersFromGatewayApiChanged();
    }

    emit defaultServerIdChanged(defaultServerId);
    emit defaultServerIndexChanged(defaultServerIndex());
}

QString ServersUiController::getDefaultServerId() const
{
    return m_serversController->getDefaultServerId();
}

QString ServersUiController::getDefaultServerName() const
{
    const QString defaultServerId = m_serversController->getDefaultServerId();
    for (const auto &description : m_orderedServerDescriptions) {
        if (description.serverId == defaultServerId) {
            return description.serverName;
        }
    }
    return QString();
}

QString ServersUiController::getDefaultServerDefaultContainerName() const
{
    const QString defaultServerId = m_serversController->getDefaultServerId();
    for (const auto &description : m_orderedServerDescriptions) {
        if (description.serverId == defaultServerId) {
            return ContainerUtils::containerHumanNames().value(description.defaultContainer);
        }
    }
    return QString();
}

QString ServersUiController::getDefaultServerDescriptionCollapsed() const
{
    const QString defaultServerId = m_serversController->getDefaultServerId();
    for (const auto &description : m_orderedServerDescriptions) {
        if (description.serverId == defaultServerId) {
            return description.collapsedServerDescription;
        }
    }
    return QString();
}

QString ServersUiController::getDefaultServerImagePathCollapsed() const
{
    const QString defaultServerId = m_serversController->getDefaultServerId();
    for (const auto &description : m_orderedServerDescriptions) {
        if (description.serverId == defaultServerId) {
            if (!description.isApiV2 || description.apiServerCountryCode.isEmpty()) {
                return "";
            }
            const QString imageCode = apiUtils::countryCodeBaseForFlag(description.apiServerCountryCode.toUpper());
            if (imageCode.isEmpty()) {
                return QString();
            }
            return QString("qrc:/countriesFlags/images/flagKit/%1.svg").arg(imageCode);
        }
    }
    return "";
}

QString ServersUiController::getDefaultServerDescriptionExpanded() const
{
    const QString defaultServerId = m_serversController->getDefaultServerId();
    for (const auto &description : m_orderedServerDescriptions) {
        if (description.serverId == defaultServerId) {
            return description.expandedServerDescription;
        }
    }
    return QString();
}

bool ServersUiController::isDefaultServerDefaultContainerHasSplitTunneling() const
{
    const QString defaultServerId = m_serversController->getDefaultServerId();
    const DockerContainer defaultContainer = m_serversController->getDefaultContainer(defaultServerId);
    const ContainerConfig containerConfig = m_serversController->getContainerConfig(defaultServerId, defaultContainer);
    
    if (defaultContainer == DockerContainer::Awg || defaultContainer == DockerContainer::WireGuard) {
        auto hasSplitTunnelingFromAllowedIps = [](const QStringList& allowedIps, const QString& nativeConfig) -> bool {
            bool hasSplitTunneling = !allowedIps.isEmpty() && !allowedIps.contains("0.0.0.0/0");
            if (!hasSplitTunneling && !nativeConfig.isEmpty()) {
                hasSplitTunneling = nativeConfig.contains("AllowedIPs") 
                    && !nativeConfig.contains("AllowedIPs = 0.0.0.0/0, ::/0");
            }
            return hasSplitTunneling;
        };
        
        if (defaultContainer == DockerContainer::Awg) {
            if (const auto* awgConfig = containerConfig.getAwgProtocolConfig()) {
                if (awgConfig->hasClientConfig()) {
                    return hasSplitTunnelingFromAllowedIps(
                        awgConfig->clientConfig->allowedIps,
                        awgConfig->clientConfig->nativeConfig
                    );
                }
            }
        } else if (defaultContainer == DockerContainer::WireGuard) {
            if (const auto* wgConfig = containerConfig.getWireGuardProtocolConfig()) {
                if (wgConfig->hasClientConfig()) {
                    return hasSplitTunnelingFromAllowedIps(
                        wgConfig->clientConfig->allowedIps,
                        wgConfig->clientConfig->nativeConfig
                    );
                }
            }
        }
        return false;
    } else if (defaultContainer == DockerContainer::OpenVpn) {
        if (const auto* ovpnConfig = containerConfig.getOpenVpnProtocolConfig()) {
            if (ovpnConfig->hasClientConfig()) {
                return !ovpnConfig->clientConfig->nativeConfig.isEmpty() 
                    && !ovpnConfig->clientConfig->nativeConfig.contains("redirect-gateway");
            }
        }
    }
    return false;
}

bool ServersUiController::isDefaultServerFromApi() const
{
    const QString defaultServerId = m_serversController->getDefaultServerId();
    for (const auto &description : m_orderedServerDescriptions) {
        if (description.serverId == defaultServerId) {
            return description.isApiV2;
        }
    }
    return false;
}

int ServersUiController::getProcessedContainerIndex() const
{
    return m_processedContainerIndex;
}

void ServersUiController::setProcessedContainerIndex(int index)
{
    if (m_processedContainerIndex != index) {
        m_processedContainerIndex = index;
        m_containersModel->setProcessedContainerIndex(index);
        emit processedContainerIndexChanged(m_processedContainerIndex);
    }
}

QString ServersUiController::getProcessedServerId() const
{
    return m_processedServerId;
}

void ServersUiController::setProcessedServerId(const QString &serverId)
{
    const int index = serverId.isEmpty() ? -1 : serverIndexForId(serverId);
    if (!serverId.isEmpty() && index < 0) {
        return;
    }

    if (m_processedServerIndex != index || m_processedServerId != serverId) {
        m_processedServerIndex = index;
        m_processedServerId = serverId;
        m_serversModel->setProcessedServerIndex(index);

        if (index >= 0) {
            updateContainersModel();
            for (const auto &description : m_orderedServerDescriptions) {
                if (description.serverId == serverId) {
                    setProcessedContainerIndex(static_cast<int>(description.defaultContainer));
                    break;
                }
            }

            for (const auto &description : m_orderedServerDescriptions) {
                if (description.serverId != serverId) {
                    continue;
                }
                if (description.isApiV2) {
                    if (description.isCountrySelectionAvailable && !description.apiAvailableCountries.isEmpty()) {
                        emit updateApiCountryModel();
                    }
                    emit updateApiServicesModel();
                }
                break;
            }
        }

        emit processedServerIdChanged(m_processedServerId);
        emit processedServerIndexChanged(m_processedServerIndex);
    }
}

int ServersUiController::getProcessedServerIndex() const
{
    return m_processedServerIndex;
}

void ServersUiController::setProcessedServerIndex(int index)
{
    if (index < 0) {
        setProcessedServerId(QString());
        return;
    }
    const QString id = getServerId(index);
    if (!id.isEmpty()) {
        setProcessedServerId(id);
    }
}

int ServersUiController::defaultServerIndex() const
{
    return rowForServerId(m_orderedServerDescriptions, getDefaultServerId());
}

bool ServersUiController::processedServerIsPremium() const
{
    for (const auto &description : m_orderedServerDescriptions) {
        if (description.serverId == m_processedServerId) {
            return description.isPremium;
        }
    }
    return false;
}

const ServerCredentials ServersUiController::getProcessedServerCredentials() const
{
    return m_serversController->getServerCredentials(m_processedServerId);
}

bool ServersUiController::isDefaultServerCurrentlyProcessed() const
{
    return m_serversController->getDefaultServerId() == m_processedServerId;
}

bool ServersUiController::isProcessedServerHasWriteAccess() const
{
    ServerCredentials credentials = m_serversController->getServerCredentials(m_processedServerId);
    return (!credentials.userName.isEmpty() && !credentials.secretData.isEmpty());
}

QString ServersUiController::getDefaultServerDescription(const QString &serverId) const
{
    for (const auto &description : m_orderedServerDescriptions) {
        if (description.serverId == serverId) {
            return description.baseDescription;
        }
    }
    return QString();
}

bool ServersUiController::hasServersFromGatewayApi() const
{
    return listHasServersFromGatewayApi();
}

bool ServersUiController::isAdVisible() const
{
    const QString defaultServerId = m_serversController->getDefaultServerId();
    if (defaultServerId.isEmpty()) {
        return false;
    }
    for (const auto &description : m_orderedServerDescriptions) {
        if (description.serverId == defaultServerId) {
            return description.isAdVisible;
        }
    }
    return false;
}

QString ServersUiController::adHeader() const
{
    const QString defaultServerId = m_serversController->getDefaultServerId();
    if (defaultServerId.isEmpty()) {
        return QString();
    }
    for (const auto &description : m_orderedServerDescriptions) {
        if (description.serverId == defaultServerId) {
            return description.adHeader;
        }
    }
    return QString();
}

QString ServersUiController::adDescription() const
{
    const QString defaultServerId = m_serversController->getDefaultServerId();
    if (defaultServerId.isEmpty()) {
        return QString();
    }
    for (const auto &description : m_orderedServerDescriptions) {
        if (description.serverId == defaultServerId) {
            return description.adDescription;
        }
    }
    return QString();
}

QString ServersUiController::getServerId(int index) const
{
    if (index < 0 || index >= m_orderedServerDescriptions.size()) {
        return QString();
    }
    return m_orderedServerDescriptions.at(index).serverId;
}

int ServersUiController::getServerIndexById(const QString &serverId) const
{
    return rowForServerId(m_orderedServerDescriptions, serverId);
}

void ServersUiController::updateContainersModel()
{
    if (m_processedServerId.isEmpty()) {
        return;
    }
    const QMap<DockerContainer, ContainerConfig> containers =
            m_serversController->getServerContainersMap(m_processedServerId);
    m_containersModel->updateModel(containers);
}

void ServersUiController::updateDefaultServerContainersModel()
{
    const QString defaultServerId = m_serversController->getDefaultServerId();
    if (defaultServerId.isEmpty()) {
        return;
    }
    const QMap<DockerContainer, ContainerConfig> containers =
            m_serversController->getServerContainersMap(defaultServerId);
    m_defaultServerContainersModel->updateModel(containers);
}

QStringList ServersUiController::getAllInstalledServicesName(int serverIndex) const
{
    QStringList servicesName;
    const QString serverId = getServerId(serverIndex);
    const QMap<DockerContainer, ContainerConfig> containers = m_serversController->getServerContainersMap(serverId);

    for (auto it = containers.begin(); it != containers.end(); ++it) {
        DockerContainer container = it.key();
        if (ContainerUtils::containerService(container) == ServiceType::Other) {
            if (container == DockerContainer::Dns) {
                servicesName.append("DNS");
            } else if (container == DockerContainer::Sftp) {
                servicesName.append("SFTP");
            } else if (container == DockerContainer::TorWebSite) {
                servicesName.append("TOR");
            } else if (container == DockerContainer::Socks5Proxy) {
                servicesName.append("SOCKS5");
            } else if (container == DockerContainer::MtProxy) {
                servicesName.append("MTProxy");
            }
        }
    }
    servicesName.sort();
    return servicesName;
}

int ServersUiController::serverIndexForId(const QString &serverId) const
{
    return rowForServerId(m_orderedServerDescriptions, serverId);
}

bool ServersUiController::listHasServersFromGatewayApi() const
{
    return descriptionsHaveGatewayServers(m_orderedServerDescriptions);
}

