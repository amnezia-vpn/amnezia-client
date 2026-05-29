#include "allowedDnsModel.h"

AllowedDnsModel::AllowedDnsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int AllowedDnsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_dnsServers.size();
}

QVariant AllowedDnsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(rowCount()))
        return QVariant();

    switch (role) {
    case IpRole:
        return m_dnsServers.at(index.row());
    default:
        return QVariant();
    }
}

void AllowedDnsModel::updateModel(const QStringList &dnsServers)
{
    beginResetModel();
    m_dnsServers = dnsServers;
    endResetModel();
}

QHash<int, QByteArray> AllowedDnsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IpRole] = "ip";
    return roles;
}
