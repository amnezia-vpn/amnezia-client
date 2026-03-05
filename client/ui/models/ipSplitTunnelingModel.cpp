#include "ipSplitTunnelingModel.h"

IpSplitTunnelingModel::IpSplitTunnelingModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int IpSplitTunnelingModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_sites.size();
}

QVariant IpSplitTunnelingModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(rowCount()))
        return QVariant();

    switch (role) {
    case UrlRole: {
        return m_sites.at(index.row()).first;
        break;
    }
    case IpRole: {
        return m_sites.at(index.row()).second;
        break;
    }
    default: {
        return true;
    }
    }

    return QVariant();
}

void IpSplitTunnelingModel::updateModel(const QVector<QPair<QString, QString>> &sites)
{
    beginResetModel();
    m_sites = sites;
    endResetModel();
}

QHash<int, QByteArray> IpSplitTunnelingModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[UrlRole] = "url";
    roles[IpRole] = "ip";
    return roles;
}
