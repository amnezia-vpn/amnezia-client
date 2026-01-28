#include "wireguardConfigModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;
using namespace ProtocolUtils;

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
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerUtils::allContainers().size()) {
        return false;
    }

    QString strValue = value.toString();

    switch (role) {
    case Roles::SubnetAddressRole: m_protocolConfig.serverConfig.subnetAddress = strValue; break;
    case Roles::PortRole: m_protocolConfig.serverConfig.port = strValue; break;
    case Roles::ClientMtuRole: {
        if (!m_protocolConfig.clientConfig.has_value()) {
            m_protocolConfig.clientConfig = amnezia::WireGuardClientConfig{};
        }
        m_protocolConfig.clientConfig->mtu = strValue;
        break;
    }
    default:
        return false;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant WireGuardConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return QVariant();
    }

    switch (role) {
    case Roles::SubnetAddressRole: return m_protocolConfig.serverConfig.subnetAddress;
    case Roles::PortRole: return m_protocolConfig.serverConfig.port;
    case Roles::ClientMtuRole: {
        if (m_protocolConfig.clientConfig.has_value()) {
            return m_protocolConfig.clientConfig->mtu;
        }
        return QString(protocols::wireguard::defaultMtu);
    }
    }

    return QVariant();
}

void WireGuardConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::WireGuardProtocolConfig &protocolConfig)
{
    beginResetModel();
    m_container = container;
    
    m_protocolConfig = protocolConfig;
    
    applyDefaultsToServerConfig(m_protocolConfig.serverConfig);
    
    if (!m_protocolConfig.clientConfig.has_value()) {
        m_protocolConfig.clientConfig = amnezia::WireGuardClientConfig{};
    }
    applyDefaultsToClientConfig(m_protocolConfig.clientConfig.value());
    
    m_originalProtocolConfig = m_protocolConfig;
    
    endResetModel();
}

void WireGuardConfigModel::applyDefaultsToServerConfig(amnezia::WireGuardServerConfig& config)
{
    if (config.subnetAddress.isEmpty()) {
        config.subnetAddress = protocols::wireguard::defaultSubnetAddress;
    }
    if (config.port.isEmpty()) {
        config.port = protocols::wireguard::defaultPort;
    }
    if (config.transportProto.isEmpty()) {
        config.transportProto = ProtocolUtils::transportProtoToString(
            ProtocolUtils::defaultTransportProto(amnezia::Proto::WireGuard), amnezia::Proto::WireGuard);
    }
}

void WireGuardConfigModel::applyDefaultsToClientConfig(amnezia::WireGuardClientConfig& config)
{
    if (config.mtu.isEmpty()) {
        config.mtu = protocols::wireguard::defaultMtu;
    }
}

amnezia::WireGuardProtocolConfig WireGuardConfigModel::getProtocolConfig()
{
    bool serverSettingsChanged = !m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig);
    
    if (serverSettingsChanged) {
        m_protocolConfig.clearClientConfig();
    }
    
    return m_protocolConfig;
}

bool WireGuardConfigModel::isServerSettingsEqual()
{
    return m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig);
}

QHash<int, QByteArray> WireGuardConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[SubnetAddressRole] = "subnetAddress";
    roles[PortRole] = "port";
    roles[ClientMtuRole] = "clientMtu";

    return roles;
}

