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

    QJsonObject containerConfig = generateContainerConfig(container, port, transportProto);

    if (shouldCreateServer && isServerAlreadyExists(serverCredentials)) {
        result.errorCode = ErrorCode::InternalError;
        return result;
    }

    QMap<DockerContainer, QJsonObject> installedContainers;
    ErrorCode errorCode = getAlreadyInstalledContainers(serverCredentials, serverController, installedContainers);
    if (errorCode != ErrorCode::NoError) {
        result.errorCode = errorCode;
        return result;
    }

    QJsonObject serverConfig;
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

    QMap<DockerContainer, QJsonObject> installedContainers;
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    result.errorCode = getAlreadyInstalledContainers(serverCredentials, serverController, installedContainers);

    if (result.errorCode == ErrorCode::NoError) {
        VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

        for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
            auto container = iterator.key();
            QJsonObject containerConfig = iterator.value();

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
                                                          QMap<DockerContainer, QJsonObject> &installedContainers)
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
                QJsonObject containerConfigObject;
                containerConfigObject.insert(config_key::container, ContainerProps::containerToString(container));

                auto containerProto = ContainerProps::defaultProtocol(container);
                auto containerProtoString = ProtocolProps::protoToString(containerProto);

                QJsonObject protoConfigObject;
                protoConfigObject.insert(config_key::port, port);
                protoConfigObject.insert(config_key::transport_proto, transportProto);

                containerConfigObject.insert(containerProtoString, protoConfigObject);
                installedContainers.insert(container, containerConfigObject);
            }
        }
    }

    return ErrorCode::NoError;
}

ErrorCode InstallController::installServer(const DockerContainer container, 
                                       const QMap<DockerContainer, QJsonObject> &installedContainers,
                                       const ServerCredentials &serverCredentials,
                                       const QSharedPointer<ServerController> &serverController,
                                       QString &finishMessage, QJsonObject &serverConfig)
{
    if (installedContainers.size() > 1) {
        finishMessage += tr("\nAdded containers that were already installed on the server");
    }

    serverConfig.insert(config_key::hostName, serverCredentials.hostName);
    serverConfig.insert(config_key::userName, serverCredentials.userName);
    serverConfig.insert(config_key::password, serverCredentials.secretData);
    serverConfig.insert(config_key::port, serverCredentials.port);
    serverConfig.insert(config_key::description, m_settings->nextAvailableServerName());

    QJsonArray containerConfigs;
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);
    
    for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
        auto containerConfig = iterator.value();

        if (ContainerProps::isSupportedByCurrentPlatform(container)) {
            auto errorCode = vpnConfigurationController.createProtocolConfigForContainer(serverCredentials, iterator.key(),
                                                                                         containerConfig);
            if (errorCode != ErrorCode::NoError) {
                return errorCode;
            }
            containerConfigs.append(containerConfig);
        } else {
            containerConfigs.append(containerConfig);
        }
    }

    serverConfig.insert(config_key::containers, containerConfigs);
    serverConfig.insert(config_key::defaultContainer, ContainerProps::containerToString(container));

    int serverIndex = m_settings->nextAvailableServerIndex();
    m_settings->setServerConfig(serverIndex, serverConfig);
    m_settings->setDefaultServerIndex(serverIndex);

    finishMessage = tr("Server '%1' was successfully added").arg(serverCredentials.hostName);
    return ErrorCode::NoError;
}

ErrorCode InstallController::installContainer(const DockerContainer container,
                                           const QMap<DockerContainer, QJsonObject> &installedContainers,
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

QJsonObject InstallController::generateContainerConfig(const DockerContainer container, int port, 
                                                      const TransportProto transportProto)
{
    QJsonObject config;
    auto mainProto = ContainerProps::defaultProtocol(container);
    for (auto protocol : ContainerProps::protocolsForContainer(container)) {
        QJsonObject containerConfig;

        if (protocol == mainProto) {
            containerConfig.insert(config_key::port, QString::number(port));
            containerConfig.insert(config_key::transport_proto, ProtocolProps::transportProtoToString(transportProto, protocol));

            if (container == DockerContainer::Awg) {
                QString junkPacketCount = QString::number(QRandomGenerator::global()->bounded(2, 5));
                QString junkPacketMinSize = QString::number(10);
                QString junkPacketMaxSize = QString::number(50);

                int s1 = QRandomGenerator::global()->bounded(15, 150);
                int s2 = QRandomGenerator::global()->bounded(15, 150);

                QSet<int> usedValues;
                usedValues.insert(s1);

                while (usedValues.contains(s2) || s1 + AwgConstant::messageInitiationSize == s2 + AwgConstant::messageResponseSize) {
                    s2 = QRandomGenerator::global()->bounded(15, 150);
                }
                usedValues.insert(s2);

                QString initPacketJunkSize = QString::number(s1);
                QString responsePacketJunkSize = QString::number(s2);

                QSet<QString> headersValue;
                while (headersValue.size() != 4) {
                    auto max = (std::numeric_limits<qint32>::max)();
                    headersValue.insert(QString::number(QRandomGenerator::global()->bounded(5, max)));
                }

                auto headersValueList = headersValue.values();

                QString initPacketMagicHeader = headersValueList.at(0);
                QString responsePacketMagicHeader = headersValueList.at(1);
                QString underloadPacketMagicHeader = headersValueList.at(2);
                QString transportPacketMagicHeader = headersValueList.at(3);

                containerConfig[config_key::junkPacketCount] = junkPacketCount;
                containerConfig[config_key::junkPacketMinSize] = junkPacketMinSize;
                containerConfig[config_key::junkPacketMaxSize] = junkPacketMaxSize;
                containerConfig[config_key::initPacketJunkSize] = initPacketJunkSize;
                containerConfig[config_key::responsePacketJunkSize] = responsePacketJunkSize;
                containerConfig[config_key::initPacketMagicHeader] = initPacketMagicHeader;
                containerConfig[config_key::responsePacketMagicHeader] = responsePacketMagicHeader;
                containerConfig[config_key::underloadPacketMagicHeader] = underloadPacketMagicHeader;
                containerConfig[config_key::transportPacketMagicHeader] = transportPacketMagicHeader;
            } else if (container == DockerContainer::Sftp) {
                containerConfig.insert(config_key::userName, protocols::sftp::defaultUserName);
                containerConfig.insert(config_key::password, Utils::getRandomString(16));
            } else if (container == DockerContainer::Socks5Proxy) {
                containerConfig.insert(config_key::userName, protocols::socks5Proxy::defaultUserName);
                containerConfig.insert(config_key::password, Utils::getRandomString(16));
            }

            config.insert(config_key::container, ContainerProps::containerToString(container));
        }
        config.insert(ProtocolProps::protoToString(protocol), containerConfig);
    }
    
    return config;
} 
