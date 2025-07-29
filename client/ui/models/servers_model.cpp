#include "servers_model.h"

#include "core/api/apiDefs.h"
#include "core/controllers/serverController.h"
#include "core/models/servers/apiV1ServerConfig.h"
#include "core/models/servers/apiV2ServerConfig.h"
#include "core/models/servers/selfHostedServerConfig.h"
#include "core/models/servers/serverConfig.h"
#include "core/networkUtilities.h"

#ifdef Q_OS_IOS
    #include <AmneziaVPN-Swift.h>
#endif

#include "core/api/apiUtils.h"

namespace
{
    namespace configKey
    {
        constexpr char apiConfig[] = "api_config";
        constexpr char serviceInfo[] = "service_info";
        constexpr char availableCountries[] = "available_countries";
        constexpr char serverCountryCode[] = "server_country_code";
        constexpr char serverCountryName[] = "server_country_name";
        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serviceType[] = "service_type";
        constexpr char serviceProtocol[] = "service_protocol";

        constexpr char publicKeyInfo[] = "public_key";
        constexpr char expiresAt[] = "expires_at";
    }
}

ServersModel::ServersModel(std::shared_ptr<Settings> settings, QObject *parent) : m_settings(settings), QAbstractListModel(parent)
{
    m_isAmneziaDnsEnabled = m_settings->useAmneziaDns();

    connect(this, &ServersModel::defaultServerIndexChanged, this, &ServersModel::defaultServerNameChanged);

    connect(this, &ServersModel::defaultServerIndexChanged, this, [this](const int serverIndex) {
        auto defaultContainer = ContainerProps::containerFromString(m_servers1.at(serverIndex)->defaultContainer);
        emit ServersModel::defaultServerDefaultContainerChanged(defaultContainer);
        emit ServersModel::defaultServerNameChanged();
        updateDefaultServerContainersModel();
    });

    connect(this, &ServersModel::processedServerIndexChanged, this, &ServersModel::processedServerChanged);
    connect(this, &ServersModel::dataChanged, this, &ServersModel::processedServerChanged);
}

int ServersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_servers1.size());
}

bool ServersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_servers1.size())) {
        return false;
    }

    QSharedPointer<ServerConfig> serverConfig = m_servers1.at(index.row());
    ServerConfigVariant variant = ServerConfig::getServerConfigVariant(serverConfig);

    switch (role) {
    case NameRole: {
        std::visit([&value](const auto &ptr) -> void { ptr->name = value.toString(); }, variant);
        serverConfig->nameOverriddenByUser = true;

        m_settings->editServer(index.row(), serverConfig->toJson());
        m_servers1.replace(index.row(), serverConfig);
        if (index.row() == m_defaultServerIndex) {
            emit defaultServerNameChanged();
        }
        break;
    }
    default: {
        return true;
    }
    }

    emit dataChanged(index, index);
    return true;
}

bool ServersModel::setData(const int index, const QVariant &value, int role)
{
    QModelIndex modelIndex = this->index(index);
    return setData(modelIndex, value, role);
}

