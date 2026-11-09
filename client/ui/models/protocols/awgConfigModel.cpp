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

    bool isAwgToggleOn(const QString &value)
    {
        return value.compare(QLatin1String(protocols::awg::awgBoolOn), Qt::CaseInsensitive) == 0;
    }
} // namespace

AwgConfigModel::AwgConfigModel(QObject *parent) : QObject(parent)
{
}

QString AwgConfigModel::subnetAddress() const
{
    return m_protocolConfig.serverConfig.subnetAddress;
}

void AwgConfigModel::setSubnetAddress(const QString &value)
{
    if (m_protocolConfig.serverConfig.subnetAddress == value) {
        return;
    }
    m_protocolConfig.serverConfig.subnetAddress = value;
    emit changed();
}

QString AwgConfigModel::port() const
{
    return m_protocolConfig.serverConfig.port;
}

void AwgConfigModel::setPort(const QString &value)
{
    if (m_protocolConfig.serverConfig.port == value) {
        return;
    }
    m_protocolConfig.serverConfig.port = value;
    emit changed();
}

QString AwgConfigModel::clientMtu() const
{
    return m_protocolConfig.clientConfig->mtu;
}

void AwgConfigModel::setClientMtu(const QString &value)
{
    if (m_protocolConfig.clientConfig->mtu == value) {
        return;
    }
    m_protocolConfig.clientConfig->mtu = value;
    emit changed();
}

QString AwgConfigModel::clientJunkPacketCount() const
{
    return m_protocolConfig.clientConfig->junkPacketCount;
}

void AwgConfigModel::setClientJunkPacketCount(const QString &value)
{
    if (m_protocolConfig.clientConfig->junkPacketCount == value) {
        return;
    }
    m_protocolConfig.clientConfig->junkPacketCount = value;
    emit changed();
}

QString AwgConfigModel::clientJunkPacketMinSize() const
{
    return m_protocolConfig.clientConfig->junkPacketMinSize;
}

void AwgConfigModel::setClientJunkPacketMinSize(const QString &value)
{
    if (m_protocolConfig.clientConfig->junkPacketMinSize == value) {
        return;
    }
    m_protocolConfig.clientConfig->junkPacketMinSize = value;
    emit changed();
}

QString AwgConfigModel::clientJunkPacketMaxSize() const
{
    return m_protocolConfig.clientConfig->junkPacketMaxSize;
}

void AwgConfigModel::setClientJunkPacketMaxSize(const QString &value)
{
    if (m_protocolConfig.clientConfig->junkPacketMaxSize == value) {
        return;
    }
    m_protocolConfig.clientConfig->junkPacketMaxSize = value;
    emit changed();
}

QString AwgConfigModel::clientSpecialJunk1() const
{
    return m_protocolConfig.clientConfig->specialJunk1;
}

void AwgConfigModel::setClientSpecialJunk1(const QString &value)
{
    if (m_protocolConfig.clientConfig->specialJunk1 == value) {
        return;
    }
    m_protocolConfig.clientConfig->specialJunk1 = value;
    emit changed();
}

QString AwgConfigModel::clientSpecialJunk2() const
{
    return m_protocolConfig.clientConfig->specialJunk2;
}

void AwgConfigModel::setClientSpecialJunk2(const QString &value)
{
    if (m_protocolConfig.clientConfig->specialJunk2 == value) {
        return;
    }
    m_protocolConfig.clientConfig->specialJunk2 = value;
    emit changed();
}

QString AwgConfigModel::clientSpecialJunk3() const
{
    return m_protocolConfig.clientConfig->specialJunk3;
}

void AwgConfigModel::setClientSpecialJunk3(const QString &value)
{
    if (m_protocolConfig.clientConfig->specialJunk3 == value) {
        return;
    }
    m_protocolConfig.clientConfig->specialJunk3 = value;
    emit changed();
}

QString AwgConfigModel::clientSpecialJunk4() const
{
    return m_protocolConfig.clientConfig->specialJunk4;
}

