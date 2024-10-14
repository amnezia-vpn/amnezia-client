#include "importController.h"

#include <QFile>
#include <QFileInfo>
#include <QQuickItem>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QUrlQuery>

#include "core/errorstrings.h"
#include "core/serialization/serialization.h"
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
        const QString openVpnConfigPatternProto1 = "proto tcp";
        const QString openVpnConfigPatternProto2 = "proto udp";
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
        } else if (config.contains(amneziaConfigPattern) || config.contains(amneziaFreeConfigPattern) || config.contains(amneziaPremiumConfigPattern)
                   || (config.contains(amneziaConfigPatternHostName) && config.contains(amneziaConfigPatternUserName)
                       && config.contains(amneziaConfigPatternPassword))) {
            return ConfigTypes::Amnezia;
        } else if (config.contains(openVpnConfigPatternCli)
                   && (config.contains(openVpnConfigPatternProto1) || config.contains(openVpnConfigPatternProto2))
                   && (config.contains(openVpnConfigPatternDriver1) || config.contains(openVpnConfigPatternDriver2))) {
            return ConfigTypes::OpenVpn;
        } else if (config.contains(wireguardConfigPatternSectionInterface) && config.contains(wireguardConfigPatternSectionPeer)) {
            return ConfigTypes::WireGuard;
        } else if ((config.contains(xrayConfigPatternInbound)) && (config.contains(xrayConfigPatternOutbound))) {
            return ConfigTypes::Xray;
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
    QFile file(fileName);

    if (file.open(QIODevice::ReadOnly)) {
        QString data = file.readAll();

        m_configFileName = QFileInfo(file.fileName()).fileName();
        return extractConfigFromData(data);
    }

    emit importErrorOccurred(ErrorCode::ImportOpenConfigError, false);
    return false;
}

