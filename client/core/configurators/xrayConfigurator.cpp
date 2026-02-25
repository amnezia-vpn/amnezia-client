#include "xrayConfigurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include "logger.h"

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/selfhosted/scriptsRegistry.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"

namespace {
Logger logger("XrayConfigurator");
}

XrayConfigurator::XrayConfigurator(SshSession* sshSession, QObject *parent)
    : ConfiguratorBase(sshSession, parent)
{
}

QString XrayConfigurator::prepareServerConfig(const ServerCredentials &credentials, DockerContainer container,
                                               const ContainerConfig &containerConfig,
                                               const DnsSettings &dnsSettings,
                                               ErrorCode &errorCode)
{
    // Generate new UUID for client
    QString clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Get current server config
    QString currentConfig = m_sshSession->getTextFileFromContainer(
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
    if (!serverConfig.contains(amnezia::protocols::xray::inbounds)) {
        logger.error() << "Server config missing 'inbounds' field";
        errorCode = ErrorCode::InternalError;
        return "";
    }

    QJsonArray inbounds = serverConfig[amnezia::protocols::xray::inbounds].toArray();
    if (inbounds.isEmpty()) {
        logger.error() << "Server config has empty 'inbounds' array";
        errorCode = ErrorCode::InternalError;
        return "";
    }
    
    QJsonObject inbound = inbounds[0].toObject();
    if (!inbound.contains(amnezia::protocols::xray::settings)) {
        logger.error() << "Inbound missing 'settings' field";
        errorCode = ErrorCode::InternalError;
        return "";
    }

    QJsonObject settings = inbound[amnezia::protocols::xray::settings].toObject();
    if (!settings.contains(amnezia::protocols::xray::clients)) {
        logger.error() << "Settings missing 'clients' field";
        errorCode = ErrorCode::InternalError;
        return "";
    }

    QJsonArray clients = settings[amnezia::protocols::xray::clients].toArray();
    
    // Create configuration for new client
    QJsonObject clientConfig {
        {amnezia::protocols::xray::id, clientId},
        {amnezia::protocols::xray::flow, "xtls-rprx-vision"}
    };
    
    clients.append(clientConfig);
    
    // Update config
    settings[amnezia::protocols::xray::clients] = clients;
    inbound[amnezia::protocols::xray::settings] = settings;
    inbounds[0] = inbound;
    serverConfig[amnezia::protocols::xray::inbounds] = inbounds;
    
    // Save updated config to server
    QString updatedConfig = QJsonDocument(serverConfig).toJson();
    errorCode = m_sshSession->uploadTextFileToContainer(
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
    errorCode = m_sshSession->runScript(
        credentials, 
        m_sshSession->replaceVars(restartScript, amnezia::genBaseVars(credentials, container, dnsSettings.primaryDns, dnsSettings.secondaryDns))
    );

    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Failed to restart container";
        return "";
    }

    return clientId;
}

ProtocolConfig XrayConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                               const ContainerConfig &containerConfig,
                                               const DnsSettings &dnsSettings,
                                               ErrorCode &errorCode)
{
    const XrayServerConfig* serverConfig = nullptr;
    if (auto* xrayConfig = containerConfig.protocolConfig.as<XrayProtocolConfig>()) {
        serverConfig = &xrayConfig->serverConfig;
    }
    
    QString xrayClientId = prepareServerConfig(credentials, container, containerConfig, dnsSettings, errorCode);
    if (errorCode != ErrorCode::NoError || xrayClientId.isEmpty()) {
        logger.error() << "Failed to prepare server config";
        errorCode = ErrorCode::InternalError;
        return XrayProtocolConfig{};
    }

    amnezia::ScriptVars vars = amnezia::genBaseVars(credentials, container, dnsSettings.primaryDns, dnsSettings.secondaryDns);
    vars.append(amnezia::genProtocolVarsForContainer(container, containerConfig));
    QString config = m_sshSession->replaceVars(amnezia::scriptData(ProtocolScriptType::xray_template, container), vars);
    
    if (config.isEmpty()) {
        logger.error() << "Failed to get config template";
        errorCode = ErrorCode::InternalError;
        return XrayProtocolConfig{};
    }

    QString xrayPublicKey =
            m_sshSession->getTextFileFromContainer(container, credentials, amnezia::protocols::xray::PublicKeyPath, errorCode);
    if (errorCode != ErrorCode::NoError || xrayPublicKey.isEmpty()) {
        logger.error() << "Failed to get public key";
        errorCode = ErrorCode::InternalError;
        return XrayProtocolConfig{};
    }
    xrayPublicKey.replace("\n", "");
    
    QString xrayShortId =
            m_sshSession->getTextFileFromContainer(container, credentials, amnezia::protocols::xray::shortidPath, errorCode);
    if (errorCode != ErrorCode::NoError || xrayShortId.isEmpty()) {
        logger.error() << "Failed to get short ID";
        errorCode = ErrorCode::InternalError;
        return XrayProtocolConfig{};
    }
    xrayShortId.replace("\n", "");

    if (!config.contains("$XRAY_CLIENT_ID") || !config.contains("$XRAY_PUBLIC_KEY") || !config.contains("$XRAY_SHORT_ID")) {
        logger.error() << "Config template missing required variables:"
                      << "XRAY_CLIENT_ID:" << !config.contains("$XRAY_CLIENT_ID")
                      << "XRAY_PUBLIC_KEY:" << !config.contains("$XRAY_PUBLIC_KEY")
                      << "XRAY_SHORT_ID:" << !config.contains("$XRAY_SHORT_ID");
        errorCode = ErrorCode::InternalError;
        return XrayProtocolConfig{};
    }

    config.replace("$XRAY_CLIENT_ID", xrayClientId);
    config.replace("$XRAY_PUBLIC_KEY", xrayPublicKey);
    config.replace("$XRAY_SHORT_ID", xrayShortId);

    XrayProtocolConfig protocolConfig;
    if (serverConfig) {
        protocolConfig.serverConfig = *serverConfig;
    }
    
    XrayClientConfig clientConfig;
    clientConfig.nativeConfig = config;
    clientConfig.localPort = "";
    clientConfig.id = xrayClientId;
    
    protocolConfig.setClientConfig(clientConfig);
    
    return protocolConfig;
}
