#include "awgConfigModel.h"

#include <QJsonDocument>

#include "protocols/protocols_defs.h"

AwgConfigModel::AwgConfigModel(QObject *parent) : QAbstractListModel(parent)
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
    case Roles::SubnetAddressRole: m_serverProtocolConfig.insert(config_key::subnet_address, value.toString()); break;
    case Roles::PortRole: m_serverProtocolConfig.insert(config_key::port, value.toString()); break;

    case Roles::ClientMtuRole: m_clientProtocolConfig.insert(config_key::mtu, value.toString()); break;
    case Roles::ClientJunkPacketCountRole: m_clientProtocolConfig.insert(config_key::junkPacketCount, value.toString()); break;
    case Roles::ClientJunkPacketMinSizeRole: m_clientProtocolConfig.insert(config_key::junkPacketMinSize, value.toString()); break;
    case Roles::ClientJunkPacketMaxSizeRole: m_clientProtocolConfig.insert(config_key::junkPacketMaxSize, value.toString()); break;
    case Roles::ClientSpecialJunk1Role: m_clientProtocolConfig.insert(config_key::specialJunk1, value.toString()); break;
    case Roles::ClientSpecialJunk2Role: m_clientProtocolConfig.insert(config_key::specialJunk2, value.toString()); break;
    case Roles::ClientSpecialJunk3Role: m_clientProtocolConfig.insert(config_key::specialJunk3, value.toString()); break;
    case Roles::ClientSpecialJunk4Role: m_clientProtocolConfig.insert(config_key::specialJunk4, value.toString()); break;
    case Roles::ClientSpecialJunk5Role: m_clientProtocolConfig.insert(config_key::specialJunk5, value.toString()); break;
    case Roles::ClientControlledJunk1Role: m_clientProtocolConfig.insert(config_key::controlledJunk1, value.toString()); break;
    case Roles::ClientControlledJunk2Role: m_clientProtocolConfig.insert(config_key::controlledJunk2, value.toString()); break;
    case Roles::ClientControlledJunk3Role: m_clientProtocolConfig.insert(config_key::controlledJunk3, value.toString()); break;
    case Roles::ClientSpecialHandshakeTimeoutRole:
        m_clientProtocolConfig.insert(config_key::specialHandshakeTimeout, value.toString());
        break;
    case Roles::ServerJunkPacketCountRole: m_serverProtocolConfig.insert(config_key::junkPacketCount, value.toString()); break;
    case Roles::ServerJunkPacketMinSizeRole: m_serverProtocolConfig.insert(config_key::junkPacketMinSize, value.toString()); break;
    case Roles::ServerJunkPacketMaxSizeRole: m_serverProtocolConfig.insert(config_key::junkPacketMaxSize, value.toString()); break;
    case Roles::ServerInitPacketJunkSizeRole: m_serverProtocolConfig.insert(config_key::initPacketJunkSize, value.toString()); break;
    case Roles::ServerResponsePacketJunkSizeRole:
        m_serverProtocolConfig.insert(config_key::responsePacketJunkSize, value.toString());
        break;
    // case Roles::ServerCookieReplyPacketJunkSizeRole:
    //     m_serverProtocolConfig.insert(config_key::cookieReplyPacketJunkSize, value.toString());
    //     break;
    // case Roles::ServerTransportPacketJunkSizeRole:
    //     m_serverProtocolConfig.insert(config_key::transportPacketJunkSize, value.toString());
    //     break;
    case Roles::ServerInitPacketMagicHeaderRole: m_serverProtocolConfig.insert(config_key::initPacketMagicHeader, value.toString()); break;
    case Roles::ServerResponsePacketMagicHeaderRole:
        m_serverProtocolConfig.insert(config_key::responsePacketMagicHeader, value.toString());
        break;
    case Roles::ServerUnderloadPacketMagicHeaderRole:
        m_serverProtocolConfig.insert(config_key::underloadPacketMagicHeader, value.toString());
        break;
    case Roles::ServerTransportPacketMagicHeaderRole:
        m_serverProtocolConfig.insert(config_key::transportPacketMagicHeader, value.toString());
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
    case Roles::SubnetAddressRole: return m_serverProtocolConfig.value(config_key::subnet_address).toString();
    case Roles::PortRole: return m_serverProtocolConfig.value(config_key::port).toString();

    case Roles::ClientMtuRole: return m_clientProtocolConfig.value(config_key::mtu);
    case Roles::ClientJunkPacketCountRole: return m_clientProtocolConfig.value(config_key::junkPacketCount);
    case Roles::ClientJunkPacketMinSizeRole: return m_clientProtocolConfig.value(config_key::junkPacketMinSize);
    case Roles::ClientJunkPacketMaxSizeRole: return m_clientProtocolConfig.value(config_key::junkPacketMaxSize);
    case Roles::ClientSpecialJunk1Role: return m_clientProtocolConfig.value(config_key::specialJunk1);
    case Roles::ClientSpecialJunk2Role: return m_clientProtocolConfig.value(config_key::specialJunk2);
    case Roles::ClientSpecialJunk3Role: return m_clientProtocolConfig.value(config_key::specialJunk3);
    case Roles::ClientSpecialJunk4Role: return m_clientProtocolConfig.value(config_key::specialJunk4);
    case Roles::ClientSpecialJunk5Role: return m_clientProtocolConfig.value(config_key::specialJunk5);
    case Roles::ClientControlledJunk1Role: return m_clientProtocolConfig.value(config_key::controlledJunk1);
    case Roles::ClientControlledJunk2Role: return m_clientProtocolConfig.value(config_key::controlledJunk2);
    case Roles::ClientControlledJunk3Role: return m_clientProtocolConfig.value(config_key::controlledJunk3);
    case Roles::ClientSpecialHandshakeTimeoutRole: return m_clientProtocolConfig.value(config_key::specialHandshakeTimeout);

    case Roles::ServerJunkPacketCountRole: return m_serverProtocolConfig.value(config_key::junkPacketCount);
    case Roles::ServerJunkPacketMinSizeRole: return m_serverProtocolConfig.value(config_key::junkPacketMinSize);
    case Roles::ServerJunkPacketMaxSizeRole: return m_serverProtocolConfig.value(config_key::junkPacketMaxSize);
    case Roles::ServerInitPacketJunkSizeRole: return m_serverProtocolConfig.value(config_key::initPacketJunkSize);
    case Roles::ServerResponsePacketJunkSizeRole: return m_serverProtocolConfig.value(config_key::responsePacketJunkSize);
    // case Roles::ServerCookieReplyPacketJunkSizeRole: return m_serverProtocolConfig.value(config_key::cookieReplyPacketJunkSize);
    // case Roles::ServerTransportPacketJunkSizeRole: return m_serverProtocolConfig.value(config_key::transportPacketJunkSize);
    case Roles::ServerInitPacketMagicHeaderRole: return m_serverProtocolConfig.value(config_key::initPacketMagicHeader);
    case Roles::ServerResponsePacketMagicHeaderRole: return m_serverProtocolConfig.value(config_key::responsePacketMagicHeader);
    case Roles::ServerUnderloadPacketMagicHeaderRole: return m_serverProtocolConfig.value(config_key::underloadPacketMagicHeader);
    case Roles::ServerTransportPacketMagicHeaderRole: return m_serverProtocolConfig.value(config_key::transportPacketMagicHeader);
    }

    return QVariant();
}

