#include "serversUiController.h"

#include "core/utils/api/apiDefs.h"
#include "core/utils/api/apiUtils.h"
#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"
#include <QJsonDocument>
#include <QJsonArray>

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
    QJsonObject server = m_serversController->getServerConfig(index);
    const auto configVersion = server.value(config_key::configVersion).toInt();
    
    if (configVersion) {
        server.insert(config_key::name, name);
    } else {
        server.insert(config_key::description, name);
    }
    server.insert(config_key::nameOverriddenByUser, true);
    
    m_serversController->editServer(index, server);
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

void ServersUiController::onAddServer(QJsonObject config)
{
    Q_UNUSED(config);
    updateModel();
}

void ServersUiController::onServerEdited(int index, QJsonObject config)
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
    const QJsonObject server = m_serversController->getServerConfig(defaultIndex);
    const auto configVersion = server.value(config_key::configVersion).toInt();
    QString description = getDefaultServerDescription(server, defaultIndex);
    
    if (configVersion) {
        return description;
    }
    
    auto container = ContainerProps::containerFromString(server.value(config_key::defaultContainer).toString());
    return description += ContainerProps::containerHumanNames().value(container) + " | " + server.value(config_key::hostName).toString();
}

QString ServersUiController::getDefaultServerImagePathCollapsed() const
{
    int defaultIndex = getDefaultServerIndex();
    const QJsonObject server = m_serversController->getServerConfig(defaultIndex);
    const auto apiConfig = server.value(configKey::apiConfig).toObject();
    const auto countryCode = apiConfig.value(configKey::serverCountryCode).toString();
    
    if (countryCode.isEmpty()) {
        return "";
    }
    return QString("qrc:/countriesFlags/images/flagKit/%1.svg").arg(countryCode.toUpper());
}

QString ServersUiController::getDefaultServerDescriptionExpanded() const
{
    int defaultIndex = getDefaultServerIndex();
    const QJsonObject server = m_serversController->getServerConfig(defaultIndex);
    const auto configVersion = server.value(config_key::configVersion).toInt();
    QString description = getDefaultServerDescription(server, defaultIndex);
    
    if (configVersion) {
        return description;
    }
    
    return description += server.value(config_key::hostName).toString();
}

