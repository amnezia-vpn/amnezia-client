#include "installController.h"

#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QtConcurrent>

#include "core/api/apiUtils.h"
#include "core/controllers/selfhosted/serverController.h"

#include "core/controllers/vpnConfigurationController.h"
#include "core/models/servers/selfHostedServerConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/networkUtilities.h"
#include "logger.h"
#include "utilities.h"

namespace
{
    Logger logger("CoreInstallController");
}

InstallController::InstallController(std::shared_ptr<Settings> settings,
                                     QObject *parent)
    : QObject(parent),
      m_settings(settings)
{
}

InstallResult InstallController::installContainer(DockerContainer container, int port, TransportProto transportProto,
                                                  const ServerCredentials &serverCredentials, bool shouldCreateServer,
                                                  const QString &privateKeyPassphrase)
{
    InstallResult result;
    result.errorCode = ErrorCode::NoError;
    result.isServiceInstall = (ContainerProps::containerService(container) == ServiceType::Other);
    result.isInstalledContainerFound = false;

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));

    ContainerConfig containerConfig = generateContainerConfig(container, port, transportProto);

    if (shouldCreateServer && isServerAlreadyExists(serverCredentials)) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }

    QMap<DockerContainer, ContainerConfig> installedContainers;
    ErrorCode errorCode = getAlreadyInstalledContainers(serverCredentials, serverController, installedContainers);
    if (errorCode != ErrorCode::NoError) {
        result.errorCode = errorCode;
        return result;
    }

    QSharedPointer<ServerConfig> serverConfig;
    if (shouldCreateServer) {
        errorCode = installServer(container, installedContainers, serverCredentials, 
                                 serverController, result.message, serverConfig);
    } else {
        if (installedContainers.contains(container)) {
            result.errorCode = ErrorCode::InternalError;
            return result;
        }

        errorCode = installContainer(container, installedContainers, serverCredentials,
                                    serverController, result.message);
    }

    if (errorCode != ErrorCode::NoError) {
        result.errorCode = errorCode;
        return result;
    }

    result.errorCode = ErrorCode::NoError;
    return result;
}

InstallResult InstallController::scanServerForInstalledContainers(const ServerCredentials &serverCredentials)
{
    InstallResult result;
    result.errorCode = ErrorCode::NoError;
    result.isInstalledContainerFound = false;
    result.isServiceInstall = false;

    QMap<DockerContainer, ContainerConfig> installedContainers;
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    result.errorCode = getAlreadyInstalledContainers(serverCredentials, serverController, installedContainers);

    if (result.errorCode == ErrorCode::NoError) {
        VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

        for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
            auto container = iterator.key();
            ContainerConfig containerConfig = iterator.value();

            if (ContainerProps::isSupportedByCurrentPlatform(container)) {
                result.errorCode = vpnConfigurationController.createProtocolConfigForContainer(serverCredentials, container, containerConfig);
                if (result.errorCode != ErrorCode::NoError) {
                    return result;
                }

                emit clientAppendRequested(container, serverCredentials, containerConfig,
                                          QString("Admin [%1]").arg(QSysInfo::prettyProductName()),
                                          serverController);
            }

            result.isInstalledContainerFound = true;
        }
    }

    return result;
}

