#include "serversUiController.h"

#include "core/utils/api/apiDefs.h"
#include "core/utils/api/apiUtils.h"
#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"
#include <QJsonDocument>
#include <QJsonArray>
#include "core/models/serverConfig.h"
#include "core/models/protocolConfig.h"
#include "core/models/containerConfig.h"

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
      m_defaultServerContainersModel(defaultServerContainersModel),
      m_processedServerIndex(serversController->getDefaultServerIndex())
{
    updateModel();
}

void ServersUiController::removeServer(int index)
{
    m_serversController->removeServer(index);
    updateModel();
}

void ServersUiController::editServerName(int index, const QString &name)
{
    ServerConfig serverConfig = m_serversController->getServerConfig(index);
    
    if (ServerConfigUtils::isApiV1Config(serverConfig)) {
        ApiV1ServerConfig& apiV1 = ServerConfigUtils::asApiV1(serverConfig);
        apiV1.name = name;
    } else if (ServerConfigUtils::isApiV2Config(serverConfig)) {
        ApiV2ServerConfig& apiV2 = ServerConfigUtils::asApiV2(serverConfig);
        apiV2.name = name;
    } else {
        ServerConfigUtils::visit(serverConfig, [&name](auto& arg) {
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

void ServersUiController::onAddServer(const ServerConfig& config)
{
    Q_UNUSED(config);
    updateModel();
}

void ServersUiController::onServerEdited(int index, const ServerConfig& config)
{
    Q_UNUSED(index);
    Q_UNUSED(config);
    updateModel();
}

void ServersUiController::onServerRemoved(int index)
{
    Q_UNUSED(index);
    updateModel();
}

void ServersUiController::onDefaultServerChanged(int index)
{
    if (m_processedServerIndex == -1 || m_processedServerIndex >= m_serversController->getServersCount()) {
        m_processedServerIndex = index;
        emit processedServerIndexChanged(m_processedServerIndex);
    }
    updateModel();
    updateDefaultServerContainersModel();
    emit defaultServerIndexChanged(index);
}

void ServersUiController::updateModel()
{
    int defaultIndex = m_serversController->getDefaultServerIndex();
    bool wasEmpty = !hasServersFromGatewayApi();
    
    m_serversModel->updateModel(m_serversController->getServersArray(), defaultIndex, m_settingsController->isAmneziaDnsEnabled());
    
    if (m_processedServerIndex < 0 || m_processedServerIndex >= m_serversController->getServersCount()) {
        m_processedServerIndex = defaultIndex;
        emit processedServerIndexChanged(m_processedServerIndex);
    }
    
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
    return qvariant_cast<QString>(m_serversModel->data(defaultIndex, ServersModel::Roles::NameRole));
}

QString ServersUiController::getDefaultServerDefaultContainerName() const
{
    int defaultIndex = getDefaultServerIndex();
    auto defaultContainer = qvariant_cast<DockerContainer>(m_serversModel->data(defaultIndex, ServersModel::Roles::DefaultContainerRole));
    return ContainerProps::containerHumanNames().value(defaultContainer);
}

QString ServersUiController::getDefaultServerDescriptionCollapsed() const
{
    int defaultIndex = getDefaultServerIndex();
    const ServerConfig server = m_serversController->getServerConfig(defaultIndex);
    QString description = getDefaultServerDescription(server, defaultIndex);
    
    if (ServerConfigUtils::isApiConfig(server)) {
        return description;
    }
    
    DockerContainer container = ServerConfigUtils::defaultContainer(server);
    QString hostName = ServerConfigUtils::hostName(server);
    return description + ContainerProps::containerHumanNames().value(container) + " | " + hostName;
}

QString ServersUiController::getDefaultServerImagePathCollapsed() const
{
    int defaultIndex = getDefaultServerIndex();
    const ServerConfig server = m_serversController->getServerConfig(defaultIndex);
    
    if (ServerConfigUtils::isApiV2Config(server)) {
        const ApiV2ServerConfig& apiV2 = ServerConfigUtils::asApiV2(server);
        const QString countryCode = apiV2.apiConfig.serverCountryCode;
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
    
    if (ServerConfigUtils::isApiConfig(server)) {
        return description;
    }
    
    return description + ServerConfigUtils::hostName(server);
}

bool ServersUiController::isDefaultServerDefaultContainerHasSplitTunneling() const
{
    int defaultIndex = getDefaultServerIndex();
    const ServerConfig server = m_serversController->getServerConfig(defaultIndex);
    DockerContainer defaultContainer = ServerConfigUtils::defaultContainer(server);
    
    ContainerConfig containerConfig = ServerConfigUtils::containerConfig(server, defaultContainer);
    
    if (defaultContainer == DockerContainer::Awg || defaultContainer == DockerContainer::WireGuard) {
        QJsonObject protocolConfigJson = ProtocolConfigUtils::toJson(containerConfig.protocolConfig, ContainerProps::defaultProtocol(defaultContainer));
        QString clientProtocolConfigString = protocolConfigJson.value(config_key::last_config).toString();
        QJsonObject clientProtocolConfig = QJsonDocument::fromJson(clientProtocolConfigString.toUtf8()).object();
        return (clientProtocolConfigString.contains("AllowedIPs") && !clientProtocolConfigString.contains("AllowedIPs = 0.0.0.0/0, ::/0"))
                || (!clientProtocolConfig.value(config_key::allowed_ips).toArray().isEmpty()
                    && !clientProtocolConfig.value(config_key::allowed_ips).toArray().contains("0.0.0.0/0"));
    } else if (defaultContainer == DockerContainer::OpenVpn) {
        QJsonObject protocolConfigJson = ProtocolConfigUtils::toJson(containerConfig.protocolConfig, Proto::OpenVpn);
        QString clientProtocolConfigString = protocolConfigJson.value(config_key::last_config).toString();
        return !clientProtocolConfigString.isEmpty() && !clientProtocolConfigString.contains("redirect-gateway");
    }
    return false;
}

bool ServersUiController::isDefaultServerFromApi() const
{
    int defaultIndex = getDefaultServerIndex();
    return m_serversModel->data(defaultIndex, ServersModel::Roles::IsServerFromTelegramApiRole).toBool()
            || m_serversModel->data(defaultIndex, ServersModel::Roles::IsServerFromGatewayApiRole).toBool();
}

int ServersUiController::getProcessedServerIndex() const
{
    return m_processedServerIndex;
}

void ServersUiController::setProcessedServerIndex(int index)
{
    if (index < 0 || index >= m_serversController->getServersCount()) {
        return;
    }
    
    if (m_processedServerIndex != index) {
        m_processedServerIndex = index;
        m_serversModel->setProcessedServerIndex(index);
        updateContainersModel();
        emit processedServerIndexChanged(m_processedServerIndex);
    }
}

bool ServersUiController::processedServerIsPremium() const
{
    ServerConfig server = m_serversController->getServerConfig(m_processedServerIndex);
    if (ServerConfigUtils::isApiV1Config(server)) {
        return ServerConfigUtils::asApiV1(server).isPremium();
    } else if (ServerConfigUtils::isApiV2Config(server)) {
        return ServerConfigUtils::asApiV2(server).isPremium();
    }
    return false;
}

QString ServersUiController::getDefaultServerDescription(const ServerConfig& server, int index) const
{
    QString description;
    
    if (ServerConfigUtils::isApiV2Config(server)) {
        const ApiV2ServerConfig& apiV2 = ServerConfigUtils::asApiV2(server);
        if (!apiV2.apiConfig.serverCountryCode.isEmpty()) {
            return apiV2.apiConfig.serverCountryName;
        }
        return apiV2.description;
    } else if (ServerConfigUtils::isApiV1Config(server)) {
        const ApiV1ServerConfig& apiV1 = ServerConfigUtils::asApiV1(server);
        return apiV1.description;
    } else {
        QString desc = ServerConfigUtils::description(server);
        if (m_serversModel->data(index, ServersModel::Roles::HasWriteAccessRole).toBool()) {
            bool isAmneziaDnsEnabled = m_settingsController->isAmneziaDnsEnabled();
            if (isAmneziaDnsEnabled && isAmneziaDnsContainerInstalled(index)) {
                description += "Amnezia DNS | ";
            }
        } else {
            if (m_serversModel->data(index, ServersModel::Roles::HasAmneziaDns).toBool()) {
                description += "Amnezia DNS | ";
            }
        }
        return description + desc;
    }
}

bool ServersUiController::isAmneziaDnsContainerInstalled(int serverIndex) const
{
    const ServerConfig server = m_serversController->getServerConfig(serverIndex);
    QMap<DockerContainer, ContainerConfig> containers = ServerConfigUtils::containers(server);
    
    return containers.contains(DockerContainer::Dns);
}

bool ServersUiController::hasServersFromGatewayApi() const
{
    QJsonArray servers = m_serversController->getServersArray();
    for (int i = 0; i < servers.size(); ++i) {
        QJsonObject server = servers.at(i).toObject();
        if (server.value(config_key::configVersion).toInt() == apiDefs::ConfigSource::AmneziaGateway) {
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
    if (ServerConfigUtils::isApiV2Config(server)) {
        const ApiV2ServerConfig& apiV2 = ServerConfigUtils::asApiV2(server);
        return apiV2.apiConfig.serviceInfo.isAdVisible;
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
    if (ServerConfigUtils::isApiV2Config(server)) {
        const ApiV2ServerConfig& apiV2 = ServerConfigUtils::asApiV2(server);
        return apiV2.apiConfig.serviceInfo.adHeader;
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
    if (ServerConfigUtils::isApiV2Config(server)) {
        const ApiV2ServerConfig& apiV2 = ServerConfigUtils::asApiV2(server);
        return apiV2.apiConfig.serviceInfo.adDescription;
    }
    return QString();
}

void ServersUiController::updateContainersModel()
{
    if (m_processedServerIndex < 0 || m_processedServerIndex >= m_serversController->getServersCount()) {
        return;
    }
    ServerConfig server = m_serversController->getServerConfig(m_processedServerIndex);
    QMap<DockerContainer, ContainerConfig> containers = ServerConfigUtils::containers(server);
    QJsonArray containersArray;
    for (auto it = containers.begin(); it != containers.end(); ++it) {
        QJsonObject containerObj = it.value().toJson();
        containerObj.insert(config_key::container, ContainerProps::containerToString(it.key()));
        containersArray.append(containerObj);
    }
    m_containersModel->updateModel(containersArray);
}

void ServersUiController::updateDefaultServerContainersModel()
{
    int defaultIndex = m_serversController->getDefaultServerIndex();
    if (defaultIndex < 0 || defaultIndex >= m_serversController->getServersCount()) {
        return;
    }
    ServerConfig server = m_serversController->getServerConfig(defaultIndex);
    QMap<DockerContainer, ContainerConfig> containers = ServerConfigUtils::containers(server);
    QJsonArray containersArray;
    for (auto it = containers.begin(); it != containers.end(); ++it) {
        QJsonObject containerObj = it.value().toJson();
        containerObj.insert(config_key::container, ContainerProps::containerToString(it.key()));
        containersArray.append(containerObj);
    }
    m_defaultServerContainersModel->updateModel(containersArray);
}

QStringList ServersUiController::getAllInstalledServicesName(int serverIndex) const
{
    QStringList servicesName;
    ServerConfig server = m_serversController->getServerConfig(serverIndex);
    QMap<DockerContainer, ContainerConfig> containers = ServerConfigUtils::containers(server);
    
    for (auto it = containers.begin(); it != containers.end(); ++it) {
        DockerContainer container = it.key();
        if (ContainerProps::containerService(container) == ServiceType::Other) {
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

