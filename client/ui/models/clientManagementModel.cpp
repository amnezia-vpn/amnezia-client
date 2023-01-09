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

void ClientManagementModel::setContent(const QVector<ClientInfo> &data)
{
    beginResetModel();
    m_content = data;
    endResetModel();
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
        return m_content[index.row()].name;
    }
    if (role == CertIdRole) {
        return m_content[index.row()].certId;
    }
    if (role == CertDataRole) {
        return m_content[index.row()].certData;
    }
    return QVariant();
}

void ClientManagementModel::setData(const QModelIndex &index, QVariant data, int role)
{
    if (!index.isValid() || index.row() < 0
            || index.row() >= static_cast<int>(m_content.size())) {
        return;
    }

    if (role == NameRole) {
         m_content[index.row()].name = data.toString();
    }
    if (role == CertIdRole) {
        m_content[index.row()].certId = data.toString();
    }
    if (role == CertDataRole) {
        m_content[index.row()].certData = data.toString();
    }
    emit dataChanged(index, index);
}

QHash<int, QByteArray> ClientManagementModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "clientName";
    roles[CertIdRole] = "certId";
    roles[CertDataRole] = "certData";
    return roles;
}
