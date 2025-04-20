#include "openvpnConfigModel.h"

#include "protocols/protocols_defs.h"

#include "ui/models/protocols/utils.h"

OpenVpnConfigModel::OpenVpnConfigModel(QObject *parent) : QAbstractListModel(parent)
{
}

int OpenVpnConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool OpenVpnConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerProps::allContainers().size()) {
        return false;
    }

    switch (role) {
    case Roles::SubnetAddressRole: m_protocolConfig.insert(amnezia::config_key::subnet_address, value.toString()); break;
    case Roles::TransportProtoRole: m_protocolConfig.insert(config_key::transport_proto, value.toString()); break;
    case Roles::PortRole: m_protocolConfig.insert(config_key::port, value.toString()); break;
    case Roles::AutoNegotiateEncryprionRole: m_protocolConfig.insert(config_key::ncp_disable, !value.toBool()); break;
    case Roles::HashRole: m_protocolConfig.insert(config_key::hash, value.toString()); break;
    case Roles::CipherRole: m_protocolConfig.insert(config_key::cipher, value.toString()); break;
    case Roles::TlsAuthRole: m_protocolConfig.insert(config_key::tls_auth, value.toBool()); break;
    case Roles::BlockDnsRole: m_protocolConfig.insert(config_key::block_outside_dns, value.toBool()); break;
    case Roles::AdditionalClientCommandsRole: m_protocolConfig.insert(config_key::additional_client_config, value.toString()); break;
    case Roles::AdditionalServerCommandsRole: m_protocolConfig.insert(config_key::additional_server_config, value.toString()); break;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant OpenVpnConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }

    switch (role) {
    case Roles::SubnetAddressRole:
        return m_protocolConfig.value(amnezia::config_key::subnet_address).toString(amnezia::protocols::openvpn::defaultSubnetAddress);
    case Roles::TransportProtoRole:
        return m_protocolConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto);
    case Roles::PortRole: return m_protocolConfig.value(config_key::port).toString(protocols::openvpn::defaultPort);
    case Roles::AutoNegotiateEncryprionRole:
        return !m_protocolConfig.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable);
    case Roles::HashRole: return m_protocolConfig.value(config_key::hash).toString(protocols::openvpn::defaultHash);
    case Roles::CipherRole: return m_protocolConfig.value(config_key::cipher).toString(protocols::openvpn::defaultCipher);
    case Roles::TlsAuthRole: return m_protocolConfig.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth);
    case Roles::BlockDnsRole:
        return m_protocolConfig.value(config_key::block_outside_dns).toBool(protocols::openvpn::defaultBlockOutsideDns);
    case Roles::AdditionalClientCommandsRole:
        return m_protocolConfig.value(config_key::additional_client_config).toString(protocols::openvpn::defaultAdditionalClientConfig);
    case Roles::AdditionalServerCommandsRole:
        return m_protocolConfig.value(config_key::additional_server_config).toString(protocols::openvpn::defaultAdditionalServerConfig);
    case Roles::IsPortEditable: return m_container == DockerContainer::OpenVpn ? true : false;
    case Roles::IsTransportProtoEditable: return m_container == DockerContainer::OpenVpn ? true : false;
    case Roles::HasRemoveButton: return m_container == DockerContainer::OpenVpn ? true : false;
    }
    return QVariant();
}

void OpenVpnConfigModel::updateModel(const QJsonObject &config)
{
    beginResetModel();
    m_container = ContainerProps::containerFromString(config.value(config_key::container).toString());

    m_fullConfig = config;
    QJsonObject protocolConfig = config.value(config_key::openvpn).toObject();

    updateConfig(protocolConfig, config_key::subnet_address, amnezia::protocols::openvpn::defaultSubnetAddress);

    QString transportProto;
    if (m_container == DockerContainer::OpenVpn) {
        transportProto = protocolConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto);
    } else {
        transportProto = "tcp";
    }

    const std::pair<const char *, std::variant<const char *, bool>> defaults[] = {
        { config_key::transport_proto, transportProto.toUtf8().constData() },
        { config_key::port, protocols::openvpn::defaultPort },
        { config_key::ncp_disable, protocols::openvpn::defaultNcpDisable },
        { config_key::cipher, protocols::openvpn::defaultCipher },
        { config_key::hash, protocols::openvpn::defaultHash },
        { config_key::block_outside_dns, protocols::openvpn::defaultBlockOutsideDns },
        { config_key::tls_auth, protocols::openvpn::defaultTlsAuth },
        { config_key::additional_client_config, protocols::openvpn::defaultAdditionalClientConfig },
    };

    for (const auto &[key, def] : defaults) {
        const auto configKey = key;
        const auto defaultValue = def;
        std::visit([&](auto &&defaultValue_) { updateConfig(protocolConfig, configKey, defaultValue_); }, defaultValue);
    }

    updateConfig(protocolConfig, config_key::transport_proto, transportProto.toUtf8().constData());
    updateConfig(protocolConfig, config_key::ncp_disable, protocols::openvpn::defaultNcpDisable);
    updateConfig(protocolConfig, config_key::cipher, protocols::openvpn::defaultCipher);
    updateConfig(protocolConfig, config_key::hash, protocols::openvpn::defaultHash);
    updateConfig(protocolConfig, config_key::block_outside_dns, protocols::openvpn::defaultBlockOutsideDns);
    updateConfig(protocolConfig, config_key::port, protocols::openvpn::defaultPort);
    updateConfig(protocolConfig, config_key::tls_auth, protocols::openvpn::defaultTlsAuth);
    updateConfig(protocolConfig, config_key::additional_client_config, protocols::openvpn::defaultAdditionalClientConfig);

    endResetModel();
}

QJsonObject OpenVpnConfigModel::getConfig()
{
    m_fullConfig.insert(config_key::openvpn, m_protocolConfig);
    return m_fullConfig;
}

QHash<int, QByteArray> OpenVpnConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[SubnetAddressRole] = "subnetAddress";
    roles[TransportProtoRole] = "transportProto";
    roles[PortRole] = "port";
    roles[AutoNegotiateEncryprionRole] = "autoNegotiateEncryprion";
    roles[HashRole] = "hash";
    roles[CipherRole] = "cipher";
    roles[TlsAuthRole] = "tlsAuth";
    roles[BlockDnsRole] = "blockDns";
    roles[AdditionalClientCommandsRole] = "additionalClientCommands";
    roles[AdditionalServerCommandsRole] = "additionalServerCommands";

    roles[IsPortEditable] = "isPortEditable";
    roles[IsTransportProtoEditable] = "isTransportProtoEditable";

    roles[HasRemoveButton] = "hasRemoveButton";

    return roles;
}
