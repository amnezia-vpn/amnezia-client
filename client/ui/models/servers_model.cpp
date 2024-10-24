#include "servers_model.h"

#include "core/controllers/serverController.h"
#include "core/enums/apiEnums.h"
#include "core/networkUtilities.h"

#ifdef Q_OS_IOS
    #include <AmneziaVPN-Swift.h>
#endif

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
        constexpr char endDate[] = "end_date";
    }
}

ServersModel::ServersModel(std::shared_ptr<Settings> settings, QObject *parent) : m_settings(settings), QAbstractListModel(parent)
{
    m_isAmneziaDnsEnabled = m_settings->useAmneziaDns();

    connect(this, &ServersModel::defaultServerIndexChanged, this, &ServersModel::defaultServerNameChanged);

    connect(this, &ServersModel::defaultServerIndexChanged, this, [this](const int serverIndex) {
        auto defaultContainer =
                ContainerProps::containerFromString(m_servers.at(serverIndex).toObject().value(config_key::defaultContainer).toString());
        emit ServersModel::defaultServerDefaultContainerChanged(defaultContainer);
        emit ServersModel::defaultServerNameChanged();
        updateDefaultServerContainersModel();
    });
}

int ServersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_servers.size());
}

bool ServersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_servers.size())) {
        return false;
    }

    QJsonObject server = m_servers.at(index.row()).toObject();
    const auto configVersion = server.value(config_key::configVersion).toInt();

    switch (role) {
    case NameRole: {
        if (configVersion) {
            server.insert(config_key::name, value.toString());
        } else {
            server.insert(config_key::description, value.toString());
        }
        m_settings->editServer(index.row(), server);
        m_servers.replace(index.row(), server);
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

QVariant ServersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_servers.size())) {
        return QVariant();
    }

    const QJsonObject server = m_servers.at(index.row()).toObject();
    const auto apiConfig = server.value(configKey::apiConfig).toObject();
    const auto configVersion = server.value(config_key::configVersion).toInt();
    switch (role) {
    case NameRole: {
        if (configVersion) {
            return server.value(config_key::name).toString();
        }
        auto name = server.value(config_key::description).toString();
        if (name.isEmpty()) {
            return server.value(config_key::hostName).toString();
        }
        return name;
    }
    case ServerDescriptionRole: {
        auto description = getServerDescription(server, index.row());
        return configVersion ? description : description + server.value(config_key::hostName).toString();
    }
    case HostNameRole: return server.value(config_key::hostName).toString();
    case CredentialsRole: return QVariant::fromValue(serverCredentials(index.row()));
    case CredentialsLoginRole: return serverCredentials(index.row()).userName;
    case IsDefaultRole: return index.row() == m_defaultServerIndex;
    case IsCurrentlyProcessedRole: return index.row() == m_processedServerIndex;
    case HasWriteAccessRole: {
        auto credentials = serverCredentials(index.row());
        return (!credentials.userName.isEmpty() && !credentials.secretData.isEmpty());
    }
    case ContainsAmneziaDnsRole: {
        QString primaryDns = server.value(config_key::dns1).toString();
        return primaryDns == protocols::dns::amneziaDnsIp;
    }
    case DefaultContainerRole: {
        return ContainerProps::containerFromString(server.value(config_key::defaultContainer).toString());
    }
    case HasInstalledContainers: {
        return serverHasInstalledContainers(index.row());
    }
    case IsServerFromTelegramApiRole: {
        return server.value(config_key::configVersion).toInt() == ApiConfigSources::Telegram;
    }
    case IsServerFromGatewayApiRole: {
        return server.value(config_key::configVersion).toInt() == ApiConfigSources::AmneziaGateway;
    }
    case ApiConfigRole: {
        return apiConfig;
    }
    case IsCountrySelectionAvailableRole: {
        return !apiConfig.value(configKey::availableCountries).toArray().isEmpty();
    }
    case ApiAvailableCountriesRole: {
        return apiConfig.value(configKey::availableCountries).toArray();
    }
    case ApiServerCountryCodeRole: {
        return apiConfig.value(configKey::serverCountryCode).toString();
    }
    case HasAmneziaDns: {
        QString primaryDns = server.value(config_key::dns1).toString();
        return primaryDns == protocols::dns::amneziaDnsIp;
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
    m_servers = m_settings->serversArray();
    m_defaultServerIndex = m_settings->defaultServerIndex();
    m_processedServerIndex = m_defaultServerIndex;
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

QString ServersModel::getServerDescription(const QJsonObject &server, const int index) const
{
    const auto configVersion = server.value(config_key::configVersion).toInt();
    const auto apiConfig = server.value(configKey::apiConfig).toObject();

    QString description;

    if (configVersion && !apiConfig.value(configKey::serverCountryCode).toString().isEmpty()) {
        return apiConfig.value(configKey::serverCountryName).toString();
    } else if (configVersion) {
        return server.value(config_key::description).toString();
    } else if (data(index, HasWriteAccessRole).toBool()) {
        if (m_isAmneziaDnsEnabled && isAmneziaDnsContainerInstalled(index)) {
            description += "Amnezia DNS | ";
        }
    } else {
        if (data(index, HasAmneziaDns).toBool()) {
            description += "Amnezia DNS | ";
        }
    }
    return description;
}

const QString ServersModel::getDefaultServerDescriptionCollapsed()
{
    const QJsonObject server = m_servers.at(m_defaultServerIndex).toObject();
    const auto configVersion = server.value(config_key::configVersion).toInt();
    auto description = getServerDescription(server, m_defaultServerIndex);
    if (configVersion) {
        return description;
    }

    auto container = ContainerProps::containerFromString(server.value(config_key::defaultContainer).toString());

    return description += ContainerProps::containerHumanNames().value(container) + " | " + server.value(config_key::hostName).toString();
}

const QString ServersModel::getDefaultServerDescriptionExpanded()
{
    const QJsonObject server = m_servers.at(m_defaultServerIndex).toObject();
    const auto configVersion = server.value(config_key::configVersion).toInt();
    auto description = getServerDescription(server, m_defaultServerIndex);
    if (configVersion) {
        return description;
    }

    return description += server.value(config_key::hostName).toString();
}

const int ServersModel::getServersCount()
{
    return m_servers.count();
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
            emit updateApiLanguageModel();
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

void ServersModel::addServer(const QJsonObject &server)
{
    beginResetModel();
    m_settings->addServer(server);
    m_servers = m_settings->serversArray();
    endResetModel();
}

void ServersModel::editServer(const QJsonObject &server, const int serverIndex)
{
    m_settings->editServer(serverIndex, server);
    m_servers.replace(serverIndex, m_settings->serversArray().at(serverIndex));
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

void ServersModel::removeServer()
{
    beginResetModel();
    m_settings->removeServer(m_processedServerIndex);
    m_servers = m_settings->serversArray();

    if (m_settings->defaultServerIndex() == m_processedServerIndex) {
        setDefaultServerIndex(0);
    } else if (m_settings->defaultServerIndex() > m_processedServerIndex) {
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
    roles[ApiConfigRole] = "apiConfig";
    roles[IsCountrySelectionAvailableRole] = "isCountrySelectionAvailable";
    roles[ApiAvailableCountriesRole] = "apiAvailableCountries";
    roles[ApiServerCountryCodeRole] = "apiServerCountryCode";
    return roles;
}

ServerCredentials ServersModel::serverCredentials(int index) const
{
    const QJsonObject &s = m_servers.at(index).toObject();

    ServerCredentials credentials;
    credentials.hostName = s.value(config_key::hostName).toString();
    credentials.userName = s.value(config_key::userName).toString();
    credentials.secretData = s.value(config_key::password).toString();
    credentials.port = s.value(config_key::port).toInt();

    return credentials;
}

void ServersModel::updateContainersModel()
{
    auto containers = m_servers.at(m_processedServerIndex).toObject().value(config_key::containers).toArray();
    emit containersUpdated(containers);
}

void ServersModel::updateDefaultServerContainersModel()
{
    auto containers = m_servers.at(m_defaultServerIndex).toObject().value(config_key::containers).toArray();
    emit defaultServerContainersUpdated(containers);
}

QJsonObject ServersModel::getServerConfig(const int serverIndex)
{
    return m_servers.at(serverIndex).toObject();
}

void ServersModel::reloadDefaultServerContainerConfig()
{
    QJsonObject server = m_servers.at(m_defaultServerIndex).toObject();
    auto container = ContainerProps::containerFromString(server.value(config_key::defaultContainer).toString());

    auto containers = server.value(config_key::containers).toArray();

    auto config = m_settings->containerConfig(m_defaultServerIndex, container);
    for (auto i = 0; i < containers.size(); i++) {
        auto c = ContainerProps::containerFromString(containers.at(i).toObject().value(config_key::container).toString());
        if (c == container) {
            containers.replace(i, config);
            break;
        }
    }

    server.insert(config_key::containers, containers);
    editServer(server, m_defaultServerIndex);
}

void ServersModel::updateContainerConfig(const int containerIndex, const QJsonObject config)
{
    auto container = static_cast<DockerContainer>(containerIndex);
    QJsonObject server = m_servers.at(m_processedServerIndex).toObject();

    auto containers = server.value(config_key::containers).toArray();
    for (auto i = 0; i < containers.size(); i++) {
        auto c = ContainerProps::containerFromString(containers.at(i).toObject().value(config_key::container).toString());
        if (c == container) {
            containers.replace(i, config);
            break;
        }
    }

    server.insert(config_key::containers, containers);
    editServer(server, m_processedServerIndex);
}

void ServersModel::addContainerConfig(const int containerIndex, const QJsonObject config)
{
    auto container = static_cast<DockerContainer>(containerIndex);
    QJsonObject server = m_servers.at(m_processedServerIndex).toObject();

    auto containers = server.value(config_key::containers).toArray();
    containers.push_back(config);

    server.insert(config_key::containers, containers);

    auto defaultContainer = server.value(config_key::defaultContainer).toString();
    if (ContainerProps::containerFromString(defaultContainer) == DockerContainer::None
        && ContainerProps::containerService(container) != ServiceType::Other && ContainerProps::isSupportedByCurrentPlatform(container)) {
        server.insert(config_key::defaultContainer, ContainerProps::containerToString(container));
    }

    editServer(server, m_processedServerIndex);
}

void ServersModel::setDefaultContainer(const int serverIndex, const int containerIndex)
{
    auto container = static_cast<DockerContainer>(containerIndex);
    QJsonObject s = m_servers.at(serverIndex).toObject();
    s.insert(config_key::defaultContainer, ContainerProps::containerToString(container));
    editServer(s, serverIndex); // check
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
        QJsonObject s = m_servers.at(m_processedServerIndex).toObject();
        s.insert(config_key::containers, {});
        s.insert(config_key::defaultContainer, ContainerProps::containerToString(DockerContainer::None));

        editServer(s, m_processedServerIndex);
    }
    return errorCode;
}

ErrorCode ServersModel::rebootServer(const QSharedPointer<ServerController> &serverController)
{

    auto credentials = m_settings->serverCredentials(m_processedServerIndex);

    ErrorCode errorCode = serverController->rebootServer(credentials);
    return errorCode;
}

ErrorCode ServersModel::removeContainer(const QSharedPointer<ServerController> &serverController, const int containerIndex)
{

    auto credentials = m_settings->serverCredentials(m_processedServerIndex);
    auto dockerContainer = static_cast<DockerContainer>(containerIndex);

    ErrorCode errorCode = serverController->removeContainer(credentials, dockerContainer);

    if (errorCode == ErrorCode::NoError) {
        QJsonObject server = m_servers.at(m_processedServerIndex).toObject();

        auto containers = server.value(config_key::containers).toArray();
        for (auto it = containers.begin(); it != containers.end(); it++) {
            if (it->toObject().value(config_key::container).toString() == ContainerProps::containerToString(dockerContainer)) {
                containers.erase(it);
                break;
            }
        }

        server.insert(config_key::containers, containers);

        auto defaultContainer = ContainerProps::containerFromString(server.value(config_key::defaultContainer).toString());
        if (defaultContainer == containerIndex) {
            if (containers.empty()) {
                defaultContainer = DockerContainer::None;
            } else {
                defaultContainer =
                        ContainerProps::containerFromString(containers.begin()->toObject().value(config_key::container).toString());
            }
            server.insert(config_key::defaultContainer, ContainerProps::containerToString(defaultContainer));
        }

        editServer(server, m_processedServerIndex);
    }
    return errorCode;
}

void ServersModel::clearCachedProfile(const DockerContainer container)
{
    m_settings->clearLastConnectionConfig(m_processedServerIndex, container);
    m_servers.replace(m_processedServerIndex, m_settings->server(m_processedServerIndex));
    if (m_processedServerIndex == m_defaultServerIndex) {
        updateDefaultServerContainersModel();
    }
    updateContainersModel();
}

bool ServersModel::isAmneziaDnsContainerInstalled(const int serverIndex) const
{
    QJsonObject server = m_servers.at(serverIndex).toObject();
    auto containers = server.value(config_key::containers).toArray();
    for (auto it = containers.begin(); it != containers.end(); it++) {
        if (it->toObject().value(config_key::container).toString() == ContainerProps::containerToString(DockerContainer::Dns)) {
            return true;
        }
    }
    return false;
}

QPair<QString, QString> ServersModel::getDnsPair(int serverIndex)
{
    QPair<QString, QString> dns;

    const QJsonObject &server = m_servers.at(m_processedServerIndex).toObject();
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
    QJsonObject server = m_servers.at(serverIndex).toObject();
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

void ServersModel::toggleAmneziaDns(bool enabled)
{
    m_isAmneziaDnsEnabled = enabled;
    emit defaultServerDescriptionChanged();
}

bool ServersModel::isServerFromApiAlreadyExists(const quint16 crc)
{
    for (const auto &server : std::as_const(m_servers)) {
        if (static_cast<quint16>(server.toObject().value(config_key::crc).toInt()) == crc) {
            return true;
        }
    }
    return false;
}

bool ServersModel::isServerFromApiAlreadyExists(const QString &userCountryCode, const QString &serviceType, const QString &serviceProtocol)
{
    for (const auto &server : std::as_const(m_servers)) {
        const auto apiConfig = server.toObject().value(configKey::apiConfig).toObject();
        if (apiConfig.value(configKey::userCountryCode).toString() == userCountryCode
            && apiConfig.value(configKey::serviceType).toString() == serviceType
            && apiConfig.value(configKey::serviceProtocol).toString() == serviceProtocol) {
            return true;
        }
    }
    return false;
}

bool ServersModel::serverHasInstalledContainers(const int serverIndex) const
{
    QJsonObject server = m_servers.at(serverIndex).toObject();
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

bool ServersModel::isDefaultServerDefaultContainerHasSplitTunneling()
{
    auto server = m_servers.at(m_defaultServerIndex).toObject();
    auto defaultContainer = ContainerProps::containerFromString(server.value(config_key::defaultContainer).toString());

    auto containers = server.value(config_key::containers).toArray();
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

bool ServersModel::isServerFromApi(const int serverIndex)
{
    return data(serverIndex, IsServerFromTelegramApiRole).toBool() || data(serverIndex, IsServerFromGatewayApiRole).toBool();
}

bool ServersModel::isApiKeyExpired(const int serverIndex)
{
    auto serverConfig = m_servers.at(serverIndex).toObject();
    auto apiConfig = serverConfig.value(configKey::apiConfig).toObject();

    auto publicKeyInfo = apiConfig.value(configKey::publicKeyInfo).toObject();
    const QString endDate = publicKeyInfo.value(configKey::endDate).toString();
    if (endDate.isEmpty()) {
        publicKeyInfo.insert(configKey::endDate, QDateTime::currentDateTimeUtc().addDays(1).toString(Qt::ISODate));
        apiConfig.insert(configKey::publicKeyInfo, publicKeyInfo);
        serverConfig.insert(configKey::apiConfig, apiConfig);
        editServer(serverConfig, serverIndex);

        return false;
    }

    auto endDateDateTime = QDateTime::fromString(endDate, Qt::ISODate).toUTC();
    if (endDateDateTime < QDateTime::currentDateTimeUtc()) {
        return true;
    }
    return false;
}

void ServersModel::removeApiConfig(const int serverIndex)
{
    auto serverConfig = getServerConfig(serverIndex);

#ifdef Q_OS_IOS
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

    editServer(serverConfig, serverIndex);
}

const QString ServersModel::getDefaultServerImagePathCollapsed()
{
    const auto server = m_servers.at(m_defaultServerIndex).toObject();
    const auto apiConfig = server.value(configKey::apiConfig).toObject();
    const auto countryCode = apiConfig.value(configKey::serverCountryCode).toString();

    if (countryCode.isEmpty()) {
        return "";
    }
    return QString("qrc:/countriesFlags/images/flagKit/%1.svg").arg(countryCode.toUpper());
}
