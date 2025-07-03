#include "openvpnConfigModel.h"

#include "protocols/protocols_defs.h"

OpenVpnConfigModel::OpenVpnConfigModel(QObject *parent)
    : QAbstractListModel(parent),
      m_newOpenVpnProtocolConfig(QJsonObject(), ProtocolProps::protoToString(Proto::OpenVpn)),
      m_oldOpenVpnProtocolConfig(QJsonObject(), ProtocolProps::protoToString(Proto::OpenVpn))
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
    case Roles::SubnetAddressRole: m_newOpenVpnProtocolConfig.serverProtocolConfig.subnetAddress = value.toString(); break;
    case Roles::TransportProtoRole: m_newOpenVpnProtocolConfig.serverProtocolConfig.transportProto = value.toString(); break;
    case Roles::PortRole: m_newOpenVpnProtocolConfig.serverProtocolConfig.port = value.toString(); break;
    case Roles::AutoNegotiateEncryprionRole: m_newOpenVpnProtocolConfig.serverProtocolConfig.ncpDisable = !value.toBool(); break;
    case Roles::HashRole: m_newOpenVpnProtocolConfig.serverProtocolConfig.hash = value.toString(); break;
    case Roles::CipherRole: m_newOpenVpnProtocolConfig.serverProtocolConfig.cipher = value.toString(); break;
    case Roles::TlsAuthRole: m_newOpenVpnProtocolConfig.serverProtocolConfig.tlsAuth = value.toBool(); break;
    case Roles::BlockDnsRole: m_newOpenVpnProtocolConfig.serverProtocolConfig.blockOutsideDns = value.toBool(); break;
    case Roles::AdditionalClientCommandsRole: m_newOpenVpnProtocolConfig.serverProtocolConfig.additionalClientConfig = value.toString(); break;
    case Roles::AdditionalServerCommandsRole: m_newOpenVpnProtocolConfig.serverProtocolConfig.additionalServerConfig = value.toString(); break;
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
    case Roles::SubnetAddressRole: return m_newOpenVpnProtocolConfig.serverProtocolConfig.subnetAddress;
    case Roles::TransportProtoRole: return m_newOpenVpnProtocolConfig.serverProtocolConfig.transportProto;
    case Roles::PortRole: return m_newOpenVpnProtocolConfig.serverProtocolConfig.port;
    case Roles::AutoNegotiateEncryprionRole: return !m_newOpenVpnProtocolConfig.serverProtocolConfig.ncpDisable;
    case Roles::HashRole: return m_newOpenVpnProtocolConfig.serverProtocolConfig.hash;
    case Roles::CipherRole: return m_newOpenVpnProtocolConfig.serverProtocolConfig.cipher;
    case Roles::TlsAuthRole: return m_newOpenVpnProtocolConfig.serverProtocolConfig.tlsAuth;
    case Roles::BlockDnsRole: return m_newOpenVpnProtocolConfig.serverProtocolConfig.blockOutsideDns;
    case Roles::AdditionalClientCommandsRole: return m_newOpenVpnProtocolConfig.serverProtocolConfig.additionalClientConfig;
    case Roles::AdditionalServerCommandsRole: return m_newOpenVpnProtocolConfig.serverProtocolConfig.additionalServerConfig;
    case Roles::IsPortEditable: return true; // TODO: implement container check if needed
    case Roles::IsTransportProtoEditable: return true; // TODO: implement container check if needed
    case Roles::HasRemoveButton: return true; // TODO: implement container check if needed
    }
    return QVariant();
}

void OpenVpnConfigModel::updateModel(const OpenVpnProtocolConfig openVpnProtocolConfig)
{
    beginResetModel();
    m_newOpenVpnProtocolConfig = openVpnProtocolConfig;
    m_oldOpenVpnProtocolConfig = openVpnProtocolConfig;
    endResetModel();
}

QSharedPointer<ProtocolConfig> OpenVpnConfigModel::getConfig()
{
    if (m_oldOpenVpnProtocolConfig.hasEqualServerSettings(m_newOpenVpnProtocolConfig)) {
        m_newOpenVpnProtocolConfig.clearClientSettings();
    }
    return QSharedPointer<OpenVpnProtocolConfig>::create(m_newOpenVpnProtocolConfig);
}

bool OpenVpnConfigModel::isServerSettingsEqual()
{
    return m_oldOpenVpnProtocolConfig.hasEqualServerSettings(m_newOpenVpnProtocolConfig);
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
