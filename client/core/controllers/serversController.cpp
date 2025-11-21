#include "serversController.h"
#include "serverController.h"
#include "core/networkUtilities.h"
#include "core/api/apiDefs.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <AmneziaVPN-Swift.h>
#endif

namespace
{
    namespace configKey
    {
        constexpr char apiConfig[] = "api_config";
        constexpr char publicKeyInfo[] = "public_key";
        constexpr char expiresAt[] = "expires_at";
        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serviceType[] = "service_type";
    }
}

ServersController::ServersController(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
    recomputeGatewayStacks();
}

void ServersController::addServer(const QJsonObject &server)
{
    m_settings->addServer(server);
    recomputeGatewayStacks();
    emit serverAdded(server);
}

void ServersController::editServer(int index, const QJsonObject &server)
{
    m_settings->editServer(index, server);
    recomputeGatewayStacks();
    emit serverEdited(index, server);
}

void ServersController::removeServer(int index)
{
    m_settings->removeServer(index);
    
    // Adjust default server index if necessary
    int defaultIndex = m_settings->defaultServerIndex();
    if (defaultIndex == index) {
        m_settings->setDefaultServer(0);
    } else if (defaultIndex > index) {
        m_settings->setDefaultServer(defaultIndex - 1);
    }
    
    if (m_settings->serversCount() == 0) {
        m_settings->setDefaultServer(-1);
    }
    
    recomputeGatewayStacks();
    emit serverRemoved(index);
}

void ServersController::setDefaultServerIndex(int index)
{
    m_settings->setDefaultServer(index);
    emit defaultServerChanged(index);
}

void ServersController::setDefaultContainer(int serverIndex, DockerContainer container)
{
    m_settings->setDefaultContainer(serverIndex, container);
}

void ServersController::updateContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config)
{
    QJsonObject server = m_settings->server(serverIndex);
    QJsonArray containers = server.value(config_key::containers).toArray();
    
    for (int i = 0; i < containers.size(); i++) {
        auto c = ContainerProps::containerFromString(containers.at(i).toObject().value(config_key::container).toString());
        if (c == container) {
            containers.replace(i, config);
            break;
        }
    }
    
    server.insert(config_key::containers, containers);
    m_settings->editServer(serverIndex, server);
    emit serverEdited(serverIndex, server);
}

void ServersController::addContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config)
{
    QJsonObject server = m_settings->server(serverIndex);
    QJsonArray containers = server.value(config_key::containers).toArray();
    containers.push_back(config);
    
    server.insert(config_key::containers, containers);
    
    auto defaultContainer = server.value(config_key::defaultContainer).toString();
    if (ContainerProps::containerFromString(defaultContainer) == DockerContainer::None
        && ContainerProps::containerService(container) != ServiceType::Other 
        && ContainerProps::isSupportedByCurrentPlatform(container)) {
        server.insert(config_key::defaultContainer, ContainerProps::containerToString(container));
    }
    
    m_settings->editServer(serverIndex, server);
    emit serverEdited(serverIndex, server);
}

ErrorCode ServersController::removeContainer(ServerController *serverController, int serverIndex, DockerContainer container)
{
    auto credentials = m_settings->serverCredentials(serverIndex);
    ErrorCode errorCode = serverController->removeContainer(credentials, container);
    
    if (errorCode == ErrorCode::NoError) {
        QJsonObject server = m_settings->server(serverIndex);
        QJsonArray containers = server.value(config_key::containers).toArray();
        
        for (auto it = containers.begin(); it != containers.end(); it++) {
            if (it->toObject().value(config_key::container).toString() == ContainerProps::containerToString(container)) {
                containers.erase(it);
                break;
            }
        }
        
        server.insert(config_key::containers, containers);
        
        auto defaultContainer = ContainerProps::containerFromString(server.value(config_key::defaultContainer).toString());
        if (defaultContainer == container) {
            if (containers.empty()) {
                defaultContainer = DockerContainer::None;
            } else {
                defaultContainer = ContainerProps::containerFromString(containers.begin()->toObject().value(config_key::container).toString());
            }
            server.insert(config_key::defaultContainer, ContainerProps::containerToString(defaultContainer));
        }
        
        m_settings->editServer(serverIndex, server);
        emit serverEdited(serverIndex, server);
    }
    
    return errorCode;
}