QVariant ServersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_servers1.size())) {
        return QVariant();
    }

    QSharedPointer<ServerConfig> serverConfig = m_servers1.at(index.row());
    ServerConfigVariant variant = ServerConfig::getServerConfigVariant(serverConfig);

    switch (role) {
    case NameRole: {
        return std::visit([](const auto &ptr) -> QString { return ptr->name; }, variant);
    }
    case ServerDescriptionRole: {
        return getServerDescription(index.row());
    }
    case HostNameRole: return serverConfig->hostName;
    case CredentialsRole: return QVariant::fromValue(serverCredentials(index.row()));
    case CredentialsLoginRole: return serverCredentials(index.row()).userName;
    case IsDefaultRole: return index.row() == m_defaultServerIndex;
    case IsCurrentlyProcessedRole: return index.row() == m_processedServerIndex;
    case HasWriteAccessRole: {
        auto credentials = serverCredentials(index.row());
        return (!credentials.userName.isEmpty() && !credentials.secretData.isEmpty());
    }
    case ContainsAmneziaDnsRole: {
        return serverConfig->dns1 == protocols::dns::amneziaDnsIp;
    }
    case DefaultContainerRole: {
        return ContainerProps::containerFromString(serverConfig->defaultContainer);
    }
    case HasInstalledContainers: {
        return serverHasInstalledContainers(index.row());
    }
    case IsServerFromTelegramApiRole: {
        return serverConfig->type == amnezia::ServerConfigType::ApiV1;
    }
    case IsServerFromGatewayApiRole: {
        return serverConfig->type == amnezia::ServerConfigType::ApiV2;
    }
    case IsCountrySelectionAvailableRole: {
        return !qSharedPointerCast<ApiV2ServerConfig>(serverConfig)->apiConfig.availableCountries.isEmpty();
    }
    case ApiAvailableCountriesRole: {
        return QVariant::fromValue(qSharedPointerCast<ApiV2ServerConfig>(serverConfig)->apiConfig.availableCountries);
    }
    case ApiServerCountryCodeRole: {
        return qSharedPointerCast<ApiV2ServerConfig>(serverConfig)->apiConfig.serverCountryCode;
    }
    }

    return QVariant();
}

QVariant ServersModel::data(const int index, int role) const
{
    QModelIndex modelIndex = this->index(index);
    return data(modelIndex, role);
}

void ServersModel::resetModel()
{
    beginResetModel();
    auto servers = m_settings->serversArray();
    m_defaultServerIndex = m_settings->defaultServerIndex();
    m_processedServerIndex = m_defaultServerIndex;

    for (auto server : servers) {
        auto serverConfig = ServerConfig::createServerConfig(server.toObject());
        m_servers1.push_back(serverConfig);
    }

    endResetModel();
    emit defaultServerIndexChanged(m_defaultServerIndex);
}

void ServersModel::setDefaultServerIndex(const int index)
{
    m_settings->setDefaultServer(index);
    m_defaultServerIndex = m_settings->defaultServerIndex();
    emit defaultServerIndexChanged(m_defaultServerIndex);
}

const int ServersModel::getDefaultServerIndex()
{
    return m_defaultServerIndex;
}

const QString ServersModel::getDefaultServerName()
{
    return qvariant_cast<QString>(data(m_defaultServerIndex, NameRole));
}

QString ServersModel::getServerDescription(const int index) const
{
    auto serverConfig = m_servers1.at(index);
    switch (serverConfig->type) {
    case amnezia::ServerConfigType::ApiV1: return qSharedPointerCast<ApiV1ServerConfig>(serverConfig)->description;
    case amnezia::ServerConfigType::ApiV2: {
        auto apiV2ServerConfig = qSharedPointerCast<ApiV2ServerConfig>(serverConfig);
        if (apiV2ServerConfig->apiConfig.serverCountryCode.isEmpty()) {
            return apiV2ServerConfig->description;
        } else {
            return apiV2ServerConfig->apiConfig.serverCountryName;
        }
    }
    case amnezia::ServerConfigType::SelfHosted: {
        QString description;
        if (data(index, HasWriteAccessRole).toBool()) {
            if (m_isAmneziaDnsEnabled && isAmneziaDnsContainerInstalled(index)) {
                description += "Amnezia DNS | " + serverConfig->hostName;
            }
        } else {
            if (data(index, ContainsAmneziaDnsRole).toBool()) {
                description += "Amnezia DNS | " + serverConfig->hostName;
            }
        }
        return description;
    }
    }
}

