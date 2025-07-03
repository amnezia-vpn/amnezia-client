#include "shadowsocksConfigModel.h"

#include "protocols/protocols_defs.h"

ShadowSocksConfigModel::ShadowSocksConfigModel(QObject *parent)
    : QAbstractListModel(parent),
      m_newShadowsocksProtocolConfig(QJsonObject(), ProtocolProps::protoToString(Proto::ShadowSocks)),
      m_oldShadowsocksProtocolConfig(QJsonObject(), ProtocolProps::protoToString(Proto::ShadowSocks))
{
}

int ShadowSocksConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool ShadowSocksConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerProps::allContainers().size()) {
        return false;
    }

    switch (role) {
    case Roles::PortRole: m_newShadowsocksProtocolConfig.serverProtocolConfig.port = value.toString(); break;
    case Roles::CipherRole: m_newShadowsocksProtocolConfig.serverProtocolConfig.cipher = value.toString(); break;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant ShadowSocksConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }

    switch (role) {
    case Roles::PortRole: return m_newShadowsocksProtocolConfig.serverProtocolConfig.port;
    case Roles::CipherRole: return m_newShadowsocksProtocolConfig.serverProtocolConfig.cipher;
    case Roles::IsPortEditableRole: return true; // TODO: implement container check if needed
    case Roles::IsCipherEditableRole: return true; // TODO: implement container check if needed
    }

    return QVariant();
}

void ShadowSocksConfigModel::updateModel(const ShadowsocksProtocolConfig shadowsocksProtocolConfig)
{
    beginResetModel();
    m_newShadowsocksProtocolConfig = shadowsocksProtocolConfig;
    m_oldShadowsocksProtocolConfig = shadowsocksProtocolConfig;
    endResetModel();
}

QSharedPointer<ProtocolConfig> ShadowSocksConfigModel::getConfig()
{
    if (m_oldShadowsocksProtocolConfig.hasEqualServerSettings(m_newShadowsocksProtocolConfig)) {
        m_newShadowsocksProtocolConfig.clearClientSettings();
    }
    return QSharedPointer<ShadowsocksProtocolConfig>::create(m_newShadowsocksProtocolConfig);
}

bool ShadowSocksConfigModel::isServerSettingsEqual()
{
    return m_oldShadowsocksProtocolConfig.hasEqualServerSettings(m_newShadowsocksProtocolConfig);
}

QHash<int, QByteArray> ShadowSocksConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[PortRole] = "port";
    roles[CipherRole] = "cipher";
    roles[IsPortEditableRole] = "isPortEditable";
    roles[IsCipherEditableRole] = "isCipherEditable";

    return roles;
}

