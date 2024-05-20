#include "shadowsocksConfigModel.h"

#include "protocols/protocols_defs.h"

ShadowSocksConfigModel::ShadowSocksConfigModel(QObject *parent) : QAbstractListModel(parent)
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
    case Roles::PortRole: m_protocolConfig.insert(config_key::port, value.toString()); break;
    case Roles::CipherRole: m_protocolConfig.insert(config_key::cipher, value.toString()); break;
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
    case Roles::PortRole: return m_protocolConfig.value(config_key::port).toString(protocols::shadowsocks::defaultPort);
    case Roles::CipherRole:
        return m_protocolConfig.value(config_key::cipher).toString(protocols::shadowsocks::defaultCipher);
    case Roles::IsPortEditableRole: return m_container == DockerContainer::ShadowSocks ? true : false;
    case Roles::IsCipherEditableRole: return m_container == DockerContainer::ShadowSocks ? true : false;
    }

    return QVariant();
}

void ShadowSocksConfigModel::updateModel(const QJsonObject &config)
{
    beginResetModel();
    m_container = ContainerProps::containerFromString(config.value(config_key::container).toString());

    m_fullConfig = config;
    QJsonObject protocolConfig = config.value(config_key::shadowsocks).toObject();

    m_protocolConfig.insert(config_key::cipher,
                            protocolConfig.value(config_key::cipher).toString(protocols::shadowsocks::defaultCipher));

    m_protocolConfig.insert(config_key::port,
                            protocolConfig.value(config_key::port).toString(protocols::shadowsocks::defaultPort));

    endResetModel();
}

QJsonObject ShadowSocksConfigModel::getConfig()
{
    m_fullConfig.insert(config_key::shadowsocks, m_protocolConfig);
    return m_fullConfig;
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
