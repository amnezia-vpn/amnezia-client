#include "ikev2ConfigModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/protocols/ikev2ProtocolConfig.h"

using namespace amnezia;

Ikev2ConfigModel::Ikev2ConfigModel(QObject *parent) : QAbstractListModel(parent)
{
}

int Ikev2ConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool Ikev2ConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }

    QString strValue = value.toString();

    switch (role) {
    case Roles::PortRole: 
        break;
    case Roles::CipherRole: 
        break;
    default:
        return false;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant Ikev2ConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return QVariant();
    }

    switch (role) {
    case Roles::PortRole: 
        return QString("");
    case Roles::CipherRole: 
        return QString("");
    }

    return QVariant();
}

void Ikev2ConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::Ikev2ProtocolConfig &protocolConfig)
{
    beginResetModel();
    m_container = container;
    m_protocolConfig = protocolConfig;
    m_originalProtocolConfig = m_protocolConfig;
    
    endResetModel();
}

amnezia::Ikev2ProtocolConfig Ikev2ConfigModel::getProtocolConfig()
{
    return m_protocolConfig;
}

QHash<int, QByteArray> Ikev2ConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[PortRole] = "port";
    roles[CipherRole] = "cipher";

    return roles;
}
