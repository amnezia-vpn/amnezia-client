#include "serversUiController.h"

#include "core/utils/api/apiEnums.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include <QJsonDocument>
#include <QJsonArray>
#include "core/models/serverConfig.h"
#include "core/models/protocolConfig.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"

using namespace amnezia;

namespace
{
    namespace configKey
    {
        constexpr char apiConfig[] = "api_config";
        constexpr char serverCountryCode[] = "server_country_code";
        constexpr char serverCountryName[] = "server_country_name";
        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serviceType[] = "service_type";
    }
}

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

void ServersUiController::removeServer(int index)
{
    m_serversController->removeServer(index);
    updateModel();
}

void ServersUiController::editServerName(int index, const QString &name)
{
    ServerConfig serverConfig = m_serversController->getServerConfig(index);
    
    if (serverConfig.isApiV1()) {
        ApiV1ServerConfig* apiV1 = serverConfig.as<ApiV1ServerConfig>();
        if (apiV1) {
            apiV1->name = name;
        }
    } else if (serverConfig.isApiV2()) {
        ApiV2ServerConfig* apiV2 = serverConfig.as<ApiV2ServerConfig>();
        if (apiV2) {
            apiV2->name = name;
            apiV2->nameOverriddenByUser = true;
        }
    } else {
        serverConfig.visit([&name](auto& arg) {
            arg.description = name;
        });
    }
    
    m_serversController->editServer(index, serverConfig);
    updateModel();
}

void ServersUiController::setDefaultServerIndex(int index)
{
    m_serversController->setDefaultServerIndex(index);
    updateModel();
    emit defaultServerIndexChanged(index);
}

void ServersUiController::setDefaultContainer(int serverIndex, int containerIndex)
{
    auto container = static_cast<DockerContainer>(containerIndex);
    m_serversController->setDefaultContainer(serverIndex, container);
    updateModel();
}

void ServersUiController::toggleAmneziaDns(bool enabled)
{
    m_settingsController->toggleAmneziaDns(enabled);
    updateModel();
}

void ServersUiController::onDefaultServerChanged(int index)
{
    setProcessedServerIndex(index);
    updateModel();
    updateDefaultServerContainersModel();
    emit defaultServerIndexChanged(index);
}

void ServersUiController::updateModel()
{
    int defaultIndex = m_serversController->getDefaultServerIndex();
    bool wasEmpty = !hasServersFromGatewayApi();
    int serversCount = m_serversController->getServersCount();

    if (m_processedServerIndex >= serversCount) {
        setProcessedServerIndex(defaultIndex);
    } else if (m_processedServerIndex >= 0) {
        setProcessedServerIndex(m_processedServerIndex);
    }
    
    m_serversModel->updateModel(m_serversController->getServers(), defaultIndex, m_settingsController->isAmneziaDnsEnabled());
    
    
    updateContainersModel();
    updateDefaultServerContainersModel();
    
    bool isEmpty = !hasServersFromGatewayApi();
    if (wasEmpty != isEmpty) {
        emit hasServersFromGatewayApiChanged();
    }
    
    emit defaultServerIndexChanged(defaultIndex);
}

int ServersUiController::getDefaultServerIndex() const
{
    return m_serversController->getDefaultServerIndex();
}

QString ServersUiController::getDefaultServerName() const
{
    int defaultIndex = getDefaultServerIndex();
    return m_serversController->getServerConfig(defaultIndex).displayName();
}

QString ServersUiController::getDefaultServerDefaultContainerName() const
{
    int defaultIndex = getDefaultServerIndex();
    const ServerConfig server = m_serversController->getServerConfig(defaultIndex);
    return ContainerUtils::containerHumanNames().value(server.defaultContainer());
}

QString ServersUiController::getDefaultServerDescriptionCollapsed() const
{
    int defaultIndex = getDefaultServerIndex();
    const ServerConfig server = m_serversController->getServerConfig(defaultIndex);
    QString description = getDefaultServerDescription(server, defaultIndex);
    
    if (server.isApiConfig()) {
        return description;
    }
    
    DockerContainer container = server.defaultContainer();
    QString containerName = ContainerUtils::containerHumanNames().value(container);
    QString protocolVersion;
    QString hostName = server.hostName();
    
    if (ContainerUtils::isAwgContainer(container)) {
        ContainerConfig containerConfig = server.containerConfig(container);
        if (auto* awgProtocolConfig = containerConfig.getAwgProtocolConfig()) {
            QString version = awgProtocolConfig->serverConfig.protocolVersion;
            if (version == protocols::awg::awgV2) {
                protocolVersion = QObject::tr(" (version 2)");
            } else if (version == protocols::awg::awgV1_5) {
                protocolVersion = QObject::tr(" (version 1.5)");
            }
            
            if (container == DockerContainer::Awg && !awgProtocolConfig->serverConfig.isThirdPartyConfig) {
                containerName = "AmneziaWG Legacy";
            }
        }
    }
    
    return description + containerName + protocolVersion + " | " + hostName;
}

