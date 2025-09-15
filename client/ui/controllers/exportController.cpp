#include "exportController.h"

#include <QBuffer>
#include <QDataStream>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QStandardPaths>

#include "core/controllers/vpnConfigurationController.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/cloakProtocolConfig.h"
#include "core/models/protocols/ipsecProtocolConfig.h"
#include "core/models/protocols/openvpnProtocolConfig.h"
#include "core/models/protocols/shadowSocksProtocolConfig.h"
#include "core/models/protocols/wireguardProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/qrCodeUtils.h"
#include "systemController.h"

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
    QSharedPointer<const ServerConfig> serverConfig = m_serversModel->getServerConfig(serverIndex);
    auto selfHostedServerConfig = qSharedPointerCast<const SelfHostedServerConfig>(serverConfig);

    SelfHostedServerConfig exportServerConfig = *selfHostedServerConfig;

    for (auto &containerConfig : exportServerConfig.containerConfigs) {
        for (auto &protocolConfig : containerConfig.protocolConfigs) {
            ProtocolConfigVariant variant = ProtocolConfig::getProtocolConfigVariant(protocolConfig);
            std::visit(
                    [](const auto &ptr) -> void {
                        if constexpr (ProtocolConfig::isVpnProtocol<decltype(*ptr)>()) {
                            if (ptr)
                                ptr->clearClientSettings();
                        }
                    },
                    variant);
        }
    }

    QJsonObject exportConfigJson = exportServerConfig.toJson();
    QByteArray compressedConfig = QJsonDocument(exportConfigJson).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    m_config = QString("vpn://%1").arg(QString(compressedConfig.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));

    m_qrCodes = qrCodeUtils::generateQrCodeImageSeries(compressedConfig);
    emit exportConfigChanged();
}

void ExportController::generateConnectionConfig(const QString &clientName)
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    QSharedPointer<const ServerConfig> serverConfig = m_serversModel->getServerConfig(serverIndex);
    auto selfHostedServerConfig = qSharedPointerCast<const SelfHostedServerConfig>(serverConfig);
    const amnezia::ServerCredentials &credentials = selfHostedServerConfig->serverCredentials;

    DockerContainer containerType = static_cast<DockerContainer>(m_containersModel->getProcessedContainerIndex());

    QString containerName = ContainerProps::containerToString(containerType);
    ContainerConfig containerConfig = selfHostedServerConfig->containerConfigs[containerName];

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

    ErrorCode errorCode = vpnConfigurationController.createClientProtocolConfigs(credentials, containerConfig);

    // errorCode = m_clientManagementModel->appendClient(containerType, credentials, containerConfig, clientName, serverController);
    // if (errorCode != ErrorCode::NoError) {
    //     emit exportErrorOccurred(errorCode);
    //     return;
    // }

    SelfHostedServerConfig exportServerConfig = *selfHostedServerConfig;
    if (!errorCode) {
        exportServerConfig.containerConfigs.clear();
        exportServerConfig.containerConfigs[containerName] = containerConfig;
        exportServerConfig.defaultContainerName = containerName;
        exportServerConfig.defaultContainerType = containerType;

        auto dnsPair = m_serversModel->getDnsPair(serverIndex);
        exportServerConfig.dns1 = dnsPair.first;
        exportServerConfig.dns2 = dnsPair.second;
    }

    QJsonObject exportConfigJson = exportServerConfig.toJson();
    QByteArray compressedConfig = QJsonDocument(exportConfigJson).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    m_config = QString("vpn://%1").arg(QString(compressedConfig.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));

    m_qrCodes = qrCodeUtils::generateQrCodeImageSeries(compressedConfig);
    emit exportConfigChanged();
}

ErrorCode ExportController::generateNativeConfig(const DockerContainer container, const QString &clientName, const Proto &protocol,
                                                 QString &nativeConfig)
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    auto dnsPair = m_serversModel->getDnsPair(serverIndex);
    bool isApiConfig = qvariant_cast<bool>(m_serversModel->data(serverIndex, ServersModel::IsServerFromTelegramApiRole));
    QSharedPointer<const ServerConfig> serverConfig = m_serversModel->getServerConfig(serverIndex);
    auto selfHostedServerConfig = qSharedPointerCast<const SelfHostedServerConfig>(serverConfig);
    const amnezia::ServerCredentials &credentials = selfHostedServerConfig->serverCredentials;

    QString containerName = ContainerProps::containerToString(container);
    ContainerConfig containerConfig;
    if (selfHostedServerConfig->containerConfigs.contains(containerName)) {
        containerConfig = selfHostedServerConfig->containerConfigs[containerName];
    }
    containerConfig.containerName = containerName;
    containerConfig.containerType = container;

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

    QSharedPointer<ProtocolConfig> protocolConfig;
    ErrorCode errorCode = vpnConfigurationController.createClientProtocolConfig(credentials, containerConfig, protocol, protocolConfig);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    vpnConfigurationController.processNativeConfigForExport(dnsPair, protocolConfig);
    nativeConfig = ProtocolConfig::getNativeConfig(protocolConfig);

    // if (protocol == Proto::OpenVpn || protocol == Proto::WireGuard || protocol == Proto::Awg || protocol == Proto::Xray) {
    //     // Extract client ID from the generated native config for client management
    //     QString clientId = jsonNativeConfig.value(config_key::clientId).toString();
    //     errorCode = m_clientManagementModel->appendClient(clientId, clientName, container, credentials, serverController);
    // }
    return errorCode;
}