void AwgConfigModel::setClientSpecialJunk4(const QString &value)
{
    if (m_protocolConfig.clientConfig->specialJunk4 == value) {
        return;
    }
    m_protocolConfig.clientConfig->specialJunk4 = value;
    emit changed();
}

QString AwgConfigModel::clientSpecialJunk5() const
{
    return m_protocolConfig.clientConfig->specialJunk5;
}

void AwgConfigModel::setClientSpecialJunk5(const QString &value)
{
    if (m_protocolConfig.clientConfig->specialJunk5 == value) {
        return;
    }
    m_protocolConfig.clientConfig->specialJunk5 = value;
    emit changed();
}

QString AwgConfigModel::clientContentPaddingAddition() const
{
    return m_protocolConfig.clientConfig->contentPaddingAddition;
}

void AwgConfigModel::setClientContentPaddingAddition(const QString &value)
{
    if (m_protocolConfig.clientConfig->contentPaddingAddition == value) {
        return;
    }
    m_protocolConfig.clientConfig->contentPaddingAddition = value;
    emit changed();
}

QString AwgConfigModel::clientRekeyAfterTime() const
{
    return m_protocolConfig.clientConfig->rekeyAfterTime;
}

void AwgConfigModel::setClientRekeyAfterTime(const QString &value)
{
    if (m_protocolConfig.clientConfig->rekeyAfterTime == value) {
        return;
    }
    m_protocolConfig.clientConfig->rekeyAfterTime = value;
    emit changed();
}

QString AwgConfigModel::clientRekeyTimeout() const
{
    return m_protocolConfig.clientConfig->rekeyTimeout;
}

void AwgConfigModel::setClientRekeyTimeout(const QString &value)
{
    if (m_protocolConfig.clientConfig->rekeyTimeout == value) {
        return;
    }
    m_protocolConfig.clientConfig->rekeyTimeout = value;
    emit changed();
}

QString AwgConfigModel::clientRejectAfterTime() const
{
    return m_protocolConfig.clientConfig->rejectAfterTime;
}

void AwgConfigModel::setClientRejectAfterTime(const QString &value)
{
    if (m_protocolConfig.clientConfig->rejectAfterTime == value) {
        return;
    }
    m_protocolConfig.clientConfig->rejectAfterTime = value;
    emit changed();
}

QString AwgConfigModel::clientKeepaliveTimeout() const
{
    return m_protocolConfig.clientConfig->keepaliveTimeout;
}

void AwgConfigModel::setClientKeepaliveTimeout(const QString &value)
{
    if (m_protocolConfig.clientConfig->keepaliveTimeout == value) {
        return;
    }
    m_protocolConfig.clientConfig->keepaliveTimeout = value;
    emit changed();
}

QString AwgConfigModel::clientMaxHandshakeAttempts() const
{
    return m_protocolConfig.clientConfig->maxHandshakeAttempts;
}

void AwgConfigModel::setClientMaxHandshakeAttempts(const QString &value)
{
    if (m_protocolConfig.clientConfig->maxHandshakeAttempts == value) {
        return;
    }
    m_protocolConfig.clientConfig->maxHandshakeAttempts = value;
    emit changed();
}

bool AwgConfigModel::clientHeaderProtectionEnabled() const
{
    return !m_protocolConfig.clientConfig->headerProtectionKey.isEmpty();
}

bool AwgConfigModel::clientRandomTrailers() const
{
    return isAwgToggleOn(m_protocolConfig.clientConfig->randomTrailers);
}

void AwgConfigModel::setClientRandomTrailers(bool value)
{
    if (clientRandomTrailers() == value) {
        return;
    }
    m_protocolConfig.clientConfig->randomTrailers = awgToggleValue(value);
    emit changed();
}

bool AwgConfigModel::clientDisableCookies() const
{
    return isAwgToggleOn(m_protocolConfig.clientConfig->disableCookies);
}

