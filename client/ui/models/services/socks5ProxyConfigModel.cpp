#include "socks5ProxyConfigModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;

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
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }

    QString strValue = value.toString();

    switch (role) {
    case Roles::PortRole: m_protocolConfig.port = strValue; break;
    case Roles::UserNameRole: m_protocolConfig.userName = strValue; break;
    case Roles::PasswordRole: m_protocolConfig.password = strValue; break;
    default:
        return false;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant Socks5ProxyConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return QVariant();
    }

    switch (role) {
    case Roles::PortRole: return m_protocolConfig.port;
    case Roles::UserNameRole: return m_protocolConfig.userName;
    case Roles::PasswordRole: return m_protocolConfig.password;
    }

    return QVariant();
}

void Socks5ProxyConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::Socks5ProxyProtocolConfig &protocolConfig)
{
    beginResetModel();
    m_container = container;
    m_protocolConfig = protocolConfig;
    endResetModel();
}

amnezia::Socks5ProxyProtocolConfig Socks5ProxyConfigModel::getProtocolConfig()
{
    return m_protocolConfig;
}

QHash<int, QByteArray> Socks5ProxyConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[PortRole] = "port";
    roles[UserNameRole] = "username";
    roles[PasswordRole] = "password";

    return roles;
}