ErrorCode ServersController::removeAllContainers(ServerController *serverController, int serverIndex)
{
    ErrorCode errorCode = serverController->removeAllContainers(m_settings->serverCredentials(serverIndex));
    
    if (errorCode == ErrorCode::NoError) {
        QJsonObject server = m_settings->server(serverIndex);
        server.insert(config_key::containers, QJsonArray());
        server.insert(config_key::defaultContainer, ContainerProps::containerToString(DockerContainer::None));
        
        m_settings->editServer(serverIndex, server);
        emit serverEdited(serverIndex, server);
    }
    
    return errorCode;
}

ErrorCode ServersController::rebootServer(ServerController *serverController, int serverIndex)
{
    auto credentials = m_settings->serverCredentials(serverIndex);
    return serverController->rebootServer(credentials);
}

void ServersController::clearCachedProfile(int serverIndex, DockerContainer container)
{
    m_settings->clearLastConnectionConfig(serverIndex, container);
    QJsonObject server = m_settings->server(serverIndex);
    emit serverEdited(serverIndex, server);
}

void ServersController::reloadContainerConfig(int serverIndex, DockerContainer container)
{
    QJsonObject server = m_settings->server(serverIndex);
    QJsonArray containers = server.value(config_key::containers).toArray();
    
    auto config = m_settings->containerConfig(serverIndex, container);
    for (int i = 0; i < containers.size(); i++) {
        auto c = ContainerProps::containerFromString(containers.at(i).toObject().value(config_key::container).toString());
        if (c == container) {
            containers.replace(i, config);
            break;
        }
    }
    
    server.insert(config_key::containers, containers);
    m_settings->editServer(serverIndex, server);
    emit serverEdited(serverIndex, server);
}

void ServersController::removeApiConfig(int serverIndex)
{
    auto serverConfig = m_settings->server(serverIndex);

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    QString vpncName = QString("%1 (%2) %3")
                               .arg(serverConfig[config_key::description].toString())
                               .arg(serverConfig[config_key::hostName].toString())
                               .arg(serverConfig[config_key::vpnproto].toString());

    AmneziaVPN::removeVPNC(vpncName.toStdString());
#endif

    serverConfig.remove(config_key::dns1);
    serverConfig.remove(config_key::dns2);
    serverConfig.remove(config_key::containers);
    serverConfig.remove(config_key::hostName);

    auto apiConfig = serverConfig.value(configKey::apiConfig).toObject();
    apiConfig.remove(configKey::publicKeyInfo);
    serverConfig.insert(configKey::apiConfig, apiConfig);

    serverConfig.insert(config_key::defaultContainer, ContainerProps::containerToString(DockerContainer::None));

    m_settings->editServer(serverIndex, serverConfig);
    emit serverEdited(serverIndex, serverConfig);
}

bool ServersController::isApiKeyExpired(int serverIndex) const
{
    auto serverConfig = m_settings->server(serverIndex);
    auto apiConfig = serverConfig.value(configKey::apiConfig).toObject();

    auto publicKeyInfo = apiConfig.value(configKey::publicKeyInfo).toObject();
    const QString expiresAt = publicKeyInfo.value(configKey::expiresAt).toString();
    
    if (expiresAt.isEmpty()) {
        return false;
    }

    auto expiresAtDateTime = QDateTime::fromString(expiresAt, Qt::ISODate).toUTC();
    if (expiresAtDateTime < QDateTime::currentDateTimeUtc()) {
        return true;
    }
    
    return false;
}

QPair<QString, QString> ServersController::getDnsPair(int serverIndex, bool isAmneziaDnsEnabled) const
{
    QPair<QString, QString> dns;
    
    const QJsonObject &server = m_settings->server(serverIndex);
    const auto containers = server.value(config_key::containers).toArray();
    
    bool isDnsContainerInstalled = false;
    for (const QJsonValue &container : containers) {
        if (ContainerProps::containerFromString(container.toObject().value(config_key::container).toString()) == DockerContainer::Dns) {
            isDnsContainerInstalled = true;
        }
    }
    
    dns.first = server.value(config_key::dns1).toString();
    dns.second = server.value(config_key::dns2).toString();
    
    if (dns.first.isEmpty() || !NetworkUtilities::checkIPv4Format(dns.first)) {
        if (isAmneziaDnsEnabled && isDnsContainerInstalled) {
            dns.first = protocols::dns::amneziaDnsIp;
        } else {
            dns.first = m_settings->primaryDns();
        }
    }
    
    if (dns.second.isEmpty() || !NetworkUtilities::checkIPv4Format(dns.second)) {
        dns.second = m_settings->secondaryDns();
    }
    
    return dns;
}

