#include "servers_model.h"

#include "core/controllers/serverController.h"

ServersModel::ServersModel(std::shared_ptr<Settings> settings, QObject *parent)
    : m_settings(settings), QAbstractListModel(parent)
{
    m_servers = m_settings->serversArray();
    m_defaultServerIndex = m_settings->defaultServerIndex();
    m_currentlyProcessedServerIndex = m_defaultServerIndex;

    connect(this, &ServersModel::defaultServerIndexChanged, this, &ServersModel::defaultServerNameChanged);
    connect(this, &ServersModel::defaultContainerChanged, this, &ServersModel::defaultServerDescriptionChanged);
    connect(this, &ServersModel::defaultServerIndexChanged, this, [this](const int serverIndex) {
        auto defaultContainer = ContainerProps::containerFromString(m_servers.at(serverIndex).toObject().value(config_key::defaultContainer).toString());
        emit ServersModel::defaultContainerChanged(defaultContainer);
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

    switch (role) {
    case NameRole: {
        server.insert(config_key::description, value.toString());
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
        if (configVersion) {
            return server.value(config_key::description).toString();
        }
        return server.value(config_key::hostName).toString();
    }
    case HostNameRole: return server.value(config_key::hostName).toString();
    case CredentialsRole: return QVariant::fromValue(serverCredentials(index.row()));
    case CredentialsLoginRole: return serverCredentials(index.row()).userName;
    case IsDefaultRole: return index.row() == m_defaultServerIndex;
    case IsCurrentlyProcessedRole: return index.row() == m_currentlyProcessedServerIndex;
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
    m_currentlyProcessedServerIndex = m_defaultServerIndex;
    endResetModel();
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

const QString ServersModel::getDefaultServerHostName()
{
    return qvariant_cast<QString>(data(m_defaultServerIndex, HostNameRole));
}

QString ServersModel::getDefaultServerDescription(const QJsonObject &server)
{
    const auto configVersion = server.value(config_key::configVersion).toInt();

    QString description;

    if (configVersion) {
        return server.value(config_key::description).toString();
    } else if (isDefaultServerHasWriteAccess()) {
        if (m_isAmneziaDnsEnabled
            && isAmneziaDnsContainerInstalled(m_defaultServerIndex)) {
            description += "Amnezia DNS | ";
        }
    } else {
        if (isDefaultServerConfigContainsAmneziaDns()) {
            description += "Amnezia DNS | ";
        }
    }
    return description;
}

const QString ServersModel::getDefaultServerDescriptionCollapsed()
{
    const QJsonObject server = m_servers.at(m_defaultServerIndex).toObject();
    const auto configVersion = server.value(config_key::configVersion).toInt();
    auto description = getDefaultServerDescription(server);
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
    auto description = getDefaultServerDescription(server);
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

void ServersModel::setCurrentlyProcessedServerIndex(const int index)
{
    m_currentlyProcessedServerIndex = index;
    updateContainersModel();
    emit currentlyProcessedServerIndexChanged(m_currentlyProcessedServerIndex);
}

int ServersModel::getCurrentlyProcessedServerIndex()
{
    return m_currentlyProcessedServerIndex;
}

QString ServersModel::getCurrentlyProcessedServerHostName()
{
    return qvariant_cast<QString>(data(m_currentlyProcessedServerIndex, HostNameRole));
}

const ServerCredentials ServersModel::getCurrentlyProcessedServerCredentials()
{
    return serverCredentials(m_currentlyProcessedServerIndex);
}

const ServerCredentials ServersModel::getServerCredentials(const int index)
{
    return serverCredentials(index);
}

bool ServersModel::isDefaultServerCurrentlyProcessed()
{
    return m_defaultServerIndex == m_currentlyProcessedServerIndex;
}

bool ServersModel::isCurrentlyProcessedServerHasWriteAccess()
{
    return qvariant_cast<bool>(data(m_currentlyProcessedServerIndex, HasWriteAccessRole));
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

void ServersModel::editServer(const QJsonObject &server)
{
    beginResetModel();
    m_settings->editServer(m_currentlyProcessedServerIndex, server);
    m_servers = m_settings->serversArray();
    endResetModel();
    updateContainersModel();
}

void ServersModel::removeServer()
{
    beginResetModel();
    m_settings->removeServer(m_currentlyProcessedServerIndex);
    m_servers = m_settings->serversArray();

    if (m_settings->defaultServerIndex() == m_currentlyProcessedServerIndex) {
        setDefaultServerIndex(0);
    } else if (m_settings->defaultServerIndex() > m_currentlyProcessedServerIndex) {
        setDefaultServerIndex(m_settings->defaultServerIndex() - 1);
    }

    if (m_settings->serversCount() == 0) {
        setDefaultServerIndex(-1);
    }
    endResetModel();
}

bool ServersModel::isDefaultServerConfigContainsAmneziaDns()
{
    const QJsonObject server = m_servers.at(m_defaultServerIndex).toObject();
    QString primaryDns = server.value(config_key::dns1).toString();
    return primaryDns == protocols::dns::amneziaDnsIp;
}

QHash<int, QByteArray> ServersModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[NameRole] = "serverName";
    roles[NameRole] = "name";
    roles[ServerDescriptionRole] = "serverDescription";

    roles[HostNameRole] = "hostName";

    roles[CredentialsRole] = "credentials";
    roles[CredentialsLoginRole] = "credentialsLogin";

    roles[IsDefaultRole] = "isDefault";
    roles[IsCurrentlyProcessedRole] = "isCurrentlyProcessed";

    roles[HasWriteAccessRole] = "hasWriteAccess";

    roles[ContainsAmneziaDnsRole] = "containsAmneziaDns";

    roles[DefaultContainerRole] = "defaultContainer";
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
    auto containers = m_servers.at(m_currentlyProcessedServerIndex).toObject().value(config_key::containers).toArray();
    emit containersUpdated(containers);
}

QJsonObject ServersModel::getDefaultServerConfig()
{
    return m_servers.at(m_defaultServerIndex).toObject();
}

void ServersModel::reloadContainerConfig()
{
    QJsonObject server = m_servers.at(m_currentlyProcessedServerIndex).toObject();
    auto container = ContainerProps::containerFromString(server.value(config_key::defaultContainer).toString());

    auto containers = server.value(config_key::containers).toArray();

    auto config = m_settings->containerConfig(m_currentlyProcessedServerIndex, container);
    for (auto i = 0; i < containers.size(); i++) {
        auto c = ContainerProps::containerFromString(containers.at(i).toObject().value(config_key::container).toString());
        if (c == container) {
            containers.replace(i, config);
            break;
        }
    }

    server.insert(config_key::containers, containers);
    editServer(server);
}

void ServersModel::updateContainerConfig(const int containerIndex, const QJsonObject config)
{
    auto container = static_cast<DockerContainer>(containerIndex);
    QJsonObject server = m_servers.at(m_currentlyProcessedServerIndex).toObject();

    auto containers = server.value(config_key::containers).toArray();
    for (auto i = 0; i < containers.size(); i++) {
        auto c = ContainerProps::containerFromString(containers.at(i).toObject().value(config_key::container).toString());
        if (c == container) {
            containers.replace(i, config);
            break;
        }
    }

    server.insert(config_key::containers, containers);

    auto defaultContainer = server.value(config_key::defaultContainer).toString();
    if ((ContainerProps::containerFromString(defaultContainer) == DockerContainer::None || ContainerProps::containerService(container) != ServiceType::Other)) {
        server.insert(config_key::defaultContainer, ContainerProps::containerToString(container));
    }

    editServer(server);
}

void ServersModel::addContainerConfig(const int containerIndex, const QJsonObject config)
{
    auto container = static_cast<DockerContainer>(containerIndex);
    QJsonObject server = m_servers.at(m_currentlyProcessedServerIndex).toObject();

    auto containers = server.value(config_key::containers).toArray();
    containers.push_back(config);

    server.insert(config_key::containers, containers);

    bool isDefaultContainerChanged = false;
    auto defaultContainer = server.value(config_key::defaultContainer).toString();
    if ((ContainerProps::containerFromString(defaultContainer) == DockerContainer::None || ContainerProps::containerService(container) != ServiceType::Other)) {
        server.insert(config_key::defaultContainer, ContainerProps::containerToString(container));
        isDefaultContainerChanged = true;
    }

    editServer(server);
    if (isDefaultContainerChanged) {
        emit defaultContainerChanged(container);
    }
}

void ServersModel::setDefaultContainer(const int containerIndex)
{
    auto container = static_cast<DockerContainer>(containerIndex);
    QJsonObject s = m_servers.at(m_currentlyProcessedServerIndex).toObject();
    s.insert(config_key::defaultContainer, ContainerProps::containerToString(container));
    editServer(s); //check
    emit defaultContainerChanged(container);
}

DockerContainer ServersModel::getDefaultContainer()
{
    return qvariant_cast<DockerContainer>(data(m_currentlyProcessedServerIndex, DefaultContainerRole));
}

const QString ServersModel::getDefaultContainerName()
{
    auto defaultContainer = getDefaultContainer();
    return ContainerProps::containerHumanNames().value(defaultContainer);
}

ErrorCode ServersModel::removeAllContainers()
{
    ServerController serverController(m_settings);
    ErrorCode errorCode =
            serverController.removeAllContainers(m_settings->serverCredentials(m_currentlyProcessedServerIndex));

    if (errorCode == ErrorCode::NoError) {
        QJsonObject s = m_servers.at(m_currentlyProcessedServerIndex).toObject();
        s.insert(config_key::containers, {});
        s.insert(config_key::defaultContainer, ContainerProps::containerToString(DockerContainer::None));

        editServer(s);
        emit defaultContainerChanged(DockerContainer::None);
    }
    return errorCode;
}

ErrorCode ServersModel::removeContainer(const int containerIndex)
{
    ServerController serverController(m_settings);
    auto credentials = m_settings->serverCredentials(m_currentlyProcessedServerIndex);
    auto dockerContainer = static_cast<DockerContainer>(containerIndex);

    ErrorCode errorCode = serverController.removeContainer(credentials, dockerContainer);

    if (errorCode == ErrorCode::NoError) {
        QJsonObject server = m_servers.at(m_currentlyProcessedServerIndex).toObject();

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
                defaultContainer = ContainerProps::containerFromString(containers.begin()->toObject().value(config_key::container).toString());
            }
            server.insert(config_key::defaultContainer, ContainerProps::containerToString(defaultContainer));
        }

        editServer(server);
        emit defaultContainerChanged(defaultContainer);
    }
    return errorCode;
}

void ServersModel::clearCachedProfiles()
{
    const auto &containers = m_settings->containers(m_currentlyProcessedServerIndex);
    for (DockerContainer container : containers.keys()) {
        m_settings->clearLastConnectionConfig(m_currentlyProcessedServerIndex, container);
    }

    m_servers.replace(m_currentlyProcessedServerIndex, m_settings->server(m_currentlyProcessedServerIndex));
    updateContainersModel();
}

bool ServersModel::isAmneziaDnsContainerInstalled(const int serverIndex)
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