const QString ServersModel::getDefaultServerDescriptionCollapsed()
{
    auto serverConfig = m_servers1.at(m_defaultServerIndex);
    auto description = getServerDescription(m_defaultServerIndex);
    auto containerName = ContainerProps::containerFromString(serverConfig->defaultContainer);

    if (serverConfig->type != ServerConfigType::SelfHosted) {
        return description;
    }

    return description += ContainerProps::containerHumanNames().value(containerName) + " | " + serverConfig->hostName;
}

const QString ServersModel::getDefaultServerDescriptionExpanded()
{
    auto serverConfig = m_servers1.at(m_defaultServerIndex);
    auto description = getServerDescription(m_defaultServerIndex);
    auto containerName = ContainerProps::containerFromString(serverConfig->defaultContainer);

    if (serverConfig->type != ServerConfigType::SelfHosted) {
        return description;
    }

    return description += serverConfig->hostName;
}

const int ServersModel::getServersCount()
{
    return m_servers1.count();
}

bool ServersModel::hasServerWithWriteAccess()
{
    for (size_t i = 0; i < getServersCount(); i++) {
        if (qvariant_cast<bool>(data(i, HasWriteAccessRole))) {
            return true;
        }
    }
    return false;
}

void ServersModel::setProcessedServerIndex(const int index)
{
    m_processedServerIndex = index;
    updateContainersModel();
    if (data(index, IsServerFromGatewayApiRole).toBool()) {
        if (data(index, IsCountrySelectionAvailableRole).toBool()) {
            emit updateApiCountryModel();
        }
        emit updateApiServicesModel();
    }
    emit processedServerIndexChanged(m_processedServerIndex);
}

int ServersModel::getProcessedServerIndex()
{
    return m_processedServerIndex;
}

const ServerCredentials ServersModel::getProcessedServerCredentials()
{
    return serverCredentials(m_processedServerIndex);
}

const ServerCredentials ServersModel::getServerCredentials(const int index)
{
    return serverCredentials(index);
}

bool ServersModel::isDefaultServerCurrentlyProcessed()
{
    return m_defaultServerIndex == m_processedServerIndex;
}

bool ServersModel::isDefaultServerFromApi()
{
    return data(m_defaultServerIndex, IsServerFromTelegramApiRole).toBool()
            || data(m_defaultServerIndex, IsServerFromGatewayApiRole).toBool();
}

bool ServersModel::isProcessedServerHasWriteAccess()
{
    return qvariant_cast<bool>(data(m_processedServerIndex, HasWriteAccessRole));
}

bool ServersModel::isDefaultServerHasWriteAccess()
{
    return qvariant_cast<bool>(data(m_defaultServerIndex, HasWriteAccessRole));
}

void ServersModel::addServer(const QSharedPointer<ServerConfig> &serverConfig)
{
    beginResetModel();
    m_settings->addServer(serverConfig->toJson());
    auto servers = m_settings->serversArray();
    for (auto server : servers) {
        auto serverConfig = ServerConfig::createServerConfig(server.toObject());
        m_servers1.push_back(serverConfig);
    }
    endResetModel();
}

void ServersModel::editServer(const QSharedPointer<ServerConfig> &serverConfig, const int serverIndex)
{
    m_settings->editServer(serverIndex, serverConfig->toJson());
    m_servers1[serverIndex] = serverConfig;
    emit dataChanged(index(serverIndex, 0), index(serverIndex, 0));

    if (serverIndex == m_defaultServerIndex) {
        updateDefaultServerContainersModel();
    }
    updateContainersModel();

    if (serverIndex == m_defaultServerIndex) {
        auto defaultContainer = qvariant_cast<DockerContainer>(getDefaultServerData("defaultContainer"));
        emit defaultServerDefaultContainerChanged(defaultContainer);
    }
}

void ServersModel::removeProcessedServer()
{
    removeServer(m_processedServerIndex);
}

