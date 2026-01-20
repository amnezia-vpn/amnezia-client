#include "importController.h"

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

#include "containers/containers_defs.h"
#include "core/utils/api/apiDefs.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/serialization/serialization.h"
#include "core/utils/utilities.h"
#include "core/protocols/protocolsDefs.h"
#include "core/models/serverConfig.h"

using namespace amnezia;

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

ImportController::ImportController(QServersRepository* serversRepository,
                                   QAppSettingsRepository* appSettingsRepository,
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
        configType = ConfigTypes::Xray;
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
        configType = ConfigTypes::Xray;
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

        if (apiUtils::isServerFromApi(result.config)) {
            auto apiConfig = result.config.value(apiDefs::key::apiConfig).toObject();
            apiConfig[apiDefs::key::vpnKey] = data;
            result.config[apiDefs::key::apiConfig] = apiConfig;
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
        if (m_serversRepository->serversCount() == 0) {
            emit restoreAppConfig(config.toUtf8());
            result.errorCode = ErrorCode::NoError;
            return result;
        } else {
            result.errorCode = ErrorCode::ImportInvalidConfigError;
            return result;
        }
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

    QJsonObject dataObj = QJsonDocument::fromJson(data).object();
    if (!dataObj.isEmpty()) {
        result.config = dataObj;
        result.configType = ConfigTypes::Amnezia;
        return result;
    }

    QByteArray ba_uncompressed = qUncompress(data);
    if (!ba_uncompressed.isEmpty()) {
        result.config = QJsonDocument::fromJson(ba_uncompressed).object();
        result.configType = ConfigTypes::Amnezia;
        return result;
    }

    ConfigTypes configType = checkConfigFormat(data);
    if (configType == ConfigTypes::Invalid) {
        QByteArray ba = QByteArray::fromBase64(data, QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        QByteArray baUncompressed = qUncompress(ba);

        if (!baUncompressed.isEmpty()) {
            ba = baUncompressed;
        }

        if (!ba.isEmpty()) {
            result.config = QJsonDocument::fromJson(ba).object();
            result.configType = ConfigTypes::Amnezia;
            return result;
        }
    }

    result.errorCode = ErrorCode::ImportInvalidConfigError;
    return result;
}

void ImportController::importConfig(const QJsonObject &config)
{
    ServerCredentials credentials;
    credentials.hostName = config.value(config_key::hostName).toString();
    credentials.port = config.value(config_key::port).toInt();
    credentials.userName = config.value(config_key::userName).toString();
    credentials.secretData = config.value(config_key::password).toString();

    if (credentials.isValid() || config.contains(config_key::containers)) {
        ServerConfig serverConfig = ServerConfigUtils::fromJson(config);
        m_serversRepository->addServer(serverConfig);
        emit importFinished();
    } else if (config.contains(config_key::configVersion)) {
        quint16 crc = qChecksum(QJsonDocument(config).toJson());
        QVector<ServerConfig> servers = m_serversRepository->servers();
        bool exists = false;
        for (const ServerConfig& serverConfig : servers) {
            if (static_cast<quint16>(ServerConfigUtils::crc(serverConfig)) == crc) {
                exists = true;
                break;
            }
        }
        
        if (exists) {
            emit importErrorOccurred(ErrorCode::ApiConfigAlreadyAdded, true);
        } else {
            QJsonObject configWithCrc = config;
            configWithCrc.insert(config_key::crc, crc);
            ServerConfig serverConfig = ServerConfigUtils::fromJson(configWithCrc);
            m_serversRepository->addServer(serverConfig);
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
    auto containers = result.value(config_key::containers).toArray();
    if (!containers.isEmpty()) {
        auto container = containers.at(0).toObject();
        auto serverProtocolConfig = container.value(ContainerProps::containerTypeToProtocolString(DockerContainer::WireGuard)).toObject();
        auto clientProtocolConfig = QJsonDocument::fromJson(serverProtocolConfig.value(config_key::last_config).toString().toUtf8()).object();

        QString junkPacketCount = QString::number(QRandomGenerator::global()->bounded(4, 7));
        QString junkPacketMinSize = QString::number(10);
        QString junkPacketMaxSize = QString::number(50);
        clientProtocolConfig[config_key::junkPacketCount] = junkPacketCount;
        clientProtocolConfig[config_key::junkPacketMinSize] = junkPacketMinSize;
        clientProtocolConfig[config_key::junkPacketMaxSize] = junkPacketMaxSize;
        clientProtocolConfig[config_key::initPacketJunkSize] = "0";
        clientProtocolConfig[config_key::responsePacketJunkSize] = "0";
        clientProtocolConfig[config_key::initPacketMagicHeader] = "1";
        clientProtocolConfig[config_key::responsePacketMagicHeader] = "2";
        clientProtocolConfig[config_key::underloadPacketMagicHeader] = "3";
        clientProtocolConfig[config_key::transportPacketMagicHeader] = "4";

        clientProtocolConfig[config_key::cookieReplyPacketJunkSize] = "0";
        clientProtocolConfig[config_key::transportPacketJunkSize] = "0";

        clientProtocolConfig[config_key::isObfuscationEnabled] = true;

        serverProtocolConfig[config_key::last_config] = QString(QJsonDocument(clientProtocolConfig).toJson());
        container["wireguard"] = serverProtocolConfig;
        containers.replace(0, container);
        result[config_key::containers] = containers;
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
    openVpnConfig[config_key::config] = data;

    QJsonObject lastConfig;
    lastConfig[config_key::last_config] = QString(QJsonDocument(openVpnConfig).toJson());
    lastConfig[config_key::isThirdPartyConfig] = true;

    QJsonObject containers;
    containers.insert(config_key::container, QJsonValue("amnezia-openvpn"));
    containers.insert(config_key::openvpn, QJsonValue(lastConfig));

    QJsonArray arr;
    arr.push_back(containers);

    QString hostName;
    const static QRegularExpression hostNameRegExp("remote\\s+([^\\s]+)");
    QRegularExpressionMatch hostNameMatch = hostNameRegExp.match(data);
    if (hostNameMatch.hasMatch()) {
        hostName = hostNameMatch.captured(1);
    }

    QJsonObject config;
    config[config_key::containers] = arr;
    config[config_key::defaultContainer] = "amnezia-openvpn";
    config[config_key::description] = m_appSettingsRepository->nextAvailableServerName();

    const static QRegularExpression dnsRegExp("dhcp-option DNS (\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b)");
    QRegularExpressionMatchIterator dnsMatch = dnsRegExp.globalMatch(data);
    if (dnsMatch.hasNext()) {
        config[config_key::dns1] = dnsMatch.next().captured(1);
    }
    if (dnsMatch.hasNext()) {
        config[config_key::dns2] = dnsMatch.next().captured(1);
    }

    config[config_key::hostName] = hostName;

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
    lastConfig[config_key::config] = data;

    auto url { QUrl::fromUserInput(configMap.value("Endpoint")) };
    QString hostName;
    QString port;
    if (!url.host().isEmpty()) {
        hostName = url.host();
    } else {
        qDebug() << "Key parameter 'Endpoint' is missing or has an invalid format";
        return QJsonObject();
    }

    if (url.port() != -1) {
        port = QString::number(url.port());
    } else {
        port = protocols::wireguard::defaultPort;
    }

    lastConfig[config_key::hostName] = hostName;
    lastConfig[config_key::port] = port.toInt();

    if (!configMap.value("PrivateKey").isEmpty() && !configMap.value("Address").isEmpty() && !configMap.value("PublicKey").isEmpty()) {
        lastConfig[config_key::client_priv_key] = configMap.value("PrivateKey");
        lastConfig[config_key::client_ip] = configMap.value("Address");

        if (!configMap.value("PresharedKey").isEmpty()) {
            lastConfig[config_key::psk_key] = configMap.value("PresharedKey");
        } else if (!configMap.value("PreSharedKey").isEmpty()) {
            lastConfig[config_key::psk_key] = configMap.value("PreSharedKey");
        }

        lastConfig[config_key::server_pub_key] = configMap.value("PublicKey");
    } else {
        qDebug() << "One of the key parameters is missing (PrivateKey, Address, PublicKey)";
        return QJsonObject();
    }

    if (!configMap.value("MTU").isEmpty()) {
        lastConfig[config_key::mtu] = configMap.value("MTU");
    }

    if (!configMap.value("PersistentKeepalive").isEmpty()) {
        lastConfig[config_key::persistent_keep_alive] = configMap.value("PersistentKeepalive");
    }

    QJsonArray allowedIpsJsonArray = QJsonArray::fromStringList(configMap.value("AllowedIPs").split(", "));

    lastConfig[config_key::allowed_ips] = allowedIpsJsonArray;

    QString protocolName = "wireguard";
    QString protocolVersion;
    ConfigTypes detectedType = ConfigTypes::WireGuard;

    const QStringList requiredJunkFields = { config_key::junkPacketCount,           config_key::junkPacketMinSize,
                                             config_key::junkPacketMaxSize,         config_key::initPacketJunkSize,
                                             config_key::responsePacketJunkSize,    config_key::initPacketMagicHeader,
                                             config_key::responsePacketMagicHeader, config_key::underloadPacketMagicHeader,
                                             config_key::transportPacketMagicHeader };

    const QStringList optionalJunkFields = { config_key::cookieReplyPacketJunkSize,
                                             config_key::transportPacketJunkSize,
                                             config_key::specialJunk1,    config_key::specialJunk2,    config_key::specialJunk3,
                                             config_key::specialJunk4,    config_key::specialJunk5
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

        bool hasCookieReplyPacketJunkSize = !configMap.value(config_key::cookieReplyPacketJunkSize).isEmpty();
        bool hasTransportPacketJunkSize = !configMap.value(config_key::transportPacketJunkSize).isEmpty();
        bool hasSpecialJunk = !configMap.value(config_key::specialJunk1).isEmpty() ||
                              !configMap.value(config_key::specialJunk2).isEmpty() ||
                              !configMap.value(config_key::specialJunk3).isEmpty() ||
                              !configMap.value(config_key::specialJunk4).isEmpty() ||
                              !configMap.value(config_key::specialJunk5).isEmpty();

        if (hasCookieReplyPacketJunkSize && hasTransportPacketJunkSize) {
            protocolVersion = "2";
        } else if (hasSpecialJunk && !hasCookieReplyPacketJunkSize && !hasTransportPacketJunkSize) {
            protocolVersion = "1.5";
        }
        protocolName = "awg";
        detectedType = ConfigTypes::Awg;
    }

    if (!configMap.value("MTU").isEmpty()) {
        lastConfig[config_key::mtu] = configMap.value("MTU");
    } else {
        lastConfig[config_key::mtu] = (protocolName == "awg") 
                                       ? protocols::awg::defaultMtu 
                                       : protocols::wireguard::defaultMtu;
    }

    QJsonObject wireguardConfig;
    wireguardConfig[config_key::last_config] = QString(QJsonDocument(lastConfig).toJson());
    wireguardConfig[config_key::isThirdPartyConfig] = true;
    wireguardConfig[config_key::port] = port;
    wireguardConfig[config_key::transport_proto] = "udp";
    if (protocolName == "awg" && !protocolVersion.isEmpty()) {
        wireguardConfig[config_key::protocolVersion] = protocolVersion;
    }

    QJsonObject containers;
    containers.insert(config_key::container, QJsonValue("amnezia-" + protocolName));
    containers.insert(protocolName, QJsonValue(wireguardConfig));

    QJsonArray arr;
    arr.push_back(containers);

    QJsonObject config;
    config[config_key::containers] = arr;
    config[config_key::defaultContainer] = "amnezia-" + protocolName;
    config[config_key::description] = m_appSettingsRepository->nextAvailableServerName();

    const static QRegularExpression dnsRegExp(
            "DNS = "
            "(\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b).*(\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b)");
    QRegularExpressionMatch dnsMatch = dnsRegExp.match(data);
    if (dnsMatch.hasMatch()) {
        config[config_key::dns1] = dnsMatch.captured(1);
        config[config_key::dns2] = dnsMatch.captured(2);
    }

    config[config_key::hostName] = hostName;

    configType = detectedType;
    return config;
}

QJsonObject ImportController::extractXrayConfig(const QString &data, ConfigTypes configType, const QString &description) const
{
    QJsonParseError parserErr;
    QJsonDocument jsonConf = QJsonDocument::fromJson(data.toLocal8Bit(), &parserErr);

    QJsonObject xrayVpnConfig;
    xrayVpnConfig[config_key::config] = jsonConf.toJson().constData();
    QJsonObject lastConfig;
    lastConfig[config_key::last_config] = jsonConf.toJson().constData();
    lastConfig[config_key::isThirdPartyConfig] = true;

    QJsonObject containers;
    containers.insert(config_key::container, QJsonValue("amnezia-xray"));
    containers.insert(config_key::xray, QJsonValue(lastConfig));

    QJsonArray arr;
    arr.push_back(containers);

    QString hostName;

    const static QRegularExpression hostNameRegExp("\"address\":\\s*\"([^\"]+)");
    QRegularExpressionMatch hostNameMatch = hostNameRegExp.match(data);
    if (hostNameMatch.hasMatch()) {
        hostName = hostNameMatch.captured(1);
    }

    QJsonObject config;
    config[config_key::containers] = arr;
    config[config_key::defaultContainer] = "amnezia-xray";
    if (description.isEmpty()) {
        config[config_key::description] = m_appSettingsRepository->nextAvailableServerName();
    } else {
        config[config_key::description] = description;
    }
    config[config_key::hostName] = hostName;

    return config;
}

void ImportController::checkForMaliciousStrings(QJsonObject &serverConfig, QString &warningText) const
{
    const QJsonArray &containers = serverConfig[config_key::containers].toArray();
    for (const QJsonValue &container : containers) {
        auto containerConfig = container.toObject();
        auto containerName = containerConfig[config_key::container].toString();
        if (containerName == ContainerProps::containerToString(DockerContainer::OpenVpn)) {

            QString protocolConfig =
                    containerConfig[ProtocolProps::protoToString(Proto::OpenVpn)].toObject()[config_key::last_config].toString();
            QString protocolConfigJson = QJsonDocument::fromJson(protocolConfig.toUtf8()).object()[config_key::config].toString();

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
    auto containers = config.value(config_key::containers).toArray();
    for (auto i = 0; i < containers.size(); i++) {
        auto container = containers.at(i).toObject();
        auto dockerContainer = ContainerProps::containerFromString(container.value(config_key::container).toString());
        if (ContainerProps::isAwgContainer(dockerContainer) || dockerContainer == DockerContainer::WireGuard) {
            auto containerConfig = container.value(ContainerProps::containerTypeToProtocolString(dockerContainer)).toObject();
            auto protocolConfig = containerConfig.value(config_key::last_config).toString();
            if (protocolConfig.isEmpty()) {
                return;
            }

            QJsonObject jsonConfig = QJsonDocument::fromJson(protocolConfig.toUtf8()).object();
            jsonConfig[config_key::mtu] =
                    ContainerProps::isAwgContainer(dockerContainer) ? protocols::awg::defaultMtu : protocols::wireguard::defaultMtu;

            containerConfig[config_key::last_config] = QString(QJsonDocument(jsonConfig).toJson());

            container[ContainerProps::containerTypeToProtocolString(dockerContainer)] = containerConfig;
            containers.replace(i, container);
            config.insert(config_key::containers, containers);
        }
    }
}