void AwgConfigModel::setClientDisableCookies(bool value)
{
    if (clientDisableCookies() == value) {
        return;
    }
    m_protocolConfig.clientConfig->disableCookies = awgToggleValue(value);
    emit changed();
}

QString AwgConfigModel::serverJunkPacketCount() const
{
    return m_protocolConfig.serverConfig.junkPacketCount;
}

void AwgConfigModel::setServerJunkPacketCount(const QString &value)
{
    if (m_protocolConfig.serverConfig.junkPacketCount == value) {
        return;
    }
    m_protocolConfig.serverConfig.junkPacketCount = value;
    emit changed();
}

QString AwgConfigModel::serverJunkPacketMinSize() const
{
    return m_protocolConfig.serverConfig.junkPacketMinSize;
}

void AwgConfigModel::setServerJunkPacketMinSize(const QString &value)
{
    if (m_protocolConfig.serverConfig.junkPacketMinSize == value) {
        return;
    }
    m_protocolConfig.serverConfig.junkPacketMinSize = value;
    emit changed();
}

QString AwgConfigModel::serverJunkPacketMaxSize() const
{
    return m_protocolConfig.serverConfig.junkPacketMaxSize;
}

void AwgConfigModel::setServerJunkPacketMaxSize(const QString &value)
{
    if (m_protocolConfig.serverConfig.junkPacketMaxSize == value) {
        return;
    }
    m_protocolConfig.serverConfig.junkPacketMaxSize = value;
    emit changed();
}

QString AwgConfigModel::serverInitPacketJunkSize() const
{
    return m_protocolConfig.serverConfig.initPacketJunkSize;
}

void AwgConfigModel::setServerInitPacketJunkSize(const QString &value)
{
    if (m_protocolConfig.serverConfig.initPacketJunkSize == value) {
        return;
    }
    m_protocolConfig.serverConfig.initPacketJunkSize = value;
    emit changed();
}

QString AwgConfigModel::serverResponsePacketJunkSize() const
{
    return m_protocolConfig.serverConfig.responsePacketJunkSize;
}

void AwgConfigModel::setServerResponsePacketJunkSize(const QString &value)
{
    if (m_protocolConfig.serverConfig.responsePacketJunkSize == value) {
        return;
    }
    m_protocolConfig.serverConfig.responsePacketJunkSize = value;
    emit changed();
}

QString AwgConfigModel::serverCookieReplyPacketJunkSize() const
{
    return m_protocolConfig.serverConfig.cookieReplyPacketJunkSize;
}

void AwgConfigModel::setServerCookieReplyPacketJunkSize(const QString &value)
{
    if (m_protocolConfig.serverConfig.cookieReplyPacketJunkSize == value) {
        return;
    }
    m_protocolConfig.serverConfig.cookieReplyPacketJunkSize = value;
    emit changed();
}

QString AwgConfigModel::serverTransportPacketJunkSize() const
{
    return m_protocolConfig.serverConfig.transportPacketJunkSize;
}

void AwgConfigModel::setServerTransportPacketJunkSize(const QString &value)
{
    if (m_protocolConfig.serverConfig.transportPacketJunkSize == value) {
        return;
    }
    m_protocolConfig.serverConfig.transportPacketJunkSize = value;
    emit changed();
}

QString AwgConfigModel::serverInitPacketMagicHeader() const
{
    return m_protocolConfig.serverConfig.initPacketMagicHeader;
}

void AwgConfigModel::setServerInitPacketMagicHeader(const QString &value)
{
    if (m_protocolConfig.serverConfig.initPacketMagicHeader == value) {
        return;
    }
    m_protocolConfig.serverConfig.initPacketMagicHeader = value;
    emit changed();
}

QString AwgConfigModel::serverResponsePacketMagicHeader() const
{
    return m_protocolConfig.serverConfig.responsePacketMagicHeader;
}