void ServersModel::removeServer(const int serverIndex)
{
    beginResetModel();
    m_settings->removeServer(serverIndex);
    auto servers = m_settings->serversArray();
    for (auto server : servers) {
        auto serverConfig = ServerConfig::createServerConfig(server.toObject());
        m_servers1.push_back(serverConfig);
    }

    if (m_settings->defaultServerIndex() == serverIndex) {
        setDefaultServerIndex(0);
    } else if (m_settings->defaultServerIndex() > serverIndex) {
        setDefaultServerIndex(m_settings->defaultServerIndex() - 1);
    }

    if (m_settings->serversCount() == 0) {
        setDefaultServerIndex(-1);
    }
    setProcessedServerIndex(m_defaultServerIndex);
    endResetModel();
}

QHash<int, QByteArray> ServersModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[NameRole] = "serverName";
    roles[NameRole] = "name";
    roles[ServerDescriptionRole] = "serverDescription";
    roles[CollapsedServerDescriptionRole] = "collapsedServerDescription";
    roles[ExpandedServerDescriptionRole] = "expandedServerDescription";

    roles[HostNameRole] = "hostName";

    roles[CredentialsRole] = "credentials";
    roles[CredentialsLoginRole] = "credentialsLogin";

    roles[IsDefaultRole] = "isDefault";
    roles[IsCurrentlyProcessedRole] = "isCurrentlyProcessed";

    roles[HasWriteAccessRole] = "hasWriteAccess";

    roles[ContainsAmneziaDnsRole] = "containsAmneziaDns";

    roles[DefaultContainerRole] = "defaultContainer";
    roles[HasInstalledContainers] = "hasInstalledContainers";

    roles[IsServerFromTelegramApiRole] = "isServerFromTelegramApi";
    roles[IsServerFromGatewayApiRole] = "isServerFromGatewayApi";
    roles[IsCountrySelectionAvailableRole] = "isCountrySelectionAvailable";
    roles[ApiAvailableCountriesRole] = "apiAvailableCountries";
    roles[ApiServerCountryCodeRole] = "apiServerCountryCode";
    return roles;
}

ServerCredentials ServersModel::serverCredentials(int index) const
{
    const auto serverConfig = m_servers1.at(index);
    return qSharedPointerCast<SelfHostedServerConfig>(serverConfig)->serverCredentials;
}

void ServersModel::updateContainersModel()
{
    auto containerConfigs = m_servers1.at(m_processedServerIndex)->containerConfigs;
    emit containersUpdated(containerConfigs);
}

void ServersModel::updateDefaultServerContainersModel()
{
    auto containerConfigs = m_servers1.at(m_defaultServerIndex)->containerConfigs;
    emit defaultServerContainersUpdated(containerConfigs);
}

QSharedPointer<const ServerConfig> ServersModel::getServerConfig(const int serverIndex)
{
    return m_servers1.at(serverIndex);
}

void ServersModel::setDefaultContainer(const int serverIndex, const int containerIndex)
{
    auto container = static_cast<DockerContainer>(containerIndex);
    auto serverConfig = m_servers1.at(serverIndex);
    serverConfig->defaultContainer = ContainerProps::containerToString(container);
    editServer(serverConfig, serverIndex);
}

const QString ServersModel::getDefaultServerDefaultContainerName()
{
    auto defaultContainer = qvariant_cast<DockerContainer>(getDefaultServerData("defaultContainer"));
    return ContainerProps::containerHumanNames().value(defaultContainer);
}

ErrorCode ServersModel::removeAllContainers(const QSharedPointer<ServerController> &serverController)
{
    ErrorCode errorCode = serverController->removeAllContainers(m_settings->serverCredentials(m_processedServerIndex));

    if (errorCode == ErrorCode::NoError) {
        auto serverConfig = m_servers1.at(m_processedServerIndex);
        serverConfig->containerConfigs.clear();
        editServer(serverConfig, m_processedServerIndex);
    }
    return errorCode;
}

