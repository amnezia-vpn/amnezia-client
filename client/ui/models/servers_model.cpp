#include "servers_model.h"

ServersModel::ServersModel(std::shared_ptr<Settings> settings, QObject *parent) : m_settings(settings), QAbstractListModel(parent)
{
    const QJsonArray &servers = m_settings->serversArray();
    int defaultServer = m_settings->defaultServerIndex();
    QVector<ServerModelContent> serverListContent;
    for(int i = 0; i < servers.size(); i++) {
        ServerModelContent c;
        auto server = servers.at(i).toObject();
        c.desc = server.value(config_key::description).toString();
        c.address = server.value(config_key::hostName).toString();
        if (c.desc.isEmpty()) {
            c.desc = c.address;
        }
        c.isDefault = (i == defaultServer);
        serverListContent.push_back(c);
    }
    setContent(serverListContent);
}

void ServersModel::clearData()
{
    beginResetModel();
    m_content.clear();
    endResetModel();
}

void ServersModel::setContent(const QVector<ServerModelContent> &data)
{
    beginResetModel();
    m_content = data;
    endResetModel();
}

int ServersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_content.size());
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
            || index.row() >= static_cast<int>(m_content.size())) {
        return QVariant();
    }
    if (role == DescRole) {
        return m_content[index.row()].desc;
    }
    if (role == AddressRole) {
        return m_content[index.row()].address;
    }
    if (role == IsDefaultRole) {
        return m_content[index.row()].isDefault;
    }
    return QVariant();
}


