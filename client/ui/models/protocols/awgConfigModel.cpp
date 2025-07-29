#include "awgConfigModel.h"

#include <QJsonDocument>

#include "protocols/protocols_defs.h"

AwgConfigModel::AwgConfigModel(QObject *parent)
    : QAbstractListModel(parent),
      m_newAwgProtocolConfig(ProtocolProps::protoToString(Proto::Awg)),
      m_oldAwgProtocolConfig(ProtocolProps::protoToString(Proto::Awg))
{
}

int AwgConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool AwgConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerProps::allContainers().size()) {
        return false;
    }

    switch (role) {
    case Roles::SubnetAddressRole: m_newAwgProtocolConfig.serverProtocolConfig.subnetAddress = value.toString(); break;
    case Roles::PortRole: m_newAwgProtocolConfig.serverProtocolConfig.port = value.toString(); break;

    case Roles::ClientMtuRole: m_newAwgProtocolConfig.clientProtocolConfig.wireGuardData.mtu = value.toString(); break;
    case Roles::ClientJunkPacketCountRole: m_newAwgProtocolConfig.clientProtocolConfig.awgData.junkPacketCount = value.toString(); break;
    case Roles::ClientJunkPacketMinSizeRole:
        m_newAwgProtocolConfig.clientProtocolConfig.awgData.junkPacketMinSize = value.toString();
        break;
    case Roles::ClientJunkPacketMaxSizeRole:
        m_newAwgProtocolConfig.clientProtocolConfig.awgData.junkPacketMaxSize = value.toString();
        break;
    case Roles::ClientSpecialJunk1Role: m_newAwgProtocolConfig.clientProtocolConfig.awgData.specialJunk1 = value.toString(); break;
    case Roles::ClientSpecialJunk2Role: m_newAwgProtocolConfig.clientProtocolConfig.awgData.specialJunk2 = value.toString(); break;
    case Roles::ClientSpecialJunk3Role: m_newAwgProtocolConfig.clientProtocolConfig.awgData.specialJunk3 = value.toString(); break;
    case Roles::ClientSpecialJunk4Role: m_newAwgProtocolConfig.clientProtocolConfig.awgData.specialJunk4 = value.toString(); break;
    case Roles::ClientSpecialJunk5Role: m_newAwgProtocolConfig.clientProtocolConfig.awgData.specialJunk5 = value.toString(); break;
    case Roles::ClientControlledJunk1Role: m_newAwgProtocolConfig.clientProtocolConfig.awgData.controlledJunk1 = value.toString(); break;
    case Roles::ClientControlledJunk2Role: m_newAwgProtocolConfig.clientProtocolConfig.awgData.controlledJunk2 = value.toString(); break;
    case Roles::ClientControlledJunk3Role: m_newAwgProtocolConfig.clientProtocolConfig.awgData.controlledJunk3 = value.toString(); break;
    case Roles::ClientSpecialHandshakeTimeoutRole:
        m_newAwgProtocolConfig.clientProtocolConfig.awgData.specialHandshakeTimeout = value.toString();
        break;

    case Roles::ServerJunkPacketCountRole: m_newAwgProtocolConfig.serverProtocolConfig.awgData.junkPacketCount = value.toString(); break;
    case Roles::ServerJunkPacketMinSizeRole:
        m_newAwgProtocolConfig.serverProtocolConfig.awgData.junkPacketMinSize = value.toString();
        break;
    case Roles::ServerJunkPacketMaxSizeRole:
        m_newAwgProtocolConfig.serverProtocolConfig.awgData.junkPacketMaxSize = value.toString();
        break;
    case Roles::ServerInitPacketJunkSizeRole:
        m_newAwgProtocolConfig.serverProtocolConfig.awgData.initPacketJunkSize = value.toString();
        break;
    case Roles::ServerResponsePacketJunkSizeRole:
        m_newAwgProtocolConfig.serverProtocolConfig.awgData.responsePacketJunkSize = value.toString();
        break;
    case Roles::ServerInitPacketMagicHeaderRole:
        m_newAwgProtocolConfig.serverProtocolConfig.awgData.initPacketMagicHeader = value.toString();
        break;
    case Roles::ServerResponsePacketMagicHeaderRole:
        m_newAwgProtocolConfig.serverProtocolConfig.awgData.responsePacketMagicHeader = value.toString();
        break;
    case Roles::ServerUnderloadPacketMagicHeaderRole:
        m_newAwgProtocolConfig.serverProtocolConfig.awgData.underloadPacketMagicHeader = value.toString();
        break;
    case Roles::ServerTransportPacketMagicHeaderRole:
        m_newAwgProtocolConfig.serverProtocolConfig.awgData.transportPacketMagicHeader = value.toString();
        break;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant AwgConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }

    switch (role) {
    case Roles::SubnetAddressRole: return m_newAwgProtocolConfig.serverProtocolConfig.subnetAddress;
    case Roles::PortRole: return m_newAwgProtocolConfig.serverProtocolConfig.port;

    case Roles::ClientMtuRole: return m_newAwgProtocolConfig.clientProtocolConfig.wireGuardData.mtu;
    case Roles::ClientJunkPacketCountRole: return m_newAwgProtocolConfig.clientProtocolConfig.awgData.junkPacketCount;
    case Roles::ClientJunkPacketMinSizeRole: return m_newAwgProtocolConfig.clientProtocolConfig.awgData.junkPacketMinSize;
    case Roles::ClientJunkPacketMaxSizeRole: return m_newAwgProtocolConfig.clientProtocolConfig.awgData.junkPacketMaxSize;
    case Roles::ClientSpecialJunk1Role: return m_newAwgProtocolConfig.clientProtocolConfig.awgData.specialJunk1;
    case Roles::ClientSpecialJunk2Role: return m_newAwgProtocolConfig.clientProtocolConfig.awgData.specialJunk2;
    case Roles::ClientSpecialJunk3Role: return m_newAwgProtocolConfig.clientProtocolConfig.awgData.specialJunk3;
    case Roles::ClientSpecialJunk4Role: return m_newAwgProtocolConfig.clientProtocolConfig.awgData.specialJunk4;
    case Roles::ClientSpecialJunk5Role: return m_newAwgProtocolConfig.clientProtocolConfig.awgData.specialJunk5;
    case Roles::ClientControlledJunk1Role: return m_newAwgProtocolConfig.clientProtocolConfig.awgData.controlledJunk1;
    case Roles::ClientControlledJunk2Role: return m_newAwgProtocolConfig.clientProtocolConfig.awgData.controlledJunk2;
    case Roles::ClientControlledJunk3Role: return m_newAwgProtocolConfig.clientProtocolConfig.awgData.controlledJunk3;
    case Roles::ClientSpecialHandshakeTimeoutRole: return m_newAwgProtocolConfig.clientProtocolConfig.awgData.specialHandshakeTimeout;

    case Roles::ServerJunkPacketCountRole: return m_newAwgProtocolConfig.serverProtocolConfig.awgData.junkPacketCount;
    case Roles::ServerJunkPacketMinSizeRole: return m_newAwgProtocolConfig.serverProtocolConfig.awgData.junkPacketMinSize;
    case Roles::ServerJunkPacketMaxSizeRole: return m_newAwgProtocolConfig.serverProtocolConfig.awgData.junkPacketMaxSize;
    case Roles::ServerInitPacketJunkSizeRole: return m_newAwgProtocolConfig.serverProtocolConfig.awgData.initPacketJunkSize;
    case Roles::ServerResponsePacketJunkSizeRole: return m_newAwgProtocolConfig.serverProtocolConfig.awgData.responsePacketJunkSize;
    case Roles::ServerInitPacketMagicHeaderRole: return m_newAwgProtocolConfig.serverProtocolConfig.awgData.initPacketMagicHeader;
    case Roles::ServerResponsePacketMagicHeaderRole: return m_newAwgProtocolConfig.serverProtocolConfig.awgData.responsePacketMagicHeader;
    case Roles::ServerUnderloadPacketMagicHeaderRole: return m_newAwgProtocolConfig.serverProtocolConfig.awgData.underloadPacketMagicHeader;
    case Roles::ServerTransportPacketMagicHeaderRole: return m_newAwgProtocolConfig.serverProtocolConfig.awgData.transportPacketMagicHeader;
    }

    return QVariant();
}

