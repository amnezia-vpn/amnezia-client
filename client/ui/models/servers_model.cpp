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

    if (role == DescRole) {
        auto description = server.value(config_key::description).toString();
        if (description.isEmpty()) {
            return server.value(config_key::hostName).toString();
        }
        return description;
    }
    if (role == AddressRole) {
        return server.value(config_key::hostName).toString();
    }
    if (role == IsDefaultRole) {
        return index.row() == m_settings->defaultServerIndex();
    }
    return QVariant();


//    if (!index.isValid() || index.row() < 0
//            || index.row() >= static_cast<int>(m_data.size())) {
//        return QVariant();
//    }
//    if (role == DescRole) {
//        return m_data[index.row()].desc;
//    }
//    if (role == AddressRole) {
//        return m_data[index.row()].address;
//    }
//    if (role == IsDefaultRole) {
//        return m_data[index.row()].isDefault;
//    }
//    return QVariant();
}

void ServersModel::setDefaultServerIndex(int index)
{
//    beginResetModel();
    m_settings->setDefaultServer(index);
    //    endResetModel();
}

int ServersModel::getDefaultServerIndex()
{
    return m_settings->defaultServerIndex();
}

QHash<int, QByteArray> ServersModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[DescRole] = "desc";
    roles[AddressRole] = "address";
    roles[IsDefaultRole] = "is_default";
    return roles;
}
