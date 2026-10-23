#include "sftpConfigModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;

SftpConfigModel::SftpConfigModel(QObject *parent) : QAbstractListModel(parent)
{
}

int SftpConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool SftpConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
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

QVariant SftpConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return QVariant();
    }

    switch (role) {
    case Roles::PortRole: return m_protocolConfig.port;
    case Roles::UserNameRole:
        return m_protocolConfig.userName.isEmpty() ? QString(protocols::sftp::defaultUserName) : m_protocolConfig.userName;
    case Roles::PasswordRole: return m_protocolConfig.password;
    }

    return QVariant();
}

void SftpConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::SftpProtocolConfig &protocolConfig)
{
    beginResetModel();
    m_container = container;
    m_protocolConfig = protocolConfig;
    applyDefaults(m_protocolConfig);
    endResetModel();
}

amnezia::SftpProtocolConfig SftpConfigModel::getProtocolConfig()
{
    return m_protocolConfig;
}

void SftpConfigModel::applyDefaults(amnezia::SftpProtocolConfig& config)
{
    if (config.userName.isEmpty()) {
        config.userName = protocols::sftp::defaultUserName;
    }
}

QHash<int, QByteArray> SftpConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[PortRole] = "port";
    roles[UserNameRole] = "username";
    roles[PasswordRole] = "password";

    return roles;
}

