#include "exportController.h"

#include <QBuffer>
#include <QDataStream>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QStandardPaths>

#include "configurators/awg_configurator.h"
#include "configurators/cloak_configurator.h"
#include "configurators/openvpn_configurator.h"
#include "configurators/shadowsocks_configurator.h"
#include "configurators/wireguard_configurator.h"
#include "core/errorstrings.h"
#include "systemController.h"
#ifdef Q_OS_ANDROID
    #include "platforms/android/android_utils.h"
#endif
#include "qrcodegen.hpp"

ExportController::ExportController(const QSharedPointer<ServersModel> &serversModel,
                                   const QSharedPointer<ContainersModel> &containersModel,
                                   const QSharedPointer<ClientManagementModel> &clientManagementModel,
                                   const std::shared_ptr<Settings> &settings,
                                   const std::shared_ptr<VpnConfigurator> &configurator, QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_clientManagementModel(clientManagementModel),
      m_settings(settings),
      m_configurator(configurator)
{
#ifdef Q_OS_ANDROID
    m_authResultNotifier.reset(new AuthResultNotifier);
    m_authResultReceiver.reset(new AuthResultReceiver(m_authResultNotifier));
    connect(m_authResultNotifier.get(), &AuthResultNotifier::authFailed, this,
            [this]() { emit exportErrorOccurred(tr("Access error!")); });
    connect(m_authResultNotifier.get(), &AuthResultNotifier::authSuccessful, this,
            &ExportController::generateFullAccessConfig);
#endif
}

void ExportController::generateFullAccessConfig()
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    QJsonObject config = m_settings->server(serverIndex);

    QJsonArray containers = config.value(config_key::containers).toArray();
    for (auto i = 0; i < containers.size(); i++) {
        auto containerConfig = containers.at(i).toObject();
        auto containerType = ContainerProps::containerFromString(containerConfig.value(config_key::container).toString());

        for (auto protocol : ContainerProps::protocolsForContainer(containerType)) {
            auto protocolConfig = containerConfig.value(ProtocolProps::protoToString(protocol)).toObject();

            protocolConfig.remove(config_key::last_config);
            containerConfig[ProtocolProps::protoToString(protocol)] = protocolConfig;
        }

        containers.replace(i, containerConfig);
    }
    config[config_key::containers] = containers;

    QByteArray compressedConfig = QJsonDocument(config).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    m_config = QString("vpn://%1")
                       .arg(QString(compressedConfig.toBase64(QByteArray::Base64UrlEncoding
                                                              | QByteArray::OmitTrailingEquals)));

    m_qrCodes = generateQrCodeImageSeries(compressedConfig);
    emit exportConfigChanged();
}

#if defined(Q_OS_ANDROID)
void ExportController::generateFullAccessConfigAndroid()
{
    /* We use builtin keyguard for ssh key export protection on Android */
    QJniObject activity = AndroidUtils::getActivity();
    auto appContext = activity.callObjectMethod("getApplicationContext", "()Landroid/content/Context;");
    if (appContext.isValid()) {
        auto intent = QJniObject::callStaticObjectMethod("org/amnezia/vpn/AuthHelper", "getAuthIntent",
                                                         "(Landroid/content/Context;)Landroid/content/Intent;",
                                                         appContext.object());
        if (intent.isValid()) {
            if (intent.object<jobject>() != nullptr) {
                QtAndroidPrivate::startActivity(intent.object<jobject>(), 1, m_authResultReceiver.get());
            }
        } else {
            generateFullAccessConfig();
        }
    }
}
#endif

void ExportController::generateConnectionConfig(const QString &clientName)
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);

    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getCurrentlyProcessedContainerIndex());
    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    containerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    ErrorCode errorCode = ErrorCode::NoError;
    for (Proto protocol : ContainerProps::protocolsForContainer(container)) {
        QJsonObject protocolConfig = m_settings->protocolConfig(serverIndex, container, protocol);

        QString clientId;
        QString vpnConfig = m_configurator->genVpnProtocolConfig(credentials, container, containerConfig, protocol,
                                                                 clientId, &errorCode);
        if (errorCode) {
            emit exportErrorOccurred(errorString(errorCode));
            return;
        }
        protocolConfig.insert(config_key::last_config, vpnConfig);
        containerConfig.insert(ProtocolProps::protoToString(protocol), protocolConfig);
        if (protocol == Proto::OpenVpn || protocol == Proto::Awg || protocol == Proto::WireGuard) {
            errorCode = m_clientManagementModel->appendClient(clientId, clientName, container, credentials);
            if (errorCode) {
                emit exportErrorOccurred(errorString(errorCode));
                return;
            }
        }
    }

    QJsonObject config = m_settings->server(serverIndex); // todo change to servers_model
    if (!errorCode) {
        config.remove(config_key::userName);
        config.remove(config_key::password);
        config.remove(config_key::port);
        config.insert(config_key::containers, QJsonArray { containerConfig });
        config.insert(config_key::defaultContainer, ContainerProps::containerToString(container));

        auto dns = m_configurator->getDnsForConfig(serverIndex);
        config.insert(config_key::dns1, dns.first);
        config.insert(config_key::dns2, dns.second);
    }

    QByteArray compressedConfig = QJsonDocument(config).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    m_config = QString("vpn://%1")
                       .arg(QString(compressedConfig.toBase64(QByteArray::Base64UrlEncoding
                                                              | QByteArray::OmitTrailingEquals)));

    m_qrCodes = generateQrCodeImageSeries(compressedConfig);
    emit exportConfigChanged();
}