void AwgConfigModel::updateModel(const QJsonObject &config)
{
    beginResetModel();
    m_container = ContainerProps::containerFromString(config.value(config_key::container).toString());

    m_fullConfig = config;

    QJsonObject serverProtocolConfig = config.value(config_key::awg).toObject();

    auto defaultTransportProto = ProtocolProps::transportProtoToString(ProtocolProps::defaultTransportProto(Proto::Awg), Proto::Awg);
    m_serverProtocolConfig.insert(config_key::transport_proto,
                                  serverProtocolConfig.value(config_key::transport_proto).toString(defaultTransportProto));
    m_serverProtocolConfig[config_key::last_config] = serverProtocolConfig.value(config_key::last_config);
    m_serverProtocolConfig[config_key::subnet_address] =
            serverProtocolConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress);
    m_serverProtocolConfig[config_key::port] = serverProtocolConfig.value(config_key::port).toString(protocols::awg::defaultPort);
    m_serverProtocolConfig[config_key::junkPacketCount] =
            serverProtocolConfig.value(config_key::junkPacketCount).toString(protocols::awg::defaultJunkPacketCount);
    m_serverProtocolConfig[config_key::junkPacketMinSize] =
            serverProtocolConfig.value(config_key::junkPacketMinSize).toString(protocols::awg::defaultJunkPacketMinSize);
    m_serverProtocolConfig[config_key::junkPacketMaxSize] =
            serverProtocolConfig.value(config_key::junkPacketMaxSize).toString(protocols::awg::defaultJunkPacketMaxSize);
    m_serverProtocolConfig[config_key::initPacketJunkSize] =
            serverProtocolConfig.value(config_key::initPacketJunkSize).toString(protocols::awg::defaultInitPacketJunkSize);
    m_serverProtocolConfig[config_key::responsePacketJunkSize] =
            serverProtocolConfig.value(config_key::responsePacketJunkSize).toString(protocols::awg::defaultResponsePacketJunkSize);
    // m_serverProtocolConfig[config_key::cookieReplyPacketJunkSize] =
    //         serverProtocolConfig.value(config_key::cookieReplyPacketJunkSize).toString(protocols::awg::defaultCookieReplyPacketJunkSize);
    // m_serverProtocolConfig[config_key::transportPacketJunkSize] =
    //         serverProtocolConfig.value(config_key::transportPacketJunkSize).toString(protocols::awg::defaultTransportPacketJunkSize);
    m_serverProtocolConfig[config_key::initPacketMagicHeader] =
            serverProtocolConfig.value(config_key::initPacketMagicHeader).toString(protocols::awg::defaultInitPacketMagicHeader);
    m_serverProtocolConfig[config_key::responsePacketMagicHeader] =
            serverProtocolConfig.value(config_key::responsePacketMagicHeader).toString(protocols::awg::defaultResponsePacketMagicHeader);
    m_serverProtocolConfig[config_key::underloadPacketMagicHeader] =
            serverProtocolConfig.value(config_key::underloadPacketMagicHeader).toString(protocols::awg::defaultUnderloadPacketMagicHeader);
    m_serverProtocolConfig[config_key::transportPacketMagicHeader] =
            serverProtocolConfig.value(config_key::transportPacketMagicHeader).toString(protocols::awg::defaultTransportPacketMagicHeader);

    auto lastConfig = m_serverProtocolConfig.value(config_key::last_config).toString();
    QJsonObject clientProtocolConfig = QJsonDocument::fromJson(lastConfig.toUtf8()).object();
    m_clientProtocolConfig[config_key::mtu] = clientProtocolConfig[config_key::mtu].toString(protocols::awg::defaultMtu);
    m_clientProtocolConfig[config_key::junkPacketCount] =
            clientProtocolConfig.value(config_key::junkPacketCount).toString(m_serverProtocolConfig[config_key::junkPacketCount].toString());
    m_clientProtocolConfig[config_key::junkPacketMinSize] =
            clientProtocolConfig.value(config_key::junkPacketMinSize).toString(m_serverProtocolConfig[config_key::junkPacketMinSize].toString());
    m_clientProtocolConfig[config_key::junkPacketMaxSize] =
            clientProtocolConfig.value(config_key::junkPacketMaxSize).toString(m_serverProtocolConfig[config_key::junkPacketMaxSize].toString());
    m_clientProtocolConfig[config_key::specialJunk1] =
            clientProtocolConfig.value(config_key::specialJunk1).toString(protocols::awg::defaultSpecialJunk1);
    m_clientProtocolConfig[config_key::specialJunk2] =
            clientProtocolConfig.value(config_key::specialJunk2).toString(protocols::awg::defaultSpecialJunk2);
    m_clientProtocolConfig[config_key::specialJunk3] =
            clientProtocolConfig.value(config_key::specialJunk3).toString(protocols::awg::defaultSpecialJunk3);
    m_clientProtocolConfig[config_key::specialJunk4] =
            clientProtocolConfig.value(config_key::specialJunk4).toString(protocols::awg::defaultSpecialJunk4);
    m_clientProtocolConfig[config_key::specialJunk5] =
            clientProtocolConfig.value(config_key::specialJunk5).toString(protocols::awg::defaultSpecialJunk5);
    m_clientProtocolConfig[config_key::controlledJunk1] =
            clientProtocolConfig.value(config_key::controlledJunk1).toString(protocols::awg::defaultControlledJunk1);
    m_clientProtocolConfig[config_key::controlledJunk2] =
            clientProtocolConfig.value(config_key::controlledJunk2).toString(protocols::awg::defaultControlledJunk2);
    m_clientProtocolConfig[config_key::controlledJunk3] =
            clientProtocolConfig.value(config_key::controlledJunk3).toString(protocols::awg::defaultControlledJunk3);
    m_clientProtocolConfig[config_key::specialHandshakeTimeout] =
            clientProtocolConfig.value(config_key::specialHandshakeTimeout).toString(protocols::awg::defaultSpecialHandshakeTimeout);
    endResetModel();
}

