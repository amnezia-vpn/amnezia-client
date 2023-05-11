#include "servers_model.h"

ServersModel::ServersModel(std::shared_ptr<Settings> settings, QObject *parent) : m_settings(settings), QAbstractListModel(parent)
{
    refresh();
}

void ServersModel::refresh()
{
    beginResetModel();
    const QJsonArray &servers = m_settings->serversArray();
    int defaultServer = m_settings->defaultServerIndex();
    QVector<ServerModelContent> serverListContent;
    for(int i = 0; i < servers.size(); i++) {
        ServerModelContent content;
        auto server = servers.at(i).toObject();
        content.desc = server.value(config_key::description).toString();
        content.address = server.value(config_key::hostName).toString();
        if (content.desc.isEmpty()) {
            content.desc = content.address;
        }
        content.isDefault = (i == defaultServer);
        serverListContent.push_back(content);
    }
    m_data = serverListContent;
    endResetModel();
}

int ServersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_data.size());
}

QHash<int, QByteArray> ServersModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[DescRole] = "desc";
    roles[AddressRole] = "address";
    roles[IsDefaultRole] = "is_default";
    return roles;
}

QVariant ServersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
            || index.row() >= static_cast<int>(m_data.size())) {
        return QVariant();
    }
    if (role == DescRole) {
        return m_data[index.row()].desc;
    }
    if (role == AddressRole) {
        return m_data[index.row()].address;
    }
    if (role == IsDefaultRole) {
        return m_data[index.row()].isDefault;
    }
    return QVariant();
}

void ServersModel::setDefaultServerIndex(int index)
{

}


