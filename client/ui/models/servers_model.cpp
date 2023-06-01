#include "servers_model.h"

ServersModel::ServersModel(std::shared_ptr<Settings> settings, QObject *parent) : m_settings(settings), QAbstractListModel(parent)
{

}

int ServersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_settings->serversCount());
}

bool ServersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0
        || index.row() >= static_cast<int>(m_settings->serversCount())) {
        return false;
    }

    switch (role) {
        case IsDefaultRole: m_settings->setDefaultServer(index.row());
        default: return true;
    }

    emit dataChanged(index, index);
    return true;
}

QVariant ServersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_settings->serversCount())) {
        return QVariant();
    }

    const QJsonArray &servers = m_settings->serversArray();
    const QJsonObject server = servers.at(index.row()).toObject();

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
    case IsDefaultRole:
        return index.row() == m_settings->defaultServerIndex();
    case IsCurrentlyProcessedRole:
        return index.row() == m_currenlyProcessedServerIndex;
    }

    return QVariant();
}

const int ServersModel::getDefaultServerIndex()
{
    return m_settings->defaultServerIndex();
}

const int ServersModel::getServersCount()
{
    return m_settings->serversCount();
}

void ServersModel::setCurrentlyProcessedServerIndex(int index)
{
    m_currenlyProcessedServerIndex = index;
}

bool ServersModel::isDefaultServerCurrentlyProcessed()
{
    return m_settings->defaultServerIndex() == m_currenlyProcessedServerIndex;
}

ServerCredentials ServersModel::getCurrentlyProcessedServerCredentials()
{
    return qvariant_cast<ServerCredentials>(data(index(m_currenlyProcessedServerIndex), CredentialsRole));
}

void ServersModel::addServer(const QJsonObject &server)
{
    beginResetModel();
    m_settings->addServer(server);
    endResetModel();
}

void ServersModel::removeServer()
{
    beginResetModel();
    m_settings->removeServer(m_currenlyProcessedServerIndex);

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
    roles[IsDefaultRole] = "isDefault";
    roles[IsCurrentlyProcessedRole] = "isCurrentlyProcessed";
    return roles;
}
