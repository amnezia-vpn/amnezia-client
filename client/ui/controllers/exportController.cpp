#include "exportController.h"

#include <QBuffer>
#include <QDataStream>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QStandardPaths>

#include "configurators/openvpn_configurator.h"
#include "configurators/wireguard_configurator.h"
#include "core/errorstrings.h"
#include "systemController.h"
#ifdef Q_OS_ANDROID
    #include "platforms/android/androidutils.h"
#endif
#include "qrcodegen.hpp"

ExportController::ExportController(const QSharedPointer<ServersModel> &serversModel,
                                   const QSharedPointer<ContainersModel> &containersModel,
                                   const std::shared_ptr<Settings> &settings,
                                   const std::shared_ptr<VpnConfigurator> &configurator, QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
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

    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    QJsonObject config = m_settings->server(serverIndex);

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

void ExportController::generateConnectionConfig()
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    ServerCredentials credentials =
            qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));

    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getCurrentlyProcessedContainerIndex());
    QModelIndex containerModelIndex = m_containersModel->index(container);
    QJsonObject containerConfig =
            qvariant_cast<QJsonObject>(m_containersModel->data(containerModelIndex, ContainersModel::Roles::ConfigRole));
    containerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    ErrorCode errorCode = ErrorCode::NoError;
    for (Proto protocol : ContainerProps::protocolsForContainer(container)) {
        QJsonObject protocolConfig = m_settings->protocolConfig(serverIndex, container, protocol);

        QString vpnConfig =
                m_configurator->genVpnProtocolConfig(credentials, container, containerConfig, protocol, &errorCode);
        if (errorCode) {
            emit exportErrorOccurred(errorString(errorCode));
            return;
        }
        protocolConfig.insert(config_key::last_config, vpnConfig);
        containerConfig.insert(ProtocolProps::protoToString(protocol), protocolConfig);
    }

    QJsonObject config = m_settings->server(serverIndex);
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

void ExportController::generateOpenVpnConfig()
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    ServerCredentials credentials =
            qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));

    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getCurrentlyProcessedContainerIndex());
    QModelIndex containerModelIndex = m_containersModel->index(container);
    QJsonObject containerConfig =
            qvariant_cast<QJsonObject>(m_containersModel->data(containerModelIndex, ContainersModel::Roles::ConfigRole));
    containerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    ErrorCode errorCode = ErrorCode::NoError;
    QString config =
            m_configurator->openVpnConfigurator->genOpenVpnConfig(credentials, container, containerConfig, &errorCode);
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

    emit exportConfigChanged();
}

void ExportController::generateWireGuardConfig()
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    ServerCredentials credentials =
            qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));

    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getCurrentlyProcessedContainerIndex());
    QModelIndex containerModelIndex = m_containersModel->index(container);
    QJsonObject containerConfig =
            qvariant_cast<QJsonObject>(m_containersModel->data(containerModelIndex, ContainersModel::Roles::ConfigRole));
    containerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    ErrorCode errorCode = ErrorCode::NoError;
    QString config = m_configurator->wireguardConfigurator->genWireguardConfig(credentials, container, containerConfig,
                                                                               &errorCode);
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

    emit exportConfigChanged();
}

QString ExportController::getConfig()
{
    return m_config;
}

QList<QString> ExportController::getQrCodes()
{
    return m_qrCodes;
}

void ExportController::exportConfig(const QString &fileName)
{
    SystemController::saveFile(fileName, m_config);
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
        QString svg = QString::fromStdString(toSvgString(qr, 0));
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
    m_qrCodes.clear();
}
