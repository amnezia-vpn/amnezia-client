#include "protocols_model.h"

ProtocolsModel::ProtocolsModel(QObject *parent) :
    QAbstractListModel(parent)
{

}

int ProtocolsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ProtocolProps::allProtocols().size();
}

QHash<int, QByteArray> ProtocolsModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name_role";
    roles[DescRole] = "desc_role";
    roles[ServiceTypeRole] = "service_type_role";
    roles[IsInstalledRole] = "is_installed_role";
    return roles;
}

QVariant ProtocolsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
            || index.row() >= ProtocolProps::allProtocols().size()) {
        return QVariant();
    }

    Protocol p = ProtocolProps::allProtocols().at(index.row());
    if (role == NameRole) {
        return ProtocolProps::protocolHumanNames().value(p);
    }
    if (role == DescRole) {
        return ProtocolProps::protocolDescriptions().value(p);
    }
    if (role == ServiceTypeRole) {
        return ProtocolProps::protocolService(p);
    }
    if (role == IsInstalledRole) {
        return ContainerProps::protocolsForContainer(m_selectedDockerContainer).contains(p);
    }
    return QVariant();
}

void ProtocolsModel::setSelectedServerIndex(int index)
{
    beginResetModel();
    m_selectedServerIndex = index;
    endResetModel();
}

void ProtocolsModel::setSelectedDockerContainer(DockerContainer c)
{
    beginResetModel();
    m_selectedDockerContainer = c;
    endResetModel();
}


