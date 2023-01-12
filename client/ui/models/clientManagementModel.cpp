#include "clientManagementModel.h"

ClientManagementModel::ClientManagementModel(std::shared_ptr<Settings> settings, QObject *parent) :
    m_settings(settings),
    QAbstractListModel(parent)
{

}

void ClientManagementModel::clearData()
{
    beginResetModel();
    m_content.clear();
    endResetModel();
}

void ClientManagementModel::setContent(const QVector<QVariant> &data)
{
    beginResetModel();
    m_content = data;
    endResetModel();
}

QJsonObject ClientManagementModel::getContent(Proto protocol)
{
    QJsonObject clientsTable;
    for (const auto &item : m_content) {
        if (protocol == Proto::OpenVpn) {
            clientsTable[item.toJsonObject()["openvpnCertId"].toString()] = item.toJsonObject();
        } else if (protocol == Proto::WireGuard) {
            clientsTable[item.toJsonObject()["wireguardPublicKey"].toString()] = item.toJsonObject();
        }
    }
    return clientsTable;
}

int ClientManagementModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_content.size());
}

QVariant ClientManagementModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
            || index.row() >= static_cast<int>(m_content.size())) {
        return QVariant();
    }

    if (role == NameRole) {
        return m_content[index.row()].toJsonObject()["clientName"].toString();
    } else if (role == OpenVpnCertIdRole) {
        return m_content[index.row()].toJsonObject()["openvpnCertId"].toString();
    } else if (role == OpenVpnCertDataRole) {
        return m_content[index.row()].toJsonObject()["openvpnCertData"].toString();
    } else if (role == WireGuardPublicKey) {
        return m_content[index.row()].toJsonObject()["wireguardPublicKey"].toString();
    }

    return QVariant();
}

void ClientManagementModel::setData(const QModelIndex &index, QVariant data, int role)
{
    if (!index.isValid() || index.row() < 0
            || index.row() >= static_cast<int>(m_content.size())) {
        return;
    }

    auto client = m_content[index.row()].toJsonObject();
    if (role == NameRole) {
        client["clientName"] = data.toString();
    } else if (role == OpenVpnCertIdRole) {
        client["openvpnCertId"] = data.toString();
    } else if (role == OpenVpnCertDataRole) {
        client["openvpnCertData"] = data.toString();
    } else if (role == WireGuardPublicKey) {
        client["wireguardPublicKey"] = data.toString();
    } else {
        return;
    }
    if (m_content[index.row()] != client) {
        m_content[index.row()] = client;
        emit dataChanged(index, index);
    }
}

QHash<int, QByteArray> ClientManagementModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "clientName";
    roles[OpenVpnCertIdRole] = "openvpnCertId";
    roles[OpenVpnCertDataRole] = "openvpnCertData";
    roles[WireGuardPublicKey] = "wireguardPublicKey";
    return roles;
}