QString ServersUiController::getDefaultServerImagePathCollapsed() const
{
    int defaultIndex = getDefaultServerIndex();
    const ServerConfig server = m_serversController->getServerConfig(defaultIndex);
    
    if (server.isApiV2()) {
        const ApiV2ServerConfig* apiV2 = server.as<ApiV2ServerConfig>();
        if (!apiV2) return QString();
        const QString countryCode = apiV2->apiConfig.serverCountryCode;
        if (countryCode.isEmpty()) {
            return "";
        }
        return QString("qrc:/countriesFlags/images/flagKit/%1.svg").arg(countryCode.toUpper());
    }
    return "";
}

QString ServersUiController::getDefaultServerDescriptionExpanded() const
{
    int defaultIndex = getDefaultServerIndex();
    const ServerConfig server = m_serversController->getServerConfig(defaultIndex);
    QString description = getDefaultServerDescription(server, defaultIndex);
    
    if (server.isApiConfig()) {
        return description;
    }
    
    return description + server.hostName();
}

bool ServersUiController::isDefaultServerDefaultContainerHasSplitTunneling() const
{
    int defaultIndex = getDefaultServerIndex();
    const ServerConfig server = m_serversController->getServerConfig(defaultIndex);
    DockerContainer defaultContainer = server.defaultContainer();
    
    ContainerConfig containerConfig = server.containerConfig(defaultContainer);
    
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
    int defaultIndex = getDefaultServerIndex();
    const ServerConfig server = m_serversController->getServerConfig(defaultIndex);
    const int configVersion = server.configVersion();
    return configVersion == apiDefs::ConfigSource::Telegram
            || configVersion == apiDefs::ConfigSource::AmneziaGateway;
}

