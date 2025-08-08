#include "importController.h"

#include <QFile>
#include <QFileInfo>
#include <QQuickItem>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QUrlQuery>

#include "core/api/apiDefs.h"
#include "core/api/apiUtils.h"
#include "core/errorstrings.h"
#include "core/qrCodeUtils.h"
#include "core/serialization/serialization.h"
#include "protocols/protocols_defs.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/wireguardProtocolConfig.h"
#include "core/models/protocols/openvpnProtocolConfig.h"
#include "core/models/protocols/shadowsocksProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/models/protocols/cloakProtocolConfig.h"
#include "core/models/protocols/protocolConfig.h"
#include "core/models/servers/selfHostedServerConfig.h"
#include "core/models/servers/serverConfig.h"
#include <variant>
#include "core/models/servers/apiV1ServerConfig.h"
#include "core/models/servers/apiV2ServerConfig.h"
#include "core/models/containers/containerConfig.h"
#include "systemController.h"
#include "utilities.h"

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif
#ifdef Q_OS_IOS
    #include <CoreFoundation/CoreFoundation.h>
#endif

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

#if defined Q_OS_ANDROID
    ImportController *mInstance = nullptr;
#endif
} // namespace

ImportController::ImportController(const QSharedPointer<ServersModel> &serversModel, const QSharedPointer<ContainersModel> &containersModel,
                                   const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_containersModel(containersModel), m_settings(settings)
{
#ifdef Q_OS_ANDROID
    mInstance = this;
#endif
}

bool ImportController::extractConfigFromFile(const QString &fileName)
{
    QString data;
    if (!SystemController::readFile(fileName, data)) {
        emit importErrorOccurred(ErrorCode::ImportOpenConfigError, false);
        return false;
    }
    m_configFileName = QFileInfo(QFile(fileName).fileName()).fileName();
#ifdef Q_OS_ANDROID
    if (m_configFileName.isEmpty()) {
        m_configFileName = AndroidController::instance()->getFileName(fileName);
    }
#endif
    return extractConfigFromData(data);
}

bool ImportController::extractConfigFromData(QString data)
{
    m_maliciousWarningText.clear();

    QString config = data;
    QString prefix;
    QString errormsg;

    if (config.startsWith("vless://")) {
        m_configType = ConfigTypes::Xray;
        m_config = extractXrayConfig(
                Utils::JsonToString(serialization::vless::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact),
                prefix);
        return m_config.empty() ? false : true;
    }

    if (config.startsWith("vmess://") && config.contains("@")) {
        m_configType = ConfigTypes::Xray;
        m_config = extractXrayConfig(
                Utils::JsonToString(serialization::vmess_new::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact),
                prefix);
        return m_config.empty() ? false : true;
    }

    if (config.startsWith("vmess://")) {
        m_configType = ConfigTypes::Xray;
        m_config = extractXrayConfig(
                Utils::JsonToString(serialization::vmess::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact),
                prefix);
        return m_config.empty() ? false : true;
    }

    if (config.startsWith("trojan://")) {
        m_configType = ConfigTypes::Xray;
        m_config = extractXrayConfig(
                Utils::JsonToString(serialization::trojan::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact),
                prefix);
        return m_config.empty() ? false : true;
    }

    if (config.startsWith("ss://") && !config.contains("plugin=")) {
        m_configType = ConfigTypes::ShadowSocks;
        m_config = extractXrayConfig(
                Utils::JsonToString(serialization::ss::Deserialize(config, &prefix, &errormsg), QJsonDocument::JsonFormat::Compact), prefix);
        return m_config.empty() ? false : true;
    }

    if (config.startsWith("ssd://")) {
        QStringList tmp;
        QList<std::pair<QString, QJsonObject>> servers = serialization::ssd::Deserialize(config, &prefix, &tmp);
        m_configType = ConfigTypes::ShadowSocks;
        // Took only first config from list
        if (!servers.isEmpty()) {
            m_config = extractXrayConfig(servers.first().first);
        }
        return m_config.empty() ? false : true;
    }

    m_configType = checkConfigFormat(config);
    if (m_configType == ConfigTypes::Invalid) {
        config.replace("vpn://", "");
        QByteArray ba = QByteArray::fromBase64(config.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        QByteArray baUncompressed = qUncompress(ba);
        if (!baUncompressed.isEmpty()) {
            ba = baUncompressed;
        }

        config = ba;
        m_configType = checkConfigFormat(config);
    }

    switch (m_configType) {
    case ConfigTypes::OpenVpn: {
        m_config = extractOpenVpnConfig(config);
        if (m_config) {
            checkForMaliciousStrings(m_config);
            return true;
        }
        return false;
    }
    case ConfigTypes::Awg:
    case ConfigTypes::WireGuard: {
        m_config = extractWireGuardConfig(config);
        return m_config ? true : false;
    }
    case ConfigTypes::Xray: {
        m_config = extractXrayConfig(config);
        return m_config ? true : false;
    }
    case ConfigTypes::Amnezia: {
        QJsonObject configJson = QJsonDocument::fromJson(config.toUtf8()).object();

        if (apiUtils::isServerFromApi(configJson)) {
            auto apiConfig = configJson.value(apiDefs::key::apiConfig).toObject();
            apiConfig[apiDefs::key::vpnKey] = data;
            configJson[apiDefs::key::apiConfig] = apiConfig;
        }
        m_config = ServerConfig::createServerConfig(configJson);

        processAmneziaConfig(m_config);
        if (m_config) {
            checkForMaliciousStrings(m_config);
            return true;
        }
        return false;
    }
    case ConfigTypes::Backup: {
        if (!m_serversModel->getServersCount()) {
            emit restoreAppConfig(config.toUtf8());
        } else {
            emit importErrorOccurred(ErrorCode::ImportInvalidConfigError, false);
        }
        break;
    }
    case ConfigTypes::Invalid: {
        emit importErrorOccurred(ErrorCode::ImportInvalidConfigError, false);
        break;
    }
    }
    return false;
}

bool ImportController::extractConfigFromQr(const QByteArray &data)
{
    QJsonObject dataObj = QJsonDocument::fromJson(data).object();
    if (!dataObj.isEmpty()) {
        m_config = ServerConfig::createServerConfig(dataObj);
        return true;
    }

    QByteArray ba_uncompressed = qUncompress(data);
    if (!ba_uncompressed.isEmpty()) {
        QJsonObject configObj = QJsonDocument::fromJson(ba_uncompressed).object();
        m_config = ServerConfig::createServerConfig(configObj);
        return true;
    }

    m_configType = checkConfigFormat(data);
    if (m_configType == ConfigTypes::Invalid) {
        QByteArray ba = QByteArray::fromBase64(data, QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        QByteArray baUncompressed = qUncompress(ba);

        if (!baUncompressed.isEmpty()) {
            ba = baUncompressed;
        }

        if (!ba.isEmpty()) {
            QJsonObject configObj = QJsonDocument::fromJson(ba).object();
            m_config = ServerConfig::createServerConfig(configObj);
            return true;
        }
    }

    return false;
}

QString ImportController::getConfig()
{
    if (!m_config) {
        return QString();
    }
    return QJsonDocument(m_config->toJson()).toJson(QJsonDocument::Indented);
}

QString ImportController::getConfigFileName()
{
    return m_configFileName;
}

QString ImportController::getMaliciousWarningText()
{
    return m_maliciousWarningText;
}

bool ImportController::isNativeWireGuardConfig()
{
    return m_configType == ConfigTypes::WireGuard;
}

void ImportController::processNativeWireGuardConfig()
{
    if (!m_config) return;
    
    auto selfHostedConfig = qSharedPointerCast<SelfHostedServerConfig>(m_config);
    if (!selfHostedConfig) return;
    
    QString wireguardContainerName = ContainerProps::containerTypeToString(DockerContainer::WireGuard);
    if (!selfHostedConfig->containerConfigs.contains(wireguardContainerName)) {
        return;
    }
    
    ContainerConfig wireguardContainer = selfHostedConfig->containerConfigs.value(wireguardContainerName);
    if (!wireguardContainer.protocolConfigs.contains("wireguard")) {
        return;
    }
    
    auto wireguardProtocol = wireguardContainer.protocolConfigs.value("wireguard");
    
    QJsonObject awgProtocolConfigJson;
    awgProtocolConfigJson[config_key::protocolName] = "awg";
    awgProtocolConfigJson[config_key::last_config] = wireguardProtocol->toJson().value(config_key::last_config);
    
    auto awgConfig = QSharedPointer<AwgProtocolConfig>::create(awgProtocolConfigJson, "awg");
    
    QString junkPacketCount = QString::number(QRandomGenerator::global()->bounded(2, 5));
    awgConfig->serverProtocolConfig.junkPacketCount = junkPacketCount;
    awgConfig->serverProtocolConfig.junkPacketMinSize = "10";
    awgConfig->serverProtocolConfig.junkPacketMaxSize = "50";
    awgConfig->serverProtocolConfig.initPacketJunkSize = "0";
    awgConfig->serverProtocolConfig.responsePacketJunkSize = "0";
    awgConfig->serverProtocolConfig.initPacketMagicHeader = "1";
    awgConfig->serverProtocolConfig.responsePacketMagicHeader = "2";
    awgConfig->serverProtocolConfig.underloadPacketMagicHeader = "3";
    awgConfig->serverProtocolConfig.transportPacketMagicHeader = "4";
    
    wireguardContainer.protocolConfigs["wireguard"] = awgConfig;
    selfHostedConfig->containerConfigs[wireguardContainerName] = wireguardContainer;
}

void ImportController::importConfig()
{
    if (!m_config) {
        emit importErrorOccurred(ErrorCode::ImportInvalidConfigError, false);
        return;
    }

    if (auto selfHostedConfig = qSharedPointerCast<SelfHostedServerConfig>(m_config)) {
        if (selfHostedConfig->serverCredentials.isValid() || !selfHostedConfig->containerConfigs.isEmpty()) {
            m_serversModel->addServer(m_config->toJson());
        emit importFinished();
        } else {
            qDebug() << "Failed to import profile - invalid credentials or no containers";
            emit importErrorOccurred(ErrorCode::ImportInvalidConfigError, false);
        }
    } else if (apiUtils::isServerFromApi(m_config->toJson())) {
        quint16 crc = qChecksum(QJsonDocument(m_config->toJson()).toJson());
        if (m_serversModel->isServerFromApiAlreadyExists(crc)) {
            emit importErrorOccurred(ErrorCode::ApiConfigAlreadyAdded, true);
        } else {
            QJsonObject configJson = m_config->toJson();
            configJson[config_key::crc] = crc;
            auto updatedConfig = ServerConfig::createServerConfig(configJson);

            m_serversModel->addServer(updatedConfig->toJson());
            emit importFinished();
        }
    } else {
        qDebug() << "Failed to import profile - unknown config type";
        qDebug().noquote() << QJsonDocument(m_config->toJson()).toJson();
        emit importErrorOccurred(ErrorCode::ImportInvalidConfigError, false);
    }

    m_config.reset();
    m_configFileName.clear();
    m_maliciousWarningText.clear();
}

QSharedPointer<ServerConfig> ImportController::extractOpenVpnConfig(const QString &data)
{
    QString hostName;
    const static QRegularExpression hostNameRegExp("remote\\s+([^\\s]+)");
    QRegularExpressionMatch hostNameMatch = hostNameRegExp.match(data);
    if (hostNameMatch.hasMatch()) {
        hostName = hostNameMatch.captured(1);
    }

    auto serverConfig = QSharedPointer<SelfHostedServerConfig>::create();
    serverConfig->hostName = hostName;
    serverConfig->description = m_settings->nextAvailableServerName();
    serverConfig->defaultContainer = "amnezia-openvpn";

    const static QRegularExpression dnsRegExp("dhcp-option DNS (\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b)");
    QRegularExpressionMatchIterator dnsMatch = dnsRegExp.globalMatch(data);
    if (dnsMatch.hasNext()) {
        serverConfig->dns1 = dnsMatch.next().captured(1);
    }
    if (dnsMatch.hasNext()) {
        serverConfig->dns2 = dnsMatch.next().captured(1);
    }

    auto openVpnProtocolConfig = QSharedPointer<OpenVpnProtocolConfig>::create();
    openVpnProtocolConfig->clientProtocolConfig.nativeConfig = data;
    openVpnProtocolConfig->isThirdPartyConfig = true;

    ContainerConfig containerConfig;
    containerConfig.container = DockerContainer::OpenVpn;
    containerConfig.protocolConfigs["openvpn"] = openVpnProtocolConfig;

    serverConfig->containerConfigs["amnezia-openvpn"] = containerConfig;

    return serverConfig;
}

QSharedPointer<ServerConfig> ImportController::extractWireGuardConfig(const QString &data)
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

    auto url { QUrl::fromUserInput(configMap.value("Endpoint")) };
    QString hostName;
    QString port;
    if (!url.host().isEmpty()) {
        hostName = url.host();
    } else {
        qDebug() << "Key parameter 'Endpoint' is missing or has an invalid format";
        emit importErrorOccurred(ErrorCode::ImportInvalidConfigError, false);
        return nullptr;
    }

    if (url.port() != -1) {
        port = QString::number(url.port());
    } else {
        port = protocols::wireguard::defaultPort;
    }

    if (configMap.value("PrivateKey").isEmpty() || configMap.value("Address").isEmpty() || configMap.value("PublicKey").isEmpty()) {
        qDebug() << "One of the key parameters is missing (PrivateKey, Address, PublicKey)";
        emit importErrorOccurred(ErrorCode::ImportInvalidConfigError, false);
        return nullptr;
    }

    const QStringList requiredJunkFields = { config_key::junkPacketCount,           config_key::junkPacketMinSize,
                                             config_key::junkPacketMaxSize,         config_key::initPacketJunkSize,
                                             config_key::responsePacketJunkSize,    config_key::initPacketMagicHeader,
                                             config_key::responsePacketMagicHeader, config_key::underloadPacketMagicHeader,
                                             config_key::transportPacketMagicHeader };

    const QStringList optionalJunkFields = { config_key::specialJunk1,    config_key::specialJunk2,    config_key::specialJunk3,
                                             config_key::specialJunk4,    config_key::specialJunk5,    config_key::controlledJunk1,
                                             config_key::controlledJunk2, config_key::controlledJunk3, config_key::specialHandshakeTimeout
    };

    bool hasAllRequiredFields = std::all_of(requiredJunkFields.begin(), requiredJunkFields.end(),
                                            [&configMap](const QString &field) { return !configMap.value(field).isEmpty(); });

    auto serverConfig = QSharedPointer<SelfHostedServerConfig>::create();
    serverConfig->hostName = hostName;
    serverConfig->description = m_settings->nextAvailableServerName();

    const static QRegularExpression dnsRegExp(
            "DNS = "
            "(\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b).*(\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b)");
    QRegularExpressionMatch dnsMatch = dnsRegExp.match(data);
    if (dnsMatch.hasMatch()) {
        serverConfig->dns1 = dnsMatch.captured(1);
        serverConfig->dns2 = dnsMatch.captured(2);
    }

    ContainerConfig containerConfig;
    QSharedPointer<ProtocolConfig> protocolConfig;

    if (hasAllRequiredFields) {
        m_configType = ConfigTypes::Awg;
        serverConfig->defaultContainer = "amnezia-awg";
        containerConfig.container = DockerContainer::Awg;

        auto awgConfig = QSharedPointer<AwgProtocolConfig>::create();
        awgConfig->clientProtocolConfig.nativeConfig = data;
        awgConfig->clientProtocolConfig.clientPrivateKey = configMap.value("PrivateKey");
        awgConfig->clientProtocolConfig.clientIp = configMap.value("Address");
        awgConfig->serverProtocolConfig.serverPublicKey = configMap.value("PublicKey");
        
        if (!configMap.value("PresharedKey").isEmpty()) {
            awgConfig->clientProtocolConfig.pskKey = configMap.value("PresharedKey");
        } else if (!configMap.value("PreSharedKey").isEmpty()) {
            awgConfig->clientProtocolConfig.pskKey = configMap.value("PreSharedKey");
        }

        awgConfig->serverProtocolConfig.hostName = hostName;
        awgConfig->serverProtocolConfig.port = port.toInt();
        awgConfig->serverProtocolConfig.mtu = configMap.value("MTU").isEmpty() ? protocols::awg::defaultMtu : configMap.value("MTU").toInt();
        
        if (!configMap.value("PersistentKeepalive").isEmpty()) {
            awgConfig->clientProtocolConfig.persistentKeepalive = configMap.value("PersistentKeepalive");
        }

        QStringList allowedIps = configMap.value("AllowedIPs").split(", ");
        awgConfig->clientProtocolConfig.allowedIps = allowedIps;

        for (const QString &field : requiredJunkFields) {
            QString value = configMap.value(field);
            if (field == config_key::junkPacketCount) awgConfig->serverProtocolConfig.junkPacketCount = value;
            else if (field == config_key::junkPacketMinSize) awgConfig->serverProtocolConfig.junkPacketMinSize = value;
            else if (field == config_key::junkPacketMaxSize) awgConfig->serverProtocolConfig.junkPacketMaxSize = value;
            else if (field == config_key::initPacketJunkSize) awgConfig->serverProtocolConfig.initPacketJunkSize = value;
            else if (field == config_key::responsePacketJunkSize) awgConfig->serverProtocolConfig.responsePacketJunkSize = value;
            else if (field == config_key::initPacketMagicHeader) awgConfig->serverProtocolConfig.initPacketMagicHeader = value;
            else if (field == config_key::responsePacketMagicHeader) awgConfig->serverProtocolConfig.responsePacketMagicHeader = value;
            else if (field == config_key::underloadPacketMagicHeader) awgConfig->serverProtocolConfig.underloadPacketMagicHeader = value;
            else if (field == config_key::transportPacketMagicHeader) awgConfig->serverProtocolConfig.transportPacketMagicHeader = value;
        }

        for (const QString &field : optionalJunkFields) {
            QString value = configMap.value(field);
            if (!value.isEmpty()) {
                if (field == config_key::specialJunk1) awgConfig->serverProtocolConfig.specialJunk1 = value;
                else if (field == config_key::specialJunk2) awgConfig->serverProtocolConfig.specialJunk2 = value;
                else if (field == config_key::specialJunk3) awgConfig->serverProtocolConfig.specialJunk3 = value;
                else if (field == config_key::specialJunk4) awgConfig->serverProtocolConfig.specialJunk4 = value;
                else if (field == config_key::specialJunk5) awgConfig->serverProtocolConfig.specialJunk5 = value;
                else if (field == config_key::controlledJunk1) awgConfig->serverProtocolConfig.controlledJunk1 = value;
                else if (field == config_key::controlledJunk2) awgConfig->serverProtocolConfig.controlledJunk2 = value;
                else if (field == config_key::controlledJunk3) awgConfig->serverProtocolConfig.controlledJunk3 = value;
                else if (field == config_key::specialHandshakeTimeout) awgConfig->serverProtocolConfig.specialHandshakeTimeout = value;
            }
        }

        awgConfig->isThirdPartyConfig = true;
        protocolConfig = awgConfig;
        containerConfig.protocolConfigs["awg"] = protocolConfig;
    } else {
        serverConfig->defaultContainer = "amnezia-wireguard";
        containerConfig.container = DockerContainer::WireGuard;

        auto wgConfig = QSharedPointer<WireGuardProtocolConfig>::create();
        wgConfig->clientProtocolConfig.nativeConfig = data;
        wgConfig->clientProtocolConfig.clientPrivateKey = configMap.value("PrivateKey");
        wgConfig->clientProtocolConfig.clientIp = configMap.value("Address");
        wgConfig->serverProtocolConfig.serverPublicKey = configMap.value("PublicKey");
        
        if (!configMap.value("PresharedKey").isEmpty()) {
            wgConfig->clientProtocolConfig.pskKey = configMap.value("PresharedKey");
        } else if (!configMap.value("PreSharedKey").isEmpty()) {
            wgConfig->clientProtocolConfig.pskKey = configMap.value("PreSharedKey");
        }

        wgConfig->serverProtocolConfig.hostName = hostName;
        wgConfig->serverProtocolConfig.port = port.toInt();
        wgConfig->serverProtocolConfig.mtu = configMap.value("MTU").isEmpty() ? protocols::wireguard::defaultMtu : configMap.value("MTU").toInt();
        
        if (!configMap.value("PersistentKeepalive").isEmpty()) {
            wgConfig->clientProtocolConfig.persistentKeepalive = configMap.value("PersistentKeepalive");
        }

        QStringList allowedIps = configMap.value("AllowedIPs").split(", ");
        wgConfig->clientProtocolConfig.allowedIps = allowedIps;

        wgConfig->isThirdPartyConfig = true;
        protocolConfig = wgConfig;
        containerConfig.protocolConfigs["wireguard"] = protocolConfig;
    }

    serverConfig->containerConfigs[serverConfig->defaultContainer] = containerConfig;

    return serverConfig;
}

QSharedPointer<ServerConfig> ImportController::extractXrayConfig(const QString &data, const QString &description)
{
    QJsonParseError parserErr;
    QJsonDocument jsonConf = QJsonDocument::fromJson(data.toLocal8Bit(), &parserErr);

    QString hostName;
    const static QRegularExpression hostNameRegExp("\"address\":\\s*\"([^\"]+)");
    QRegularExpressionMatch hostNameMatch = hostNameRegExp.match(data);
    if (hostNameMatch.hasMatch()) {
        hostName = hostNameMatch.captured(1);
    }

    auto serverConfig = QSharedPointer<SelfHostedServerConfig>::create();
    serverConfig->hostName = hostName;
    serverConfig->description = description.isEmpty() ? m_settings->nextAvailableServerName() : description;

    ContainerConfig containerConfig;
    QSharedPointer<ProtocolConfig> protocolConfig;

    if (m_configType == ConfigTypes::ShadowSocks) {
        serverConfig->defaultContainer = "amnezia-ssxray";
        containerConfig.container = DockerContainer::ShadowSocks;

        auto shadowsocksConfig = QSharedPointer<ShadowsocksProtocolConfig>::create();
        shadowsocksConfig->clientProtocolConfig.nativeConfig = data;
        shadowsocksConfig->isThirdPartyConfig = true;

        protocolConfig = shadowsocksConfig;
        containerConfig.protocolConfigs["ssxray"] = protocolConfig;
    } else {
        serverConfig->defaultContainer = "amnezia-xray";
        containerConfig.container = DockerContainer::Xray;

        auto xrayConfig = QSharedPointer<XrayProtocolConfig>::create();
        xrayConfig->clientProtocolConfig.nativeConfig = data;
        xrayConfig->isThirdPartyConfig = true;

        protocolConfig = xrayConfig;
        containerConfig.protocolConfigs["xray"] = protocolConfig;
    }

    serverConfig->containerConfigs[serverConfig->defaultContainer] = containerConfig;

    return serverConfig;
}

#ifdef Q_OS_ANDROID
static QMutex qrDecodeMutex;

// static
bool ImportController::decodeQrCode(const QString &code)
{
    QMutexLocker lock(&qrDecodeMutex);

    if (!mInstance->m_isQrCodeProcessed) {
        mInstance->m_qrCodeChunks.clear();
        mInstance->m_isQrCodeProcessed = true;
        mInstance->m_totalQrCodeChunksCount = 0;
        mInstance->m_receivedQrCodeChunksCount = 0;
    }
    return mInstance->parseQrCodeChunk(code);
}
#endif

#if defined Q_OS_ANDROID || defined Q_OS_IOS
void ImportController::startDecodingQr()
{
    m_qrCodeChunks.clear();
    m_totalQrCodeChunksCount = 0;
    m_receivedQrCodeChunksCount = 0;

    #if defined Q_OS_IOS
    m_isQrCodeProcessed = true;
    #endif
    #if defined Q_OS_ANDROID
    AndroidController::instance()->startQrReaderActivity();
    #endif
}

void ImportController::stopDecodingQr()
{
    emit qrDecodingFinished();
}

bool ImportController::parseQrCodeChunk(const QString &code)
{
    // qDebug() << code;
    if (!m_isQrCodeProcessed)
        return false;

    // check if chunk received
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

        if (m_qrCodeChunks.size() == m_totalQrCodeChunksCount) {
            QByteArray data;

            for (int i = 0; i < m_totalQrCodeChunksCount; ++i) {
                data.append(m_qrCodeChunks.value(i));
            }

            bool ok = extractConfigFromQr(data);
            if (ok) {
                m_isQrCodeProcessed = false;
                qDebug() << "stopDecodingQr";
                stopDecodingQr();
                return true;
            } else {
                qDebug() << "error while extracting data from qr";
                m_qrCodeChunks.clear();
                m_totalQrCodeChunksCount = 0;
                m_receivedQrCodeChunksCount = 0;
            }
        }
    } else {
        bool ok = extractConfigFromQr(ba);
        if (ok) {
            m_isQrCodeProcessed = false;
            qDebug() << "stopDecodingQr";
            stopDecodingQr();
            return true;
        }
    }
    return false;
}

double ImportController::getQrCodeScanProgressBarValue()
{
    return (1.0 / m_totalQrCodeChunksCount) * m_receivedQrCodeChunksCount;
}

QString ImportController::getQrCodeScanProgressString()
{
    return tr("Scanned %1 of %2.").arg(m_receivedQrCodeChunksCount).arg(m_totalQrCodeChunksCount);
}
#endif

void ImportController::checkForMaliciousStrings(const QSharedPointer<ServerConfig> &serverConfig)
{
    if (!serverConfig) return;
    
    auto selfHostedConfig = qSharedPointerCast<SelfHostedServerConfig>(serverConfig);
    if (!selfHostedConfig) return;

    for (const auto &containerEntry : selfHostedConfig->containerConfigs) {
        const ContainerConfig &containerConfig = containerEntry.second;
        
        if (containerConfig.container == DockerContainer::OpenVpn || 
            containerConfig.container == DockerContainer::Cloak || 
            containerConfig.container == DockerContainer::ShadowSocks) {

            QString protocolConfigJson;
            
            for (const auto &protocolEntry : containerConfig.protocolConfigs) {
                auto protocolConfig = protocolEntry.second;
                ProtocolConfigVariant variant = ProtocolConfig::getProtocolConfigVariant(protocolConfig);
                
                std::visit([&protocolConfigJson](const auto &config) -> void {
                    using ConfigType = std::decay_t<decltype(*config)>;
                    if constexpr (std::is_same_v<ConfigType, OpenVpnProtocolConfig> ||
                                  std::is_same_v<ConfigType, CloakProtocolConfig> ||
                                  std::is_same_v<ConfigType, ShadowsocksProtocolConfig>) {
                        protocolConfigJson = config->clientProtocolConfig.nativeConfig;
                    }
                }, variant);
                
                if (!protocolConfigJson.isEmpty()) {
                    break;
                }
            }

            if (!protocolConfigJson.isEmpty()) {
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

            m_maliciousWarningText = tr("This configuration contains an OpenVPN setup. OpenVPN configurations can include malicious "
                                        "scripts, so only add it if you fully trust the provider of this config. ");

            if (!maliciousStrings.isEmpty()) {
                m_maliciousWarningText.push_back(tr("<br>In the imported configuration, potentially dangerous lines were found:"));
                for (const auto &string : maliciousStrings) {
                    m_maliciousWarningText.push_back(QString("<br><i>%1</i>").arg(string));
                    }
                }
            }
        }
    }
}

void ImportController::processAmneziaConfig(QSharedPointer<ServerConfig> &config)
{
    if (!config) return;
    
    auto selfHostedConfig = qSharedPointerCast<SelfHostedServerConfig>(config);
    if (!selfHostedConfig) return;
    
    for (auto &containerEntry : selfHostedConfig->containerConfigs) {
        QString containerName = containerEntry.first;
        ContainerConfig &containerConfig = containerEntry.second;
        
        DockerContainer dockerContainer = ContainerProps::containerFromString(containerName);
        if (dockerContainer == DockerContainer::Awg || dockerContainer == DockerContainer::WireGuard) {
            QString protocolName = ContainerProps::containerTypeToString(dockerContainer);
            if (containerConfig.protocolConfigs.contains(protocolName)) {
                auto protocolConfig = containerConfig.protocolConfigs.value(protocolName);
                
                if (auto wgConfig = qSharedPointerCast<WireGuardProtocolConfig>(protocolConfig)) {
                    wgConfig->serverProtocolConfig.mtu = 
                        dockerContainer == DockerContainer::Awg ? protocols::awg::defaultMtu : protocols::wireguard::defaultMtu;
                } else if (auto awgConfig = qSharedPointerCast<AwgProtocolConfig>(protocolConfig)) {
                    awgConfig->serverProtocolConfig.mtu = 
                        dockerContainer == DockerContainer::Awg ? protocols::awg::defaultMtu : protocols::wireguard::defaultMtu;
                }
            }
        }
    }
}

