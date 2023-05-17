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
//    if (role == DescRole) {
//        return m_data[index.row()].desc;
//    }
//    if (role == AddressRole) {
//        return m_data[index.row()].address;
//    }
//    if (role == IsDefaultRole) {
//        return m_data[index.row()].isDefault;
//    }
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
    }

    return QVariant();
}

//todo mode to setData?
void ServersModel::setDefaultServerIndex(int index)
{
//    beginResetModel();
    m_settings->setDefaultServer(index);
    //    endResetModel();
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

ServerCredentials ServersModel::getCurrentlyProcessedServerCredentials()
{
    return qvariant_cast<ServerCredentials>(data(index(m_currenlyProcessedServerIndex), CredentialsRole));
}

QHash<int, QByteArray> ServersModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[HostNameRole] = "hostName";
    roles[CredentialsRole] = "credentials";
    roles[IsDefaultRole] = "isDefault";
    return roles;
}