void ExportController::generateOpenVpnConfig(const QString &clientName)
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);

    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getCurrentlyProcessedContainerIndex());
    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    containerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    ErrorCode errorCode = ErrorCode::NoError;
    QString clientId;
    QString config = m_configurator->openVpnConfigurator->genOpenVpnConfig(credentials, container, containerConfig,
                                                                           clientId, &errorCode);
    if (errorCode) {
        emit exportErrorOccurred(errorString(errorCode));
        return;
    }
    config = m_configurator->processConfigWithExportSettings(serverIndex, container, Proto::OpenVpn, config);

    auto configJson = QJsonDocument::fromJson(config.toUtf8()).object();
    QStringList lines = configJson.value(config_key::config).toString().replace("\r", "").split("\n");
    for (const QString &line : lines) {
        m_config.append(line + "\n");
    }

    m_qrCodes = generateQrCodeImageSeries(m_config.toUtf8());

    errorCode = m_clientManagementModel->appendClient(clientId, clientName, container, credentials);
    if (errorCode) {
        emit exportErrorOccurred(errorString(errorCode));
        return;
    }

    emit exportConfigChanged();
}

void ExportController::generateWireGuardConfig(const QString &clientName)
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);

    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getCurrentlyProcessedContainerIndex());
    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    containerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    QString clientId;
    ErrorCode errorCode = ErrorCode::NoError;
    QString config = m_configurator->wireguardConfigurator->genWireguardConfig(credentials, container, containerConfig,
                                                                               clientId, &errorCode);
    if (errorCode) {
        emit exportErrorOccurred(errorString(errorCode));
        return;
    }
    config = m_configurator->processConfigWithExportSettings(serverIndex, container, Proto::WireGuard, config);

    auto configJson = QJsonDocument::fromJson(config.toUtf8()).object();
    QStringList lines = configJson.value(config_key::config).toString().replace("\r", "").split("\n");
    for (const QString &line : lines) {
        m_config.append(line + "\n");
    }

    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(m_config.toUtf8(), qrcodegen::QrCode::Ecc::LOW);
    m_qrCodes << svgToBase64(QString::fromStdString(toSvgString(qr, 1)));

    errorCode = m_clientManagementModel->appendClient(clientId, clientName, container, credentials);
    if (errorCode) {
        emit exportErrorOccurred(errorString(errorCode));
        return;
    }

    emit exportConfigChanged();
}

void ExportController::generateAwgConfig(const QString &clientName)
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);

    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getCurrentlyProcessedContainerIndex());
    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    containerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    QString clientId;
    ErrorCode errorCode = ErrorCode::NoError;
    QString config = m_configurator->awgConfigurator->genAwgConfig(credentials, container, containerConfig,
                                                                               clientId, &errorCode);
    if (errorCode) {
        emit exportErrorOccurred(errorString(errorCode));
        return;
    }
    config = m_configurator->processConfigWithExportSettings(serverIndex, container, Proto::Awg, config);

    auto configJson = QJsonDocument::fromJson(config.toUtf8()).object();
    QStringList lines = configJson.value(config_key::config).toString().replace("\r", "").split("\n");
    for (const QString &line : lines) {
        m_config.append(line + "\n");
    }

    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(m_config.toUtf8(), qrcodegen::QrCode::Ecc::LOW);
    m_qrCodes << svgToBase64(QString::fromStdString(toSvgString(qr, 1)));

    errorCode = m_clientManagementModel->appendClient(clientId, clientName, container, credentials);
    if (errorCode) {
        emit exportErrorOccurred(errorString(errorCode));
        return;
    }

    emit exportConfigChanged();
}

