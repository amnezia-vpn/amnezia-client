#include "wireguardConfigModel.h"

#include <QJsonDocument>

#include "protocols/protocols_defs.h"

WireGuardConfigModel::WireGuardConfigModel(QObject *parent)
    : QAbstractListModel(parent),
      m_newWireGuardProtocolConfig(QJsonObject(), ProtocolProps::protoToString(Proto::WireGuard)),
      m_oldWireGuardProtocolConfig(QJsonObject(), ProtocolProps::protoToString(Proto::WireGuard))
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
    case Roles::SubnetAddressRole: m_newWireGuardProtocolConfig.serverProtocolConfig.subnetAddress = value.toString(); break;
    case Roles::PortRole: m_newWireGuardProtocolConfig.serverProtocolConfig.port = value.toString(); break;
    case Roles::ClientMtuRole: m_newWireGuardProtocolConfig.clientProtocolConfig.wireGuardData.mtu = value.toString(); break;
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
    case Roles::SubnetAddressRole: return m_newWireGuardProtocolConfig.serverProtocolConfig.subnetAddress;
    case Roles::PortRole: return m_newWireGuardProtocolConfig.serverProtocolConfig.port;
    case Roles::ClientMtuRole: return m_newWireGuardProtocolConfig.clientProtocolConfig.wireGuardData.mtu;
    }

    return QVariant();
}

void WireGuardConfigModel::updateModel(const WireGuardProtocolConfig wireGuardProtocolConfig)
{
    beginResetModel();
    m_newWireGuardProtocolConfig = wireGuardProtocolConfig;
    m_oldWireGuardProtocolConfig = wireGuardProtocolConfig;
    endResetModel();
}

QSharedPointer<ProtocolConfig> WireGuardConfigModel::getConfig()
{
    if (m_oldWireGuardProtocolConfig.hasEqualServerSettings(m_newWireGuardProtocolConfig)) {
        m_newWireGuardProtocolConfig.clearClientSettings();
    }
    return QSharedPointer<WireGuardProtocolConfig>::create(m_newWireGuardProtocolConfig);
}

bool WireGuardConfigModel::isServerSettingsEqual()
{
    return m_oldWireGuardProtocolConfig.hasEqualServerSettings(m_newWireGuardProtocolConfig);
}

QHash<int, QByteArray> WireGuardConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[SubnetAddressRole] = "subnetAddress";
    roles[PortRole] = "port";
    roles[ClientMtuRole] = "clientMtu";

    return roles;
}
