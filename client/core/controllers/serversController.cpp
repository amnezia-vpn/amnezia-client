#include "serversController.h"
#include "core/utils/networkUtilities.h"
#include "core/utils/api/apiDefs.h"
#include "protocols/protocols_defs.h"

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

ServersController::ServersController(ServersRepository* serversRepository, 
                                      AppSettingsRepository* appSettingsRepository,
                                      QObject *parent)
    : QObject(parent), m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository)
{
    recomputeGatewayStacks();
}

void ServersController::addServer(const QJsonObject &server)
{
    m_serversRepository->addServer(server);
    recomputeGatewayStacks();
}

void ServersController::editServer(int index, const QJsonObject &server)
{
    m_serversRepository->editServer(index, server);
    recomputeGatewayStacks();
}

void ServersController::removeServer(int index)
{
    m_serversRepository->removeServer(index);
    recomputeGatewayStacks();
}

void ServersController::setDefaultServerIndex(int index)
{
    m_serversRepository->setDefaultServer(index);
}

void ServersController::setDefaultContainer(int serverIndex, DockerContainer container)
{
    m_serversRepository->setDefaultContainer(serverIndex, container);
}

void ServersController::updateContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config)
{
    QJsonObject server = m_serversRepository->server(serverIndex);
    QJsonArray containers = server.value(config_key::containers).toArray();
    
    for (int i = 0; i < containers.size(); i++) {
        auto c = ContainerProps::containerFromString(containers.at(i).toObject().value(config_key::container).toString());
        if (c == container) {
            containers.replace(i, config);
            break;
        }
    }
    
    server.insert(config_key::containers, containers);
    m_serversRepository->editServer(serverIndex, server);
}

void ServersController::addContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config)
{
    QJsonObject server = m_serversRepository->server(serverIndex);
    QJsonArray containers = server.value(config_key::containers).toArray();
    containers.push_back(config);
    
    server.insert(config_key::containers, containers);
    
    auto defaultContainer = server.value(config_key::defaultContainer).toString();
    if (ContainerProps::containerFromString(defaultContainer) == DockerContainer::None
        && ContainerProps::containerService(container) != ServiceType::Other 
        && ContainerProps::isSupportedByCurrentPlatform(container)) {
        server.insert(config_key::defaultContainer, ContainerProps::containerToString(container));
    }
    
    m_serversRepository->editServer(serverIndex, server);
}

void ServersController::clearCachedProfile(int serverIndex, DockerContainer container)
{
    m_serversRepository->clearLastConnectionConfig(serverIndex, container);
}

void ServersController::reloadContainerConfig(int serverIndex, DockerContainer container)
{
    QJsonObject server = m_serversRepository->server(serverIndex);
    QJsonArray containers = server.value(config_key::containers).toArray();
    
    auto config = m_serversRepository->containerConfig(serverIndex, container);
    for (int i = 0; i < containers.size(); i++) {
        auto c = ContainerProps::containerFromString(containers.at(i).toObject().value(config_key::container).toString());
        if (c == container) {
            containers.replace(i, config);
            break;
        }
    }
    
    server.insert(config_key::containers, containers);
    m_serversRepository->editServer(serverIndex, server);
}

QJsonArray ServersController::getServersArray() const
{
    return m_serversRepository->serversArray();
}

QJsonObject ServersController::getContainerConfig(int serverIndex, DockerContainer container) const
{
    return m_serversRepository->containerConfig(serverIndex, container);
}

int ServersController::getDefaultServerIndex() const
{
    return m_serversRepository->defaultServerIndex();
}

int ServersController::getServersCount() const
{
    return m_serversRepository->serversCount();
}

QJsonObject ServersController::getServerConfig(int serverIndex) const
{
    return m_serversRepository->server(serverIndex);
}

ServerCredentials ServersController::getServerCredentials(int serverIndex) const
{
    return m_serversRepository->serverCredentials(serverIndex);
}

QPair<QString, QString> ServersController::getDnsPair(int serverIndex, bool isAmneziaDnsEnabled) const
{
    QPair<QString, QString> dns;
    
    const QJsonObject &server = m_serversRepository->server(serverIndex);
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
            dns.first = m_appSettingsRepository->primaryDns();
        }
    }
    
    if (dns.second.isEmpty() || !NetworkUtilities::checkIPv4Format(dns.second)) {
        dns.second = m_appSettingsRepository->secondaryDns();
    }
    
    return dns;
}

bool ServersController::isServerFromApiAlreadyExists(const quint16 crc) const
{
    auto servers = m_serversRepository->serversArray();
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
    QJsonArray servers = m_serversRepository->serversArray();

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
    auto servers = m_serversRepository->serversArray();
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

bool ServersController::hasInstalledContainers(int serverIndex) const
{
    QJsonObject server = m_serversRepository->server(serverIndex);
    const auto containers = server.value(config_key::containers).toArray();
    for (auto it = containers.begin(); it != containers.end(); it++) {
        auto container = ContainerProps::containerFromString(it->toObject().value(config_key::container).toString());
        if (ContainerProps::containerService(container) == ServiceType::Vpn) {
            return true;
        }
        if (container == DockerContainer::SSXray) {
            return true;
        }
    }
    return false;
}

