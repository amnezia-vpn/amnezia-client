#include "ikev2ConfigModel.h"

#include "protocols/protocols_defs.h"

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

QVariant Ikev2ConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }

    switch (role) {
    case Roles::PortRole: return m_protocolConfig.value(config_key::port).toString(protocols::shadowsocks::defaultPort);
    case Roles::CipherRole: return m_protocolConfig.value(config_key::cipher).toString(protocols::shadowsocks::defaultCipher);
    }

    return QVariant();
}

void Ikev2ConfigModel::updateModel(const QJsonObject &config)
{
    beginResetModel();
    m_container = ContainerProps::containerFromString(config.value(config_key::container).toString());

    m_fullConfig = config;
    QJsonObject protocolConfig = config.value(config_key::shadowsocks).toObject();

    m_protocolConfig.insert(config_key::cipher, protocolConfig.value(config_key::cipher).toString(protocols::shadowsocks::defaultCipher));
    m_protocolConfig.insert(config_key::port, protocolConfig.value(config_key::port).toString(protocols::shadowsocks::defaultPort));

    endResetModel();
}

QJsonObject Ikev2ConfigModel::getConfig()
{
    m_fullConfig.insert(config_key::shadowsocks, m_protocolConfig);
    return m_fullConfig;
}

QHash<int, QByteArray> Ikev2ConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[PortRole] = "port";
    roles[CipherRole] = "cipher";

    return roles;
}
