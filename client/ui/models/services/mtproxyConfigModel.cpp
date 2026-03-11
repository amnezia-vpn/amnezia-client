#include "mtproxyConfigModel.h"
#include "protocols/protocols_defs.h"

MtproxyConfigModel::MtproxyConfigModel(QObject *parent) : QAbstractListModel(parent)
{
}

int MtproxyConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool MtproxyConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }
    switch (role) {
    case Roles::PortRole:   m_protocolConfig.insert(config_key::port, value.toString()); break;
    case Roles::SecretRole: m_protocolConfig.insert(config_key::secret, value.toString()); break;
    case Roles::TagRole:    m_protocolConfig.insert(config_key::tag, value.toString()); break;
    }
    emit dataChanged(index, index, QList{role});
    return true;
}

QVariant MtproxyConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }
    switch (role) {
    case Roles::PortRole:   return m_protocolConfig.value(config_key::port).toString();
    case Roles::SecretRole: return m_protocolConfig.value(config_key::secret).toString();
    case Roles::TagRole:    return m_protocolConfig.value(config_key::tag).toString();
    }
    return QVariant();
}

void MtproxyConfigModel::updateModel(const QJsonObject &config)
{
    beginResetModel();
    m_container = ContainerProps::containerFromString(config.value(config_key::container).toString());
    m_fullConfig = config;
    const QJsonObject protocolConfig = config.value(config_key::mtproxy).toObject();
    m_protocolConfig.insert(config_key::port,   protocolConfig.value(config_key::port).toString());
    m_protocolConfig.insert(config_key::secret, protocolConfig.value(config_key::secret).toString());
    m_protocolConfig.insert(config_key::tag,    protocolConfig.value(config_key::tag).toString());
    endResetModel();
}

QJsonObject MtproxyConfigModel::getConfig()
{
    m_fullConfig.insert(config_key::mtproxy, m_protocolConfig);
    return m_fullConfig;
}

QHash<int, QByteArray> MtproxyConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[PortRole]   = "port";
    roles[SecretRole] = "secret";
    roles[TagRole]    = "tag";
    return roles;
}