void ExportController::generateOpenVpnConfig(const QString &clientName)
{
    QString nativeConfig;
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

    QStringList lines = nativeConfig.replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        m_config.append(line + "\n");
    }

    m_qrCodes = qrCodeUtils::generateQrCodeImageSeries(m_config.toUtf8());
    emit exportConfigChanged();
}

void ExportController::generateWireGuardConfig(const QString &clientName)
{
    QString nativeConfig;
    ErrorCode errorCode = generateNativeConfig(DockerContainer::WireGuard, clientName, Proto::WireGuard, nativeConfig);
    if (errorCode) {
        emit exportErrorOccurred(errorCode);
        return;
    }

    QStringList lines = nativeConfig.replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        m_config.append(line + "\n");
    }

    auto qr = qrCodeUtils::generateQrCode(m_config.toUtf8());
    m_qrCodes << qrCodeUtils::svgToBase64(QString::fromStdString(toSvgString(qr, 1)));

    emit exportConfigChanged();
}

void ExportController::generateAwgConfig(const QString &clientName)
{
    QString nativeConfig;
    ErrorCode errorCode = generateNativeConfig(DockerContainer::Awg, clientName, Proto::Awg, nativeConfig);
    if (errorCode) {
        emit exportErrorOccurred(errorCode);
        return;
    }

    QStringList lines = nativeConfig.replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        m_config.append(line + "\n");
    }

    auto qr = qrCodeUtils::generateQrCode(m_config.toUtf8());
    m_qrCodes << qrCodeUtils::svgToBase64(QString::fromStdString(toSvgString(qr, 1)));

    emit exportConfigChanged();
}

void ExportController::generateShadowSocksConfig()
{
    QString nativeConfig;
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

    QStringList lines = nativeConfig.replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        m_config.append(line + "\n");
    }

    QJsonObject nativeConfigJson = QJsonDocument::fromJson(nativeConfig.toUtf8()).object();

    m_nativeConfigString = QString("%1:%2@%3:%4")
                                   .arg(nativeConfigJson.value("method").toString(), nativeConfigJson.value("password").toString(),
                                        nativeConfigJson.value("server").toString(), nativeConfigJson.value("server_port").toString());

    m_nativeConfigString = "ss://" + m_nativeConfigString.toUtf8().toBase64();

    auto qr = qrCodeUtils::generateQrCode(m_nativeConfigString.toUtf8());
    m_qrCodes << qrCodeUtils::svgToBase64(QString::fromStdString(toSvgString(qr, 1)));

    emit exportConfigChanged();
}

void ExportController::generateCloakConfig()
{
    QString nativeConfig;
    ErrorCode errorCode = generateNativeConfig(DockerContainer::Cloak, "", Proto::Cloak, nativeConfig);
    if (errorCode) {
        emit exportErrorOccurred(errorCode);
        return;
    }

    QJsonObject nativeConfigJson = QJsonDocument::fromJson(nativeConfig.toUtf8()).object();
    nativeConfigJson.remove(config_key::transport_proto);
    nativeConfigJson.insert("ProxyMethod", "shadowsocks");

    QStringList lines = QString(QJsonDocument(nativeConfigJson).toJson()).replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        m_config.append(line + "\n");
    }

    emit exportConfigChanged();
}

void ExportController::generateXrayConfig(const QString &clientName)
{
    QString nativeConfig;
    ErrorCode errorCode = generateNativeConfig(DockerContainer::Xray, clientName, Proto::Xray, nativeConfig);
    if (errorCode) {
        emit exportErrorOccurred(errorCode);
        return;
    }

    QStringList lines = nativeConfig.replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
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

void ExportController::updateClientManagementModel(const DockerContainer container, amnezia::ServerCredentials credentials)
{
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    ErrorCode errorCode = m_clientManagementModel->updateModel(container, credentials, serverController);
    if (errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(errorCode);
    }
}

void ExportController::revokeConfig(const int row, const DockerContainer container, amnezia::ServerCredentials credentials)
{
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    ErrorCode errorCode =
        m_clientManagementModel->revokeClient(row, container, credentials, m_serversModel->getProcessedServerIndex(), serverController);
    if (errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(errorCode);
    }
    emit revokeConfigCompleted();
}

void ExportController::renameClient(const int row, const QString &clientName, const DockerContainer container,
                                    amnezia::ServerCredentials credentials)
{
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    ErrorCode errorCode = m_clientManagementModel->renameClient(row, clientName, container, credentials, serverController);
    if (errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(errorCode);
    }
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
