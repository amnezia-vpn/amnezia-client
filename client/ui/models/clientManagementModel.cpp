#include "clientManagementModel.h"

#include <QJsonObject>

#include "core/utils/constants/configKeys.h"

using namespace amnezia;

ClientManagementModel::ClientManagementModel(QObject *parent)
    : QAbstractListModel(parent)
{
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
    case ClientIdRole: return client.value(configKey::clientId).toString();
    case ClientNameRole: return userData.value(configKey::clientName).toString();
    case CreationDateRole: return userData.value(configKey::creationDate).toString();
    case LatestHandshakeRole: return userData.value(configKey::latestHandshake).toString();
    case DataReceivedRole: return userData.value(configKey::dataReceived).toString();
    case DataSentRole: return userData.value(configKey::dataSent).toString();
    case AllowedIpsRole: return userData.value(configKey::allowedIps).toString();
    }

    return QVariant();
}

void ClientManagementModel::updateModel(const QJsonArray &clients)
{
    beginResetModel();
    m_clientsTable = clients;
    endResetModel();
}

void ClientManagementModel::updateClientName(int row, const QString &newName)
{
    if (row < 0 || row >= m_clientsTable.size()) {
        return;
    }
    QJsonObject client = m_clientsTable.at(row).toObject();
    QJsonObject userData = client.value(configKey::userData).toObject();
    userData[configKey::clientName] = newName;
    client[configKey::userData] = userData;
    m_clientsTable.replace(row, client);
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, { ClientNameRole });
}

QHash<int, QByteArray> ClientManagementModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ClientIdRole] = "clientId";
    roles[ClientNameRole] = "clientName";
    roles[CreationDateRole] = "creationDate";
    roles[LatestHandshakeRole] = "latestHandshake";
    roles[DataReceivedRole] = "dataReceived";
    roles[DataSentRole] = "dataSent";
    roles[AllowedIpsRole] = "allowedIps";
    return roles;
}
