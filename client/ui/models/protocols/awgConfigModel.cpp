#include "awgConfigModel.h"

#include <QJsonDocument>

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/protocols/awgProtocolConfig.h"

using namespace amnezia;
using namespace ProtocolUtils;

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
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerUtils::allContainers().size()) {
        return false;
    }

    QString strValue = value.toString();

    switch (role) {
    case Roles::SubnetAddressRole: m_protocolConfig.serverConfig.subnetAddress = strValue; break;
    case Roles::PortRole: m_protocolConfig.serverConfig.port = strValue; break;

    case Roles::ClientMtuRole: m_protocolConfig.clientConfig->mtu = strValue; break;
    case Roles::ClientJunkPacketCountRole: m_protocolConfig.clientConfig->junkPacketCount = strValue; break;
    case Roles::ClientJunkPacketMinSizeRole: m_protocolConfig.clientConfig->junkPacketMinSize = strValue; break;
    case Roles::ClientJunkPacketMaxSizeRole: m_protocolConfig.clientConfig->junkPacketMaxSize = strValue; break;
    case Roles::ClientSpecialJunk1Role: m_protocolConfig.clientConfig->specialJunk1 = strValue; break;
    case Roles::ClientSpecialJunk2Role: m_protocolConfig.clientConfig->specialJunk2 = strValue; break;
    case Roles::ClientSpecialJunk3Role: m_protocolConfig.clientConfig->specialJunk3 = strValue; break;
    case Roles::ClientSpecialJunk4Role: m_protocolConfig.clientConfig->specialJunk4 = strValue; break;
    case Roles::ClientSpecialJunk5Role: m_protocolConfig.clientConfig->specialJunk5 = strValue; break;
    case Roles::ServerJunkPacketCountRole: m_protocolConfig.serverConfig.junkPacketCount = strValue; break;
    case Roles::ServerJunkPacketMinSizeRole: m_protocolConfig.serverConfig.junkPacketMinSize = strValue; break;
    case Roles::ServerJunkPacketMaxSizeRole: m_protocolConfig.serverConfig.junkPacketMaxSize = strValue; break;
    case Roles::ServerInitPacketJunkSizeRole: m_protocolConfig.serverConfig.initPacketJunkSize = strValue; break;
    case Roles::ServerResponsePacketJunkSizeRole: m_protocolConfig.serverConfig.responsePacketJunkSize = strValue; break;
    case Roles::ServerCookieReplyPacketJunkSizeRole: m_protocolConfig.serverConfig.cookieReplyPacketJunkSize = strValue; break;
    case Roles::ServerTransportPacketJunkSizeRole: m_protocolConfig.serverConfig.transportPacketJunkSize = strValue; break;
    case Roles::ServerInitPacketMagicHeaderRole: m_protocolConfig.serverConfig.initPacketMagicHeader = strValue; break;
    case Roles::ServerResponsePacketMagicHeaderRole: m_protocolConfig.serverConfig.responsePacketMagicHeader = strValue; break;
    case Roles::ServerUnderloadPacketMagicHeaderRole: m_protocolConfig.serverConfig.underloadPacketMagicHeader = strValue; break;
    case Roles::ServerTransportPacketMagicHeaderRole: m_protocolConfig.serverConfig.transportPacketMagicHeader = strValue; break;
    case Roles::ServerSpecialJunk1Role: m_protocolConfig.serverConfig.specialJunk1 = strValue; break;
    case Roles::ServerSpecialJunk2Role: m_protocolConfig.serverConfig.specialJunk2 = strValue; break;
    case Roles::ServerSpecialJunk3Role: m_protocolConfig.serverConfig.specialJunk3 = strValue; break;
    case Roles::ServerSpecialJunk4Role: m_protocolConfig.serverConfig.specialJunk4 = strValue; break;
    case Roles::ServerSpecialJunk5Role: m_protocolConfig.serverConfig.specialJunk5 = strValue; break;
    default:
        return false;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant AwgConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return QVariant();
    }

    switch (role) {
    case Roles::SubnetAddressRole: return m_protocolConfig.serverConfig.subnetAddress;
    case Roles::PortRole: return m_protocolConfig.serverConfig.port;

    case Roles::ClientMtuRole: return m_protocolConfig.clientConfig->mtu;
    case Roles::ClientJunkPacketCountRole: return m_protocolConfig.clientConfig->junkPacketCount;
    case Roles::ClientJunkPacketMinSizeRole: return m_protocolConfig.clientConfig->junkPacketMinSize;
    case Roles::ClientJunkPacketMaxSizeRole: return m_protocolConfig.clientConfig->junkPacketMaxSize;
    case Roles::ClientSpecialJunk1Role: return m_protocolConfig.clientConfig->specialJunk1;
    case Roles::ClientSpecialJunk2Role: return m_protocolConfig.clientConfig->specialJunk2;
    case Roles::ClientSpecialJunk3Role: return m_protocolConfig.clientConfig->specialJunk3;
    case Roles::ClientSpecialJunk4Role: return m_protocolConfig.clientConfig->specialJunk4;
    case Roles::ClientSpecialJunk5Role: return m_protocolConfig.clientConfig->specialJunk5;

    case Roles::ServerJunkPacketCountRole: return m_protocolConfig.serverConfig.junkPacketCount;
    case Roles::ServerJunkPacketMinSizeRole: return m_protocolConfig.serverConfig.junkPacketMinSize;
    case Roles::ServerJunkPacketMaxSizeRole: return m_protocolConfig.serverConfig.junkPacketMaxSize;
    case Roles::ServerInitPacketJunkSizeRole: return m_protocolConfig.serverConfig.initPacketJunkSize;
    case Roles::ServerResponsePacketJunkSizeRole: return m_protocolConfig.serverConfig.responsePacketJunkSize;
    case Roles::ServerCookieReplyPacketJunkSizeRole: return m_protocolConfig.serverConfig.cookieReplyPacketJunkSize;
    case Roles::ServerTransportPacketJunkSizeRole: return m_protocolConfig.serverConfig.transportPacketJunkSize;
    case Roles::ServerInitPacketMagicHeaderRole: return m_protocolConfig.serverConfig.initPacketMagicHeader;
    case Roles::ServerResponsePacketMagicHeaderRole: return m_protocolConfig.serverConfig.responsePacketMagicHeader;
    case Roles::ServerUnderloadPacketMagicHeaderRole: return m_protocolConfig.serverConfig.underloadPacketMagicHeader;
    case Roles::ServerTransportPacketMagicHeaderRole: return m_protocolConfig.serverConfig.transportPacketMagicHeader;
    case Roles::ServerSpecialJunk1Role: return m_protocolConfig.serverConfig.specialJunk1;
    case Roles::ServerSpecialJunk2Role: return m_protocolConfig.serverConfig.specialJunk2;
    case Roles::ServerSpecialJunk3Role: return m_protocolConfig.serverConfig.specialJunk3;
    case Roles::ServerSpecialJunk4Role: return m_protocolConfig.serverConfig.specialJunk4;
    case Roles::ServerSpecialJunk5Role: return m_protocolConfig.serverConfig.specialJunk5;

    case Roles::IsAwg2Role: return m_protocolConfig.serverConfig.protocolVersion == protocols::awg::awgV2;
    }

    return QVariant();
}

void AwgConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::AwgProtocolConfig &protocolConfig)
{
    beginResetModel();
    m_container = container;
    
    m_protocolConfig = protocolConfig;
    
    applyDefaultsToServerConfig(m_protocolConfig.serverConfig);
    
    if (!m_protocolConfig.clientConfig.has_value()) {
        m_protocolConfig.clientConfig = amnezia::AwgClientConfig{};
    }
    applyDefaultsToClientConfig(m_protocolConfig.clientConfig.value());

    m_originalProtocolConfig = m_protocolConfig;

    endResetModel();
}

void AwgConfigModel::applyDefaultsToServerConfig(amnezia::AwgServerConfig& config)
{
    if (config.subnetAddress.isEmpty()) {
        config.subnetAddress = protocols::wireguard::defaultSubnetAddress;
    }
    if (config.port.isEmpty()) {
        config.port = protocols::awg::defaultPort;
    }
    if (config.transportProto.isEmpty()) {
        config.transportProto = ProtocolUtils::transportProtoToString(
            ProtocolUtils::defaultTransportProto(amnezia::Proto::Awg), amnezia::Proto::Awg);
    }
    if (config.junkPacketCount.isEmpty()) {
        config.junkPacketCount = protocols::awg::defaultJunkPacketCount;
    }
    if (config.junkPacketMinSize.isEmpty()) {
        config.junkPacketMinSize = protocols::awg::defaultJunkPacketMinSize;
    }
    if (config.junkPacketMaxSize.isEmpty()) {
        config.junkPacketMaxSize = protocols::awg::defaultJunkPacketMaxSize;
    }
    if (config.initPacketJunkSize.isEmpty()) {
        config.initPacketJunkSize = protocols::awg::defaultInitPacketJunkSize;
    }
    if (config.responsePacketJunkSize.isEmpty()) {
        config.responsePacketJunkSize = protocols::awg::defaultResponsePacketJunkSize;
    }
    if (config.protocolVersion == protocols::awg::awgV2) {
        if (config.cookieReplyPacketJunkSize.isEmpty()) {
            config.cookieReplyPacketJunkSize = protocols::awg::defaultCookieReplyPacketJunkSize;
        }
        if (config.transportPacketJunkSize.isEmpty()) {
            config.transportPacketJunkSize = protocols::awg::defaultTransportPacketJunkSize;
        }
    }
    if (config.initPacketMagicHeader.isEmpty()) {
        config.initPacketMagicHeader = protocols::awg::defaultInitPacketMagicHeader;
    }
    if (config.responsePacketMagicHeader.isEmpty()) {
        config.responsePacketMagicHeader = protocols::awg::defaultResponsePacketMagicHeader;
    }
    if (config.underloadPacketMagicHeader.isEmpty()) {
        config.underloadPacketMagicHeader = protocols::awg::defaultUnderloadPacketMagicHeader;
    }
    if (config.transportPacketMagicHeader.isEmpty()) {
        config.transportPacketMagicHeader = protocols::awg::defaultTransportPacketMagicHeader;
    }
    if (config.specialJunk1.isEmpty()) {
        config.specialJunk1 = protocols::awg::defaultSpecialJunk1;
    }
    if (config.specialJunk2.isEmpty()) {
        config.specialJunk2 = protocols::awg::defaultSpecialJunk2;
    }
    if (config.specialJunk3.isEmpty()) {
        config.specialJunk3 = protocols::awg::defaultSpecialJunk3;
    }
    if (config.specialJunk4.isEmpty()) {
        config.specialJunk4 = protocols::awg::defaultSpecialJunk4;
    }
    if (config.specialJunk5.isEmpty()) {
        config.specialJunk5 = protocols::awg::defaultSpecialJunk5;
    }
}