QJsonObject AwgConfigModel::getConfig()
{
    const AwgConfig oldConfig(m_fullConfig.value(config_key::awg).toObject());
    const AwgConfig newConfig(m_serverProtocolConfig);

    if (!oldConfig.hasEqualServerSettings(newConfig)) {
        m_serverProtocolConfig.remove(config_key::last_config);
    } else {
        auto lastConfig = m_serverProtocolConfig.value(config_key::last_config).toString();
        QJsonObject jsonConfig = QJsonDocument::fromJson(lastConfig.toUtf8()).object();
        jsonConfig[config_key::mtu] = m_clientProtocolConfig[config_key::mtu];
        jsonConfig[config_key::junkPacketCount] = m_clientProtocolConfig[config_key::junkPacketCount];
        jsonConfig[config_key::junkPacketMinSize] = m_clientProtocolConfig[config_key::junkPacketMinSize];
        jsonConfig[config_key::junkPacketMaxSize] = m_clientProtocolConfig[config_key::junkPacketMaxSize];
        jsonConfig[config_key::specialJunk1] = m_clientProtocolConfig[config_key::specialJunk1];
        jsonConfig[config_key::specialJunk2] = m_clientProtocolConfig[config_key::specialJunk2];
        jsonConfig[config_key::specialJunk3] = m_clientProtocolConfig[config_key::specialJunk3];
        jsonConfig[config_key::specialJunk4] = m_clientProtocolConfig[config_key::specialJunk4];
        jsonConfig[config_key::specialJunk5] = m_clientProtocolConfig[config_key::specialJunk5];
        jsonConfig[config_key::controlledJunk1] = m_clientProtocolConfig[config_key::controlledJunk1];
        jsonConfig[config_key::controlledJunk2] = m_clientProtocolConfig[config_key::controlledJunk2];
        jsonConfig[config_key::controlledJunk3] = m_clientProtocolConfig[config_key::controlledJunk3];
        jsonConfig[config_key::specialHandshakeTimeout] = m_clientProtocolConfig[config_key::specialHandshakeTimeout];

        m_serverProtocolConfig[config_key::last_config] = QString(QJsonDocument(jsonConfig).toJson());
    }

    m_fullConfig.insert(config_key::awg, m_serverProtocolConfig);
    return m_fullConfig;
}

