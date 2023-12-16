#include "importController.h"

#include <QFile>
#include <QFileInfo>
#include <QQuickItem>
#include <QStandardPaths>

#include "core/errorstrings.h"
#ifdef Q_OS_ANDROID
    #include "../../platforms/android/android_controller.h"
    #include "../../platforms/android/androidutils.h"
    #include <QJniObject>
#endif
#ifdef Q_OS_IOS
    #include <CoreFoundation/CoreFoundation.h>
#endif

namespace
{
    enum class ConfigTypes {
        Amnezia,
        OpenVpn,
        WireGuard
    };

    ConfigTypes checkConfigFormat(const QString &config)
    {
        const QString openVpnConfigPatternCli = "client";
        const QString openVpnConfigPatternProto1 = "proto tcp";
        const QString openVpnConfigPatternProto2 = "proto udp";
        const QString openVpnConfigPatternDriver1 = "dev tun";
        const QString openVpnConfigPatternDriver2 = "dev tap";

        const QString wireguardConfigPatternSectionInterface = "[Interface]";
        const QString wireguardConfigPatternSectionPeer = "[Peer]";

        if (config.contains(openVpnConfigPatternCli)
            && (config.contains(openVpnConfigPatternProto1) || config.contains(openVpnConfigPatternProto2))
            && (config.contains(openVpnConfigPatternDriver1) || config.contains(openVpnConfigPatternDriver2))) {
            return ConfigTypes::OpenVpn;
        } else if (config.contains(wireguardConfigPatternSectionInterface)
                   && config.contains(wireguardConfigPatternSectionPeer)) {
            return ConfigTypes::WireGuard;
        }
        return ConfigTypes::Amnezia;
    }

#if defined Q_OS_ANDROID
    ImportController *mInstance = nullptr;
#endif

#ifdef Q_OS_ANDROID
    constexpr auto AndroidCameraActivity = "org.amnezia.vpn.qt.CameraActivity";
#endif
} // namespace

