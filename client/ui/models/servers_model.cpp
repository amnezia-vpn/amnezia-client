#include "servers_model.h"

ServersModel::ServersModel(std::shared_ptr<Settings> settings, QObject *parent) : m_settings(settings), QAbstractListModel(parent)
{
    m_servers = m_settings->serversArray();
    m_defaultServerIndex = m_settings->defaultServerIndex();
}

int ServersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_servers.size());
}

bool ServersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0
        || index.row() >= static_cast<int>(m_servers.size())) {
        return false;
    }

    QJsonObject server = m_servers.at(index.row()).toObject();

    switch (role) {
    case NameRole: {
        server.insert(config_key::description, value.toString());
        m_settings->editServer(index.row(), server);
        m_servers.replace(index.row(), server);
        break;
    }
    case IsDefaultRole: {
        m_settings->setDefaultServer(index.row());
        m_defaultServerIndex = m_settings->defaultServerIndex();
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
    case HostNameRole:
        return server.value(config_key::hostName).toString();
    case CredentialsRole:
        return QVariant::fromValue(m_settings->serverCredentials(index.row()));
    case CredentialsLoginRole:
        return m_settings->serverCredentials(index.row()).userName;
    case IsDefaultRole:
        return index.row() == m_defaultServerIndex;
    case IsCurrentlyProcessedRole:
        return index.row() == m_currenlyProcessedServerIndex;
    }

    return QVariant();
}

QVariant ServersModel::data(const int index, int role) const
{
    QModelIndex modelIndex = this->index(index);
    return data(modelIndex, role);
}

const int ServersModel::getDefaultServerIndex()
{
    return m_defaultServerIndex;
}

const int ServersModel::getServersCount()
{
    return m_servers.count();
}

void ServersModel::setCurrentlyProcessedServerIndex(int index)
{
    m_currenlyProcessedServerIndex = index;
}

int ServersModel::getCurrentlyProcessedServerIndex()
{
    return m_currenlyProcessedServerIndex;
}

bool ServersModel::isDefaultServerCurrentlyProcessed()
{
    return m_defaultServerIndex == m_currenlyProcessedServerIndex;
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
    m_settings->removeServer(m_currenlyProcessedServerIndex);
    m_servers = m_settings->serversArray();

    if (m_settings->defaultServerIndex() == m_currenlyProcessedServerIndex) {
        m_settings->setDefaultServer(0);
    } else if (m_settings->defaultServerIndex() > m_currenlyProcessedServerIndex) {
        m_settings->setDefaultServer(m_settings->defaultServerIndex() - 1);
    }

    if (m_settings->serversCount() == 0) {
        m_settings->setDefaultServer(-1);
    }
    endResetModel();
}

QHash<int, QByteArray> ServersModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[HostNameRole] = "hostName";
    roles[CredentialsRole] = "credentials";
    roles[CredentialsLoginRole] = "credentialsLogin";
    roles[IsDefaultRole] = "isDefault";
    roles[IsCurrentlyProcessedRole] = "isCurrentlyProcessed";
    return roles;
}
