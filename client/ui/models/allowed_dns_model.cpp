#include "allowed_dns_model.h"

AllowedDnsModel::AllowedDnsModel(std::shared_ptr<Settings> settings, QObject *parent)
    : QAbstractListModel(parent), m_settings(settings)
{
    fillDnsServers();
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

bool AllowedDnsModel::addDns(const QString &ip)
{
    if (m_dnsServers.contains(ip)) {
        return false;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_dnsServers.append(ip);
    m_settings->setAllowedDnsServers(m_dnsServers);
    endInsertRows();
    return true;
}

void AllowedDnsModel::addDnsList(const QStringList &dnsServers, bool replaceExisting)
{
    beginResetModel();
    
    if (replaceExisting) {
        m_dnsServers.clear();
    }
    
    for (const QString &ip : dnsServers) {
        if (!m_dnsServers.contains(ip)) {
            m_dnsServers.append(ip);
        }
    }
    
    m_settings->setAllowedDnsServers(m_dnsServers);
    endResetModel();
}

void AllowedDnsModel::removeDns(QModelIndex index)
{
    if (!index.isValid() || index.row() >= m_dnsServers.size()) {
        return;
    }

    beginRemoveRows(QModelIndex(), index.row(), index.row());
    m_dnsServers.removeAt(index.row());
    m_settings->setAllowedDnsServers(m_dnsServers);
    endRemoveRows();
}

QStringList AllowedDnsModel::getCurrentDnsServers()
{
    return m_dnsServers;
}

QHash<int, QByteArray> AllowedDnsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IpRole] = "ip";
    return roles;
}

void AllowedDnsModel::fillDnsServers()
{
    m_dnsServers = m_settings->allowedDnsServers();
}
