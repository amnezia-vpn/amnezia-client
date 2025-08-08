#include "clientManagementModel.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "core/controllers/selfhosted/clientManagementController.h"
#include "logger.h"

namespace
{
    Logger logger("ClientManagementModel");

    namespace configKey
    {
        constexpr char clientId[] = "clientId";
        constexpr char clientName[] = "clientName";
        constexpr char container[] = "container";
        constexpr char userData[] = "userData";
        constexpr char creationDate[] = "creationDate";
        constexpr char latestHandshake[] = "latestHandshake";
        constexpr char dataReceived[] = "dataReceived";
        constexpr char dataSent[] = "dataSent";
        constexpr char allowedIps[] = "allowedIps";
    }
}

ClientManagementModel::ClientManagementModel(QSharedPointer<ClientManagementController> clientManagementController,
                                           QObject *parent)
    : QAbstractListModel(parent), 
      m_clientManagementController(clientManagementController)
{
    connect(m_clientManagementController.data(), &ClientManagementController::clientsDataUpdated,
            this, &ClientManagementModel::onClientsDataUpdated);
    connect(m_clientManagementController.data(), &ClientManagementController::clientRenamed,
            this, &ClientManagementModel::onClientRenamed);
    connect(m_clientManagementController.data(), &ClientManagementController::adminConfigRevoked,
            this, &ClientManagementModel::adminConfigRevoked);
}

int ClientManagementModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_clientsList.size());
}

QVariant ClientManagementModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_clientsList.size())) {
        return QVariant();
    }
    const ClientInfo &client = m_clientsList.at(index.row());

    switch (role) {
    case ClientNameRole: return client.clientName;
    case CreationDateRole: return client.creationDate.toString();
    case LatestHandshakeRole: return client.latestHandshake;
    case DataReceivedRole: return client.dataReceived;
    case DataSentRole: return client.dataSent;
    case AllowedIpsRole: return client.allowedIps;
    }

    return QVariant();
}

void ClientManagementModel::onClientsDataUpdated(const QList<ClientInfo> &clientsList)
{
    beginResetModel();
    m_clientsList = clientsList;
    endResetModel();
}

void ClientManagementModel::onClientRenamed(const int row, const QString &newName)
{
    Q_UNUSED(newName)
    if (row >= 0 && row < m_clientsList.size()) {
        emit dataChanged(index(row, 0), index(row, 0));
    }
}



QHash<int, QByteArray> ClientManagementModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ClientNameRole] = "clientName";
    roles[CreationDateRole] = "creationDate";
    roles[LatestHandshakeRole] = "latestHandshake";
    roles[DataReceivedRole] = "dataReceived";
    roles[DataSentRole] = "dataSent";
    roles[AllowedIpsRole] = "allowedIps";
    return roles;
}
