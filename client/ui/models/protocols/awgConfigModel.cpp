#include "awgConfigModel.h"

#include <QJsonDocument>

#include "core/configurators/wireguardConfigurator.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/protocols/awgProtocolConfig.h"

using namespace amnezia;
using namespace ProtocolUtils;

namespace
{
    QString awgToggleValue(bool enabled)
    {
        return enabled ? QString(protocols::awg::awgBoolOn) : QString();
    }
} // namespace

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
    case Roles::ClientContentPaddingAdditionRole: m_protocolConfig.clientConfig->contentPaddingAddition = strValue; break;
    case Roles::ClientRekeyAfterTimeRole: m_protocolConfig.clientConfig->rekeyAfterTime = strValue; break;
    case Roles::ClientRekeyTimeoutRole: m_protocolConfig.clientConfig->rekeyTimeout = strValue; break;
    case Roles::ClientRejectAfterTimeRole: m_protocolConfig.clientConfig->rejectAfterTime = strValue; break;
    case Roles::ClientKeepaliveTimeoutRole: m_protocolConfig.clientConfig->keepaliveTimeout = strValue; break;
    case Roles::ClientMaxHandshakeAttemptsRole: m_protocolConfig.clientConfig->maxHandshakeAttempts = strValue; break;
    case Roles::ClientRandomTrailersRole:
        m_protocolConfig.clientConfig->randomTrailers = awgToggleValue(value.toBool());
        break;
    case Roles::ClientDisableCookiesRole:
        m_protocolConfig.clientConfig->disableCookies = awgToggleValue(value.toBool());
        break;
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
    case Roles::ServerContentPaddingAdditionRole: m_protocolConfig.serverConfig.contentPaddingAddition = strValue; break;
    case Roles::ServerRekeyAfterTimeRole: m_protocolConfig.serverConfig.rekeyAfterTime = strValue; break;
    case Roles::ServerRekeyTimeoutRole: m_protocolConfig.serverConfig.rekeyTimeout = strValue; break;
    case Roles::ServerRejectAfterTimeRole: m_protocolConfig.serverConfig.rejectAfterTime = strValue; break;
    case Roles::ServerKeepaliveTimeoutRole: m_protocolConfig.serverConfig.keepaliveTimeout = strValue; break;
    case Roles::ServerMaxHandshakeAttemptsRole: m_protocolConfig.serverConfig.maxHandshakeAttempts = strValue; break;
    case Roles::ServerHeaderProtectionEnabledRole: {
        if (value.toBool()) {
            if (m_protocolConfig.serverConfig.headerProtectionKey.isEmpty()) {
                const QString originalKey = m_originalProtocolConfig.serverConfig.headerProtectionKey;
                m_protocolConfig.serverConfig.headerProtectionKey =
                        originalKey.isEmpty() ? WireguardConfigurator::genClientKeys().clientPrivKey : originalKey;
            }
        } else {
            m_protocolConfig.serverConfig.headerProtectionKey.clear();
        }
        break;
    }
    case Roles::ServerRandomTrailersRole:
        m_protocolConfig.serverConfig.randomTrailers = awgToggleValue(value.toBool());
        break;
    case Roles::ServerDisableCookiesRole:
        m_protocolConfig.serverConfig.disableCookies = awgToggleValue(value.toBool());
        break;
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
    case Roles::ClientContentPaddingAdditionRole: return m_protocolConfig.clientConfig->contentPaddingAddition;
    case Roles::ClientRekeyAfterTimeRole: return m_protocolConfig.clientConfig->rekeyAfterTime;
    case Roles::ClientRekeyTimeoutRole: return m_protocolConfig.clientConfig->rekeyTimeout;
    case Roles::ClientRejectAfterTimeRole: return m_protocolConfig.clientConfig->rejectAfterTime;
    case Roles::ClientKeepaliveTimeoutRole: return m_protocolConfig.clientConfig->keepaliveTimeout;
    case Roles::ClientMaxHandshakeAttemptsRole: return m_protocolConfig.clientConfig->maxHandshakeAttempts;
    case Roles::ClientHeaderProtectionEnabledRole: return !m_protocolConfig.clientConfig->headerProtectionKey.isEmpty();
    case Roles::ClientRandomTrailersRole:
        return m_protocolConfig.clientConfig->randomTrailers.compare(QLatin1String(protocols::awg::awgBoolOn),
                                                                     Qt::CaseInsensitive)
                == 0;
    case Roles::ClientDisableCookiesRole:
        return m_protocolConfig.clientConfig->disableCookies.compare(QLatin1String(protocols::awg::awgBoolOn),
                                                                     Qt::CaseInsensitive)
                == 0;

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

    case Roles::ServerContentPaddingAdditionRole: return m_protocolConfig.serverConfig.contentPaddingAddition;
    case Roles::ServerRekeyAfterTimeRole: return m_protocolConfig.serverConfig.rekeyAfterTime;
    case Roles::ServerRekeyTimeoutRole: return m_protocolConfig.serverConfig.rekeyTimeout;
    case Roles::ServerRejectAfterTimeRole: return m_protocolConfig.serverConfig.rejectAfterTime;
    case Roles::ServerKeepaliveTimeoutRole: return m_protocolConfig.serverConfig.keepaliveTimeout;
    case Roles::ServerMaxHandshakeAttemptsRole: return m_protocolConfig.serverConfig.maxHandshakeAttempts;
    case Roles::ServerHeaderProtectionEnabledRole: return !m_protocolConfig.serverConfig.headerProtectionKey.isEmpty();
    case Roles::ServerRandomTrailersRole:
        return m_protocolConfig.serverConfig.randomTrailers.compare(QLatin1String(protocols::awg::awgBoolOn),
                                                                    Qt::CaseInsensitive)
                == 0;
    case Roles::ServerDisableCookiesRole:
        return m_protocolConfig.serverConfig.disableCookies.compare(QLatin1String(protocols::awg::awgBoolOn),
                                                                    Qt::CaseInsensitive)
                == 0;

    case Roles::IsAwg2Role: {
        QString version = serverProtocolVersion();
        return version == protocols::awg::awgV2 || version == protocols::awg::awgV3;
    }
    case Roles::IsAwg3Role: return serverProtocolVersion() == protocols::awg::awgV3;
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