bool AwgConfigModel::isHeadersEqual(const QString &h1, const QString &h2, const QString &h3, const QString &h4)
{
    return (h1 == h2) || (h1 == h3) || (h1 == h4) || (h2 == h3) || (h2 == h4) || (h3 == h4);
}

bool AwgConfigModel::isPacketSizeEqual(const int s1, const int s2)
{
    return (AwgConstant::messageInitiationSize + s1 == AwgConstant::messageResponseSize + s2);
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
    const AwgConfig oldConfig(m_fullConfig.value(config_key::awg).toObject());
    const AwgConfig newConfig(m_serverProtocolConfig);

    return oldConfig.hasEqualServerSettings(newConfig);
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

AwgConfig::AwgConfig(const QJsonObject &serverProtocolConfig)
{
    auto lastConfig = serverProtocolConfig.value(config_key::last_config).toString();
    QJsonObject clientProtocolConfig = QJsonDocument::fromJson(lastConfig.toUtf8()).object();
    clientMtu = clientProtocolConfig[config_key::mtu].toString(protocols::awg::defaultMtu);
    clientJunkPacketCount = clientProtocolConfig.value(config_key::junkPacketCount).toString(protocols::awg::defaultJunkPacketCount);
    clientJunkPacketMinSize = clientProtocolConfig.value(config_key::junkPacketMinSize).toString(protocols::awg::defaultJunkPacketMinSize);
    clientJunkPacketMaxSize = clientProtocolConfig.value(config_key::junkPacketMaxSize).toString(protocols::awg::defaultJunkPacketMaxSize);
    clientSpecialJunk1 = clientProtocolConfig.value(config_key::specialJunk1).toString(protocols::awg::defaultSpecialJunk1);
    clientSpecialJunk2 = clientProtocolConfig.value(config_key::specialJunk2).toString(protocols::awg::defaultSpecialJunk2);
    clientSpecialJunk3 = clientProtocolConfig.value(config_key::specialJunk3).toString(protocols::awg::defaultSpecialJunk3);
    clientSpecialJunk4 = clientProtocolConfig.value(config_key::specialJunk4).toString(protocols::awg::defaultSpecialJunk4);
    clientSpecialJunk5 = clientProtocolConfig.value(config_key::specialJunk5).toString(protocols::awg::defaultSpecialJunk5);
    clientControlledJunk1 = clientProtocolConfig.value(config_key::controlledJunk1).toString(protocols::awg::defaultControlledJunk1);
    clientControlledJunk2 = clientProtocolConfig.value(config_key::controlledJunk2).toString(protocols::awg::defaultControlledJunk2);
    clientControlledJunk3 = clientProtocolConfig.value(config_key::controlledJunk3).toString(protocols::awg::defaultControlledJunk3);
    clientSpecialHandshakeTimeout =
            clientProtocolConfig.value(config_key::specialHandshakeTimeout).toString(protocols::awg::defaultSpecialHandshakeTimeout);

    subnetAddress = serverProtocolConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress);
    port = serverProtocolConfig.value(config_key::port).toString(protocols::awg::defaultPort);
    serverJunkPacketCount = serverProtocolConfig.value(config_key::junkPacketCount).toString(protocols::awg::defaultJunkPacketCount);
    serverJunkPacketMinSize = serverProtocolConfig.value(config_key::junkPacketMinSize).toString(protocols::awg::defaultJunkPacketMinSize);
    serverJunkPacketMaxSize = serverProtocolConfig.value(config_key::junkPacketMaxSize).toString(protocols::awg::defaultJunkPacketMaxSize);
    serverInitPacketJunkSize = serverProtocolConfig.value(config_key::initPacketJunkSize).toString(protocols::awg::defaultInitPacketJunkSize);
    serverResponsePacketJunkSize =
            serverProtocolConfig.value(config_key::responsePacketJunkSize).toString(protocols::awg::defaultResponsePacketJunkSize);
    // serverCookieReplyPacketJunkSize =
    //         serverProtocolConfig.value(config_key::cookieReplyPacketJunkSize).toString(protocols::awg::defaultCookieReplyPacketJunkSize);
    // serverTransportPacketJunkSize =
    //         serverProtocolConfig.value(config_key::transportPacketJunkSize).toString(protocols::awg::defaultTransportPacketJunkSize);
    serverInitPacketMagicHeader =
            serverProtocolConfig.value(config_key::initPacketMagicHeader).toString(protocols::awg::defaultInitPacketMagicHeader);
    serverResponsePacketMagicHeader =
            serverProtocolConfig.value(config_key::responsePacketMagicHeader).toString(protocols::awg::defaultResponsePacketMagicHeader);
    serverUnderloadPacketMagicHeader =
            serverProtocolConfig.value(config_key::underloadPacketMagicHeader).toString(protocols::awg::defaultUnderloadPacketMagicHeader);
    serverTransportPacketMagicHeader =
            serverProtocolConfig.value(config_key::transportPacketMagicHeader).toString(protocols::awg::defaultTransportPacketMagicHeader);
}