void AwgConfigModel::updateModel(const AwgProtocolConfig awgProtocolConfig)
{
    beginResetModel();
    m_newAwgProtocolConfig = awgProtocolConfig;
    m_oldAwgProtocolConfig = awgProtocolConfig;
    endResetModel();
}

QSharedPointer<ProtocolConfig> AwgConfigModel::getConfig()
{
    if (m_oldAwgProtocolConfig.hasEqualServerSettings(m_newAwgProtocolConfig)) {
        m_newAwgProtocolConfig.clearClientSettings();
    }
    return QSharedPointer<AwgProtocolConfig>::create(m_newAwgProtocolConfig);
}

bool AwgConfigModel::isHeadersEqual(const QString &h1, const QString &h2, const QString &h3, const QString &h4)
{
    return (h1 == h2) || (h1 == h3) || (h1 == h4) || (h2 == h3) || (h2 == h4) || (h3 == h4);
}

bool AwgConfigModel::isPacketSizeEqual(const int s1, const int s2)
{
    return (awg::messageInitiationSize + s1 == awg::messageResponseSize + s2);
}

// bool AwgConfigModel::isPacketSizeEqual(const int s1, const int s2, const int s3, const int s4)
// {
//     int initSize = AwgConstant::messageInitiationSize + s1;
//     int responseSize = AwgConstant::messageResponseSize + s2;
//     int cookieSize = AwgConstant::messageCookieReplySize + s3;
//     int transportSize = AwgConstant::messageTransportSize + s4;

