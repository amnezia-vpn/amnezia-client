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
    case Roles::PortRole: m_protocolConfig.insert(config_key::port, value.toString()); break;
    case Roles::MtuRole: m_mtu = value.toString(); break;
    case Roles::JunkPacketCountRole: m_protocolConfig.insert(config_key::junkPacketCount, value.toString()); break;
    case Roles::JunkPacketMinSizeRole: m_protocolConfig.insert(config_key::junkPacketMinSize, value.toString()); break;
    case Roles::JunkPacketMaxSizeRole: m_protocolConfig.insert(config_key::junkPacketMaxSize, value.toString()); break;
    case Roles::InitPacketJunkSizeRole:
        m_protocolConfig.insert(config_key::initPacketJunkSize, value.toString());
        break;
    case Roles::ResponsePacketJunkSizeRole:
        m_protocolConfig.insert(config_key::responsePacketJunkSize, value.toString());
        break;
    case Roles::InitPacketMagicHeaderRole:
        m_protocolConfig.insert(config_key::initPacketMagicHeader, value.toString());
        break;
    case Roles::ResponsePacketMagicHeaderRole:
        m_protocolConfig.insert(config_key::responsePacketMagicHeader, value.toString());
        break;
    case Roles::UnderloadPacketMagicHeaderRole:
        m_protocolConfig.insert(config_key::underloadPacketMagicHeader, value.toString());
        break;
    case Roles::TransportPacketMagicHeaderRole:
        m_protocolConfig.insert(config_key::transportPacketMagicHeader, value.toString());
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
    case Roles::PortRole: return m_protocolConfig.value(config_key::port).toString();
    case Roles::MtuRole: return m_mtu;
    case Roles::JunkPacketCountRole: return m_protocolConfig.value(config_key::junkPacketCount);
    case Roles::JunkPacketMinSizeRole: return m_protocolConfig.value(config_key::junkPacketMinSize);
    case Roles::JunkPacketMaxSizeRole: return m_protocolConfig.value(config_key::junkPacketMaxSize);
    case Roles::InitPacketJunkSizeRole: return m_protocolConfig.value(config_key::initPacketJunkSize);
    case Roles::ResponsePacketJunkSizeRole: return m_protocolConfig.value(config_key::responsePacketJunkSize);
    case Roles::InitPacketMagicHeaderRole: return m_protocolConfig.value(config_key::initPacketMagicHeader);
    case Roles::ResponsePacketMagicHeaderRole: return m_protocolConfig.value(config_key::responsePacketMagicHeader);
    case Roles::UnderloadPacketMagicHeaderRole: return m_protocolConfig.value(config_key::underloadPacketMagicHeader);
    case Roles::TransportPacketMagicHeaderRole: return m_protocolConfig.value(config_key::transportPacketMagicHeader);
    }

    return QVariant();
}

void AwgConfigModel::updateModel(const QJsonObject &config)
{
    beginResetModel();
    m_container = ContainerProps::containerFromString(config.value(config_key::container).toString());

    m_fullConfig = config;

    QJsonObject protocolConfig = config.value(config_key::awg).toObject();

    m_protocolConfig[config_key::last_config] = protocolConfig.value(config_key::last_config);
    m_protocolConfig[config_key::port] = protocolConfig.value(config_key::port).toString(protocols::awg::defaultPort);
    m_protocolConfig[config_key::junkPacketCount] =
            protocolConfig.value(config_key::junkPacketCount).toString(protocols::awg::defaultJunkPacketCount);
    m_protocolConfig[config_key::junkPacketMinSize] =
            protocolConfig.value(config_key::junkPacketMinSize).toString(protocols::awg::defaultJunkPacketMinSize);
    m_protocolConfig[config_key::junkPacketMaxSize] =
            protocolConfig.value(config_key::junkPacketMaxSize).toString(protocols::awg::defaultJunkPacketMaxSize);
    m_protocolConfig[config_key::initPacketJunkSize] =
            protocolConfig.value(config_key::initPacketJunkSize).toString(protocols::awg::defaultInitPacketJunkSize);
    m_protocolConfig[config_key::responsePacketJunkSize] =
            protocolConfig.value(config_key::responsePacketJunkSize).toString(protocols::awg::defaultResponsePacketJunkSize);
    m_protocolConfig[config_key::initPacketMagicHeader] =
            protocolConfig.value(config_key::initPacketMagicHeader).toString(protocols::awg::defaultInitPacketMagicHeader);
    m_protocolConfig[config_key::responsePacketMagicHeader] =
            protocolConfig.value(config_key::responsePacketMagicHeader)
                    .toString(protocols::awg::defaultResponsePacketMagicHeader);
    m_protocolConfig[config_key::underloadPacketMagicHeader] =
            protocolConfig.value(config_key::underloadPacketMagicHeader)
                    .toString(protocols::awg::defaultUnderloadPacketMagicHeader);
    m_protocolConfig[config_key::transportPacketMagicHeader] =
            protocolConfig.value(config_key::transportPacketMagicHeader)
                    .toString(protocols::awg::defaultTransportPacketMagicHeader);

    auto lastConfig = m_protocolConfig.value(config_key::last_config).toString();
    QJsonObject jsonConfig = QJsonDocument::fromJson(lastConfig.toUtf8()).object();
    m_mtu = jsonConfig[config_key::mtu].toString(protocols::awg::defaultMtu);

    endResetModel();
}

