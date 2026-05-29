#include "openvpnConfigModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;

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
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerUtils::allContainers().size()) {
        return false;
    }

    QString strValue = value.toString();
    bool boolValue = value.toBool();

    switch (role) {
    case Roles::SubnetAddressRole: m_protocolConfig.serverConfig.subnetAddress = strValue; break;
    case Roles::TransportProtoRole: m_protocolConfig.serverConfig.transportProto = strValue; break;
    case Roles::PortRole: m_protocolConfig.serverConfig.port = strValue; break;
    case Roles::AutoNegotiateEncryprionRole: m_protocolConfig.serverConfig.ncpDisable = !boolValue; break;
    case Roles::HashRole: m_protocolConfig.serverConfig.hash = strValue; break;
    case Roles::CipherRole: m_protocolConfig.serverConfig.cipher = strValue; break;
    case Roles::TlsAuthRole: m_protocolConfig.serverConfig.tlsAuth = boolValue; break;
    case Roles::BlockDnsRole: {
        if (!m_protocolConfig.clientConfig.has_value()) {
            m_protocolConfig.clientConfig = amnezia::OpenVpnClientConfig{};
        }
        m_protocolConfig.clientConfig->blockOutsideDns = boolValue;
        break;
    }
    case Roles::AdditionalClientCommandsRole: m_protocolConfig.serverConfig.additionalClientConfig = strValue; break;
    case Roles::AdditionalServerCommandsRole: m_protocolConfig.serverConfig.additionalServerConfig = strValue; break;
    default:
        return false;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant OpenVpnConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return QVariant();
    }

    switch (role) {
    case Roles::SubnetAddressRole: return m_protocolConfig.serverConfig.subnetAddress;
    case Roles::TransportProtoRole: return m_protocolConfig.serverConfig.transportProto;
    case Roles::PortRole: return m_protocolConfig.serverConfig.port;
    case Roles::AutoNegotiateEncryprionRole: return !m_protocolConfig.serverConfig.ncpDisable;
    case Roles::HashRole: return m_protocolConfig.serverConfig.hash;
    case Roles::CipherRole: return m_protocolConfig.serverConfig.cipher;
    case Roles::TlsAuthRole: return m_protocolConfig.serverConfig.tlsAuth;
    case Roles::BlockDnsRole: {
        if (m_protocolConfig.clientConfig.has_value()) {
            return m_protocolConfig.clientConfig->blockOutsideDns;
        }
        return protocols::openvpn::defaultBlockOutsideDns;
    }
    case Roles::AdditionalClientCommandsRole: return m_protocolConfig.serverConfig.additionalClientConfig;
    case Roles::AdditionalServerCommandsRole: return m_protocolConfig.serverConfig.additionalServerConfig;
    case Roles::IsPortEditable: return m_container == DockerContainer::OpenVpn ? true : false;
    case Roles::IsTransportProtoEditable: return m_container == DockerContainer::OpenVpn ? true : false;
    case Roles::HasRemoveButton: return m_container == DockerContainer::OpenVpn ? true : false;
    }
    return QVariant();
}

void OpenVpnConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::OpenVpnProtocolConfig &protocolConfig)
{
    beginResetModel();
    m_container = container;
    
    m_protocolConfig = protocolConfig;
    
    applyDefaultsToServerConfig(m_protocolConfig.serverConfig);
    
    if (!m_protocolConfig.clientConfig.has_value()) {
        m_protocolConfig.clientConfig = amnezia::OpenVpnClientConfig{};
    }
    applyDefaultsToClientConfig(m_protocolConfig.clientConfig.value());
    
    m_originalProtocolConfig = m_protocolConfig;
    
    endResetModel();
}

void OpenVpnConfigModel::applyDefaultsToServerConfig(amnezia::OpenVpnServerConfig& config)
{
    if (config.subnetAddress.isEmpty()) {
        config.subnetAddress = protocols::openvpn::defaultSubnetAddress;
    }
    if (config.port.isEmpty()) {
        config.port = protocols::openvpn::defaultPort;
    }
    if (config.transportProto.isEmpty()) {
        if (m_container == DockerContainer::OpenVpn) {
            config.transportProto = protocols::openvpn::defaultTransportProto;
        } else {
            config.transportProto = "tcp";
        }
    }
    if (config.cipher.isEmpty()) {
        config.cipher = protocols::openvpn::defaultCipher;
    }
    if (config.hash.isEmpty()) {
        config.hash = protocols::openvpn::defaultHash;
    }
}

void OpenVpnConfigModel::applyDefaultsToClientConfig(amnezia::OpenVpnClientConfig& config)
{
    if (!config.blockOutsideDns && !m_protocolConfig.serverConfig.additionalClientConfig.isEmpty()) {
        config.blockOutsideDns = protocols::openvpn::defaultBlockOutsideDns;
    }
}

amnezia::OpenVpnProtocolConfig OpenVpnConfigModel::getProtocolConfig()
{
    bool serverSettingsChanged = !m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig);
    
    if (serverSettingsChanged) {
        m_protocolConfig.clearClientConfig();
    }
    
    return m_protocolConfig;
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
