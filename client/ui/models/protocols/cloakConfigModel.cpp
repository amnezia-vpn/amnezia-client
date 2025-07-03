#include "cloakConfigModel.h"

#include "protocols/protocols_defs.h"

CloakConfigModel::CloakConfigModel(QObject *parent)
    : QAbstractListModel(parent),
      m_newCloakProtocolConfig(QJsonObject(), ProtocolProps::protoToString(Proto::Cloak)),
      m_oldCloakProtocolConfig(QJsonObject(), ProtocolProps::protoToString(Proto::Cloak))
{
}

int CloakConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool CloakConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerProps::allContainers().size()) {
        return false;
    }

    switch (role) {
    case Roles::PortRole: m_newCloakProtocolConfig.serverProtocolConfig.port = value.toString(); break;
    case Roles::CipherRole: m_newCloakProtocolConfig.serverProtocolConfig.cipher = value.toString(); break;
    case Roles::SiteRole: m_newCloakProtocolConfig.serverProtocolConfig.site = value.toString(); break;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant CloakConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }

    switch (role) {
    case Roles::PortRole: return m_newCloakProtocolConfig.serverProtocolConfig.port;
    case Roles::CipherRole: return m_newCloakProtocolConfig.serverProtocolConfig.cipher;
    case Roles::SiteRole: return m_newCloakProtocolConfig.serverProtocolConfig.site;
    }

    return QVariant();
}

void CloakConfigModel::updateModel(const CloakProtocolConfig cloakProtocolConfig)
{
    beginResetModel();
    m_newCloakProtocolConfig = cloakProtocolConfig;
    m_oldCloakProtocolConfig = cloakProtocolConfig;
    endResetModel();
}

QSharedPointer<ProtocolConfig> CloakConfigModel::getConfig()
{
    if (m_oldCloakProtocolConfig.hasEqualServerSettings(m_newCloakProtocolConfig)) {
        m_newCloakProtocolConfig.clearClientSettings();
    }
    return QSharedPointer<CloakProtocolConfig>::create(m_newCloakProtocolConfig);
}

bool CloakConfigModel::isServerSettingsEqual()
{
    return m_oldCloakProtocolConfig.hasEqualServerSettings(m_newCloakProtocolConfig);
}

QHash<int, QByteArray> CloakConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[PortRole] = "port";
    roles[CipherRole] = "cipher";
    roles[SiteRole] = "site";

    return roles;
}

