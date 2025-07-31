#include "allowed_dns_model.h"
#include "core/controllers/dnsController.h"

AllowedDnsModel::AllowedDnsModel(QSharedPointer<DnsController> dnsController,
                                 QObject *parent)
    : QAbstractListModel(parent), 
      m_dnsController(dnsController)
{
    // Connect to controller signals
    connect(m_dnsController.data(), &DnsController::dnsAdded,
            this, &AllowedDnsModel::onDnsAdded);
    connect(m_dnsController.data(), &DnsController::dnsListAdded,
            this, &AllowedDnsModel::onDnsListAdded);
    connect(m_dnsController.data(), &DnsController::dnsRemoved,
            this, &AllowedDnsModel::onDnsRemoved);

    // Initialize data
    refreshData();
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
    return m_dnsController->addDns(ip);
}

void AllowedDnsModel::addDnsList(const QStringList &dnsServers, bool replaceExisting)
{
    m_dnsController->addDnsList(dnsServers, replaceExisting);
}

void AllowedDnsModel::removeDns(QModelIndex index)
{
    if (!index.isValid() || index.row() >= m_dnsServers.size()) {
        return;
    }

    const QString &ip = m_dnsServers.at(index.row());
    m_dnsController->removeDns(ip);
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

void AllowedDnsModel::onDnsAdded(const QString &ip)
{
    Q_UNUSED(ip)
    beginResetModel();
    refreshData();
    endResetModel();
}

void AllowedDnsModel::onDnsListAdded(const QStringList &dnsServers)
{
    Q_UNUSED(dnsServers)
    beginResetModel();
    refreshData();
    endResetModel();
}

void AllowedDnsModel::onDnsRemoved(const QString &ip)
{
    Q_UNUSED(ip)
    beginResetModel();
    refreshData();
    endResetModel();
}

void AllowedDnsModel::refreshData()
{
    m_dnsServers = m_dnsController->getAllowedDnsServers();
}
