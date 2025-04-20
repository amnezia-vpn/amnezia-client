#include "xrayConfigModel.h"

#include "protocols/protocols_defs.h"

#include "ui/models/protocols/utils.h"

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

    if (protocolConfig.contains(config_key::transport_proto)) {
        auto transportProto = protocolConfig.value(config_key::transport_proto)
                                      .toString(ProtocolProps::transportProtoToString(
                                              ProtocolProps::defaultTransportProto(Proto::Xray), Proto::Xray));
        m_protocolConfig[config_key::transport_proto] = transportProto;
    }

    const std::pair<const char *, const char *> defaults[] = {
            { config_key::port, protocols::xray::defaultPort },
            { config_key::site, protocols::xray::defaultSite },
    };

    for (const auto &[key, def] : defaults)
        updateConfig(protocolConfig, key, def);

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
