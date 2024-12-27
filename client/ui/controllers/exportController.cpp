#include "exportController.h"

#include <QBuffer>
#include <QDataStream>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QStandardPaths>

#include "core/controllers/vpnConfigurationController.h"
#include "systemController.h"
#include "qrcodegen.hpp"

ExportController::ExportController(const QSharedPointer<ServersModel> &serversModel, const QSharedPointer<ContainersModel> &containersModel,
                                   const QSharedPointer<ClientManagementModel> &clientManagementModel,
                                   const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_clientManagementModel(clientManagementModel),
      m_settings(settings)
{
}

void ExportController::generateFullAccessConfig()
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    QJsonObject serverConfig = m_serversModel->getServerConfig(serverIndex);

    QJsonArray containers = serverConfig.value(config_key::containers).toArray();
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
    serverConfig[config_key::containers] = containers;

    QByteArray compressedConfig = QJsonDocument(serverConfig).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    m_config = QString("vpn://%1").arg(QString(compressedConfig.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));

    m_qrCodes = generateQrCodeImageSeries(compressedConfig);
    emit exportConfigChanged();
}

void ExportController::generateConnectionConfig(const QString &clientName)
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);

    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getProcessedContainerIndex());
    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    containerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);
    ErrorCode errorCode = vpnConfigurationController.createProtocolConfigForContainer(credentials, container, containerConfig);

    errorCode = m_clientManagementModel->appendClient(container, credentials, containerConfig, clientName, serverController);
    if (errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(errorCode);
        return;
    }

    QJsonObject serverConfig = m_serversModel->getServerConfig(serverIndex);
    if (!errorCode) {
        serverConfig.remove(config_key::userName);
        serverConfig.remove(config_key::password);
        serverConfig.remove(config_key::port);
        serverConfig.insert(config_key::containers, QJsonArray { containerConfig });
        serverConfig.insert(config_key::defaultContainer, ContainerProps::containerToString(container));

        auto dns = m_serversModel->getDnsPair(serverIndex);
        serverConfig.insert(config_key::dns1, dns.first);
        serverConfig.insert(config_key::dns2, dns.second);
    }

    QByteArray compressedConfig = QJsonDocument(serverConfig).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    m_config = QString("vpn://%1").arg(QString(compressedConfig.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));

    m_qrCodes = generateQrCodeImageSeries(compressedConfig);
    emit exportConfigChanged();
}

ErrorCode ExportController::generateNativeConfig(const DockerContainer container, const QString &clientName, const Proto &protocol,
                                                 QJsonObject &jsonNativeConfig)
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    auto dns = m_serversModel->getDnsPair(serverIndex);
    bool isApiConfig = qvariant_cast<bool>(m_serversModel->data(serverIndex, ServersModel::IsServerFromTelegramApiRole));

    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    containerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

    QString protocolConfigString;
    ErrorCode errorCode = vpnConfigurationController.createProtocolConfigString(isApiConfig, dns, credentials, container, containerConfig,
                                                                                protocol, protocolConfigString);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    jsonNativeConfig = QJsonDocument::fromJson(protocolConfigString.toUtf8()).object();

    if (protocol == Proto::OpenVpn || protocol == Proto::WireGuard || protocol == Proto::Awg || protocol == Proto::Xray) {
        errorCode = m_clientManagementModel->appendClient(jsonNativeConfig, clientName, container, credentials, serverController);
    }
    return errorCode;
}

void ExportController::generateOpenVpnConfig(const QString &clientName)
{
    QJsonObject nativeConfig;
    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getProcessedContainerIndex());
    ErrorCode errorCode = ErrorCode::NoError;

    if (container == DockerContainer::Cloak || container == DockerContainer::ShadowSocks) {
        errorCode = generateNativeConfig(container, clientName, Proto::OpenVpn, nativeConfig);
    } else {
        errorCode = generateNativeConfig(container, clientName, ContainerProps::defaultProtocol(container), nativeConfig);
    }

    if (errorCode) {
        emit exportErrorOccurred(errorCode);
        return;
    }

    QStringList lines = nativeConfig.value(config_key::config).toString().replace("\r", "").split("\n");
    for (const QString &line : lines) {
        m_config.append(line + "\n");
    }

    m_qrCodes = generateQrCodeImageSeries(m_config.toUtf8());
    emit exportConfigChanged();
}

