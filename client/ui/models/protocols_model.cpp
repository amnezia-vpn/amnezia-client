#include "protocols_model.h"

ProtocolsModel::ProtocolsModel(QObject *parent) :
    QAbstractListModel(parent)
{

}

int ProtocolsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return amnezia::allContainers().size();
}

QHash<int, QByteArray> ProtocolsModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name_role";
    roles[DescRole] = "desc_role";
    roles[isVpnTypeRole] = "is_vpn_role";
    roles[isOtherTypeRole] = "is_other_role";
    roles[isInstalledRole] = "is_installed_role";
    return roles;
}

QVariant ProtocolsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
            || index.row() >= amnezia::allContainers().size()) {
        return QVariant();
    }

    Protocol p = amnezia::allProtocols().at(index.row());
    if (role == NameRole) {
        return protocolHumanNames().value(p);
    }
    if (role == DescRole) {
        return protocolDescriptions().value(p);
    }
    if (role == isVpnTypeRole) {
        return isProtocolVpnType(p);
    }
//    if (role == isOtherTypeRole) {
//        return isContainerVpnType(c)
//    }
    if (role == isInstalledRole) {
        return protocolsForContainer(m_selectedDockerContainer).contains(p);
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


