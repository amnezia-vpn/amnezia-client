#include "servers_model.h"

ServersModel::ServersModel(std::shared_ptr<Settings> settings, QObject *parent)
    : m_settings(settings), QAbstractListModel(parent)
{
    m_servers = m_settings->serversArray();
    m_defaultServerIndex = m_settings->defaultServerIndex();
    m_currentlyProcessedServerIndex = m_defaultServerIndex;

    connect(this, &ServersModel::defaultServerIndexChanged, this, &ServersModel::defaultServerNameChanged);
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

    switch (role) {
    case NameRole: {
        auto description = server.value(config_key::description).toString();
        if (description.isEmpty()) {
            return server.value(config_key::hostName).toString();
        }
        return description;
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

void ServersModel::updateContainersConfig()
{
    auto server = m_settings->server(m_currentlyProcessedServerIndex);
    m_servers.replace(m_currentlyProcessedServerIndex, server);
}

QHash<int, QByteArray> ServersModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[HostNameRole] = "hostName";
    roles[CredentialsRole] = "credentials";
    roles[CredentialsLoginRole] = "credentialsLogin";
    roles[IsDefaultRole] = "isDefault";
    roles[IsCurrentlyProcessedRole] = "isCurrentlyProcessed";
    roles[HasWriteAccessRole] = "hasWriteAccess";
    roles[ContainsAmneziaDnsRole] = "containsAmneziaDns";
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