void ExportController::generateShadowSocksConfig()
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);

    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getCurrentlyProcessedContainerIndex());
    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    containerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    ErrorCode errorCode = ErrorCode::NoError;
    QString config = m_configurator->shadowSocksConfigurator->genShadowSocksConfig(credentials, container,
                                                                                   containerConfig, &errorCode);

    config = m_configurator->processConfigWithExportSettings(serverIndex, container, Proto::ShadowSocks, config);
    QJsonObject configJson = QJsonDocument::fromJson(config.toUtf8()).object();

    QStringList lines = QString(QJsonDocument(configJson).toJson()).replace("\r", "").split("\n");
    for (const QString &line : lines) {
        m_config.append(line + "\n");
    }

    m_nativeConfigString =
            QString("%1:%2@%3:%4")
                    .arg(configJson.value("method").toString(), configJson.value("password").toString(),
                         configJson.value("server").toString(), configJson.value("server_port").toString());

    m_nativeConfigString = "ss://" + m_nativeConfigString.toUtf8().toBase64();

    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(m_nativeConfigString.toUtf8(), qrcodegen::QrCode::Ecc::LOW);
    m_qrCodes << svgToBase64(QString::fromStdString(toSvgString(qr, 1)));

    emit exportConfigChanged();
}

void ExportController::generateCloakConfig()
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);

    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getCurrentlyProcessedContainerIndex());
    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    containerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    ErrorCode errorCode = ErrorCode::NoError;
    QString config =
            m_configurator->cloakConfigurator->genCloakConfig(credentials, container, containerConfig, &errorCode);

    if (errorCode) {
        emit exportErrorOccurred(errorString(errorCode));
        return;
    }
    config = m_configurator->processConfigWithExportSettings(serverIndex, container, Proto::Cloak, config);
    QJsonObject configJson = QJsonDocument::fromJson(config.toUtf8()).object();

    configJson.remove(config_key::transport_proto);
    configJson.insert("ProxyMethod", "shadowsocks");

    QStringList lines = QString(QJsonDocument(configJson).toJson()).replace("\r", "").split("\n");
    for (const QString &line : lines) {
        m_config.append(line + "\n");
    }

    emit exportConfigChanged();
}

QString ExportController::getConfig()
{
    return m_config;
}

QString ExportController::getNativeConfigString()
{
    return m_nativeConfigString;
}

QList<QString> ExportController::getQrCodes()
{
    return m_qrCodes;
}

void ExportController::exportConfig(const QString &fileName)
{
    SystemController::saveFile(fileName, m_config);
}

void ExportController::updateClientManagementModel(const DockerContainer container, ServerCredentials credentials)
{
    ErrorCode errorCode = m_clientManagementModel->updateModel(container, credentials);
    if (errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(errorString(errorCode));
    }
}

void ExportController::revokeConfig(const int row, const DockerContainer container, ServerCredentials credentials)
{
    ErrorCode errorCode = m_clientManagementModel->revokeClient(row, container, credentials,
                                                                m_serversModel->getProcessedServerIndex());
    if (errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(errorString(errorCode));
    }
}

void ExportController::renameClient(const int row, const QString &clientName, const DockerContainer container, ServerCredentials credentials)
{
    ErrorCode errorCode = m_clientManagementModel->renameClient(row, clientName, container, credentials);
    if (errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(errorString(errorCode));
    }
}

QList<QString> ExportController::generateQrCodeImageSeries(const QByteArray &data)
{
    double k = 850;

    quint8 chunksCount = std::ceil(data.size() / k);
    QList<QString> chunks;
    for (int i = 0; i < data.size(); i = i + k) {
        QByteArray chunk;
        QDataStream s(&chunk, QIODevice::WriteOnly);
        s << amnezia::qrMagicCode << chunksCount << (quint8)std::round(i / k) << data.mid(i, k);

        QByteArray ba = chunk.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(ba, qrcodegen::QrCode::Ecc::LOW);
        QString svg = QString::fromStdString(toSvgString(qr, 1));
        chunks.append(svgToBase64(svg));
    }

    return chunks;
}

QString ExportController::svgToBase64(const QString &image)
{
    return "data:image/svg;base64," + QString::fromLatin1(image.toUtf8().toBase64().data());
}

int ExportController::getQrCodesCount()
{
    return m_qrCodes.size();
}

void ExportController::clearPreviousConfig()
{
    m_config.clear();
    m_nativeConfigString.clear();
    m_qrCodes.clear();

    emit exportConfigChanged();
}