QJsonArray ServersController::getServersArray() const
{
    return m_settings->serversArray();
}

QJsonObject ServersController::getContainerConfig(int serverIndex, DockerContainer container) const
{
    return m_settings->containerConfig(serverIndex, container);
}

bool ServersController::isAmneziaDnsEnabled() const
{
    return m_settings->useAmneziaDns();
}

void ServersController::setAmneziaDnsEnabled(bool enabled)
{
    m_settings->setUseAmneziaDns(enabled);
}

QString ServersController::getNextAvailableServerName() const
{
    return m_settings->nextAvailableServerName();
}

int ServersController::getDefaultServerIndex() const
{
    return m_settings->defaultServerIndex();
}

int ServersController::getServersCount() const
{
    return m_settings->serversCount();
}

QJsonObject ServersController::getServerConfig(int serverIndex) const
{
    return m_settings->server(serverIndex);
}

ServerCredentials ServersController::getServerCredentials(int serverIndex) const
{
    return m_settings->serverCredentials(serverIndex);
}

bool ServersController::isServerFromApiAlreadyExists(const quint16 crc) const
{
    auto servers = m_settings->serversArray();
    for (const auto &server : servers) {
        if (static_cast<quint16>(server.toObject().value(config_key::crc).toInt()) == crc) {
            return true;
        }
    }
    return false;
}

ServersController::GatewayStacks ServersController::gatewayStacks() const
{
    return m_gatewayStacks;
}

void ServersController::recomputeGatewayStacks()
{
    GatewayStacks computed;
    bool hasNewTags = false;
    QJsonArray servers = m_settings->serversArray();

    for (int i = 0; i < servers.count(); ++i) {
        QJsonObject server = servers.at(i).toObject();
        if (server.value(config_key::configVersion).toInt() == apiDefs::ConfigSource::AmneziaGateway) {
            const QJsonObject apiConfig = server.value(configKey::apiConfig).toObject();

            const QString userCountryCode = apiConfig.value(configKey::userCountryCode).toString();
            const QString serviceType = apiConfig.value(configKey::serviceType).toString();

            if (!userCountryCode.isEmpty()) {
                if (!m_gatewayStacks.userCountryCodes.contains(userCountryCode)) {
                    hasNewTags = true;
                }
                computed.userCountryCodes.insert(userCountryCode);
            }

            if (!serviceType.isEmpty()) {
                if (!m_gatewayStacks.serviceTypes.contains(serviceType)) {
                    hasNewTags = true;
                }
                computed.serviceTypes.insert(serviceType);
            }
        }
    }

    m_gatewayStacks = std::move(computed);
    if (hasNewTags) {
        emit gatewayStacksExpanded();
    }
}

bool ServersController::GatewayStacks::operator==(const GatewayStacks &other) const
{
    return userCountryCodes == other.userCountryCodes && serviceTypes == other.serviceTypes;
}

QJsonObject ServersController::GatewayStacks::toJson() const
{
    QJsonObject json;
    
    QJsonArray userCountryCodesArray;
    for (const QString &code : userCountryCodes) {
        userCountryCodesArray.append(code);
    }
    json["user_country_code"] = userCountryCodesArray;
    
    QJsonArray serviceTypesArray;
    for (const QString &type : serviceTypes) {
        serviceTypesArray.append(type);
    }
    json["service_type"] = serviceTypesArray;
    
    return json;
}

bool ServersController::isServerFromApiAlreadyExists(const QString &userCountryCode, const QString &serviceType, const QString &serviceProtocol) const
{
    auto servers = m_settings->serversArray();
    for (const auto &server : servers) {
        const auto apiConfig = server.toObject().value("api_config").toObject();
        if (apiConfig.value("user_country_code").toString() == userCountryCode
            && apiConfig.value("service_type").toString() == serviceType
            && apiConfig.value("service_protocol").toString() == serviceProtocol) {
            return true;
        }
    }
    return false;
}