ErrorCode InstallController::getAlreadyInstalledContainers(const ServerCredentials &credentials,
                                                          const QSharedPointer<ServerController> &serverController,
                                                          QMap<DockerContainer, ContainerConfig> &installedContainers)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };
    auto cbReadStdErr = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    QString script = QString("sudo docker ps --format '{{.Names}} {{.Ports}}'");

    ErrorCode errorCode = serverController->runScript(credentials, script, cbReadStdOut, cbReadStdErr);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    auto containersInfo = stdOut.split("\n");
    for (auto &containerInfo : containersInfo) {
        if (containerInfo.isEmpty()) {
            continue;
        }
        const static QRegularExpression containerAndPortRegExp("(amnezia[-a-z0-9]*).*?:([0-9]*)->[0-9]*/(udp|tcp).*");
        QRegularExpressionMatch containerAndPortMatch = containerAndPortRegExp.match(containerInfo);
        if (containerAndPortMatch.hasMatch()) {
            QString containerName = containerAndPortMatch.captured(1);
            QString port = containerAndPortMatch.captured(2);
            QString transportProto = containerAndPortMatch.captured(3);

            DockerContainer container = ContainerProps::containerFromString(containerName);
            if (container != DockerContainer::None) {
                ContainerConfig containerConfig;
                containerConfig.containerName = ContainerProps::containerToString(container);

                auto containerProto = ContainerProps::defaultProtocol(container);
                auto containerProtoString = ProtocolProps::protoToString(containerProto);

                // Create appropriate protocol config based on container type
                QSharedPointer<ProtocolConfig> protocolConfig;
                
                // Create a temporary QJsonObject to construct the protocol config
                QJsonObject protoConfigObject;
                protoConfigObject.insert(config_key::port, port);
                protoConfigObject.insert(config_key::transport_proto, transportProto);
                
                // Create the appropriate protocol config based on type
                switch (containerProto) {
                case Proto::OpenVpn:
                    protocolConfig = QSharedPointer<OpenVpnProtocolConfig>::create(protoConfigObject, containerProtoString);
                    break;
                case Proto::WireGuard:
                    protocolConfig = QSharedPointer<WireGuardProtocolConfig>::create(protoConfigObject, containerProtoString);
                    break;
                case Proto::Awg:
                    protocolConfig = QSharedPointer<AwgProtocolConfig>::create(protoConfigObject, containerProtoString);
                    break;
                case Proto::Cloak:
                    protocolConfig = QSharedPointer<CloakProtocolConfig>::create(protoConfigObject, containerProtoString);
                    break;
                case Proto::ShadowSocks:
                    protocolConfig = QSharedPointer<ShadowsocksProtocolConfig>::create(protoConfigObject, containerProtoString);
                    break;
                case Proto::Xray:
                case Proto::SSXray:
                    protocolConfig = QSharedPointer<XrayProtocolConfig>::create(protoConfigObject, containerProtoString);
                    break;
                case Proto::Ikev2:
                    protocolConfig = QSharedPointer<Ikev2ProtocolConfig>::create(protoConfigObject, containerProtoString);
                    break;
                case Proto::Sftp:
                    protocolConfig = QSharedPointer<SftpProtocolConfig>::create(protoConfigObject, containerProtoString);
                    break;
                case Proto::Socks5Proxy:
                    protocolConfig = QSharedPointer<Socks5ProtocolConfig>::create(protoConfigObject, containerProtoString);
                    break;
                default:
                    continue; // Skip unknown protocols
                }
                
                if (protocolConfig) {
                    containerConfig.protocolConfigs.insert(containerProtoString, protocolConfig);
                    installedContainers.insert(container, containerConfig);
                }
            }
        }
    }

    return ErrorCode::NoError;
}

ErrorCode InstallController::installServer(const DockerContainer container,
                                          const QMap<DockerContainer, ContainerConfig> &installedContainers,
                                          const ServerCredentials &serverCredentials,
                                          const QSharedPointer<ServerController> &serverController,
                                          QString &finishMessage, QSharedPointer<ServerConfig> &serverConfig)
{
    if (installedContainers.size() > 1) {
        finishMessage += tr("\nAdded containers that were already installed on the server");
    }

    // Create a SelfHostedServerConfig and populate it properly
    serverConfig = QSharedPointer<SelfHostedServerConfig>::create(QJsonObject());
    auto selfHostedConfig = qSharedPointerCast<SelfHostedServerConfig>(serverConfig);
    
    selfHostedConfig->hostName = serverCredentials.hostName;
    selfHostedConfig->serverCredentials = serverCredentials;
    selfHostedConfig->name = m_settings->nextAvailableServerName();
    selfHostedConfig->defaultContainer = ContainerProps::containerToString(container);
    selfHostedConfig->type = amnezia::ServerConfigType::SelfHosted;

    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);
    
    for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
        auto containerConfig = iterator.value();
        QString containerName = ContainerProps::containerToString(iterator.key());

        if (ContainerProps::isSupportedByCurrentPlatform(iterator.key())) {
            auto errorCode = vpnConfigurationController.createProtocolConfigForContainer(serverCredentials, iterator.key(),
                                                                                         containerConfig);
            if (errorCode != ErrorCode::NoError) {
                return errorCode;
            }
        }
        
        selfHostedConfig->containerConfigs.insert(containerName, containerConfig);
    }

    int serverIndex = m_settings->nextAvailableServerIndex();
    m_settings->setServerConfig(serverIndex, serverConfig->toJson());
    m_settings->setDefaultServerIndex(serverIndex);

    finishMessage = tr("Server '%1' was successfully added").arg(serverCredentials.hostName);
    return ErrorCode::NoError;
}

ErrorCode InstallController::installContainer(const DockerContainer container,
                                           const QMap<DockerContainer, ContainerConfig> &installedContainers,
                                           const ServerCredentials &serverCredentials,
                                           const QSharedPointer<ServerController> &serverController,
                                           QString &finishMessage)
{
    bool isInstalledContainerAddedToGui = false;

    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);
    QList<QJsonObject> allContainerConfigs;
    
    for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
        QJsonObject containerConfig = iterator.value();

        if (ContainerProps::isSupportedByCurrentPlatform(container)) {
            auto errorCode = vpnConfigurationController.createProtocolConfigForContainer(serverCredentials, iterator.key(), containerConfig);
            if (errorCode != ErrorCode::NoError) {
                return errorCode;
            }
            allContainerConfigs.append(containerConfig);
        } else {
            allContainerConfigs.append(containerConfig);
        }

        if (container != iterator.key()) {
            isInstalledContainerAddedToGui = true;
        }
    }
    
    if (isInstalledContainerAddedToGui) {
        finishMessage += tr("\nAlready installed containers were found on the server. "
                           "All installed containers have been added to the application");
    }

    finishMessage = tr("Container '%1' was successfully added").arg(ContainerProps::containerHumanNames().value(container));
    return ErrorCode::NoError;
}

