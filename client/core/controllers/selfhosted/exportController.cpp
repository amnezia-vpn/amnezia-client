#include "exportController.h"

#include <QBuffer>
#include <QDataStream>
#include <QJsonDocument>

#include "core/controllers/vpnConfigurationController.h"
#include "core/controllers/selfhosted/serverController.h"
#include "core/controllers/selfhosted/clientManagementController.h"
#include "core/qrCodeUtils.h"
#include <QEventLoop>
#include <QTimer>

ExportController::ExportController(std::shared_ptr<Settings> settings,
                                   QObject *parent)
    : QObject(parent),
      m_settings(settings),
      m_lastClientAppendResult(ErrorCode::NoError),
      m_lastNativeConfigAppendResult(ErrorCode::NoError),
      m_waitingForClientAppend(false),
      m_waitingForNativeConfigAppend(false)
{
}

ExportConfigResult ExportController::generateFullAccessConfig(const QSharedPointer<ServerConfig> &serverConfig)
{
    ExportConfigResult result;
    result.errorCode = ErrorCode::NoError;

    // Create a copy of the ServerConfig and clean last_config from protocol configs
    auto modifiedServerConfig = QSharedPointer<ServerConfig>::create(*serverConfig);
    
    for (auto &containerConfig : modifiedServerConfig->containerConfigs) {
        for (auto &protocolConfig : containerConfig.protocolConfigs) {
            // Protocol configs will automatically exclude last_config when serialized to JSON for export
            // No need to manually remove it here as the toJson() method handles this
        }
    }

    QByteArray compressedConfig = QJsonDocument(modifiedServerConfig->toJson()).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    result.config = QString("vpn://%1").arg(QString(compressedConfig.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
    result.qrCodes = generateQrCodeSeries(compressedConfig);

    return result;
}

ExportConfigResult ExportController::generateConnectionConfig(const QString &clientName,
                                                             const ServerCredentials &credentials,
                                                             const DockerContainer container,
                                                             const ContainerConfig &containerConfig,
                                                             const QSharedPointer<ServerConfig> &serverConfig,
                                                             const QPair<QString, QString> &dnsSettings)
{
    ExportConfigResult result;
    
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);
    
    // Use the provided ContainerConfig directly
    ContainerConfig modifiedContainerConfig = containerConfig;
    
    result.errorCode = vpnConfigurationController.createProtocolConfigForContainer(credentials, container, modifiedContainerConfig);
    if (result.errorCode != ErrorCode::NoError) {
        return result;
    }

    m_waitingForClientAppend = true;
    emit clientAppendRequested(container, credentials, modifiedContainerConfig, clientName, serverController);
    
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(30000);
    
    connect(this, &ExportController::onClientAppendCompleted, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    timer.start();
    loop.exec();
    
    m_waitingForClientAppend = false;
    result.errorCode = m_lastClientAppendResult;
    if (result.errorCode != ErrorCode::NoError) {
        return result;
    }

    // Create a modified ServerConfig for export with only the specific container
    auto exportServerConfig = QSharedPointer<ServerConfig>::create(*serverConfig);
    
    // Remove credentials (they are not needed in export)
    exportServerConfig->containerConfigs.clear();
    
    // Add only the specific container being exported
    QString containerName = ContainerProps::containerToString(container);
    exportServerConfig->containerConfigs.insert(containerName, modifiedContainerConfig);
    exportServerConfig->defaultContainer = containerName;
    exportServerConfig->dns1 = dnsSettings.first;
    exportServerConfig->dns2 = dnsSettings.second;

    QByteArray compressedConfig = QJsonDocument(exportServerConfig->toJson()).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    result.config = QString("vpn://%1").arg(QString(compressedConfig.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
    result.qrCodes = generateQrCodeSeries(compressedConfig);

    return result;
}

ExportConfigResult ExportController::generateOpenVpnConfig(const QString &clientName,
                                                        const ServerCredentials &credentials,
                                                        const DockerContainer container,
                                                        const ContainerConfig &containerConfig,
                                                        const QPair<QString, QString> &dnsSettings,
                                                        bool isApiConfig)
{
    ExportConfigResult result;
    QJsonObject nativeConfig;
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));

    Proto protocol = Proto::OpenVpn;
    if (container == DockerContainer::Cloak || container == DockerContainer::ShadowSocks) {
        protocol = Proto::OpenVpn;
    } else {
        protocol = ContainerProps::defaultProtocol(container);
    }

    result.errorCode = generateNativeConfig(container, clientName, protocol, credentials, 
                                           containerConfig, dnsSettings, isApiConfig, nativeConfig, serverController);
    if (result.errorCode != ErrorCode::NoError) {
        return result;
    }

    QStringList lines = nativeConfig.value(config_key::config).toString().replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    result.qrCodes = generateQrCodeSeries(result.config.toUtf8());
    return result;
}

ExportConfigResult ExportController::generateWireGuardConfig(const QString &clientName,
                                                            const ServerCredentials &credentials,
                                                            const ContainerConfig &containerConfig,
                                                            const QPair<QString, QString> &dnsSettings,
                                                            bool isApiConfig)
{
    ExportConfigResult result;
    QJsonObject nativeConfig;
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));

    result.errorCode = generateNativeConfig(DockerContainer::WireGuard, clientName, Proto::WireGuard, 
                                           credentials, containerConfig, dnsSettings, isApiConfig, nativeConfig, serverController);
    if (result.errorCode != ErrorCode::NoError) {
        return result;
    }

    QStringList lines = nativeConfig.value(config_key::config).toString().replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    result.qrCodes << generateQrCode(result.config.toUtf8());
    return result;
}