bool ServersUiController::isDefaultServerDefaultContainerHasSplitTunneling() const
{
    int defaultIndex = getDefaultServerIndex();
    const QJsonObject server = m_serversController->getServerConfig(defaultIndex);
    auto defaultContainer = ContainerProps::containerFromString(server.value(config_key::defaultContainer).toString());
    
    const QJsonArray containers = server.value(config_key::containers).toArray();
    for (auto i = 0; i < containers.size(); i++) {
        auto container = containers.at(i).toObject();
        if (container.value(config_key::container).toString() != ContainerProps::containerToString(defaultContainer)) {
            continue;
        }
        if (defaultContainer == DockerContainer::Awg || defaultContainer == DockerContainer::WireGuard) {
            QJsonObject serverProtocolConfig = container.value(ContainerProps::containerTypeToString(defaultContainer)).toObject();
            QString clientProtocolConfigString = serverProtocolConfig.value(config_key::last_config).toString();
            QJsonObject clientProtocolConfig = QJsonDocument::fromJson(clientProtocolConfigString.toUtf8()).object();
            return (clientProtocolConfigString.contains("AllowedIPs") && !clientProtocolConfigString.contains("AllowedIPs = 0.0.0.0/0, ::/0"))
                    || (!clientProtocolConfig.value(config_key::allowed_ips).toArray().isEmpty()
                        && !clientProtocolConfig.value(config_key::allowed_ips).toArray().contains("0.0.0.0/0"));
        } else if (defaultContainer == DockerContainer::Cloak || defaultContainer == DockerContainer::OpenVpn
                   || defaultContainer == DockerContainer::ShadowSocks) {
            auto serverProtocolConfig = container.value(ContainerProps::containerTypeToString(DockerContainer::OpenVpn)).toObject();
            QString clientProtocolConfigString = serverProtocolConfig.value(config_key::last_config).toString();
            return !clientProtocolConfigString.isEmpty() && !clientProtocolConfigString.contains("redirect-gateway");
        }
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
    QJsonObject server = m_serversController->getServerConfig(m_processedServerIndex);
    return apiUtils::isPremiumServer(server);
}

QString ServersUiController::getDefaultServerDescription(const QJsonObject &server, int index) const
{
    const auto configVersion = server.value(config_key::configVersion).toInt();
    const auto apiConfig = server.value(configKey::apiConfig).toObject();
    
    QString description;
    
    if (configVersion && !apiConfig.value(configKey::serverCountryCode).toString().isEmpty()) {
        return apiConfig.value(configKey::serverCountryName).toString();
    } else if (configVersion) {
        return server.value(config_key::description).toString();
    } else if (m_serversModel->data(index, ServersModel::Roles::HasWriteAccessRole).toBool()) {
        bool isAmneziaDnsEnabled = m_settingsController->isAmneziaDnsEnabled();
        if (isAmneziaDnsEnabled && isAmneziaDnsContainerInstalled(index)) {
            description += "Amnezia DNS | ";
        }
    } else {
        if (m_serversModel->data(index, ServersModel::Roles::HasAmneziaDns).toBool()) {
            description += "Amnezia DNS | ";
        }
    }
    return description;
}

bool ServersUiController::isAmneziaDnsContainerInstalled(int serverIndex) const
{
    const QJsonObject server = m_serversController->getServerConfig(serverIndex);
    const QJsonArray containers = server.value(config_key::containers).toArray();
    
    for (const auto &containerValue : containers) {
        QJsonObject containerObj = containerValue.toObject();
        DockerContainer containerType = ContainerProps::containerFromString(containerObj.value(config_key::container).toString());
        if (containerType == DockerContainer::Dns) {
            return true;
        }
    }
    return false;
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
    QJsonObject server = m_serversController->getServerConfig(defaultIndex);
    QJsonObject apiConfig = server.value(configKey::apiConfig).toObject();
    QJsonObject serviceInfo = apiConfig.value(apiDefs::key::serviceInfo).toObject();
    return serviceInfo.value(apiDefs::key::isAdVisible).toBool(false);
}

QString ServersUiController::adHeader() const
{
    int defaultIndex = getDefaultServerIndex();
    if (defaultIndex < 0) {
        return QString();
    }
    QJsonObject server = m_serversController->getServerConfig(defaultIndex);
    QJsonObject apiConfig = server.value(configKey::apiConfig).toObject();
    QJsonObject serviceInfo = apiConfig.value(apiDefs::key::serviceInfo).toObject();
    return serviceInfo.value(apiDefs::key::adHeader).toString();
}

QString ServersUiController::adDescription() const
{
    int defaultIndex = getDefaultServerIndex();
    if (defaultIndex < 0) {
        return QString();
    }
    QJsonObject server = m_serversController->getServerConfig(defaultIndex);
    QJsonObject apiConfig = server.value(configKey::apiConfig).toObject();
    QJsonObject serviceInfo = apiConfig.value(apiDefs::key::serviceInfo).toObject();
    return serviceInfo.value(apiDefs::key::adDescription).toString();
}

void ServersUiController::updateContainersModel()
{
    if (m_processedServerIndex < 0 || m_processedServerIndex >= m_serversController->getServersCount()) {
        return;
    }
    QJsonObject server = m_serversController->getServerConfig(m_processedServerIndex);
    QJsonArray containers = server.value(config_key::containers).toArray();
    m_containersModel->updateModel(containers);
}

void ServersUiController::updateDefaultServerContainersModel()
{
    int defaultIndex = m_serversController->getDefaultServerIndex();
    if (defaultIndex < 0 || defaultIndex >= m_serversController->getServersCount()) {
        return;
    }
    QJsonObject server = m_serversController->getServerConfig(defaultIndex);
    QJsonArray containers = server.value(config_key::containers).toArray();
    m_defaultServerContainersModel->updateModel(containers);
}

QStringList ServersUiController::getAllInstalledServicesName(int serverIndex) const
{
    QStringList servicesName;
    QJsonObject server = m_serversController->getServerConfig(serverIndex);
    const auto containers = server.value(config_key::containers).toArray();
    for (auto it = containers.begin(); it != containers.end(); it++) {
        auto container = ContainerProps::containerFromString(it->toObject().value(config_key::container).toString());
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

