#include "xrayConfigModel.h"

#include "protocols/protocols_defs.h"

XrayConfigModel::XrayConfigModel(QObject *parent) : QAbstractListModel(parent)
{
}

int XrayConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool XrayConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerProps::allContainers().size()) {
        return false;
    }

    switch (role) {
    case Roles::SiteRole: m_protocolConfig.insert(config_key::site, value.toString()); break;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant XrayConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }

    switch (role) {
    case Roles::SiteRole: return m_protocolConfig.value(config_key::site).toString(protocols::xray::defaultSite);
    }

    return QVariant();
}

void XrayConfigModel::updateModel(const QJsonObject &config)
{
    beginResetModel();
    m_container = ContainerProps::containerFromString(config.value(config_key::container).toString());

    m_fullConfig = config;
    QJsonObject protocolConfig = config.value(config_key::xray).toObject();

    m_protocolConfig.insert(config_key::site,
                            protocolConfig.value(config_key::site).toString(protocols::xray::defaultSite));

    endResetModel();
}

QJsonObject XrayConfigModel::getConfig()
{
    m_fullConfig.insert(config_key::xray, m_protocolConfig);
    return m_fullConfig;
}

QHash<int, QByteArray> XrayConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[SiteRole] = "site";

    return roles;
}
