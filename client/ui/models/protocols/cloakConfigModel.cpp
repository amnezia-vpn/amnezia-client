#include "cloakConfigModel.h"

#include "protocols/protocols_defs.h"

CloakConfigModel::CloakConfigModel(QObject *parent) : QAbstractListModel(parent)
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
    case Roles::PortRole: m_protocolConfig.insert(config_key::port, value.toString()); break;
    case Roles::CipherRole: m_protocolConfig.insert(config_key::cipher, value.toString()); break;
    case Roles::SiteRole: m_protocolConfig.insert(config_key::site, value.toString()); break;
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
    case Roles::PortRole: return m_protocolConfig.value(config_key::port).toString(protocols::cloak::defaultPort);
    case Roles::CipherRole: return m_protocolConfig.value(config_key::cipher).toString(protocols::cloak::defaultCipher);
    case Roles::SiteRole: return m_protocolConfig.value(config_key::site).toString(protocols::cloak::defaultRedirSite);
    }

    return QVariant();
}

void CloakConfigModel::updateModel(const QJsonObject &config)
{
    beginResetModel();
    m_container = ContainerProps::containerFromString(config.value(config_key::container).toString());

    m_fullConfig = config;
    QJsonObject protocolConfig = config.value(config_key::cloak).toObject();

    m_protocolConfig.insert(config_key::cipher,
                            protocolConfig.value(config_key::cipher).toString(protocols::cloak::defaultCipher));

    m_protocolConfig.insert(config_key::port,
                            protocolConfig.value(config_key::port).toString(protocols::cloak::defaultPort));

    m_protocolConfig.insert(config_key::site,
                            protocolConfig.value(config_key::site).toString(protocols::cloak::defaultRedirSite));

    endResetModel();
}

QJsonObject CloakConfigModel::getConfig()
{
    m_fullConfig.insert(config_key::cloak, m_protocolConfig);
    return m_fullConfig;
}

QHash<int, QByteArray> CloakConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[PortRole] = "port";
    roles[CipherRole] = "cipher";
    roles[SiteRole] = "site";

    return roles;
}