ExportConfigResult ExportController::generateAwgConfig(const QString &clientName,
                                                    const ServerCredentials &credentials,
                                                    const ContainerConfig &containerConfig,
                                                    const QPair<QString, QString> &dnsSettings,
                                                    bool isApiConfig)
{
    ExportConfigResult result;
    QJsonObject nativeConfig;
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));

    result.errorCode = generateNativeConfig(DockerContainer::Awg, clientName, Proto::Awg, 
                                           credentials, containerConfig, dnsSettings, isApiConfig, nativeConfig, serverController);
    if (result.errorCode != ErrorCode::NoError) {
        return result;
    }

    QStringList lines = nativeConfig.value(config_key::config).toString().replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    result.qrCodes << generateQrCode(result.config.toUtf8());
    return result;
}

ExportConfigResult ExportController::generateShadowSocksConfig(const ServerCredentials &credentials,
                                                            const DockerContainer container,
                                                            const ContainerConfig &containerConfig,
                                                            const QPair<QString, QString> &dnsSettings,
                                                            bool isApiConfig)
{
    ExportConfigResult result;
    QJsonObject nativeConfig;
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));

    Proto protocol = Proto::ShadowSocks;
    if (container == DockerContainer::Cloak) {
        protocol = Proto::ShadowSocks;
    } else {
        protocol = ContainerProps::defaultProtocol(container);
    }

    result.errorCode = generateNativeConfig(container, "", protocol, credentials, 
                                           containerConfig, dnsSettings, isApiConfig, nativeConfig, serverController);
    if (result.errorCode != ErrorCode::NoError) {
        return result;
    }

    QStringList lines = QString(QJsonDocument(nativeConfig).toJson()).replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    result.nativeConfigString = QString("%1:%2@%3:%4")
                                       .arg(nativeConfig.value("method").toString(), 
                                            nativeConfig.value("password").toString(),
                                            nativeConfig.value("server").toString(), 
                                            nativeConfig.value("server_port").toString());

    result.nativeConfigString = "ss://" + result.nativeConfigString.toUtf8().toBase64();
    result.qrCodes << generateQrCode(result.nativeConfigString.toUtf8());

    return result;
}

