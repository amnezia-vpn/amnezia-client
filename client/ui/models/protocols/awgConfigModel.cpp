#include "awgConfigModel.h"

#include <QJsonDocument>

#include "protocols/protocols_defs.h"

#include "ui/models/protocols/utils.h"

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

    case Roles::ServerJunkPacketCountRole: m_serverProtocolConfig.insert(config_key::junkPacketCount, value.toString()); break;
    case Roles::ServerJunkPacketMinSizeRole: m_serverProtocolConfig.insert(config_key::junkPacketMinSize, value.toString()); break;
    case Roles::ServerJunkPacketMaxSizeRole: m_serverProtocolConfig.insert(config_key::junkPacketMaxSize, value.toString()); break;
    case Roles::ServerInitPacketJunkSizeRole: m_serverProtocolConfig.insert(config_key::initPacketJunkSize, value.toString()); break;
    case Roles::ServerResponsePacketJunkSizeRole:
        m_serverProtocolConfig.insert(config_key::responsePacketJunkSize, value.toString());
        break;
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

    case Roles::ServerJunkPacketCountRole: return m_serverProtocolConfig.value(config_key::junkPacketCount);
    case Roles::ServerJunkPacketMinSizeRole: return m_serverProtocolConfig.value(config_key::junkPacketMinSize);
    case Roles::ServerJunkPacketMaxSizeRole: return m_serverProtocolConfig.value(config_key::junkPacketMaxSize);
    case Roles::ServerInitPacketJunkSizeRole: return m_serverProtocolConfig.value(config_key::initPacketJunkSize);
    case Roles::ServerResponsePacketJunkSizeRole: return m_serverProtocolConfig.value(config_key::responsePacketJunkSize);
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

    const auto defaultTransportProto =
            ProtocolProps::transportProtoToString(ProtocolProps::defaultTransportProto(Proto::Awg), Proto::Awg);

    updateConfig(serverProtocolConfig, config_key::transport_proto, defaultTransportProto.toUtf8().constData());
    m_serverProtocolConfig[config_key::last_config] = serverProtocolConfig.value(config_key::last_config);

    const std::pair<const char *, const char *> serverDefaults[] = {
        { config_key::subnet_address, protocols::wireguard::defaultSubnetAddress },
        { config_key::port, protocols::wireguard::defaultPort },
        { config_key::junkPacketCount, protocols::awg::defaultJunkPacketCount },
        { config_key::junkPacketMinSize, protocols::awg::defaultJunkPacketMinSize },
        { config_key::junkPacketMaxSize, protocols::awg::defaultJunkPacketMaxSize },
        { config_key::initPacketJunkSize, protocols::awg::defaultInitPacketJunkSize },
        { config_key::responsePacketJunkSize, protocols::awg::defaultResponsePacketJunkSize },
        { config_key::initPacketMagicHeader, protocols::awg::defaultInitPacketMagicHeader },
        { config_key::responsePacketMagicHeader, protocols::awg::defaultResponsePacketMagicHeader },
        { config_key::underloadPacketMagicHeader, protocols::awg::defaultUnderloadPacketMagicHeader },
        { config_key::transportPacketMagicHeader, protocols::awg::defaultTransportPacketMagicHeader },
    };

    for (const auto &[key, defaultValue] : serverDefaults)
        updateConfig(serverProtocolConfig, key, defaultValue);

    const auto lastConfig = m_serverProtocolConfig.value(config_key::last_config).toString();
    QJsonObject clientProtocolConfig = QJsonDocument::fromJson(lastConfig.toUtf8()).object();

    const std::pair<const char *, const char *> clientDefaults[] = {
        { config_key::mtu, protocols::awg::defaultMtu },
        { config_key::junkPacketCount,
          m_serverProtocolConfig[config_key::junkPacketCount].toString().toUtf8().constData() },
        { config_key::junkPacketMinSize,
          m_serverProtocolConfig[config_key::junkPacketMinSize].toString().toUtf8().constData() },
        { config_key::junkPacketMaxSize,
          m_serverProtocolConfig[config_key::junkPacketMaxSize].toString().toUtf8().constData() },
    };

    for (const auto &[key, defaultValue] : clientDefaults)
        updateConfig(clientProtocolConfig, key, defaultValue);

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

    roles[ServerJunkPacketCountRole] = "serverJunkPacketCount";
    roles[ServerJunkPacketMinSizeRole] = "serverJunkPacketMinSize";
    roles[ServerJunkPacketMaxSizeRole] = "serverJunkPacketMaxSize";
    roles[ServerInitPacketJunkSizeRole] = "serverInitPacketJunkSize";
    roles[ServerResponsePacketJunkSizeRole] = "serverResponsePacketJunkSize";
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

    subnetAddress = serverProtocolConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress);
    port = serverProtocolConfig.value(config_key::port).toString(protocols::awg::defaultPort);
    serverJunkPacketCount = serverProtocolConfig.value(config_key::junkPacketCount).toString(protocols::awg::defaultJunkPacketCount);
    serverJunkPacketMinSize = serverProtocolConfig.value(config_key::junkPacketMinSize).toString(protocols::awg::defaultJunkPacketMinSize);
    serverJunkPacketMaxSize = serverProtocolConfig.value(config_key::junkPacketMaxSize).toString(protocols::awg::defaultJunkPacketMaxSize);
    serverInitPacketJunkSize = serverProtocolConfig.value(config_key::initPacketJunkSize).toString(protocols::awg::defaultInitPacketJunkSize);
    serverResponsePacketJunkSize =
            serverProtocolConfig.value(config_key::responsePacketJunkSize).toString(protocols::awg::defaultResponsePacketJunkSize);
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
        || clientJunkPacketMinSize != other.clientJunkPacketMinSize || clientJunkPacketMaxSize != other.clientJunkPacketMaxSize) {
        return false;
    }
    return true;
}
