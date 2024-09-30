#include "wireguardConfigModel.h"

#include <QJsonDocument>

#include "protocols/protocols_defs.h"

WireGuardConfigModel::WireGuardConfigModel(QObject *parent) : QAbstractListModel(parent)
{
}

int WireGuardConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool WireGuardConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerProps::allContainers().size()) {
        return false;
    }

    switch (role) {
        case Roles::PortRole: m_protocolConfig.insert(config_key::port, value.toString()); break;
        case Roles::MtuRole: m_protocolConfig.insert(config_key::mtu, value.toString()); break;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant WireGuardConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }

    switch (role) {
        case Roles::PortRole: return m_protocolConfig.value(config_key::port).toString();
        case Roles::MtuRole: return m_protocolConfig.value(config_key::mtu).toString();
    }

    return QVariant();
}

void WireGuardConfigModel::updateModel(const QJsonObject &config)
{
    beginResetModel();
    m_container = ContainerProps::containerFromString(config.value(config_key::container).toString());

    m_fullConfig = config;
    QJsonObject protocolConfig = config.value(config_key::wireguard).toObject();

    auto defaultTransportProto = ProtocolProps::transportProtoToString(ProtocolProps::defaultTransportProto(Proto::WireGuard), Proto::WireGuard);
    m_protocolConfig.insert(config_key::transport_proto,
                            protocolConfig.value(config_key::transport_proto).toString(defaultTransportProto));
    m_protocolConfig[config_key::last_config] = protocolConfig.value(config_key::last_config);
    m_protocolConfig[config_key::port] =
        protocolConfig.value(config_key::port).toString(protocols::wireguard::defaultPort);

    m_protocolConfig[config_key::mtu] =
        protocolConfig.value(config_key::mtu).toString(protocols::wireguard::defaultMtu);

    endResetModel();
}

QJsonObject WireGuardConfigModel::getConfig()
{
    const WgConfig oldConfig(m_fullConfig.value(config_key::wireguard).toObject());
    const WgConfig newConfig(m_protocolConfig);

    if (!oldConfig.hasEqualServerSettings(newConfig)) {
        m_protocolConfig.remove(config_key::last_config);
    } else {
        auto lastConfig = m_protocolConfig.value(config_key::last_config).toString();
        QJsonObject jsonConfig = QJsonDocument::fromJson(lastConfig.toUtf8()).object();
        jsonConfig[config_key::mtu] = newConfig.mtu;

        m_protocolConfig[config_key::last_config] = QString(QJsonDocument(jsonConfig).toJson());
    }

    m_fullConfig.insert(config_key::wireguard, m_protocolConfig);
    return m_fullConfig;
}

QHash<int, QByteArray> WireGuardConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[PortRole] = "port";
    roles[MtuRole] = "mtu";

    return roles;
}

WgConfig::WgConfig(const QJsonObject &jsonConfig)
{
    port = jsonConfig.value(config_key::port).toString(protocols::wireguard::defaultPort);
    mtu = jsonConfig.value(config_key::mtu).toString(protocols::wireguard::defaultMtu);
}

bool WgConfig::hasEqualServerSettings(const WgConfig &other) const
{
    if (port != other.port) {
        return false;
    }
    return true;
}

bool WgConfig::hasEqualClientSettings(const WgConfig &other) const
{
    if (mtu != other.mtu) {
        return false;
    }
    return true;
}
