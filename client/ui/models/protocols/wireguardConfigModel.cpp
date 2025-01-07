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
    case Roles::SubnetAddressRole: m_serverProtocolConfig.insert(config_key::subnet_address, value.toString()); break;
    case Roles::PortRole: m_serverProtocolConfig.insert(config_key::port, value.toString()); break;
    case Roles::ClientMtuRole: m_clientProtocolConfig.insert(config_key::mtu, value.toString()); break;
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
    case Roles::SubnetAddressRole: return m_serverProtocolConfig.value(config_key::subnet_address).toString();
    case Roles::PortRole: return m_serverProtocolConfig.value(config_key::port).toString();
    case Roles::ClientMtuRole: return m_clientProtocolConfig.value(config_key::mtu);
    }

    return QVariant();
}

void WireGuardConfigModel::updateModel(const QJsonObject &config)
{
    beginResetModel();
    m_container = ContainerProps::containerFromString(config.value(config_key::container).toString());

    m_fullConfig = config;
    QJsonObject serverProtocolConfig = config.value(config_key::wireguard).toObject();

    auto defaultTransportProto =
            ProtocolProps::transportProtoToString(ProtocolProps::defaultTransportProto(Proto::WireGuard), Proto::WireGuard);
    m_serverProtocolConfig.insert(config_key::transport_proto,
                                  serverProtocolConfig.value(config_key::transport_proto).toString(defaultTransportProto));
    m_serverProtocolConfig[config_key::last_config] = serverProtocolConfig.value(config_key::last_config);
    m_serverProtocolConfig[config_key::subnet_address] = serverProtocolConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress);
    m_serverProtocolConfig[config_key::port] = serverProtocolConfig.value(config_key::port).toString(protocols::wireguard::defaultPort);

    auto lastConfig = m_serverProtocolConfig.value(config_key::last_config).toString();
    QJsonObject clientProtocolConfig = QJsonDocument::fromJson(lastConfig.toUtf8()).object();
    m_clientProtocolConfig[config_key::mtu] = clientProtocolConfig[config_key::mtu].toString(protocols::wireguard::defaultMtu);

    endResetModel();
}

QJsonObject WireGuardConfigModel::getConfig()
{
    const WgConfig oldConfig(m_fullConfig.value(config_key::wireguard).toObject());
    const WgConfig newConfig(m_serverProtocolConfig);

    if (!oldConfig.hasEqualServerSettings(newConfig)) {
        m_serverProtocolConfig.remove(config_key::last_config);
    } else {
        auto lastConfig = m_serverProtocolConfig.value(config_key::last_config).toString();
        QJsonObject jsonConfig = QJsonDocument::fromJson(lastConfig.toUtf8()).object();
        jsonConfig[config_key::mtu] = m_clientProtocolConfig[config_key::mtu];

        m_serverProtocolConfig[config_key::last_config] = QString(QJsonDocument(jsonConfig).toJson());
    }

    m_fullConfig.insert(config_key::wireguard, m_serverProtocolConfig);
    return m_fullConfig;
}

bool WireGuardConfigModel::isServerSettingsEqual()
{
    const WgConfig oldConfig(m_fullConfig.value(config_key::wireguard).toObject());
    const WgConfig newConfig(m_serverProtocolConfig);

    return oldConfig.hasEqualServerSettings(newConfig);
}

QHash<int, QByteArray> WireGuardConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[SubnetAddressRole] = "subnetAddress";
    roles[PortRole] = "port";
    roles[ClientMtuRole] = "clientMtu";

    return roles;
}

WgConfig::WgConfig(const QJsonObject &serverProtocolConfig)
{
    auto lastConfig = serverProtocolConfig.value(config_key::last_config).toString();
    QJsonObject clientProtocolConfig = QJsonDocument::fromJson(lastConfig.toUtf8()).object();
    clientMtu = clientProtocolConfig[config_key::mtu].toString(protocols::wireguard::defaultMtu);

    subnetAddress = serverProtocolConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress);
    port = serverProtocolConfig.value(config_key::port).toString(protocols::wireguard::defaultPort);
}

bool WgConfig::hasEqualServerSettings(const WgConfig &other) const
{
    if (subnetAddress != other.subnetAddress || port != other.port) {
        return false;
    }
    return true;
}

bool WgConfig::hasEqualClientSettings(const WgConfig &other) const
{
    if (clientMtu != other.clientMtu) {
        return false;
    }
    return true;
}