ExportConfigResult ExportController::generateCloakConfig(const ServerCredentials &credentials,
                                                      const ContainerConfig &containerConfig,
                                                      const QPair<QString, QString> &dnsSettings,
                                                      bool isApiConfig)
{
    ExportConfigResult result;
    QJsonObject nativeConfig;
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));

    result.errorCode = generateNativeConfig(DockerContainer::Cloak, "", Proto::Cloak, 
                                           credentials, containerConfig, dnsSettings, isApiConfig, nativeConfig, serverController);
    if (result.errorCode != ErrorCode::NoError) {
        return result;
    }

    nativeConfig.remove(config_key::transport_proto);
    nativeConfig.insert("ProxyMethod", "shadowsocks");

    QStringList lines = QString(QJsonDocument(nativeConfig).toJson()).replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    return result;
}

ExportConfigResult ExportController::generateXrayConfig(const QString &clientName,
                                                     const ServerCredentials &credentials,
                                                     const ContainerConfig &containerConfig,
                                                     const QPair<QString, QString> &dnsSettings,
                                                     bool isApiConfig)
{
    ExportConfigResult result;
    QJsonObject nativeConfig;
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));

    result.errorCode = generateNativeConfig(DockerContainer::Xray, clientName, Proto::Xray, 
                                           credentials, containerConfig, dnsSettings, isApiConfig, nativeConfig, serverController);
    if (result.errorCode != ErrorCode::NoError) {
        return result;
    }

    QStringList lines = QString(QJsonDocument(nativeConfig).toJson()).replace("\r", "").split("\n");
    for (const QString &line : std::as_const(lines)) {
        result.config.append(line + "\n");
    }

    return result;
}



ErrorCode ExportController::generateNativeConfig(const DockerContainer container, const QString &clientName, 
                                                 const Proto &protocol, const ServerCredentials &credentials,
                                                 const ContainerConfig &containerConfig, const QPair<QString, QString> &dnsSettings,
                                                 bool isApiConfig, QJsonObject &jsonNativeConfig,
                                                 const QSharedPointer<ServerController> &serverController)
{
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

    // Use the provided ContainerConfig directly
    ContainerConfig modifiedContainerConfig = containerConfig;

    QString protocolConfigString;
    ErrorCode errorCode = vpnConfigurationController.createProtocolConfigString(isApiConfig, dnsSettings, credentials, 
                                                                                container, modifiedContainerConfig,
                                                                                protocol, protocolConfigString);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    jsonNativeConfig = QJsonDocument::fromJson(protocolConfigString.toUtf8()).object();

    if (protocol == Proto::OpenVpn || protocol == Proto::WireGuard || protocol == Proto::Awg || protocol == Proto::Xray) {
        QString protocolName = ProtocolProps::protoToString(protocol);
        auto protocolConfig = modifiedContainerConfig.protocolConfigs.value(protocolName);
        
        m_waitingForNativeConfigAppend = true;
        emit nativeConfigClientAppendRequested(protocolConfig, clientName, container, credentials, serverController);
        
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(30000);
        
        connect(this, &ExportController::onNativeConfigClientAppendCompleted, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        
        timer.start();
        loop.exec();
        
        m_waitingForNativeConfigAppend = false;
        errorCode = m_lastNativeConfigAppendResult;
    }
    
    return errorCode;
}

void ExportController::onClientAppendCompleted(ErrorCode errorCode)
{
    if (m_waitingForClientAppend) {
        m_lastClientAppendResult = errorCode;
    }
}

void ExportController::onNativeConfigClientAppendCompleted(ErrorCode errorCode)
{
    if (m_waitingForNativeConfigAppend) {
        m_lastNativeConfigAppendResult = errorCode;
    }
}

QList<QString> ExportController::generateQrCodeSeries(const QByteArray &data)
{
    return qrCodeUtils::generateQrCodeImageSeries(data);
}

QString ExportController::generateQrCode(const QByteArray &data)
{
    auto qr = qrCodeUtils::generateQrCode(data);
    return qrCodeUtils::svgToBase64(QString::fromStdString(toSvgString(qr, 1)));
} 
