#include "torConfigModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/torProtocolConfig.h"

using namespace amnezia;
using namespace ProtocolUtils;

TorConfigModel::TorConfigModel(QObject *parent) : QAbstractListModel(parent)
{
}

int TorConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool TorConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }

    QString strValue = value.toString();

    switch (role) {
    case Roles::SiteRole: m_protocolConfig.serverConfig.site = strValue; break;
    default:
        return false;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant TorConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return QVariant();
    }

    switch (role) {
    case Roles::SiteRole: return m_protocolConfig.serverConfig.site;
    }

    return QVariant();
}

void TorConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::TorProtocolConfig &protocolConfig)
{
    beginResetModel();
    m_container = container;
    m_protocolConfig = protocolConfig;
    m_originalProtocolConfig = m_protocolConfig;
    
    endResetModel();
}

amnezia::TorProtocolConfig TorConfigModel::getProtocolConfig()
{
    return m_protocolConfig;
}

QHash<int, QByteArray> TorConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[SiteRole] = "site";

    return roles;
}