void AwgConfigModel::applyDefaultsToClientConfig(amnezia::AwgClientConfig& config)
{
    if (config.mtu.isEmpty()) {
        config.mtu = protocols::awg::defaultMtu;
    }
    if (config.junkPacketCount.isEmpty()) {
        config.junkPacketCount = m_protocolConfig.serverConfig.junkPacketCount.isEmpty() 
            ? protocols::awg::defaultJunkPacketCount 
            : m_protocolConfig.serverConfig.junkPacketCount;
    }
    if (config.junkPacketMinSize.isEmpty()) {
        config.junkPacketMinSize = m_protocolConfig.serverConfig.junkPacketMinSize.isEmpty()
            ? protocols::awg::defaultJunkPacketMinSize
            : m_protocolConfig.serverConfig.junkPacketMinSize;
    }
    if (config.junkPacketMaxSize.isEmpty()) {
        config.junkPacketMaxSize = m_protocolConfig.serverConfig.junkPacketMaxSize.isEmpty()
            ? protocols::awg::defaultJunkPacketMaxSize
            : m_protocolConfig.serverConfig.junkPacketMaxSize;
    }
    if (config.specialJunk1.isEmpty()) {
        config.specialJunk1 = m_protocolConfig.serverConfig.specialJunk1.isEmpty()
            ? protocols::awg::defaultSpecialJunk1
            : m_protocolConfig.serverConfig.specialJunk1;
    }
    if (config.specialJunk2.isEmpty()) {
        config.specialJunk2 = m_protocolConfig.serverConfig.specialJunk2.isEmpty()
            ? protocols::awg::defaultSpecialJunk2
            : m_protocolConfig.serverConfig.specialJunk2;
    }
    if (config.specialJunk3.isEmpty()) {
        config.specialJunk3 = m_protocolConfig.serverConfig.specialJunk3.isEmpty()
            ? protocols::awg::defaultSpecialJunk3
            : m_protocolConfig.serverConfig.specialJunk3;
    }
    if (config.specialJunk4.isEmpty()) {
        config.specialJunk4 = m_protocolConfig.serverConfig.specialJunk4.isEmpty()
            ? protocols::awg::defaultSpecialJunk4
            : m_protocolConfig.serverConfig.specialJunk4;
    }
    if (config.specialJunk5.isEmpty()) {
        config.specialJunk5 = m_protocolConfig.serverConfig.specialJunk5.isEmpty()
            ? protocols::awg::defaultSpecialJunk5
            : m_protocolConfig.serverConfig.specialJunk5;
    }
}