bool InstallController::isServerAlreadyExists(const ServerCredentials &serverCredentials) const
{
    for (int i = 0; i < m_settings->serversCount(); i++) {
        QJsonObject serverConfig = m_settings->serverConfig(i);
        ServerCredentials existingCredentials = ServerCredentials::fromServerConfig(serverConfig);
        if (serverCredentials.hostName == existingCredentials.hostName && 
            serverCredentials.port == existingCredentials.port) {
            return true;
        }
    }
    return false;
}

ContainerConfig InstallController::generateContainerConfig(const DockerContainer container, int port,
                                                           const TransportProto transportProto)
{
    ContainerConfig containerConfig;
    containerConfig.containerName = ContainerProps::containerToString(container);
    
    auto mainProto = ContainerProps::defaultProtocol(container);
    for (auto protocol : ContainerProps::protocolsForContainer(container)) {
        QSharedPointer<ProtocolConfig> protocolConfig;
        
        if (protocol == mainProto) {
            // Create the appropriate protocol config based on the protocol type
            if (protocol == Proto::Awg && container == DockerContainer::Awg) {
                // Use the VpnConfigurationsController to create proper AwgProtocolConfig
                // For now, create a basic protocol config and set values
                protocolConfig = QSharedPointer<AwgProtocolConfig>::create(QJsonObject(), ProtocolProps::protoToString(protocol));
                auto awgConfig = qSharedPointerCast<AwgProtocolConfig>(protocolConfig);
                
                // Set server protocol config
                awgConfig->serverProtocolConfig.port = QString::number(port);
                awgConfig->serverProtocolConfig.transportProto = ProtocolProps::transportProtoToString(transportProto, protocol);
                
                // Generate AWG-specific parameters
                awgConfig->serverProtocolConfig.junkPacketCount = QString::number(QRandomGenerator::global()->bounded(2, 5));
                awgConfig->serverProtocolConfig.junkPacketMinSize = QString::number(10);
                awgConfig->serverProtocolConfig.junkPacketMaxSize = QString::number(50);
                
                int s1 = QRandomGenerator::global()->bounded(15, 150);
                int s2 = QRandomGenerator::global()->bounded(15, 150);
                QSet<int> usedValues;
                usedValues.insert(s1);
                while (usedValues.contains(s2) || s1 + AwgConstant::messageInitiationSize == s2 + AwgConstant::messageResponseSize) {
                    s2 = QRandomGenerator::global()->bounded(15, 150);
                }
                usedValues.insert(s2);
                
                awgConfig->serverProtocolConfig.initPacketJunkSize = QString::number(s1);
                awgConfig->serverProtocolConfig.responsePacketJunkSize = QString::number(s2);
                
                QSet<QString> headersValue;
                while (headersValue.size() != 4) {
                    auto max = (std::numeric_limits<qint32>::max)();
                    headersValue.insert(QString::number(QRandomGenerator::global()->bounded(5, max)));
                }
                auto headersValueList = headersValue.values();
                awgConfig->serverProtocolConfig.initPacketMagicHeader = headersValueList.at(0);
                awgConfig->serverProtocolConfig.responsePacketMagicHeader = headersValueList.at(1);
                awgConfig->serverProtocolConfig.underloadPacketMagicHeader = headersValueList.at(2);
                awgConfig->serverProtocolConfig.transportPacketMagicHeader = headersValueList.at(3);
                
            } else {
                // For other protocols, create basic protocol configs
                // This will need to be expanded for each protocol type
                protocolConfig = QSharedPointer<ProtocolConfig>::create(QJsonObject(), ProtocolProps::protoToString(protocol));
                
                // For now, store basic port and transport info as JSON until all protocol configs are properly structured
                QJsonObject tempConfig;
                tempConfig.insert(config_key::port, QString::number(port));
                tempConfig.insert(config_key::transport_proto, ProtocolProps::transportProtoToString(transportProto, protocol));
                
                if (container == DockerContainer::Sftp) {
                    tempConfig.insert(config_key::userName, protocols::sftp::defaultUserName);
                    tempConfig.insert(config_key::password, Utils::getRandomString(16));
                } else if (container == DockerContainer::Socks5Proxy) {
                    tempConfig.insert(config_key::userName, protocols::socks5Proxy::defaultUserName);
                    tempConfig.insert(config_key::password, Utils::getRandomString(16));
                }
                
                // Store this in the protocol config's internal JSON for now
                // TODO: Replace with proper protocol config structure
                protocolConfig = QSharedPointer<ProtocolConfig>::create(tempConfig, ProtocolProps::protoToString(protocol));
            }
        } else {
            // Non-main protocols get basic configs
            protocolConfig = QSharedPointer<ProtocolConfig>::create(QJsonObject(), ProtocolProps::protoToString(protocol));
        }
        
        containerConfig.protocolConfigs.insert(ProtocolProps::protoToString(protocol), protocolConfig);
    }
    
    return containerConfig;
} 