void AwgConfigModel::setServerResponsePacketMagicHeader(const QString &value)
{
    if (m_protocolConfig.serverConfig.responsePacketMagicHeader == value) {
        return;
    }
    m_protocolConfig.serverConfig.responsePacketMagicHeader = value;
    emit changed();
}

QString AwgConfigModel::serverUnderloadPacketMagicHeader() const
{
    return m_protocolConfig.serverConfig.underloadPacketMagicHeader;
}

void AwgConfigModel::setServerUnderloadPacketMagicHeader(const QString &value)
{
    if (m_protocolConfig.serverConfig.underloadPacketMagicHeader == value) {
        return;
    }
    m_protocolConfig.serverConfig.underloadPacketMagicHeader = value;
    emit changed();
}

QString AwgConfigModel::serverTransportPacketMagicHeader() const
{
    return m_protocolConfig.serverConfig.transportPacketMagicHeader;
}

void AwgConfigModel::setServerTransportPacketMagicHeader(const QString &value)
{
    if (m_protocolConfig.serverConfig.transportPacketMagicHeader == value) {
        return;
    }
    m_protocolConfig.serverConfig.transportPacketMagicHeader = value;
    emit changed();
}

QString AwgConfigModel::serverSpecialJunk1() const
{
    return m_protocolConfig.serverConfig.specialJunk1;
}

void AwgConfigModel::setServerSpecialJunk1(const QString &value)
{
    if (m_protocolConfig.serverConfig.specialJunk1 == value) {
        return;
    }
    m_protocolConfig.serverConfig.specialJunk1 = value;
    emit changed();
}

QString AwgConfigModel::serverSpecialJunk2() const
{
    return m_protocolConfig.serverConfig.specialJunk2;
}

void AwgConfigModel::setServerSpecialJunk2(const QString &value)
{
    if (m_protocolConfig.serverConfig.specialJunk2 == value) {
        return;
    }
    m_protocolConfig.serverConfig.specialJunk2 = value;
    emit changed();
}

QString AwgConfigModel::serverSpecialJunk3() const
{
    return m_protocolConfig.serverConfig.specialJunk3;
}

void AwgConfigModel::setServerSpecialJunk3(const QString &value)
{
    if (m_protocolConfig.serverConfig.specialJunk3 == value) {
        return;
    }
    m_protocolConfig.serverConfig.specialJunk3 = value;
    emit changed();
}

QString AwgConfigModel::serverSpecialJunk4() const
{
    return m_protocolConfig.serverConfig.specialJunk4;
}

void AwgConfigModel::setServerSpecialJunk4(const QString &value)
{
    if (m_protocolConfig.serverConfig.specialJunk4 == value) {
        return;
    }
    m_protocolConfig.serverConfig.specialJunk4 = value;
    emit changed();
}

QString AwgConfigModel::serverSpecialJunk5() const
{
    return m_protocolConfig.serverConfig.specialJunk5;
}

void AwgConfigModel::setServerSpecialJunk5(const QString &value)
{
    if (m_protocolConfig.serverConfig.specialJunk5 == value) {
        return;
    }
    m_protocolConfig.serverConfig.specialJunk5 = value;
    emit changed();
}

QString AwgConfigModel::serverContentPaddingAddition() const
{
    return m_protocolConfig.serverConfig.contentPaddingAddition;
}

void AwgConfigModel::setServerContentPaddingAddition(const QString &value)
{
    if (m_protocolConfig.serverConfig.contentPaddingAddition == value) {
        return;
    }
    m_protocolConfig.serverConfig.contentPaddingAddition = value;
    emit changed();
}

QString AwgConfigModel::serverRekeyAfterTime() const
{
    return m_protocolConfig.serverConfig.rekeyAfterTime;
}

void AwgConfigModel::setServerRekeyAfterTime(const QString &value)
{
    if (m_protocolConfig.serverConfig.rekeyAfterTime == value) {
        return;
    }
    m_protocolConfig.serverConfig.rekeyAfterTime = value;
    emit changed();
}

QString AwgConfigModel::serverRekeyTimeout() const
{
    return m_protocolConfig.serverConfig.rekeyTimeout;
}

