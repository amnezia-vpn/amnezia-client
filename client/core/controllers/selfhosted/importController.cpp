#include "importController.h"

#include <QDataStream>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMap>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QUrl>
#include <algorithm>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/serialization/serialization.h"
#include "core/utils/utilities.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/qrCodeUtils.h"

using namespace amnezia;
using namespace ProtocolUtils;

namespace
{
    ConfigTypes checkConfigFormat(const QString &config)
    {
        const QString openVpnConfigPatternCli = "client";
        const QString openVpnConfigPatternDriver1 = "dev tun";
        const QString openVpnConfigPatternDriver2 = "dev tap";

        const QString wireguardConfigPatternSectionInterface = "[Interface]";
        const QString wireguardConfigPatternSectionPeer = "[Peer]";

        const QString xrayConfigPatternInbound = "inbounds";
        const QString xrayConfigPatternOutbound = "outbounds";

        const QString amneziaConfigPattern = "containers";
        const QString amneziaConfigPatternHostName = "hostName";
        const QString amneziaConfigPatternUserName = "userName";
        const QString amneziaConfigPatternPassword = "password";
        const QString amneziaFreeConfigPattern = "api_key";
        const QString amneziaPremiumConfigPattern = "auth_data";
        const QString backupPattern = "Servers/serversList";

        if (config.contains(backupPattern)) {
            return ConfigTypes::Backup;
        } else if (config.contains(amneziaConfigPattern) || config.contains(amneziaFreeConfigPattern)
                   || config.contains(amneziaPremiumConfigPattern)
                   || (config.contains(amneziaConfigPatternHostName) && config.contains(amneziaConfigPatternUserName)
                       && config.contains(amneziaConfigPatternPassword))) {
            return ConfigTypes::Amnezia;
        } else if (config.contains(wireguardConfigPatternSectionInterface) && config.contains(wireguardConfigPatternSectionPeer)) {
            return ConfigTypes::WireGuard;
        } else if ((config.contains(xrayConfigPatternInbound)) && (config.contains(xrayConfigPatternOutbound))) {
            return ConfigTypes::Xray;
        } else if (config.contains(openVpnConfigPatternCli)
                   && (config.contains(openVpnConfigPatternDriver1) || config.contains(openVpnConfigPatternDriver2))) {
            return ConfigTypes::OpenVpn;
        }
        return ConfigTypes::Invalid;
    }
} // namespace

ImportController::ImportController(SecureServersRepository* serversRepository,
                                   SecureAppSettingsRepository* appSettingsRepository,
                                   QObject *parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository)
{
}