bool AwgConfig::hasEqualServerSettings(const AwgConfig &other) const
{
    if (subnetAddress != other.subnetAddress || port != other.port || serverJunkPacketCount != other.serverJunkPacketCount
        || serverJunkPacketMinSize != other.serverJunkPacketMinSize || serverJunkPacketMaxSize != other.serverJunkPacketMaxSize
        || serverInitPacketJunkSize != other.serverInitPacketJunkSize || serverResponsePacketJunkSize != other.serverResponsePacketJunkSize
        // || serverCookieReplyPacketJunkSize != other.serverCookieReplyPacketJunkSize
        // || serverTransportPacketJunkSize != other.serverTransportPacketJunkSize
        || serverInitPacketMagicHeader != other.serverInitPacketMagicHeader
        || serverResponsePacketMagicHeader != other.serverResponsePacketMagicHeader
        || serverUnderloadPacketMagicHeader != other.serverUnderloadPacketMagicHeader
        || serverTransportPacketMagicHeader != other.serverTransportPacketMagicHeader) {
        return false;
    }
    return true;
}

bool AwgConfig::hasEqualClientSettings(const AwgConfig &other) const
{
    if (clientMtu != other.clientMtu || clientJunkPacketCount != other.clientJunkPacketCount
        || clientJunkPacketMinSize != other.clientJunkPacketMinSize || clientJunkPacketMaxSize != other.clientJunkPacketMaxSize
        || clientSpecialJunk1 != other.clientSpecialJunk1 || clientSpecialJunk2 != other.clientSpecialJunk2
        || clientSpecialJunk3 != other.clientSpecialJunk3 || clientSpecialJunk4 != other.clientSpecialJunk4
        || clientSpecialJunk5 != other.clientSpecialJunk5 || clientControlledJunk1 != other.clientControlledJunk1
        || clientControlledJunk2 != other.clientControlledJunk2 || clientControlledJunk3 != other.clientControlledJunk3
        || clientSpecialHandshakeTimeout != other.clientSpecialHandshakeTimeout) {
        return false;
    }
    return true;
}