void AwgConfigModel::setServerRekeyTimeout(const QString &value)
{
    if (m_protocolConfig.serverConfig.rekeyTimeout == value) {
        return;
    }
    m_protocolConfig.serverConfig.rekeyTimeout = value;
    emit changed();
}

QString AwgConfigModel::serverRejectAfterTime() const
{
    return m_protocolConfig.serverConfig.rejectAfterTime;
}

void AwgConfigModel::setServerRejectAfterTime(const QString &value)
{
    if (m_protocolConfig.serverConfig.rejectAfterTime == value) {
        return;
    }
    m_protocolConfig.serverConfig.rejectAfterTime = value;
    emit changed();
}

QString AwgConfigModel::serverKeepaliveTimeout() const
{
    return m_protocolConfig.serverConfig.keepaliveTimeout;
}

void AwgConfigModel::setServerKeepaliveTimeout(const QString &value)
{
    if (m_protocolConfig.serverConfig.keepaliveTimeout == value) {
        return;
    }
    m_protocolConfig.serverConfig.keepaliveTimeout = value;
    emit changed();
}

QString AwgConfigModel::serverMaxHandshakeAttempts() const
{
    return m_protocolConfig.serverConfig.maxHandshakeAttempts;
}

void AwgConfigModel::setServerMaxHandshakeAttempts(const QString &value)
{
    if (m_protocolConfig.serverConfig.maxHandshakeAttempts == value) {
        return;
    }
    m_protocolConfig.serverConfig.maxHandshakeAttempts = value;
    emit changed();
}

bool AwgConfigModel::serverHeaderProtectionEnabled() const
{
    return !m_protocolConfig.serverConfig.headerProtectionKey.isEmpty();
}

void AwgConfigModel::setServerHeaderProtectionEnabled(bool value)
{
    if (serverHeaderProtectionEnabled() == value) {
        return;
    }
    if (value) {
        const QString originalKey = m_originalProtocolConfig.serverConfig.headerProtectionKey;
        m_protocolConfig.serverConfig.headerProtectionKey =
                originalKey.isEmpty() ? WireguardConfigurator::genClientKeys().clientPrivKey : originalKey;
    } else {
        m_protocolConfig.serverConfig.headerProtectionKey.clear();
    }
    emit changed();
}

bool AwgConfigModel::serverRandomTrailers() const
{
    return isAwgToggleOn(m_protocolConfig.serverConfig.randomTrailers);
}

void AwgConfigModel::setServerRandomTrailers(bool value)
{
    if (serverRandomTrailers() == value) {
        return;
    }
    m_protocolConfig.serverConfig.randomTrailers = awgToggleValue(value);
    emit changed();
}

bool AwgConfigModel::serverDisableCookies() const
{
    return isAwgToggleOn(m_protocolConfig.serverConfig.disableCookies);
}

void AwgConfigModel::setServerDisableCookies(bool value)
{
    if (serverDisableCookies() == value) {
        return;
    }
    m_protocolConfig.serverConfig.disableCookies = awgToggleValue(value);
    emit changed();
}

bool AwgConfigModel::isAwg2() const
{
    QString version = serverProtocolVersion();
    return version == protocols::awg::awgV2 || version == protocols::awg::awgV3;
}

bool AwgConfigModel::isAwg3() const
{
    return serverProtocolVersion() == protocols::awg::awgV3;
}

void AwgConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::AwgProtocolConfig &protocolConfig)
{
    m_container = container;
    
    m_protocolConfig = protocolConfig;
    
    applyDefaultsToServerConfig(m_protocolConfig.serverConfig);

    if (!m_protocolConfig.clientConfig.has_value()) {
        m_protocolConfig.clientConfig = amnezia::AwgClientConfig{};
    }
    applyDefaultsToClientConfig(m_protocolConfig.clientConfig.value());

    m_originalProtocolConfig = m_protocolConfig;

    emit changed();
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
