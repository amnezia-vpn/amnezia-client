#include "mtProxyConfigModel.h"

#include "protocols/protocols_defs.h"

using namespace amnezia;

MtProxyConfigModel::MtProxyConfigModel(QObject *parent) : QAbstractListModel(parent)
{
}

int MtProxyConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool MtProxyConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() != 0) {
        return false;
    }

    switch (role) {
    case Roles::PortRole: m_protocolConfig.insert(config_key::port, value.toString()); break;
    case Roles::SecretRole: m_protocolConfig.insert(protocols::mtProxy::mtproxySecret, value.toString()); break;
    case Roles::TagRole: m_protocolConfig.insert(protocols::mtProxy::mtproxyTag, value.toString()); break;
    default: return false;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant MtProxyConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() != 0) {
        return QVariant();
    }

    switch (role) {
    case Roles::PortRole: return m_protocolConfig.value(config_key::port).toString(protocols::mtProxy::defaultPort);
    case Roles::SecretRole: return m_protocolConfig.value(protocols::mtProxy::mtproxySecret).toString();
    case Roles::TagRole: return m_protocolConfig.value(protocols::mtProxy::mtproxyTag).toString();
    case Roles::TgLinkRole: return m_protocolConfig.value(protocols::mtProxy::mtproxyTgLink).toString();
    case Roles::TmeLinkRole: return m_protocolConfig.value(protocols::mtProxy::mtproxyTmeLink).toString();
    }

    return QVariant();
}

void MtProxyConfigModel::updateModel(const QJsonObject &config)
{
    beginResetModel();

    m_fullConfig = config;
    QJsonObject protocolConfig = config.value(config_key::mtproxy).toObject();

    m_protocolConfig.insert(config_key::port,
                            protocolConfig.value(config_key::port).toString(protocols::mtProxy::defaultPort));
    m_protocolConfig.insert(protocols::mtProxy::mtproxySecret,
                            protocolConfig.value(protocols::mtProxy::mtproxySecret).toString());
    m_protocolConfig.insert(protocols::mtProxy::mtproxyTag,
                            protocolConfig.value(protocols::mtProxy::mtproxyTag).toString());
    m_protocolConfig.insert(protocols::mtProxy::mtproxyTgLink,
                            protocolConfig.value(protocols::mtProxy::mtproxyTgLink).toString());
    m_protocolConfig.insert(protocols::mtProxy::mtproxyTmeLink,
                            protocolConfig.value(protocols::mtProxy::mtproxyTmeLink).toString());

    endResetModel();
}

QJsonObject MtProxyConfigModel::getConfig()
{
    m_fullConfig.insert(config_key::mtproxy, m_protocolConfig);
    return m_fullConfig;
}

QHash<int, QByteArray> MtProxyConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[PortRole] = "port";
    roles[SecretRole] = "secret";
    roles[TagRole] = "tag";
    roles[TgLinkRole] = "tgLink";
    roles[TmeLinkRole] = "tmeLink";

    return roles;
}