bool ImportController::extractConfigFromData(QString data)
{
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
        data.replace("vpn://", "");
        QByteArray ba = QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        QByteArray ba_uncompressed = qUncompress(ba);
        if (!ba_uncompressed.isEmpty()) {
            ba = ba_uncompressed;
        }

        config = ba;
        m_configType = checkConfigFormat(config);
    }

    switch (m_configType) {
    case ConfigTypes::OpenVpn: {
        m_config = extractOpenVpnConfig(config);
        if (!m_config.empty()) {
            checkForMaliciousStrings(m_config);
            return true;
        }
        return false;
    }
    case ConfigTypes::Awg:
    case ConfigTypes::WireGuard: {
        m_config = extractWireGuardConfig(config);
        return m_config.empty() ? false : true;
    }
    case ConfigTypes::Xray: {
        m_config = extractXrayConfig(config);
        return m_config.empty() ? false : true;
    }
    case ConfigTypes::Amnezia: {
        m_config = QJsonDocument::fromJson(config.toUtf8()).object();
        processAmneziaConfig(m_config);
        if (!m_config.empty()) {
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
        m_config = dataObj;
        return true;
    }

    QByteArray ba_uncompressed = qUncompress(data);
    if (!ba_uncompressed.isEmpty()) {
        m_config = QJsonDocument::fromJson(ba_uncompressed).object();
        return true;
    }

    return false;
}

QString ImportController::getConfig()
{
    return QJsonDocument(m_config).toJson(QJsonDocument::Indented);
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
    auto containers = m_config.value(config_key::containers).toArray();
    if (!containers.isEmpty()) {
        auto container = containers.at(0).toObject();
        auto serverProtocolConfig = container.value(ContainerProps::containerTypeToString(DockerContainer::WireGuard)).toObject();
        auto clientProtocolConfig = QJsonDocument::fromJson(serverProtocolConfig.value(config_key::last_config).toString().toUtf8()).object();

        QString junkPacketCount = QString::number(QRandomGenerator::global()->bounded(2, 5));
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

        clientProtocolConfig[config_key::isObfuscationEnabled] = true;

        serverProtocolConfig[config_key::last_config] = QString(QJsonDocument(clientProtocolConfig).toJson());
        container["wireguard"] = serverProtocolConfig;
        containers.replace(0, container);
        m_config[config_key::containers] = containers;
    }
}

void ImportController::importConfig()
{
    ServerCredentials credentials;
    credentials.hostName = m_config.value(config_key::hostName).toString();
    credentials.port = m_config.value(config_key::port).toInt();
    credentials.userName = m_config.value(config_key::userName).toString();
    credentials.secretData = m_config.value(config_key::password).toString();

    if (credentials.isValid() || m_config.contains(config_key::containers)) {
        m_serversModel->addServer(m_config);
        emit importFinished();
    } else if (m_config.contains(config_key::configVersion)) {
        quint16 crc = qChecksum(QJsonDocument(m_config).toJson());
        if (m_serversModel->isServerFromApiAlreadyExists(crc)) {
            emit importErrorOccurred(ErrorCode::ApiConfigAlreadyAdded, true);
        } else {
            m_config.insert(config_key::crc, crc);

            m_serversModel->addServer(m_config);
            emit importFinished();
        }
    } else {
        qDebug() << "Failed to import profile";
        qDebug().noquote() << QJsonDocument(m_config).toJson();
        emit importErrorOccurred(ErrorCode::ImportInvalidConfigError, false);
    }

    m_config = {};
    m_configFileName.clear();
    m_maliciousWarningText.clear();
}

QJsonObject ImportController::extractOpenVpnConfig(const QString &data)
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
    const static QRegularExpression hostNameRegExp("remote (.*) [0-9]*");
    QRegularExpressionMatch hostNameMatch = hostNameRegExp.match(data);
    if (hostNameMatch.hasMatch()) {
        hostName = hostNameMatch.captured(1);
    }

    QJsonObject config;
    config[config_key::containers] = arr;
    config[config_key::defaultContainer] = "amnezia-openvpn";
    config[config_key::description] = m_settings->nextAvailableServerName();

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

QJsonObject ImportController::extractWireGuardConfig(const QString &data)
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
        emit importErrorOccurred(ErrorCode::ImportInvalidConfigError, false);
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
        emit importErrorOccurred(ErrorCode::ImportInvalidConfigError, false);
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
    if (!configMap.value(config_key::junkPacketCount).isEmpty() && !configMap.value(config_key::junkPacketMinSize).isEmpty()
        && !configMap.value(config_key::junkPacketMaxSize).isEmpty() && !configMap.value(config_key::initPacketJunkSize).isEmpty()
        && !configMap.value(config_key::responsePacketJunkSize).isEmpty() && !configMap.value(config_key::initPacketMagicHeader).isEmpty()
        && !configMap.value(config_key::responsePacketMagicHeader).isEmpty()
        && !configMap.value(config_key::underloadPacketMagicHeader).isEmpty()
        && !configMap.value(config_key::transportPacketMagicHeader).isEmpty()) {
        lastConfig[config_key::junkPacketCount] = configMap.value(config_key::junkPacketCount);
        lastConfig[config_key::junkPacketMinSize] = configMap.value(config_key::junkPacketMinSize);
        lastConfig[config_key::junkPacketMaxSize] = configMap.value(config_key::junkPacketMaxSize);
        lastConfig[config_key::initPacketJunkSize] = configMap.value(config_key::initPacketJunkSize);
        lastConfig[config_key::responsePacketJunkSize] = configMap.value(config_key::responsePacketJunkSize);
        lastConfig[config_key::initPacketMagicHeader] = configMap.value(config_key::initPacketMagicHeader);
        lastConfig[config_key::responsePacketMagicHeader] = configMap.value(config_key::responsePacketMagicHeader);
        lastConfig[config_key::underloadPacketMagicHeader] = configMap.value(config_key::underloadPacketMagicHeader);
        lastConfig[config_key::transportPacketMagicHeader] = configMap.value(config_key::transportPacketMagicHeader);
        protocolName = "awg";
        m_configType = ConfigTypes::Awg;
    }

    if (!configMap.value("MTU").isEmpty()) {
        lastConfig[config_key::mtu] = configMap.value("MTU");
    } else {
        lastConfig[config_key::mtu] = protocolName == "awg" ? protocols::awg::defaultMtu : protocols::wireguard::defaultMtu;
    }

    QJsonObject wireguardConfig;
    wireguardConfig[config_key::last_config] = QString(QJsonDocument(lastConfig).toJson());
    wireguardConfig[config_key::isThirdPartyConfig] = true;
    wireguardConfig[config_key::port] = port;
    wireguardConfig[config_key::transport_proto] = "udp";

    QJsonObject containers;
    containers.insert(config_key::container, QJsonValue("amnezia-" + protocolName));
    containers.insert(protocolName, QJsonValue(wireguardConfig));

    QJsonArray arr;
    arr.push_back(containers);

    QJsonObject config;
    config[config_key::containers] = arr;
    config[config_key::defaultContainer] = "amnezia-" + protocolName;
    config[config_key::description] = m_settings->nextAvailableServerName();

    const static QRegularExpression dnsRegExp(
            "DNS = "
            "(\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b).*(\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b)");
    QRegularExpressionMatch dnsMatch = dnsRegExp.match(data);
    if (dnsMatch.hasMatch()) {
        config[config_key::dns1] = dnsMatch.captured(1);
        config[config_key::dns2] = dnsMatch.captured(2);
    }

    config[config_key::hostName] = hostName;

    return config;
}