ImportController::ImportResult ImportController::extractConfigFromData(const QString &data, const QString &configFileName)
{
    ImportResult result;
    result.configFileName = configFileName;
    result.maliciousWarningText.clear();

    QString config = data;
    QString prefix;
    QString errormsg;
    ConfigTypes configType = ConfigTypes::Invalid;

    if (config.startsWith("vless://")) {
        configType = ConfigTypes::Xray;
        result.config = extractXrayConfig(
                Utils::JsonToString(serialization::vless::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact),
                configType, prefix);
        if (!result.config.empty()) {
            result.configType = configType;
            return result;
        }
    }

    if (config.startsWith("vmess://") && config.contains("@")) {
        configType = ConfigTypes::Xray;
        result.config = extractXrayConfig(
                Utils::JsonToString(serialization::vmess_new::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact),
                configType, prefix);
        if (!result.config.empty()) {
            result.configType = configType;
            return result;
        }
    }

    if (config.startsWith("vmess://")) {
        configType = ConfigTypes::Xray;
        result.config = extractXrayConfig(
                Utils::JsonToString(serialization::vmess::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact),
                configType, prefix);
        if (!result.config.empty()) {
            result.configType = configType;
            return result;
        }
    }

    if (config.startsWith("trojan://")) {
        configType = ConfigTypes::Xray;
        result.config = extractXrayConfig(
                Utils::JsonToString(serialization::trojan::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact),
                configType, prefix);
        if (!result.config.empty()) {
            result.configType = configType;
            return result;
        }
    }

    if (config.startsWith("ss://") && !config.contains("plugin=")) {
        configType = ConfigTypes::ShadowSocks;
        result.config = extractXrayConfig(
                Utils::JsonToString(serialization::ss::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact),
                configType, prefix);
        if (!result.config.empty()) {
            result.configType = configType;
            return result;
        }
    }

    if (config.startsWith("ssd://")) {
        QStringList tmp;
        QList<std::pair<QString, QJsonObject>> servers = serialization::ssd::Deserialize(config, &prefix, &tmp);
        configType = ConfigTypes::ShadowSocks;
        // Took only first config from list
        if (!servers.isEmpty()) {
            result.config = extractXrayConfig(servers.first().first, configType);
        }
        if (!result.config.empty()) {
            result.configType = configType;
            return result;
        }
    }

    configType = checkConfigFormat(config);
    if (configType == ConfigTypes::Invalid) {
        config.replace("vpn://", "");
        QByteArray ba = QByteArray::fromBase64(config.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        QByteArray baUncompressed = qUncompress(ba);
        if (!baUncompressed.isEmpty()) {
            ba = baUncompressed;
        }

        config = ba;
        configType = checkConfigFormat(config);
    }

    result.configType = configType;

    switch (configType) {
    case ConfigTypes::OpenVpn: {
        result.config = extractOpenVpnConfig(config);
        if (!result.config.empty()) {
            checkForMaliciousStrings(result.config, result.maliciousWarningText);
            return result;
        }
        result.errorCode = ErrorCode::ImportInvalidConfigError;
        return result;
    }
    case ConfigTypes::Awg:
    case ConfigTypes::WireGuard: {
        result.config = extractWireGuardConfig(config, result.configType);
        result.isNativeWireGuardConfig = (result.configType == ConfigTypes::WireGuard);
        if (!result.config.empty()) {
            return result;
        }
        result.errorCode = ErrorCode::ImportInvalidConfigError;
        return result;
    }
    case ConfigTypes::Xray: {
        result.config = extractXrayConfig(config, configType);
        if (!result.config.empty()) {
            return result;
        }
        result.errorCode = ErrorCode::ImportInvalidConfigError;
        return result;
    }
    case ConfigTypes::Amnezia: {
        result.config = QJsonDocument::fromJson(config.toUtf8()).object();

        if (serverConfigUtils::isServerFromApi(result.config)) {
            auto apiConfig = result.config.value(apiDefs::key::apiConfig).toObject();
            apiConfig[apiDefs::key::vpnKey] = data;
            result.config[apiDefs::key::apiConfig] = apiConfig;
        }

        if (serverConfigUtils::isLegacyApiSubscription(serverConfigUtils::configTypeFromJson(result.config))) {
            result.errorCode = ErrorCode::LegacyApiV1NotSupportedError;
            result.config = {};
            return result;
        }

        processAmneziaConfig(result.config);
        if (!result.config.empty()) {
            checkForMaliciousStrings(result.config, result.maliciousWarningText);
            return result;
        }
        result.errorCode = ErrorCode::ImportInvalidConfigError;
        return result;
    }
    case ConfigTypes::Backup: {
        result.errorCode = ErrorCode::ImportBackupFileUseRestoreInstead;
        return result;
    }
    case ConfigTypes::Invalid: {
        result.errorCode = ErrorCode::ImportInvalidConfigError;
        result.configFileName.clear();
        return result;
    }
    }
    
    result.errorCode = ErrorCode::ImportInvalidConfigError;
    return result;
}

ImportController::ImportResult ImportController::extractConfigFromQr(const QByteArray &data)
{
    ImportResult result;

    QString dataStr = QString::fromUtf8(data);
    ConfigTypes configType = checkConfigFormat(dataStr);
    if (configType != ConfigTypes::Invalid) {
        return extractConfigFromData(dataStr, "");
    }

    QJsonObject dataObj = QJsonDocument::fromJson(data).object();
    if (!dataObj.isEmpty()) {
        result.config = dataObj;
        result.configType = ConfigTypes::Amnezia;
        return result;
    }

    QByteArray ba_uncompressed = qUncompress(data);
    if (!ba_uncompressed.isEmpty()) {
        result.config = QJsonDocument::fromJson(ba_uncompressed).object();
        if (result.config.isEmpty()) {
            result.errorCode = ErrorCode::ImportInvalidConfigError;
            return result;
        }
        result.configType = ConfigTypes::Amnezia;
        return result;
    }

    QByteArray ba = QByteArray::fromBase64(data, QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QByteArray baUncompressed = qUncompress(ba);

    if (!baUncompressed.isEmpty()) {
        ba = baUncompressed;
    }

    if (!ba.isEmpty()) {
        result.config = QJsonDocument::fromJson(ba).object();
        if (result.config.isEmpty()) {
            result.errorCode = ErrorCode::ImportInvalidConfigError;
            return result;
        }
        result.configType = ConfigTypes::Amnezia;
        return result;
    }

    result.errorCode = ErrorCode::ImportInvalidConfigError;
    return result;
}

void ImportController::startDecodingQr()
{
    m_qrCodeChunks.clear();
    m_totalQrCodeChunksCount = 0;
    m_receivedQrCodeChunksCount = 0;
    m_isQrCodeProcessed = true;
}

ImportController::QrParseResult ImportController::parseQrCodeChunk(const QString &code)
{
    QrParseResult parseResult;
    parseResult.chunksReceived = m_receivedQrCodeChunksCount;
    parseResult.chunksTotal = m_totalQrCodeChunksCount;

    if (!m_isQrCodeProcessed) {
        return parseResult;
    }

    QByteArray ba = QByteArray::fromBase64(code.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QDataStream s(&ba, QIODevice::ReadOnly);
    qint16 magic;
    s >> magic;

    if (magic == qrCodeUtils::qrMagicCode) {
        quint8 chunksCount;
        s >> chunksCount;
        if (m_totalQrCodeChunksCount != chunksCount) {
            m_qrCodeChunks.clear();
        }

        m_totalQrCodeChunksCount = chunksCount;

        quint8 chunkId;
        s >> chunkId;
        s >> m_qrCodeChunks[chunkId];
        m_receivedQrCodeChunksCount = m_qrCodeChunks.size();
        parseResult.chunksReceived = m_receivedQrCodeChunksCount;
        parseResult.chunksTotal = m_totalQrCodeChunksCount;

        if (m_qrCodeChunks.size() == m_totalQrCodeChunksCount) {
            QByteArray data;
            for (int i = 0; i < m_totalQrCodeChunksCount; ++i) {
                data.append(m_qrCodeChunks.value(i));
            }

            ImportResult result = extractConfigFromQr(data);
            if (result.errorCode == ErrorCode::NoError) {
                parseResult.success = true;
                parseResult.importResult = result;
                m_isQrCodeProcessed = false;
            } else {
                m_qrCodeChunks.clear();
                m_totalQrCodeChunksCount = 0;
                m_receivedQrCodeChunksCount = 0;
            }
        }
    } else {
        ImportResult result = extractConfigFromQr(code.toUtf8());
        if (result.errorCode != ErrorCode::NoError) {
            result = extractConfigFromQr(ba);
        }
        if (result.errorCode == ErrorCode::NoError) {
            parseResult.success = true;
            parseResult.importResult = result;
            m_isQrCodeProcessed = false;
        }
    }

    return parseResult;
}

bool ImportController::isQrDecodingActive() const
{
    return m_isQrCodeProcessed;
}

int ImportController::qrChunksReceived() const
{
    return m_receivedQrCodeChunksCount;
}

int ImportController::qrChunksTotal() const
{
    return m_totalQrCodeChunksCount;
}

void ImportController::importConfig(const QJsonObject &config)
{
    ServerCredentials credentials;
    credentials.hostName = config.value(configKey::hostName).toString();
    credentials.port = config.value(configKey::port).toInt();
    credentials.userName = config.value(configKey::userName).toString();
    credentials.secretData = config.value(configKey::password).toString();

    if (credentials.isValid() || config.contains(configKey::containers)) {
        m_serversRepository->addServer(QString(), config, serverConfigUtils::configTypeFromJson(config));
        emit importFinished();
    } else if (config.contains(configKey::configVersion)) {
        quint16 crc = qChecksum(QJsonDocument(config).toJson());
        bool hasServerWithCrc = false;
        const QVector<QString> ids = m_serversRepository->orderedServerIds();
        for (const QString &id : ids) {
            const auto apiV2 = m_serversRepository->apiV2Config(id);
            if (!apiV2.has_value()) {
                continue;
            }
            if (static_cast<quint16>(apiV2->crc) == crc) {
                hasServerWithCrc = true;
                break;
            }
        }

        if (hasServerWithCrc) {
            emit importErrorOccurred(ErrorCode::ApiConfigAlreadyAdded, true);
        } else {
            QJsonObject configWithCrc = config;
            configWithCrc.insert(configKey::crc, crc);
            m_serversRepository->addServer(QString(), configWithCrc, serverConfigUtils::configTypeFromJson(configWithCrc));
            emit importFinished();
        }
    } else {
        qDebug() << "Failed to import profile";
        qDebug().noquote() << QJsonDocument(config).toJson();
        emit importErrorOccurred(ErrorCode::ImportInvalidConfigError, false);
    }
}

QJsonObject ImportController::processNativeWireGuardConfig(const QJsonObject &config)
{
    QJsonObject result = config;
    auto containers = result.value(configKey::containers).toArray();
    if (!containers.isEmpty()) {
        auto container = containers.at(0).toObject();
        auto serverProtocolConfig = container.value(ContainerUtils::containerTypeToProtocolString(DockerContainer::WireGuard)).toObject();
        auto clientProtocolConfig = QJsonDocument::fromJson(serverProtocolConfig.value(configKey::lastConfig).toString().toUtf8()).object();

        QString junkPacketCount = QString::number(QRandomGenerator::global()->bounded(4, 7));
        QString junkPacketMinSize = QString::number(10);
        QString junkPacketMaxSize = QString::number(50);
        clientProtocolConfig[configKey::junkPacketCount] = junkPacketCount;
        clientProtocolConfig[configKey::junkPacketMinSize] = junkPacketMinSize;
        clientProtocolConfig[configKey::junkPacketMaxSize] = junkPacketMaxSize;
        clientProtocolConfig[configKey::initPacketJunkSize] = "0";
        clientProtocolConfig[configKey::responsePacketJunkSize] = "0";
        clientProtocolConfig[configKey::initPacketMagicHeader] = "1";
        clientProtocolConfig[configKey::responsePacketMagicHeader] = "2";
        clientProtocolConfig[configKey::underloadPacketMagicHeader] = "3";
        clientProtocolConfig[configKey::transportPacketMagicHeader] = "4";

        clientProtocolConfig[configKey::cookieReplyPacketJunkSize] = "0";
        clientProtocolConfig[configKey::transportPacketJunkSize] = "0";

        clientProtocolConfig[configKey::specialJunk1] = protocols::awg::defaultSpecialJunk1;

        clientProtocolConfig[configKey::isObfuscationEnabled] = true;

        serverProtocolConfig[configKey::lastConfig] = QString(QJsonDocument(clientProtocolConfig).toJson());
        container[configKey::wireguard] = serverProtocolConfig;
        containers.replace(0, container);
        result[configKey::containers] = containers;
    }
    return result;
}

ConfigTypes ImportController::checkConfigFormat(const QString &config) const
{
    return ::checkConfigFormat(config);
}

QJsonObject ImportController::extractOpenVpnConfig(const QString &data) const
{
    QJsonObject openVpnConfig;
    openVpnConfig[configKey::config] = data;

    QJsonObject lastConfig;
    lastConfig[configKey::lastConfig] = QString(QJsonDocument(openVpnConfig).toJson());
    lastConfig[configKey::isThirdPartyConfig] = true;

    QJsonObject containers;
    containers.insert(configKey::container, QJsonValue(configKey::amneziaOpenvpn));
    containers.insert(configKey::openvpn, QJsonValue(lastConfig));

    QJsonArray arr;
    arr.push_back(containers);

    QString hostName;
    const static QRegularExpression hostNameRegExp("remote\\s+([^\\s]+)");
    QRegularExpressionMatch hostNameMatch = hostNameRegExp.match(data);
    if (hostNameMatch.hasMatch()) {
        hostName = hostNameMatch.captured(1);
    }

    QJsonObject config;
    config[configKey::containers] = arr;
    config[configKey::defaultContainer] = configKey::amneziaOpenvpn;
    config[configKey::description] = m_serversRepository->nextAvailableServerName();

    const static QRegularExpression dnsRegExp("dhcp-option DNS (\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b)");
    QRegularExpressionMatchIterator dnsMatch = dnsRegExp.globalMatch(data);
    if (dnsMatch.hasNext()) {
        config[configKey::dns1] = dnsMatch.next().captured(1);
    }
    if (dnsMatch.hasNext()) {
        config[configKey::dns2] = dnsMatch.next().captured(1);
    }

    config[configKey::hostName] = hostName;

    return config;
}

QJsonObject ImportController::extractWireGuardConfig(const QString &data, ConfigTypes &configType) const
{
    QMap<QString, QString> configMap;
    auto configByLines = data.split("\n");
    for (const QString &line : configByLines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.startsWith("[") && trimmedLine.endsWith("]")) {
            continue;
        } else {
            QStringList parts = trimmedLine.split(" = ");
            if (parts.count() == 2) {
                configMap[parts.at(0).trimmed()] = parts.at(1).trimmed();
            }
        }
    }

    QJsonObject lastConfig;
    lastConfig[configKey::config] = data;

    auto url { QUrl::fromUserInput(configMap.value(protocols::wireguard::Endpoint)) };
    QString hostName;
    QString port;
    if (!url.host().isEmpty()) {
        hostName = url.host();
    } else {
        qDebug() << "Key parameter" << protocols::wireguard::Endpoint << "is missing or has an invalid format";
        return QJsonObject();
    }

    if (url.port() != -1) {
        port = QString::number(url.port());
    } else {
        port = protocols::wireguard::defaultPort;
    }

    lastConfig[configKey::hostName] = hostName;
    lastConfig[configKey::port] = port.toInt();

    if (!configMap.value(protocols::wireguard::PrivateKey).isEmpty()
            && !configMap.value(protocols::wireguard::Address).isEmpty()
            && !configMap.value(protocols::wireguard::PublicKey).isEmpty()) {
        lastConfig[configKey::clientPrivKey] = configMap.value(protocols::wireguard::PrivateKey);
        lastConfig[configKey::clientIp] = configMap.value(protocols::wireguard::Address);

        if (!configMap.value(protocols::wireguard::PresharedKey).isEmpty()) {
            lastConfig[configKey::pskKey] = configMap.value(protocols::wireguard::PresharedKey);
        } else if (!configMap.value(protocols::wireguard::PreSharedKey).isEmpty()) {
            lastConfig[configKey::pskKey] = configMap.value(protocols::wireguard::PreSharedKey);
        }

        lastConfig[configKey::serverPubKey] = configMap.value(protocols::wireguard::PublicKey);
    } else {
        qDebug() << "One of the key parameters is missing (PrivateKey, Address, PublicKey)";
        return QJsonObject();
    }

    if (!configMap.value(protocols::wireguard::MTU).isEmpty()) {
        lastConfig[configKey::mtu] = configMap.value(protocols::wireguard::MTU);
    }

    if (!configMap.value(protocols::wireguard::PersistentKeepalive).isEmpty()) {
        lastConfig[configKey::persistentKeepAlive] = configMap.value(protocols::wireguard::PersistentKeepalive);
    }

    QJsonArray allowedIpsJsonArray = QJsonArray::fromStringList(
                configMap.value(protocols::wireguard::AllowedIPs).split(", "));

    lastConfig[configKey::allowedIps] = allowedIpsJsonArray;

    QString protocolName = configKey::wireguard;
    QString protocolVersion;
    ConfigTypes detectedType = ConfigTypes::WireGuard;

    const QStringList requiredJunkFields = { configKey::junkPacketCount,           configKey::junkPacketMinSize,
                                             configKey::junkPacketMaxSize,         configKey::initPacketJunkSize,
                                             configKey::responsePacketJunkSize,    configKey::initPacketMagicHeader,
                                             configKey::responsePacketMagicHeader, configKey::underloadPacketMagicHeader,
                                             configKey::transportPacketMagicHeader };

    const QStringList optionalJunkFields = { configKey::cookieReplyPacketJunkSize,
                                             configKey::transportPacketJunkSize,
                                             configKey::specialJunk1,    configKey::specialJunk2,    configKey::specialJunk3,
                                             configKey::specialJunk4,    configKey::specialJunk5
    };

    bool hasAllRequiredFields = std::all_of(requiredJunkFields.begin(), requiredJunkFields.end(),
                                            [&configMap](const QString &field) { return !configMap.value(field).isEmpty(); });
    if (hasAllRequiredFields) {
        for (const QString &field : requiredJunkFields) {
            lastConfig[field] = configMap.value(field);
        }

        for (const QString &field : optionalJunkFields) {
            if (!configMap.value(field).isEmpty()) {
                lastConfig[field] = configMap.value(field);
            }
        }

        bool hasCookieReplyPacketJunkSize = !configMap.value(configKey::cookieReplyPacketJunkSize).isEmpty();
        bool hasTransportPacketJunkSize = !configMap.value(configKey::transportPacketJunkSize).isEmpty();
        bool hasSpecialJunk = !configMap.value(configKey::specialJunk1).isEmpty() ||
                              !configMap.value(configKey::specialJunk2).isEmpty() ||
                              !configMap.value(configKey::specialJunk3).isEmpty() ||
                              !configMap.value(configKey::specialJunk4).isEmpty() ||
                              !configMap.value(configKey::specialJunk5).isEmpty();

        if (hasCookieReplyPacketJunkSize && hasTransportPacketJunkSize) {
            protocolVersion = "2";
        } else if (hasSpecialJunk && !hasCookieReplyPacketJunkSize && !hasTransportPacketJunkSize) {
            protocolVersion = "1.5";
        }
        protocolName = configKey::awg;
        detectedType = ConfigTypes::Awg;
    }

    if (!configMap.value(protocols::wireguard::MTU).isEmpty()) {
        lastConfig[configKey::mtu] = configMap.value(protocols::wireguard::MTU);
    } else {
        lastConfig[configKey::mtu] = (protocolName == configKey::awg) 
                                       ? protocols::awg::defaultMtu 
                                       : protocols::wireguard::defaultMtu;
    }

    QJsonObject wireguardConfig;
    wireguardConfig[configKey::lastConfig] = QString(QJsonDocument(lastConfig).toJson());
    wireguardConfig[configKey::isThirdPartyConfig] = true;
    wireguardConfig[configKey::port] = port;
    wireguardConfig[configKey::transportProto] = protocols::openvpn::defaultTransportProto;
    if (protocolName == configKey::awg && !protocolVersion.isEmpty()) {
        wireguardConfig[configKey::protocolVersion] = protocolVersion;
    }

    QJsonObject containers;
    QString containerName = (protocolName == configKey::awg) ? configKey::amneziaAwg : configKey::amneziaWireguard;
    containers.insert(configKey::container, QJsonValue(containerName));
    containers.insert(protocolName, QJsonValue(wireguardConfig));

    QJsonArray arr;
    arr.push_back(containers);

    QJsonObject config;
    config[configKey::containers] = arr;
    config[configKey::defaultContainer] = containerName;
    config[configKey::description] = m_serversRepository->nextAvailableServerName();

    const static QRegularExpression dnsRegExp(
            "DNS = "
            "(\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b).*(\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b)");
    QRegularExpressionMatch dnsMatch = dnsRegExp.match(data);
    if (dnsMatch.hasMatch()) {
        config[configKey::dns1] = dnsMatch.captured(1);
        config[configKey::dns2] = dnsMatch.captured(2);
    }

    config[configKey::hostName] = hostName;

    configType = detectedType;
    return config;
}

QJsonObject ImportController::extractXrayConfig(const QString &data, ConfigTypes configType, const QString &description) const
{
    QJsonParseError parserErr;
    QJsonDocument jsonConf = QJsonDocument::fromJson(data.toLocal8Bit(), &parserErr);

    QJsonObject xrayVpnConfig;
    xrayVpnConfig[configKey::config] = jsonConf.toJson().constData();
    QJsonObject lastConfig;
    lastConfig[configKey::lastConfig] = jsonConf.toJson().constData();
    lastConfig[configKey::isThirdPartyConfig] = true;

    QJsonObject containers;
    if (configType == ConfigTypes::ShadowSocks) {
        containers.insert(configKey::ssxray, QJsonValue(lastConfig));
        containers.insert(configKey::container, QJsonValue(configKey::amneziaSsxray));
    } else {
        containers.insert(configKey::container, QJsonValue(configKey::amneziaXray));
        containers.insert(configKey::xray, QJsonValue(lastConfig));
    }

    QJsonArray arr;
    arr.push_back(containers);

    QString hostName;

    const static QRegularExpression hostNameRegExp("\"address\":\\s*\"([^\"]+)");
    QRegularExpressionMatch hostNameMatch = hostNameRegExp.match(data);
    if (hostNameMatch.hasMatch()) {
        hostName = hostNameMatch.captured(1);
    }

    QJsonObject config;
    config[configKey::containers] = arr;
    config[configKey::defaultContainer] = (configType == ConfigTypes::ShadowSocks)
            ? configKey::amneziaSsxray
            : configKey::amneziaXray;
    if (description.isEmpty()) {
        config[configKey::description] = m_serversRepository->nextAvailableServerName();
    } else {
        config[configKey::description] = description;
    }
    config[configKey::hostName] = hostName;

    return config;
}

void ImportController::checkForMaliciousStrings(const QJsonObject &serverConfig, QString &warningText) const
{
    const QJsonArray &containers = serverConfig.value(configKey::containers).toArray();
    for (const QJsonValue &container : containers) {
        auto containerConfig = container.toObject();
        auto containerName = containerConfig[configKey::container].toString();
        if (containerName == ContainerUtils::containerToString(DockerContainer::OpenVpn)) {

            QString protocolConfig =
                    containerConfig[ProtocolUtils::protoToString(Proto::OpenVpn)].toObject()[configKey::lastConfig].toString();
            QString protocolConfigJson = QJsonDocument::fromJson(protocolConfig.toUtf8()).object()[configKey::config].toString();

            // https://github.com/OpenVPN/openvpn/blob/master/doc/man-sections/script-options.rst
            QStringList dangerousTags {
                "up", "tls-verify", "ipchange", "client-connect", "route-up", "route-pre-down", "client-disconnect", "down", "learn-address", "auth-user-pass-verify"
            };

            QStringList maliciousStrings;
            QStringList lines = protocolConfigJson.split('\n', Qt::SkipEmptyParts);

            for (const QString &rawLine : lines) {
                QString line = rawLine.trimmed();

                QString command = line.section(' ', 0, 0, QString::SectionSkipEmpty);
                if (dangerousTags.contains(command, Qt::CaseInsensitive)) {
                    maliciousStrings << rawLine;
                }
            }

            warningText = "This configuration contains an OpenVPN setup. OpenVPN configurations can include malicious "
                         "scripts, so only add it if you fully trust the provider of this config. ";

            if (!maliciousStrings.isEmpty()) {
                warningText += "<br>In the imported configuration, potentially dangerous lines were found:";
                for (const auto &string : maliciousStrings) {
                    warningText += QString("<br><i>%1</i>").arg(string);
                }
            }
        }
    }
}

void ImportController::processAmneziaConfig(QJsonObject &config) const
{
    auto containers = config.value(configKey::containers).toArray();
    for (auto i = 0; i < containers.size(); i++) {
        auto container = containers.at(i).toObject();
        auto dockerContainer = ContainerUtils::containerFromString(container.value(configKey::container).toString());
        if (ContainerUtils::isAwgContainer(dockerContainer) || dockerContainer == DockerContainer::WireGuard) {
            auto containerConfig = container.value(ContainerUtils::containerTypeToProtocolString(dockerContainer)).toObject();
            auto protocolConfig = containerConfig.value(configKey::lastConfig).toString();
            if (protocolConfig.isEmpty()) {
                return;
            }

            QJsonObject jsonConfig = QJsonDocument::fromJson(protocolConfig.toUtf8()).object();
            jsonConfig[configKey::mtu] =
                    ContainerUtils::isAwgContainer(dockerContainer) ? protocols::awg::defaultMtu : protocols::wireguard::defaultMtu;

            containerConfig[configKey::lastConfig] = QString(QJsonDocument(jsonConfig).toJson());

            container[ContainerUtils::containerTypeToProtocolString(dockerContainer)] = containerConfig;
            containers.replace(i, container);
            config.insert(configKey::containers, containers);
        }
    }
}