int ServersUiController::getProcessedServerIndex() const
{
    return m_processedServerIndex;
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

void ServersUiController::setProcessedServerIndex(int index)
{
    if (index >= m_serversController->getServersCount()) {
        return;
    }

    if (m_processedServerIndex != index) {
        m_processedServerIndex = index;
        m_serversModel->setProcessedServerIndex(index);

        if (index >= 0) {
            updateContainersModel();

            ServerConfig server = m_serversController->getServerConfig(index);
            setProcessedContainerIndex(static_cast<int>(server.defaultContainer()));

            if (server.isApiV2()) {
                const ApiV2ServerConfig* apiV2 = server.as<ApiV2ServerConfig>();
                if (apiV2 && !apiV2->apiConfig.availableCountries.isEmpty()) {
                    emit updateApiCountryModel();
                }
                emit updateApiServicesModel();
            }
        }

        emit processedServerIndexChanged(m_processedServerIndex);
    }
}

bool ServersUiController::processedServerIsPremium() const
{
    ServerConfig server = m_serversController->getServerConfig(m_processedServerIndex);
    if (server.isApiV1()) {
        const ApiV1ServerConfig* apiV1 = server.as<ApiV1ServerConfig>();
        return apiV1 ? apiV1->isPremium() : false;
    } else if (server.isApiV2()) {
        const ApiV2ServerConfig* apiV2 = server.as<ApiV2ServerConfig>();
        return apiV2 ? (apiV2->isPremium() || apiV2->isExternalPremium()) : false;
    }
    return false;
}

const ServerCredentials ServersUiController::getProcessedServerCredentials() const
{
    return m_serversController->getServerCredentials(m_processedServerIndex);
}

bool ServersUiController::isDefaultServerCurrentlyProcessed() const
{
    return m_serversController->getDefaultServerIndex() == m_processedServerIndex;
}

bool ServersUiController::isProcessedServerHasWriteAccess() const
{
    ServerCredentials credentials = m_serversController->getServerCredentials(m_processedServerIndex);
    return (!credentials.userName.isEmpty() && !credentials.secretData.isEmpty());
}


QString ServersUiController::getDefaultServerDescription(const ServerConfig& server, int index) const
{
    QString description;
    
    if (server.isApiV2()) {
        const ApiV2ServerConfig* apiV2 = server.as<ApiV2ServerConfig>();
        if (!apiV2) return QString();
        if (!apiV2->apiConfig.serverCountryCode.isEmpty()) {
            return apiV2->apiConfig.serverCountryName;
        }
        return apiV2->description;
    } else if (server.isApiV1()) {
        const ApiV1ServerConfig* apiV1 = server.as<ApiV1ServerConfig>();
        return apiV1 ? apiV1->description : QString();
    } else {
        ServerCredentials credentials = m_serversController->getServerCredentials(index);
        if (!credentials.userName.isEmpty() && !credentials.secretData.isEmpty()) {
            bool isAmneziaDnsEnabled = m_settingsController->isAmneziaDnsEnabled();
            if (isAmneziaDnsEnabled && isAmneziaDnsContainerInstalled(index)) {
                description += "Amnezia DNS | ";
            }
        } else {
            if (server.dns1() == protocols::dns::amneziaDnsIp) {
                description += "Amnezia DNS | ";
            }
        }
        return description;
    }
}

bool ServersUiController::isAmneziaDnsContainerInstalled(int serverIndex) const
{
    const ServerConfig server = m_serversController->getServerConfig(serverIndex);
    QMap<DockerContainer, ContainerConfig> containers = server.containers();
    
    return containers.contains(DockerContainer::Dns);
}

bool ServersUiController::hasServersFromGatewayApi() const
{
    QVector<ServerConfig> servers = m_serversController->getServers();
    for (const ServerConfig &server : servers) {
        if (server.isApiV2()) {
            return true;
        }
    }
    return false;
}

bool ServersUiController::isAdVisible() const
{
    int defaultIndex = getDefaultServerIndex();
    if (defaultIndex < 0) {
        return false;
    }
    ServerConfig server = m_serversController->getServerConfig(defaultIndex);
    if (server.isApiV2()) {
        const ApiV2ServerConfig* apiV2 = server.as<ApiV2ServerConfig>();
        if (!apiV2) return false;
        return apiV2->apiConfig.serviceInfo.isAdVisible;
    }
    return false;
}

QString ServersUiController::adHeader() const
{
    int defaultIndex = getDefaultServerIndex();
    if (defaultIndex < 0) {
        return QString();
    }
    ServerConfig server = m_serversController->getServerConfig(defaultIndex);
    if (server.isApiV2()) {
        const ApiV2ServerConfig* apiV2 = server.as<ApiV2ServerConfig>();
        if (!apiV2) return QString();
        return apiV2->apiConfig.serviceInfo.adHeader;
    }
    return QString();
}

QString ServersUiController::adDescription() const
{
    int defaultIndex = getDefaultServerIndex();
    if (defaultIndex < 0) {
        return QString();
    }
    ServerConfig server = m_serversController->getServerConfig(defaultIndex);
    if (server.isApiV2()) {
        const ApiV2ServerConfig* apiV2 = server.as<ApiV2ServerConfig>();
        if (!apiV2) return QString();
        return apiV2->apiConfig.serviceInfo.adDescription;
    }
    return QString();
}

void ServersUiController::updateContainersModel()
{
    if (m_processedServerIndex < 0 || m_processedServerIndex >= m_serversController->getServersCount()) {
        return;
    }
    ServerConfig server = m_serversController->getServerConfig(m_processedServerIndex);
    QMap<DockerContainer, ContainerConfig> containers = server.containers();
    m_containersModel->updateModel(containers);
}

void ServersUiController::updateDefaultServerContainersModel()
{
    int defaultIndex = m_serversController->getDefaultServerIndex();
    if (defaultIndex < 0 || defaultIndex >= m_serversController->getServersCount()) {
        return;
    }
    ServerConfig server = m_serversController->getServerConfig(defaultIndex);
    QMap<DockerContainer, ContainerConfig> containers = server.containers();
    m_defaultServerContainersModel->updateModel(containers);
}

QStringList ServersUiController::getAllInstalledServicesName(int serverIndex) const
{
    QStringList servicesName;
    ServerConfig server = m_serversController->getServerConfig(serverIndex);
    QMap<DockerContainer, ContainerConfig> containers = server.containers();
    
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
            }
        }
    }
    servicesName.sort();
    return servicesName;
}

