#include "clientManagementModel.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "core/controllers/selfhosted/serverController.h"
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

ClientManagementModel::ClientManagementModel(std::shared_ptr<Settings> settings, 
                                           QSharedPointer<ClientManagementController> clientManagementController,
                                           QObject *parent)
    : QAbstractListModel(parent), 
      m_settings(settings), 
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
    return static_cast<int>(m_clientsTable.size());
}

QVariant ClientManagementModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_clientsTable.size())) {
        return QVariant();
    }

    auto client = m_clientsTable.at(index.row()).toObject();
    auto userData = client.value(configKey::userData).toObject();

    switch (role) {
    case ClientNameRole: return userData.value(configKey::clientName).toString();
    case CreationDateRole: return userData.value(configKey::creationDate).toString();
    case LatestHandshakeRole: return userData.value(configKey::latestHandshake).toString();
    case DataReceivedRole: return userData.value(configKey::dataReceived).toString();
    case DataSentRole: return userData.value(configKey::dataSent).toString();
    case AllowedIpsRole: return userData.value(configKey::allowedIps).toString();
    }

    return QVariant();
}

void ClientManagementModel::onClientsDataUpdated(const QJsonArray &clientsTable)
{
    beginResetModel();
    m_clientsTable = clientsTable;
    endResetModel();
}

void ClientManagementModel::onClientRenamed(const int row, const QString &newName)
{
    Q_UNUSED(newName)
    if (row >= 0 && row < m_clientsTable.size()) {
        emit dataChanged(index(row, 0), index(row, 0));
    }
}

void ClientManagementModel::migration(const QByteArray &clientsTableString)
{
    QJsonObject clientsTable = QJsonDocument::fromJson(clientsTableString).object();

    for (auto &clientId : clientsTable.keys()) {
        QJsonObject client;
        client[configKey::clientId] = clientId;

        QJsonObject userData;
        userData[configKey::clientName] = clientsTable.value(clientId).toObject().value(configKey::clientName);
        client[configKey::userData] = userData;

        m_clientsTable.push_back(client);
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
