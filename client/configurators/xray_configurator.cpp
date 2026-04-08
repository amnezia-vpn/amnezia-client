#include "xray_configurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include "logger.h"

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"
#include "core/scripts_registry.h"

namespace {
Logger logger("XrayConfigurator");
}

XrayConfigurator::XrayConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent)
    : ConfiguratorBase(settings, serverController, parent)
{
}

QString XrayConfigurator::prepareServerConfig(const ServerCredentials &credentials, DockerContainer container,
                                               const QJsonObject &containerConfig, ErrorCode &errorCode)
{
    // Generate new UUID for client
    QString clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Get current server config
    QString currentConfig = m_serverController->getTextFileFromContainer(
        container, credentials, amnezia::protocols::xray::serverConfigPath, errorCode);
    
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Failed to get server config file";
        return "";
    }

    // Parse current config as JSON
    QJsonDocument doc = QJsonDocument::fromJson(currentConfig.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        logger.error() << "Failed to parse server config JSON";
        errorCode = ErrorCode::InternalError;
        return "";
    }

    QJsonObject serverConfig = doc.object();
    
    // Validate server config structure
    if (!serverConfig.contains("inbounds")) {
        logger.error() << "Server config missing 'inbounds' field";
        errorCode = ErrorCode::InternalError;
        return "";
    }

    QJsonArray inbounds = serverConfig["inbounds"].toArray();
    if (inbounds.isEmpty()) {
        logger.error() << "Server config has empty 'inbounds' array";
        errorCode = ErrorCode::InternalError;
        return "";
    }
    
    QJsonObject inbound = inbounds[0].toObject();
    if (!inbound.contains("settings")) {
        logger.error() << "Inbound missing 'settings' field";
        errorCode = ErrorCode::InternalError;
        return "";
    }

    QJsonObject settings = inbound["settings"].toObject();
    if (!settings.contains("clients")) {
        logger.error() << "Settings missing 'clients' field";
        errorCode = ErrorCode::InternalError;
        return "";
    }

    QJsonArray clients = settings["clients"].toArray();

    // Detect transport type from actual server config (source of truth)
    QJsonObject streamSettings = inbound.value("streamSettings").toObject();
    QString transport = streamSettings.value("network").toString("tcp");

    // Create configuration for new client
    // XHTTP is incompatible with flow xtls-rprx-vision; flow must be empty
    QJsonObject clientConfig {
        {"id", clientId},
        {"flow", transport == "xhttp" ? QString("") : QString("xtls-rprx-vision")}
    };
    
    clients.append(clientConfig);
    
    // Update config
    settings["clients"] = clients;
    inbound["settings"] = settings;
    inbounds[0] = inbound;
    serverConfig["inbounds"] = inbounds;
    
    // Save updated config to server
    QString updatedConfig = QJsonDocument(serverConfig).toJson();
    errorCode = m_serverController->uploadTextFileToContainer(
        container, 
        credentials, 
        updatedConfig,
        amnezia::protocols::xray::serverConfigPath,
        libssh::ScpOverwriteMode::ScpOverwriteExisting
    );
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Failed to upload updated config";
        return "";
    }

    // Restart container
    QString restartScript = QString("sudo docker restart $CONTAINER_NAME");
    errorCode = m_serverController->runScript(
        credentials, 
        m_serverController->replaceVars(restartScript, m_serverController->genVarsForScript(credentials, container))
    );

    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Failed to restart container";
        return "";
    }

    return clientId;
}

QString XrayConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                       const QJsonObject &containerConfig, ErrorCode &errorCode)
{
    // Get client ID from prepareServerConfig
    QString xrayClientId = prepareServerConfig(credentials, container, containerConfig, errorCode);
    if (errorCode != ErrorCode::NoError || xrayClientId.isEmpty()) {
        logger.error() << "Failed to prepare server config";
        errorCode = ErrorCode::InternalError;
        return "";
    }

    // Read server config to auto-detect transport (server config is source of truth)
    QString serverConfigStr = m_serverController->getTextFileFromContainer(
        container, credentials, amnezia::protocols::xray::serverConfigPath, errorCode);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Failed to read server config for transport detection";
        return "";
    }

    QJsonDocument serverDoc = QJsonDocument::fromJson(serverConfigStr.toUtf8());
    QJsonObject serverConfig = serverDoc.object();
    QJsonObject inbound = serverConfig.value("inbounds").toArray().first().toObject();
    QJsonObject streamSettings = inbound.value("streamSettings").toObject();
    QString transport = streamSettings.value("network").toString("tcp");
    bool isXhttp = (transport == "xhttp");

    logger.info() << "Auto-detected server transport:" << transport;

    // Select template based on detected transport
    ProtocolScriptType templateType = isXhttp ? ProtocolScriptType::xray_template_xhttp : ProtocolScriptType::xray_template;

    QString config = m_serverController->replaceVars(amnezia::scriptData(templateType, container),
                                                     m_serverController->genVarsForScript(credentials, container, containerConfig));

    if (config.isEmpty()) {
        logger.error() << "Failed to get config template";
        errorCode = ErrorCode::InternalError;
        return "";
    }

    QString xrayPublicKey =
            m_serverController->getTextFileFromContainer(container, credentials, amnezia::protocols::xray::PublicKeyPath, errorCode);
    if (errorCode != ErrorCode::NoError || xrayPublicKey.isEmpty()) {
        logger.error() << "Failed to get public key";
        errorCode = ErrorCode::InternalError;
        return "";
    }
    xrayPublicKey.replace("\n", "");

    QString xrayShortId =
            m_serverController->getTextFileFromContainer(container, credentials, amnezia::protocols::xray::shortidPath, errorCode);
    if (errorCode != ErrorCode::NoError || xrayShortId.isEmpty()) {
        logger.error() << "Failed to get short ID";
        errorCode = ErrorCode::InternalError;
        return "";
    }
    xrayShortId.replace("\n", "");

    // For XHTTP, extract path from server config's xhttpSettings
    if (isXhttp) {
        QJsonObject xhttpSettings = streamSettings.value("xhttpSettings").toObject();
        QString xhttpPath = xhttpSettings.value("path").toString();
        if (xhttpPath.isEmpty()) {
            logger.error() << "Server config missing xhttpSettings.path";
            errorCode = ErrorCode::InternalError;
            return "";
        }
        config.replace("$XRAY_XHTTP_PATH", xhttpPath);
    }

    // Validate all required variables are present
    if (!config.contains("$XRAY_CLIENT_ID") || !config.contains("$XRAY_PUBLIC_KEY") || !config.contains("$XRAY_SHORT_ID")) {
        logger.error() << "Config template missing required variables:"
                      << "XRAY_CLIENT_ID:" << !config.contains("$XRAY_CLIENT_ID")
                      << "XRAY_PUBLIC_KEY:" << !config.contains("$XRAY_PUBLIC_KEY")
                      << "XRAY_SHORT_ID:" << !config.contains("$XRAY_SHORT_ID");
        errorCode = ErrorCode::InternalError;
        return "";
    }

    config.replace("$XRAY_CLIENT_ID", xrayClientId);
    config.replace("$XRAY_PUBLIC_KEY", xrayPublicKey);
    config.replace("$XRAY_SHORT_ID", xrayShortId);

    return config;
}
