#include "socks5ProxyConfigModel.h"

#include "protocols/protocols_defs.h"

Socks5ProxyConfigModel::Socks5ProxyConfigModel(QObject *parent) : QAbstractListModel(parent)
{
}

int Socks5ProxyConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool Socks5ProxyConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerProps::allContainers().size()) {
        return false;
    }

    switch (role) {
    case Roles::PortRole: m_protocolConfig.insert(config_key::port, value.toString()); break;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant Socks5ProxyConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }

    switch (role) {
    case Roles::PortRole: return m_protocolConfig.value(config_key::port).toString();
    case Roles::UserNameRole:
        return m_protocolConfig.value(config_key::userName).toString(protocols::sftp::defaultUserName);
    case Roles::PasswordRole: return m_protocolConfig.value(config_key::password).toString();
    }

    return QVariant();
}

void Socks5ProxyConfigModel::updateModel(const QJsonObject &config)
{
    beginResetModel();
    m_container = ContainerProps::containerFromString(config.value(config_key::container).toString());

    m_fullConfig = config;
    QJsonObject protocolConfig = config.value(config_key::socks5proxy).toObject();

    m_protocolConfig.insert(config_key::userName,
                            protocolConfig.value(config_key::userName).toString());

    m_protocolConfig.insert(config_key::password, protocolConfig.value(config_key::password).toString());

    m_protocolConfig.insert(config_key::port, protocolConfig.value(config_key::port).toString());

    endResetModel();
}

QJsonObject Socks5ProxyConfigModel::getConfig()
{
    m_fullConfig.insert(config_key::socks5proxy, m_protocolConfig);
    return m_fullConfig;
}

QHash<int, QByteArray> Socks5ProxyConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[PortRole] = "port";
    roles[UserNameRole] = "username";
    roles[PasswordRole] = "password";

    return roles;
}