void ExportController::generateWireGuardConfig(const QString &clientName)
{
    QJsonObject nativeConfig;
    ErrorCode errorCode = generateNativeConfig(DockerContainer::WireGuard, clientName, Proto::WireGuard, nativeConfig);
    if (errorCode) {
        emit exportErrorOccurred(errorCode);
        return;
    }

    QStringList lines = nativeConfig.value(config_key::config).toString().replace("\r", "").split("\n");
    for (const QString &line : lines) {
        m_config.append(line + "\n");
    }

    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(m_config.toUtf8(), qrcodegen::QrCode::Ecc::LOW);
    m_qrCodes << svgToBase64(QString::fromStdString(toSvgString(qr, 1)));

    emit exportConfigChanged();
}

void ExportController::generateAwgConfig(const QString &clientName)
{
    QJsonObject nativeConfig;
    ErrorCode errorCode = generateNativeConfig(DockerContainer::Awg, clientName, Proto::Awg, nativeConfig);
    if (errorCode) {
        emit exportErrorOccurred(errorCode);
        return;
    }

    QStringList lines = nativeConfig.value(config_key::config).toString().replace("\r", "").split("\n");
    for (const QString &line : lines) {
        m_config.append(line + "\n");
    }

    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(m_config.toUtf8(), qrcodegen::QrCode::Ecc::LOW);
    m_qrCodes << svgToBase64(QString::fromStdString(toSvgString(qr, 1)));

    emit exportConfigChanged();
}

void ExportController::generateShadowSocksConfig()
{
    QJsonObject nativeConfig;
    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getProcessedContainerIndex());
    ErrorCode errorCode = ErrorCode::NoError;

    if (container == DockerContainer::Cloak) {
        errorCode = generateNativeConfig(container, "", Proto::ShadowSocks, nativeConfig);
    } else {
        errorCode = generateNativeConfig(container, "", ContainerProps::defaultProtocol(container), nativeConfig);
    }

    if (errorCode) {
        emit exportErrorOccurred(errorCode);
        return;
    }

    QStringList lines = QString(QJsonDocument(nativeConfig).toJson()).replace("\r", "").split("\n");
    for (const QString &line : lines) {
        m_config.append(line + "\n");
    }

    m_nativeConfigString = QString("%1:%2@%3:%4")
                                   .arg(nativeConfig.value("method").toString(), nativeConfig.value("password").toString(),
                                        nativeConfig.value("server").toString(), nativeConfig.value("server_port").toString());

    m_nativeConfigString = "ss://" + m_nativeConfigString.toUtf8().toBase64();

    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(m_nativeConfigString.toUtf8(), qrcodegen::QrCode::Ecc::LOW);
    m_qrCodes << svgToBase64(QString::fromStdString(toSvgString(qr, 1)));

    emit exportConfigChanged();
}

void ExportController::generateCloakConfig()
{
    QJsonObject nativeConfig;
    ErrorCode errorCode = generateNativeConfig(DockerContainer::Cloak, "", Proto::Cloak, nativeConfig);
    if (errorCode) {
        emit exportErrorOccurred(errorCode);
        return;
    }

    nativeConfig.remove(config_key::transport_proto);
    nativeConfig.insert("ProxyMethod", "shadowsocks");

    QStringList lines = QString(QJsonDocument(nativeConfig).toJson()).replace("\r", "").split("\n");
    for (const QString &line : lines) {
        m_config.append(line + "\n");
    }

    emit exportConfigChanged();
}

void ExportController::generateXrayConfig(const QString &clientName)
{
    QJsonObject nativeConfig;
    ErrorCode errorCode = generateNativeConfig(DockerContainer::Xray, clientName, Proto::Xray, nativeConfig);
    if (errorCode) {
        emit exportErrorOccurred(errorCode);
        return;
    }

    QStringList lines = QString(QJsonDocument(nativeConfig).toJson()).replace("\r", "").split("\n");
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
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    ErrorCode errorCode = m_clientManagementModel->updateModel(container, credentials, serverController);
    if (errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(errorCode);
    }
}

void ExportController::revokeConfig(const int row, const DockerContainer container, ServerCredentials credentials)
{
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    ErrorCode errorCode =
            m_clientManagementModel->revokeClient(row, container, credentials, m_serversModel->getProcessedServerIndex(), serverController);
    if (errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(errorCode);
    }
}

void ExportController::renameClient(const int row, const QString &clientName, const DockerContainer container, ServerCredentials credentials)
{
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    ErrorCode errorCode = m_clientManagementModel->renameClient(row, clientName, container, credentials, serverController);
    if (errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(errorCode);
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