QJsonObject AwgConfigModel::getConfig()
{
    const AwgConfig oldConfig(m_fullConfig.value(config_key::awg).toObject());
    const AwgConfig newConfig(m_protocolConfig);

    if (!oldConfig.hasEqualServerSettings(newConfig)) {
        m_protocolConfig.remove(config_key::last_config);
    } else {
        auto lastConfig = m_protocolConfig.value(config_key::last_config).toString();
        QJsonObject jsonConfig = QJsonDocument::fromJson(lastConfig.toUtf8()).object();
        jsonConfig[config_key::mtu] = m_mtu;

        m_protocolConfig[config_key::last_config] = QString(QJsonDocument(jsonConfig).toJson());
    }

    m_fullConfig.insert(config_key::awg, m_protocolConfig);
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
    const AwgConfig newConfig(m_protocolConfig);

    return oldConfig.hasEqualServerSettings(newConfig);
}

QHash<int, QByteArray> AwgConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[PortRole] = "port";
    roles[MtuRole] = "mtu";
    roles[JunkPacketCountRole] = "junkPacketCount";
    roles[JunkPacketMinSizeRole] = "junkPacketMinSize";
    roles[JunkPacketMaxSizeRole] = "junkPacketMaxSize";
    roles[InitPacketJunkSizeRole] = "initPacketJunkSize";
    roles[ResponsePacketJunkSizeRole] = "responsePacketJunkSize";
    roles[InitPacketMagicHeaderRole] = "initPacketMagicHeader";
    roles[ResponsePacketMagicHeaderRole] = "responsePacketMagicHeader";
    roles[UnderloadPacketMagicHeaderRole] = "underloadPacketMagicHeader";
    roles[TransportPacketMagicHeaderRole] = "transportPacketMagicHeader";

    return roles;
}

AwgConfig::AwgConfig(const QJsonObject &jsonConfig)
{
    auto lastConfig = jsonConfig.value(config_key::last_config).toString();
    QJsonObject config = QJsonDocument::fromJson(lastConfig.toUtf8()).object();
    mtu = config[config_key::mtu].toString(protocols::awg::defaultMtu);

    port = jsonConfig.value(config_key::port).toString(protocols::awg::defaultPort);
    junkPacketCount = jsonConfig.value(config_key::junkPacketCount).toString(protocols::awg::defaultJunkPacketCount);
    junkPacketMinSize =
            jsonConfig.value(config_key::junkPacketMinSize).toString(protocols::awg::defaultJunkPacketMinSize);
    junkPacketMaxSize =
            jsonConfig.value(config_key::junkPacketMaxSize).toString(protocols::awg::defaultJunkPacketMaxSize);
    initPacketJunkSize =
            jsonConfig.value(config_key::initPacketJunkSize).toString(protocols::awg::defaultInitPacketJunkSize);
    responsePacketJunkSize =
            jsonConfig.value(config_key::responsePacketJunkSize).toString(protocols::awg::defaultResponsePacketJunkSize);
    initPacketMagicHeader =
            jsonConfig.value(config_key::initPacketMagicHeader).toString(protocols::awg::defaultInitPacketMagicHeader);
    responsePacketMagicHeader = jsonConfig.value(config_key::responsePacketMagicHeader)
                                        .toString(protocols::awg::defaultResponsePacketMagicHeader);
    underloadPacketMagicHeader = jsonConfig.value(config_key::underloadPacketMagicHeader)
                                         .toString(protocols::awg::defaultUnderloadPacketMagicHeader);
    transportPacketMagicHeader = jsonConfig.value(config_key::transportPacketMagicHeader)
                                         .toString(protocols::awg::defaultTransportPacketMagicHeader);
}

bool AwgConfig::hasEqualServerSettings(const AwgConfig &other) const
{
    if (port != other.port || junkPacketCount != other.junkPacketCount || junkPacketMinSize != other.junkPacketMinSize
        || junkPacketMaxSize != other.junkPacketMaxSize || initPacketJunkSize != other.initPacketJunkSize
        || responsePacketJunkSize != other.responsePacketJunkSize || initPacketMagicHeader != other.initPacketMagicHeader
        || responsePacketMagicHeader != other.responsePacketMagicHeader
        || underloadPacketMagicHeader != other.underloadPacketMagicHeader
        || transportPacketMagicHeader != other.transportPacketMagicHeader) {
        return false;
    }
    return true;
}

bool AwgConfig::hasEqualClientSettings(const AwgConfig &other) const
{
    if (mtu != other.mtu) {
        return false;
    }
    return true;
}
