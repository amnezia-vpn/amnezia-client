#include "serversmodel.h"

ServersModel::ServersModel(QObject *parent) :
    QAbstractListModel(parent)
{

}

void ServersModel::clearData()
{
    beginResetModel();
    content.clear();
    endResetModel();
}

void ServersModel::setContent(const std::vector<ServerModelContent> &data)
{
    beginResetModel();
    content = data;
    endResetModel();
}

int ServersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(content.size());
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
            || index.row() >= static_cast<int>(content.size())) {
        return QVariant();
    }
    if (role == DescRole) {
        return content[index.row()].desc;
    }
    if (role == AddressRole) {
        return content[index.row()].address;
    }
    if (role == IsDefaultRole) {
        return content[index.row()].isDefault;
    }
    return QVariant();
}