ImportController::ImportController(const QSharedPointer<ServersModel> &serversModel,
                                   const QSharedPointer<ContainersModel> &containersModel,
                                   const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_containersModel(containersModel), m_settings(settings)
{
#ifdef Q_OS_ANDROID
    mInstance = this;

    AndroidUtils::runOnAndroidThreadAsync([]() {
        JNINativeMethod methods[] {
            { "passDataToDecoder", "(Ljava/lang/String;)V", reinterpret_cast<void *>(onNewQrCodeDataChunk) },
        };

        QJniObject javaClass(AndroidCameraActivity);
        QJniEnvironment env;
        jclass objectClass = env->GetObjectClass(javaClass.object<jobject>());
        env->RegisterNatives(objectClass, methods, sizeof(methods) / sizeof(methods[0]));
        env->DeleteLocalRef(objectClass);
    });
#endif
}

void ImportController::extractConfigFromFile(const QString &fileName)
{
    QFile file(fileName);

    if (file.open(QIODevice::ReadOnly)) {
        QString data = file.readAll();

        extractConfigFromData(data);
        m_configFileName = QFileInfo(file.fileName()).fileName();
    }
}

void ImportController::extractConfigFromData(QString data)
{
    auto configFormat = checkConfigFormat(data);
    if (configFormat == ConfigTypes::OpenVpn) {
        m_config = extractOpenVpnConfig(data);
    } else if (configFormat == ConfigTypes::WireGuard) {
        m_config = extractWireGuardConfig(data);
    } else {
        m_config = extractAmneziaConfig(data);
    }
}

void ImportController::extractConfigFromCode(QString code)
{
    m_config = extractAmneziaConfig(code);
    m_configFileName = "";
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

void ImportController::importConfig()
{
    ServerCredentials credentials;
    credentials.hostName = m_config.value(config_key::hostName).toString();
    credentials.port = m_config.value(config_key::port).toInt();
    credentials.userName = m_config.value(config_key::userName).toString();
    credentials.secretData = m_config.value(config_key::password).toString();

    if (credentials.isValid()
        || m_config.contains(config_key::containers)
        || m_config.contains(config_key::configVersion)) { // todo
        m_serversModel->addServer(m_config);

        emit importFinished();
    } else {
        qDebug() << "Failed to import profile";
        qDebug().noquote() << QJsonDocument(m_config).toJson();
        emit importErrorOccurred(errorString(ErrorCode::ImportInvalidConfigError));
    }

    m_config = {};
    m_configFileName.clear();
}

QJsonObject ImportController::extractAmneziaConfig(QString &data)
{
    data.replace("vpn://", "");
    QByteArray ba = QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    QByteArray ba_uncompressed = qUncompress(ba);
    if (!ba_uncompressed.isEmpty()) {
        ba = ba_uncompressed;
    }

    QJsonObject config = QJsonDocument::fromJson(ba).object();

    return config;
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

    const static QRegularExpression hostNameAndPortRegExp("Endpoint = (.*):([0-9]*)");
    QRegularExpressionMatch hostNameAndPortMatch = hostNameAndPortRegExp.match(data);
    QString hostName;
    QString port;
    if (hostNameAndPortMatch.hasCaptured(1)) {
        hostName = hostNameAndPortMatch.captured(1);
    } else {
        qDebug() << "Failed to import profile";
        emit importErrorOccurred(errorString(ErrorCode::ImportInvalidConfigError));
    }

    if (hostNameAndPortMatch.hasCaptured(2)) {
        port = hostNameAndPortMatch.captured(2);
    } else {
        port = protocols::wireguard::defaultPort;
    }

    lastConfig[config_key::hostName] = hostName;
    lastConfig[config_key::port] = port.toInt();

//    if (!configMap.value("PrivateKey").isEmpty() && !configMap.value("Address").isEmpty()
//        && !configMap.value("PresharedKey").isEmpty() && !configMap.value("PublicKey").isEmpty()) {
        lastConfig[config_key::client_priv_key] = configMap.value("PrivateKey");
        lastConfig[config_key::client_ip] = configMap.value("Address");
        lastConfig[config_key::psk_key] = configMap.value("PresharedKey");
        lastConfig[config_key::server_pub_key] = configMap.value("PublicKey");
//    } else {
//        qDebug() << "Failed to import profile";
//        emit importErrorOccurred(errorString(ErrorCode::ImportInvalidConfigError));
//        return QJsonObject();
//    }

    QJsonArray allowedIpsJsonArray = QJsonArray::fromStringList(configMap.value("AllowedIPs").split(","));

    lastConfig[config_key::allowed_ips] = allowedIpsJsonArray;

    QString protocolName = "wireguard";
    if (!configMap.value(config_key::junkPacketCount).isEmpty()
        && !configMap.value(config_key::junkPacketMinSize).isEmpty()
        && !configMap.value(config_key::junkPacketMaxSize).isEmpty()
        && !configMap.value(config_key::initPacketJunkSize).isEmpty()
        && !configMap.value(config_key::responsePacketJunkSize).isEmpty()
        && !configMap.value(config_key::initPacketMagicHeader).isEmpty()
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

#ifdef Q_OS_ANDROID
void ImportController::onNewQrCodeDataChunk(JNIEnv *env, jobject thiz, jstring data)
{
    Q_UNUSED(thiz);
    const char *buffer = env->GetStringUTFChars(data, nullptr);
    if (!buffer) {
        return;
    }

    QString parcelBody(buffer);
    env->ReleaseStringUTFChars(data, buffer);

    if (mInstance != nullptr) {
        if (!mInstance->m_isQrCodeProcessed) {
            mInstance->m_qrCodeChunks.clear();
            mInstance->m_isQrCodeProcessed = true;
            mInstance->m_totalQrCodeChunksCount = 0;
            mInstance->m_receivedQrCodeChunksCount = 0;
        }
        mInstance->parseQrCodeChunk(parcelBody);
    }
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
    #if defined Q_OS_ANDROID
    QJniObject::callStaticMethod<void>(AndroidCameraActivity, "stopQrCodeReader", "()V");
    #endif
    emit qrDecodingFinished();
}

void ImportController::parseQrCodeChunk(const QString &code)
{
    // qDebug() << code;
    if (!m_isQrCodeProcessed)
        return;

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
        }
    }
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