QJsonObject ImportController::extractXrayConfig(const QString &data, const QString &description)
{
    QJsonParseError parserErr;
    QJsonDocument jsonConf = QJsonDocument::fromJson(data.toLocal8Bit(), &parserErr);

    QJsonObject xrayVpnConfig;
    xrayVpnConfig[config_key::config] = jsonConf.toJson().constData();
    QJsonObject lastConfig;
    lastConfig[config_key::last_config] = jsonConf.toJson().constData();
    lastConfig[config_key::isThirdPartyConfig] = true;

    QJsonObject containers;
    if (m_configType == ConfigTypes::ShadowSocks) {
        containers.insert(config_key::ssxray, QJsonValue(lastConfig));
        containers.insert(config_key::container, QJsonValue("amnezia-ssxray"));
    } else {
        containers.insert(config_key::container, QJsonValue("amnezia-xray"));
        containers.insert(config_key::xray, QJsonValue(lastConfig));
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
    config[config_key::containers] = arr;

    if (m_configType == ConfigTypes::ShadowSocks) {
        config[config_key::defaultContainer] = "amnezia-ssxray";
    } else {
        config[config_key::defaultContainer] = "amnezia-xray";
    }
    if (description.isEmpty()) {
        config[config_key::description] = m_settings->nextAvailableServerName();
    } else {
        config[config_key::description] = description;
    }
    config[config_key::hostName] = hostName;

    return config;
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

    if (magic == amnezia::qrMagicCode) {
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

void ImportController::checkForMaliciousStrings(const QJsonObject &serverConfig)
{
    const QJsonArray &containers = serverConfig[config_key::containers].toArray();
    for (const QJsonValue &container : containers) {
        auto containerConfig = container.toObject();
        auto containerName = containerConfig[config_key::container].toString();
        if ((containerName == ContainerProps::containerToString(DockerContainer::OpenVpn))
            || (containerName == ContainerProps::containerToString(DockerContainer::Cloak))
            || (containerName == ContainerProps::containerToString(DockerContainer::ShadowSocks))) {
            QString protocolConfig =
                    containerConfig[ProtocolProps::protoToString(Proto::OpenVpn)].toObject()[config_key::last_config].toString();
            QString protocolConfigJson = QJsonDocument::fromJson(protocolConfig.toUtf8()).object()[config_key::config].toString();

            const QRegularExpression regExp { "(\\w+-\\w+|\\w+)" };
            const size_t dangerousTagsMaxCount = 3;

            // https://github.com/OpenVPN/openvpn/blob/master/doc/man-sections/script-options.rst
            QStringList dangerousTags {
                "up", "tls-verify", "ipchange", "client-connect", "route-up", "route-pre-down", "client-disconnect", "down", "learn-address", "auth-user-pass-verify"
            };

            QStringList maliciousStrings;
            QStringList lines = protocolConfigJson.replace("\r", "").split("\n");
            for (const QString &l : lines) {
                QRegularExpressionMatch match = regExp.match(l);
                if (dangerousTags.contains(match.captured(0))) {
                    maliciousStrings << l;
                }
            }

            if (maliciousStrings.size() >= dangerousTagsMaxCount) {
                m_maliciousWarningText = tr("In the imported configuration, potentially dangerous lines were found:");
                for (const auto &string : maliciousStrings) {
                    m_maliciousWarningText.push_back(QString("<br><i>%1</i>").arg(string));
                }
            }
        }
    }
}

void ImportController::processAmneziaConfig(QJsonObject &config)
{
    auto containers = config.value(config_key::containers).toArray();
    for (auto i = 0; i < containers.size(); i++) {
        auto container = containers.at(i).toObject();
        auto dockerContainer = ContainerProps::containerFromString(container.value(config_key::container).toString());
        if (dockerContainer == DockerContainer::Awg || dockerContainer == DockerContainer::WireGuard) {
            auto containerConfig = container.value(ContainerProps::containerTypeToString(dockerContainer)).toObject();
            auto protocolConfig = containerConfig.value(config_key::last_config).toString();
            if (protocolConfig.isEmpty()) {
                return;
            }

            QJsonObject jsonConfig = QJsonDocument::fromJson(protocolConfig.toUtf8()).object();
            jsonConfig[config_key::mtu] = dockerContainer == DockerContainer::Awg ? protocols::awg::defaultMtu : protocols::wireguard::defaultMtu;

            containerConfig[config_key::last_config] = QString(QJsonDocument(jsonConfig).toJson());

            container[ContainerProps::containerTypeToString(dockerContainer)] = containerConfig;
            containers.replace(i, container);
            config.insert(config_key::containers, containers);
        }
    }
}
