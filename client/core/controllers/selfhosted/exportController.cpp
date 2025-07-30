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

ExportConfigResult ExportController::generateFullAccessConfig(const QJsonObject &serverConfig)
{
    ExportConfigResult result;
    result.errorCode = ErrorCode::NoError;

    QJsonObject modifiedServerConfig = serverConfig;
    QJsonArray containers = modifiedServerConfig.value(config_key::containers).toArray();
    
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
    modifiedServerConfig[config_key::containers] = containers;

    QByteArray compressedConfig = QJsonDocument(modifiedServerConfig).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    result.config = QString("vpn://%1").arg(QString(compressedConfig.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
    result.qrCodes = generateQrCodeSeries(compressedConfig);

    return result;
}

ExportConfigResult ExportController::generateConnectionConfig(const QString &clientName,
                                                             const ServerCredentials &credentials,
                                                             const DockerContainer container,
                                                             const QJsonObject &containerConfig,
                                                             const QJsonObject &serverConfig,
                                                             const QPair<QString, QString> &dnsSettings)
{
    ExportConfigResult result;
    
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);
    
    QJsonObject modifiedContainerConfig = containerConfig;
    modifiedContainerConfig.insert(config_key::container, ContainerProps::containerToString(container));
    
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

    QJsonObject modifiedServerConfig = serverConfig;
    modifiedServerConfig.remove(config_key::userName);
    modifiedServerConfig.remove(config_key::password);
    modifiedServerConfig.remove(config_key::port);
    modifiedServerConfig.insert(config_key::containers, QJsonArray { modifiedContainerConfig });
    modifiedServerConfig.insert(config_key::defaultContainer, ContainerProps::containerToString(container));
    modifiedServerConfig.insert(config_key::dns1, dnsSettings.first);
    modifiedServerConfig.insert(config_key::dns2, dnsSettings.second);

    QByteArray compressedConfig = QJsonDocument(modifiedServerConfig).toJson();
    compressedConfig = qCompress(compressedConfig, 8);
    result.config = QString("vpn://%1").arg(QString(compressedConfig.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
    result.qrCodes = generateQrCodeSeries(compressedConfig);

    return result;
}

ExportConfigResult ExportController::generateOpenVpnConfig(const QString &clientName,
                                                          const ServerCredentials &credentials,
                                                          const DockerContainer container,
                                                          const QJsonObject &containerConfig,
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
                                                            const QJsonObject &containerConfig,
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
                                                      const QJsonObject &containerConfig,
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
                                                              const QJsonObject &containerConfig,
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
                                                        const QJsonObject &containerConfig,
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
                                                       const QJsonObject &containerConfig,
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
                                                 const QJsonObject &containerConfig, const QPair<QString, QString> &dnsSettings,
                                                 bool isApiConfig, QJsonObject &jsonNativeConfig,
                                                 const QSharedPointer<ServerController> &serverController)
{
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

    QJsonObject modifiedContainerConfig = containerConfig;
    modifiedContainerConfig.insert(config_key::container, ContainerProps::containerToString(container));

    QString protocolConfigString;
    ErrorCode errorCode = vpnConfigurationController.createProtocolConfigString(isApiConfig, dnsSettings, credentials, 
                                                                                container, modifiedContainerConfig,
                                                                                protocol, protocolConfigString);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    jsonNativeConfig = QJsonDocument::fromJson(protocolConfigString.toUtf8()).object();

    if (protocol == Proto::OpenVpn || protocol == Proto::WireGuard || protocol == Proto::Awg || protocol == Proto::Xray) {
        m_waitingForNativeConfigAppend = true;
        emit nativeConfigClientAppendRequested(jsonNativeConfig, clientName, container, credentials, serverController);
        
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