ErrorCode ServersModel::rebootServer(const QSharedPointer<ServerController> &serverController)
{
    ErrorCode errorCode = serverController->rebootServer(m_settings->serverCredentials(m_processedServerIndex));
    return errorCode;
}

ErrorCode ServersModel::removeContainer(const QSharedPointer<ServerController> &serverController, const int containerIndex)
{

    auto credentials = m_settings->serverCredentials(m_processedServerIndex);
    auto dockerContainer = static_cast<DockerContainer>(containerIndex);

    ErrorCode errorCode = serverController->removeContainer(credentials, dockerContainer);

    if (errorCode == ErrorCode::NoError) {
        auto serverConfig = m_servers1.at(m_processedServerIndex);
        serverConfig->containerConfigs.remove(ContainerProps::containerToString(dockerContainer));

        auto defaultContainer = ContainerProps::containerFromString(serverConfig->defaultContainer);
        if (defaultContainer == containerIndex) {
            if (serverConfig->containerConfigs.empty()) {
                serverConfig->defaultContainer = ContainerProps::containerToString(DockerContainer::None);
            } else {
                serverConfig->defaultContainer = serverConfig->containerConfigs.begin()->containerName;
            }
        }

        editServer(serverConfig, m_processedServerIndex);
    }
    return errorCode;
}

void ServersModel::clearCachedProfile(const DockerContainer container)
{
    m_settings->clearLastConnectionConfig(m_processedServerIndex, container);
    auto serverConfig = ServerConfig::createServerConfig(m_settings->server(m_processedServerIndex));

    m_servers1.replace(m_processedServerIndex, serverConfig);
    if (m_processedServerIndex == m_defaultServerIndex) {
        updateDefaultServerContainersModel();
    }
    updateContainersModel();
}

bool ServersModel::isAmneziaDnsContainerInstalled(const int serverIndex) const
{
    auto serverConfig = m_servers1.at(serverIndex);
    for (const auto &container : serverConfig->containerConfigs) {
        if (container.containerName == ContainerProps::containerToString(DockerContainer::Dns)) {
            return true;
        }
    }
    return false;
}

QPair<QString, QString> ServersModel::getDnsPair(int serverIndex)
{
    QPair<QString, QString> dns;

    auto serverConfig = m_servers1.at(serverIndex);
    bool isDnsContainerInstalled = false;
    for (const auto &container : serverConfig->containerConfigs) {
        if (container.containerName == ContainerProps::containerToString(DockerContainer::Dns)) {
            isDnsContainerInstalled = true;
        }
    }

    dns.first = serverConfig->dns1;
    dns.second = serverConfig->dns2;

    if (dns.first.isEmpty() || !NetworkUtilities::checkIPv4Format(dns.first)) {
        if (m_isAmneziaDnsEnabled && isDnsContainerInstalled) {
            dns.first = protocols::dns::amneziaDnsIp;
        } else
            dns.first = m_settings->primaryDns();
    }
    if (dns.second.isEmpty() || !NetworkUtilities::checkIPv4Format(dns.second)) {
        dns.second = m_settings->secondaryDns();
    }

    qDebug() << "VpnConfigurator::getDnsForConfig" << dns.first << dns.second;
    return dns;
}

QStringList ServersModel::getAllInstalledServicesName(const int serverIndex)
{
    QStringList servicesName;
    auto serverConfig = m_servers1.at(serverIndex);
    for (const auto &container : serverConfig->containerConfigs) {
        auto containerType = ContainerProps::containerFromString(container.containerName);
        if (ContainerProps::containerService(containerType) == ServiceType::Other) {
            if (containerType == DockerContainer::Dns) {
                servicesName.append("DNS");
            } else if (containerType == DockerContainer::Sftp) {
                servicesName.append("SFTP");
            } else if (containerType == DockerContainer::TorWebSite) {
                servicesName.append("TOR");
            } else if (containerType == DockerContainer::Socks5Proxy) {
                servicesName.append("SOCKS5");
            }
        }
    }
    servicesName.sort();
    return servicesName;
}