//     return (initSize == responseSize || initSize == cookieSize || initSize == transportSize || responseSize == cookieSize
//             || responseSize == transportSize || cookieSize == transportSize);
// }

bool AwgConfigModel::isServerSettingsEqual()
{
    return m_oldAwgProtocolConfig.hasEqualServerSettings(m_newAwgProtocolConfig);
}

QHash<int, QByteArray> AwgConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[SubnetAddressRole] = "subnetAddress";
    roles[PortRole] = "port";

    roles[ClientMtuRole] = "clientMtu";
    roles[ClientJunkPacketCountRole] = "clientJunkPacketCount";
    roles[ClientJunkPacketMinSizeRole] = "clientJunkPacketMinSize";
    roles[ClientJunkPacketMaxSizeRole] = "clientJunkPacketMaxSize";
    roles[ClientSpecialJunk1Role] = "clientSpecialJunk1";
    roles[ClientSpecialJunk2Role] = "clientSpecialJunk2";
    roles[ClientSpecialJunk3Role] = "clientSpecialJunk3";
    roles[ClientSpecialJunk4Role] = "clientSpecialJunk4";
    roles[ClientSpecialJunk5Role] = "clientSpecialJunk5";
    roles[ClientControlledJunk1Role] = "clientControlledJunk1";
    roles[ClientControlledJunk2Role] = "clientControlledJunk2";
    roles[ClientControlledJunk3Role] = "clientControlledJunk3";
    roles[ClientSpecialHandshakeTimeoutRole] = "clientSpecialHandshakeTimeout";

    roles[ServerJunkPacketCountRole] = "serverJunkPacketCount";
    roles[ServerJunkPacketMinSizeRole] = "serverJunkPacketMinSize";
    roles[ServerJunkPacketMaxSizeRole] = "serverJunkPacketMaxSize";
    roles[ServerInitPacketJunkSizeRole] = "serverInitPacketJunkSize";
    roles[ServerResponsePacketJunkSizeRole] = "serverResponsePacketJunkSize";
    roles[ServerCookieReplyPacketJunkSizeRole] = "serverCookieReplyPacketJunkSize";
    roles[ServerTransportPacketJunkSizeRole] = "serverTransportPacketJunkSize";

    roles[ServerInitPacketMagicHeaderRole] = "serverInitPacketMagicHeader";
    roles[ServerResponsePacketMagicHeaderRole] = "serverResponsePacketMagicHeader";
    roles[ServerUnderloadPacketMagicHeaderRole] = "serverUnderloadPacketMagicHeader";
    roles[ServerTransportPacketMagicHeaderRole] = "serverTransportPacketMagicHeader";

    return roles;
}