QString AwgConfigModel::serverProtocolVersion() const
{
    return m_protocolConfig.serverConfig.protocolVersion;
}

void AwgConfigModel::applyDefaultsToServerConfig(amnezia::AwgServerConfig& config)
{
    if (config.subnetAddress.isEmpty()) {
        config.subnetAddress = protocols::wireguard::defaultSubnetAddress;
    }
}

void AwgConfigModel::applyDefaultsToClientConfig(amnezia::AwgClientConfig& config)
{
    if (config.mtu.isEmpty()) {
        config.mtu = protocols::awg::defaultMtu;
    }
}

amnezia::AwgProtocolConfig AwgConfigModel::getProtocolConfig()
{
    bool serverSettingsChanged = !m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig);
    
    if (serverSettingsChanged) {
        m_protocolConfig.clearClientConfig();
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
    roles[ClientContentPaddingAdditionRole] = "clientContentPaddingAddition";
    roles[ClientRekeyAfterTimeRole] = "clientRekeyAfterTime";
    roles[ClientRekeyTimeoutRole] = "clientRekeyTimeout";
    roles[ClientRejectAfterTimeRole] = "clientRejectAfterTime";
    roles[ClientKeepaliveTimeoutRole] = "clientKeepaliveTimeout";
    roles[ClientMaxHandshakeAttemptsRole] = "clientMaxHandshakeAttempts";
    roles[ClientHeaderProtectionEnabledRole] = "clientHeaderProtectionEnabled";
    roles[ClientRandomTrailersRole] = "clientRandomTrailers";
    roles[ClientDisableCookiesRole] = "clientDisableCookies";

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

    roles[ServerContentPaddingAdditionRole] = "serverContentPaddingAddition";
    roles[ServerRekeyAfterTimeRole] = "serverRekeyAfterTime";
    roles[ServerRekeyTimeoutRole] = "serverRekeyTimeout";
    roles[ServerRejectAfterTimeRole] = "serverRejectAfterTime";
    roles[ServerKeepaliveTimeoutRole] = "serverKeepaliveTimeout";
    roles[ServerMaxHandshakeAttemptsRole] = "serverMaxHandshakeAttempts";
    roles[ServerHeaderProtectionEnabledRole] = "serverHeaderProtectionEnabled";
    roles[ServerRandomTrailersRole] = "serverRandomTrailers";
    roles[ServerDisableCookiesRole] = "serverDisableCookies";

    roles[IsAwg2Role] = "isAwg2";
    roles[IsAwg3Role] = "isAwg3";

    return roles;
}