void ServersModel::toggleAmneziaDns(bool enabled)
{
    m_isAmneziaDnsEnabled = enabled;
    emit defaultServerDescriptionChanged();
}

bool ServersModel::isServerFromApiAlreadyExists(const quint16 crc)
{
    for (const auto &server : std::as_const(m_servers1)) {
        if (static_cast<quint16>(server->crc) == crc) {
            return true;
        }
    }
    return false;
}

bool ServersModel::isServerFromApiAlreadyExists(const QString &userCountryCode, const QString &serviceType, const QString &serviceProtocol)
{
    for (const auto &serverConfig : std::as_const(m_servers1)) {
        const auto apiV2ServerConfig = qSharedPointerCast<ApiV2ServerConfig>(serverConfig);
        if (apiV2ServerConfig->apiConfig.userCountryCode == userCountryCode && apiV2ServerConfig->apiConfig.serviceType == serviceType
            && apiV2ServerConfig->apiConfig.serviceProtocol == serviceProtocol) {
            return true;
        }
    }
    return false;
}

bool ServersModel::serverHasInstalledContainers(const int serverIndex) const
{
    auto server = m_servers1.at(serverIndex);
    const auto containers = server->containerConfigs;
    for (const auto &container : containers) {
        auto dockerContainer = ContainerProps::containerFromString(container.containerName);
        if (ContainerProps::containerService(dockerContainer) == ServiceType::Vpn) {
            return true;
        }
        if (dockerContainer == DockerContainer::SSXray) {
            return true;
        }
    }
    return false;
}

QVariant ServersModel::getDefaultServerData(const QString roleString)
{
    auto roles = roleNames();
    for (auto it = roles.begin(); it != roles.end(); it++) {
        if (QString(it.value()) == roleString) {
            return data(m_defaultServerIndex, it.key());
        }
    }

    return {};
}

QVariant ServersModel::getProcessedServerData(const QString roleString)
{
    auto roles = roleNames();
    for (auto it = roles.begin(); it != roles.end(); it++) {
        if (QString(it.value()) == roleString) {
            return data(m_processedServerIndex, it.key());
        }
    }

    return {};
}

bool ServersModel::setProcessedServerData(const QString &roleString, const QVariant &value)
{
    const auto roles = roleNames();
    for (auto it = roles.begin(); it != roles.end(); it++) {
        if (QString(it.value()) == roleString) {
            return setData(m_processedServerIndex, value, it.key());
        }
    }

    return false;
}

bool ServersModel::isDefaultServerDefaultContainerHasSplitTunneling()
{
    auto serverConfig = m_servers1.at(m_defaultServerIndex);
    auto defaultContainer = ContainerProps::containerFromString(serverConfig->defaultContainer);

    for (const auto &container : serverConfig->containerConfigs) {
        if (container.containerName != serverConfig->defaultContainer) {
            continue;
        }
        if (defaultContainer == DockerContainer::Awg || defaultContainer == DockerContainer::WireGuard) {
            auto protocolConfigVariant = ProtocolConfig::getProtocolConfigVariant(container.protocolConfigs[serverConfig->defaultContainer]);
            return std::visit(
                    [](const auto &ptr) -> bool {
                        if constexpr (requires {
                                          ptr->clientProtocolConfig;
                                          ptr->clientProtocolConfig.wireGuardData;
                                      }) {
                            const auto nativeConfig = ptr->clientProtocolConfig.nativeConfig;
                            const auto allowedIps = ptr->clientProtocolConfig.wireGuardData.allowedIps;

                            return (nativeConfig.contains("AllowedIPs") && !nativeConfig.contains("AllowedIPs = 0.0.0.0/0, ::/0"))
                                    || (!allowedIps.isEmpty() && !allowedIps.contains("0.0.0.0/0"));
                        } else {
                            return false;
                        }
                    },
                    protocolConfigVariant);
        } else if (defaultContainer == DockerContainer::Cloak || defaultContainer == DockerContainer::OpenVpn
                   || defaultContainer == DockerContainer::ShadowSocks) {
            auto protocolConfigVariant = ProtocolConfig::getProtocolConfigVariant(
                    container.protocolConfigs[ContainerProps::containerTypeToString(DockerContainer::OpenVpn)]);
            return std::visit(
                    [](const auto &ptr) -> bool {
                        if constexpr (requires { ptr->clientProtocolConfig; }) {
                            const auto nativeConfig = ptr->clientProtocolConfig.nativeConfig;

                            return (!nativeConfig.isEmpty() && !nativeConfig.contains("redirect-gateway"));
                        } else {
                            return false;
                        }
                    },
                    protocolConfigVariant);
        }
    }
    return false;
}