amnezia::AwgProtocolConfig AwgConfigModel::getProtocolConfig()
{
    bool serverSettingsChanged = !m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig);
    
    if (serverSettingsChanged) {
        m_protocolConfig.clearClientConfig();
    }
    
    if (m_protocolConfig.serverConfig.protocolVersion.isEmpty() || 
        m_protocolConfig.serverConfig.protocolVersion != protocols::awg::awgV2) {
        bool hasSpecialJunk = !m_protocolConfig.serverConfig.specialJunk1.trimmed().isEmpty() ||
                              !m_protocolConfig.serverConfig.specialJunk2.trimmed().isEmpty() ||
                              !m_protocolConfig.serverConfig.specialJunk3.trimmed().isEmpty() ||
                              !m_protocolConfig.serverConfig.specialJunk4.trimmed().isEmpty() ||
                              !m_protocolConfig.serverConfig.specialJunk5.trimmed().isEmpty();
        
        if (hasSpecialJunk) {
            m_protocolConfig.serverConfig.protocolVersion = protocols::awg::awgV1_5;
        } else if (m_protocolConfig.serverConfig.protocolVersion.isEmpty()) {
            m_protocolConfig.serverConfig.protocolVersion = QString();
        }
    }
    
    return m_protocolConfig;
}

bool AwgConfigModel::isServerSettingsEqual()
{
    return m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig);
}

bool AwgConfigModel::isHeadersEqual(const QString &h1, const QString &h2, const QString &h3, const QString &h4)
{
    return amnezia::AwgProtocolConfig::isHeadersEqual(h1, h2, h3, h4);
}

bool AwgConfigModel::isPacketSizeEqual(const int s1, const int s2, const int s3, const int s4)
{
    return amnezia::AwgProtocolConfig::isPacketSizeEqual(s1, s2, s3, s4);
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
    roles[ServerSpecialJunk1Role] = "serverSpecialJunk1";
    roles[ServerSpecialJunk2Role] = "serverSpecialJunk2";
    roles[ServerSpecialJunk3Role] = "serverSpecialJunk3";
    roles[ServerSpecialJunk4Role] = "serverSpecialJunk4";
    roles[ServerSpecialJunk5Role] = "serverSpecialJunk5";

    roles[IsAwg2Role] = "isAwg2";

    return roles;
}