bool ServersModel::isServerFromApi(const int serverIndex)
{
    return data(serverIndex, IsServerFromTelegramApiRole).toBool() || data(serverIndex, IsServerFromGatewayApiRole).toBool();
}

bool ServersModel::isApiKeyExpired(const int serverIndex)
{
    // auto serverConfig = m_servers1.at(serverIndex);
    // auto apiConfig = serverConfig.value(configKey::apiConfig).toObject();

    // auto publicKeyInfo = apiConfig.value(configKey::publicKeyInfo).toObject();
    // const QString expiresAt = publicKeyInfo.value(configKey::expiresAt).toString();
    // if (expiresAt.isEmpty()) {
    //     publicKeyInfo.insert(configKey::expiresAt, QDateTime::currentDateTimeUtc().addDays(1).toString(Qt::ISODate));
    //     apiConfig.insert(configKey::publicKeyInfo, publicKeyInfo);
    //     serverConfig.insert(configKey::apiConfig, apiConfig);
    //     editServer(serverConfig, serverIndex);

    //     return false;
    // }

    // auto expiresAtDateTime = QDateTime::fromString(expiresAt, Qt::ISODate).toUTC();
    // if (expiresAtDateTime < QDateTime::currentDateTimeUtc()) {
    //     return true;
    // }
    // return false;
}

void ServersModel::removeApiConfig(const int serverIndex)
{
//     auto serverConfig = m_servers1.at(serverIndex);

// #ifdef Q_OS_IOS
//     QString vpncName = QString("%1 (%2) %3")
//                                .arg(serverConfig[config_key::description].toString())
//                                .arg(serverConfig[config_key::hostName].toString())
//                                .arg(serverConfig[config_key::vpnproto].toString());

//     AmneziaVPN::removeVPNC(vpncName.toStdString());
// #endif

//     serverConfig.remove(config_key::dns1);
//     serverConfig.remove(config_key::dns2);
//     serverConfig.remove(config_key::containers);
//     serverConfig.remove(config_key::hostName);

//     auto apiConfig = serverConfig.value(configKey::apiConfig).toObject();
//     apiConfig.remove(configKey::publicKeyInfo);
//     serverConfig.insert(configKey::apiConfig, apiConfig);

//     serverConfig.insert(config_key::defaultContainer, ContainerProps::containerToString(DockerContainer::None));

//     editServer(serverConfig, serverIndex);
}

const QString ServersModel::getDefaultServerImagePathCollapsed()
{
    // const auto server = m_servers.at(m_defaultServerIndex).toObject();
    // const auto apiConfig = server.value(configKey::apiConfig).toObject();
    // const auto countryCode = apiConfig.value(configKey::serverCountryCode).toString();

    // if (countryCode.isEmpty()) {
    //     return "";
    // }
    // return QString("qrc:/countriesFlags/images/flagKit/%1.svg").arg(countryCode.toUpper());
}

bool ServersModel::processedServerIsPremium() const
{
    return apiUtils::isPremiumServer(getServerConfig(m_processedServerIndex));
}
