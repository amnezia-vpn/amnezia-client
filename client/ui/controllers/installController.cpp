#include "installController.h"

#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QStandardPaths>

#include "core/controllers/apiController.h"
#include "core/controllers/serverController.h"
#include "core/controllers/vpnConfigurationController.h"
#include "core/networkUtilities.h"
#include "logger.h"
#include "ui/models/protocols/awgConfigModel.h"
#include "ui/models/protocols/wireguardConfigModel.h"
#include "utilities.h"

namespace
{
    Logger logger("ServerController");

    namespace configKey
    {
        constexpr char serviceInfo[] = "service_info";
        constexpr char serviceType[] = "service_type";
        constexpr char serviceProtocol[] = "service_protocol";
        constexpr char userCountryCode[] = "user_country_code";

        constexpr char serverCountryCode[] = "server_country_code";
        constexpr char serverCountryName[] = "server_country_name";
        constexpr char availableCountries[] = "available_countries";

        constexpr char apiConfig[] = "api_config";
        constexpr char authData[] = "auth_data";
    }
}

InstallController::InstallController(const QSharedPointer<ServersModel> &serversModel, const QSharedPointer<ContainersModel> &containersModel,
                                     const QSharedPointer<ProtocolsModel> &protocolsModel,
                                     const QSharedPointer<ClientManagementModel> &clientManagementModel,
                                     const QSharedPointer<ApiServicesModel> &apiServicesModel, const std::shared_ptr<Settings> &settings,
                                     QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_protocolModel(protocolsModel),
      m_clientManagementModel(clientManagementModel),
      m_apiServicesModel(apiServicesModel),
      m_settings(settings)
{
}

InstallController::~InstallController()
{
#ifdef Q_OS_WINDOWS
    for (QSharedPointer<QProcess> process : m_sftpMountProcesses) {
        Utils::signalCtrl(process->processId(), CTRL_C_EVENT);
        process->kill();
        process->waitForFinished();
    }
#endif
}

void InstallController::install(DockerContainer container, int port, TransportProto transportProto)
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
                while (s1 + AwgConstant::messageInitiationSize == s2 + AwgConstant::messageResponseSize) {
                    s2 = QRandomGenerator::global()->bounded(15, 150);
                }

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

    ServerCredentials serverCredentials;
    if (m_shouldCreateServer) {
        if (isServerAlreadyExists()) {
            return;
        }
        serverCredentials = m_processedServerCredentials;
    } else {
        int serverIndex = m_serversModel->getProcessedServerIndex();
        serverCredentials = qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));
    }

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    connect(serverController.get(), &ServerController::serverIsBusy, this, &InstallController::serverIsBusy);
    connect(this, &InstallController::cancelInstallation, serverController.get(), &ServerController::cancelInstallation);

    QMap<DockerContainer, QJsonObject> installedContainers;
    ErrorCode errorCode = getAlreadyInstalledContainers(serverCredentials, serverController, installedContainers);
    if (errorCode) {
        emit installationErrorOccurred(errorCode);
        return;
    }

    QString finishMessage = "";

    if (!installedContainers.contains(container)) {
        errorCode = serverController->setupContainer(serverCredentials, container, config);
        if (errorCode) {
            emit installationErrorOccurred(errorCode);
            return;
        }

        installedContainers.insert(container, config);
        finishMessage = tr("%1 installed successfully. ").arg(ContainerProps::containerHumanNames().value(container));
    } else {
        finishMessage = tr("%1 is already installed on the server. ").arg(ContainerProps::containerHumanNames().value(container));
    }

    if (errorCode) {
        emit installationErrorOccurred(errorCode);
        return;
    }

    if (m_shouldCreateServer) {
        installServer(container, installedContainers, serverCredentials, serverController, finishMessage);
    } else {
        installContainer(container, installedContainers, serverCredentials, serverController, finishMessage);
    }
}

void InstallController::installServer(const DockerContainer container, const QMap<DockerContainer, QJsonObject> &installedContainers,
                                      const ServerCredentials &serverCredentials, const QSharedPointer<ServerController> &serverController,
                                      QString &finishMessage)
{
    if (installedContainers.size() > 1) {
        finishMessage += tr("\nAdded containers that were already installed on the server");
    }

    QJsonObject server;
    server.insert(config_key::hostName, m_processedServerCredentials.hostName);
    server.insert(config_key::userName, m_processedServerCredentials.userName);
    server.insert(config_key::password, m_processedServerCredentials.secretData);
    server.insert(config_key::port, m_processedServerCredentials.port);
    server.insert(config_key::description, m_settings->nextAvailableServerName());

    QJsonArray containerConfigs;
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);
    for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
        auto containerConfig = iterator.value();

        if (ContainerProps::isSupportedByCurrentPlatform(container)) {
            auto errorCode = vpnConfigurationController.createProtocolConfigForContainer(m_processedServerCredentials, iterator.key(),
                                                                                         containerConfig);
            if (errorCode) {
                emit installationErrorOccurred(errorCode);
                return;
            }
            containerConfigs.append(containerConfig);

            errorCode = m_clientManagementModel->appendClient(iterator.key(), serverCredentials, containerConfig,
                                                              QString("Admin [%1]").arg(QSysInfo::prettyProductName()), serverController);
            if (errorCode) {
                emit installationErrorOccurred(errorCode);
                return;
            }
        } else {
            containerConfigs.append(containerConfig);
        }
    }

    server.insert(config_key::containers, containerConfigs);
    server.insert(config_key::defaultContainer, ContainerProps::containerToString(container));

    m_serversModel->addServer(server);

    emit installServerFinished(finishMessage);
}

void InstallController::installContainer(const DockerContainer container, const QMap<DockerContainer, QJsonObject> &installedContainers,
                                         const ServerCredentials &serverCredentials,
                                         const QSharedPointer<ServerController> &serverController, QString &finishMessage)
{
    bool isInstalledContainerAddedToGui = false;

    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);
    for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
        QJsonObject containerConfig = m_containersModel->getContainerConfig(iterator.key());
        if (containerConfig.isEmpty()) {
            containerConfig = iterator.value();

            if (ContainerProps::isSupportedByCurrentPlatform(container)) {
                auto errorCode =
                        vpnConfigurationController.createProtocolConfigForContainer(serverCredentials, iterator.key(), containerConfig);
                if (errorCode) {
                    emit installationErrorOccurred(errorCode);
                    return;
                }
                m_serversModel->addContainerConfig(iterator.key(), containerConfig);

                errorCode = m_clientManagementModel->appendClient(iterator.key(), serverCredentials, containerConfig,
                                                                  QString("Admin [%1]").arg(QSysInfo::prettyProductName()), serverController);
                if (errorCode) {
                    emit installationErrorOccurred(errorCode);
                    return;
                }
            } else {
                m_serversModel->addContainerConfig(iterator.key(), containerConfig);
            }

            if (container != iterator.key()) { // skip the newly installed container
                isInstalledContainerAddedToGui = true;
            }
        }
    }
    if (isInstalledContainerAddedToGui) {
        finishMessage += tr("\nAlready installed containers were found on the server. "
                            "All installed containers have been added to the application");
    }

    emit installContainerFinished(finishMessage, ContainerProps::containerService(container) == ServiceType::Other);
}

bool InstallController::isServerAlreadyExists()
{
    for (int i = 0; i < m_serversModel->getServersCount(); i++) {
        auto modelIndex = m_serversModel->index(i);
        const ServerCredentials credentials =
                qvariant_cast<ServerCredentials>(m_serversModel->data(modelIndex, ServersModel::Roles::CredentialsRole));
        if (m_processedServerCredentials.hostName == credentials.hostName && m_processedServerCredentials.port == credentials.port) {
            emit serverAlreadyExists(i);
            return true;
        }
    }
    return false;
}

void InstallController::scanServerForInstalledContainers()
{
    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials serverCredentials =
            qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));

    QMap<DockerContainer, QJsonObject> installedContainers;
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    ErrorCode errorCode = getAlreadyInstalledContainers(serverCredentials, serverController, installedContainers);

    if (errorCode == ErrorCode::NoError) {
        bool isInstalledContainerAddedToGui = false;
        VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

        for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
            auto container = iterator.key();
            QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
            if (containerConfig.isEmpty()) {
                containerConfig = iterator.value();

                if (ContainerProps::isSupportedByCurrentPlatform(container)) {
                    auto errorCode =
                            vpnConfigurationController.createProtocolConfigForContainer(serverCredentials, container, containerConfig);
                    if (errorCode) {
                        emit installationErrorOccurred(errorCode);
                        return;
                    }
                    m_serversModel->addContainerConfig(container, containerConfig);

                    errorCode = m_clientManagementModel->appendClient(container, serverCredentials, containerConfig,
                                                                      QString("Admin [%1]").arg(QSysInfo::prettyProductName()),
                                                                      serverController);
                    if (errorCode) {
                        emit installationErrorOccurred(errorCode);
                        return;
                    }
                } else {
                    m_serversModel->addContainerConfig(container, containerConfig);
                }

                isInstalledContainerAddedToGui = true;
            }
        }

        emit scanServerFinished(isInstalledContainerAddedToGui);
        return;
    }

    emit installationErrorOccurred(errorCode);
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
            QString name = containerAndPortMatch.captured(1);
            QString port = containerAndPortMatch.captured(2);
            QString transportProto = containerAndPortMatch.captured(3);
            DockerContainer container = ContainerProps::containerFromString(name);

            QJsonObject config;
            Proto mainProto = ContainerProps::defaultProtocol(container);
            for (auto protocol : ContainerProps::protocolsForContainer(container)) {
                QJsonObject containerConfig;
                if (protocol == mainProto) {
                    containerConfig.insert(config_key::port, port);
                    containerConfig.insert(config_key::transport_proto, transportProto);

                    if (protocol == Proto::Awg) {
                        QString serverConfig = serverController->getTextFileFromContainer(container, credentials,
                                                                                          protocols::awg::serverConfigPath, errorCode);

                        QMap<QString, QString> serverConfigMap;
                        auto serverConfigLines = serverConfig.split("\n");
                        for (auto &line : serverConfigLines) {
                            auto trimmedLine = line.trimmed();
                            if (trimmedLine.startsWith("[") && trimmedLine.endsWith("]")) {
                                continue;
                            } else {
                                QStringList parts = trimmedLine.split(" = ");
                                if (parts.count() == 2) {
                                    serverConfigMap.insert(parts[0].trimmed(), parts[1].trimmed());
                                }
                            }
                        }

                        containerConfig[config_key::junkPacketCount] = serverConfigMap.value(config_key::junkPacketCount);
                        containerConfig[config_key::junkPacketMinSize] = serverConfigMap.value(config_key::junkPacketMinSize);
                        containerConfig[config_key::junkPacketMaxSize] = serverConfigMap.value(config_key::junkPacketMaxSize);
                        containerConfig[config_key::initPacketJunkSize] = serverConfigMap.value(config_key::initPacketJunkSize);
                        containerConfig[config_key::responsePacketJunkSize] = serverConfigMap.value(config_key::responsePacketJunkSize);
                        containerConfig[config_key::initPacketMagicHeader] = serverConfigMap.value(config_key::initPacketMagicHeader);
                        containerConfig[config_key::responsePacketMagicHeader] = serverConfigMap.value(config_key::responsePacketMagicHeader);
                        containerConfig[config_key::underloadPacketMagicHeader] =
                                serverConfigMap.value(config_key::underloadPacketMagicHeader);
                        containerConfig[config_key::transportPacketMagicHeader] =
                                serverConfigMap.value(config_key::transportPacketMagicHeader);
                    } else if (protocol == Proto::Sftp) {
                        stdOut.clear();
                        script = QString("sudo docker inspect --format '{{.Config.Cmd}}' %1").arg(name);

                        ErrorCode errorCode = serverController->runScript(credentials, script, cbReadStdOut, cbReadStdErr);
                        if (errorCode != ErrorCode::NoError) {
                            return errorCode;
                        }

                        auto sftpInfo = stdOut.split(":");
                        if (sftpInfo.size() < 2) {
                            logger.error() << "Key parameters for the sftp container are missing";
                            continue;
                        }
                        auto userName = sftpInfo.at(0);
                        userName = userName.remove(0, 1);
                        auto password = sftpInfo.at(1);

                        containerConfig.insert(config_key::userName, userName);
                        containerConfig.insert(config_key::password, password);
                    } else if (protocol == Proto::Socks5Proxy) {
                        QString proxyConfig = serverController->getTextFileFromContainer(container, credentials,
                                                                                         protocols::socks5Proxy::proxyConfigPath, errorCode);

                        const static QRegularExpression usernameAndPasswordRegExp("users (\\w+):CL:(\\w+)");
                        QRegularExpressionMatch usernameAndPasswordMatch = usernameAndPasswordRegExp.match(proxyConfig);

                        if (usernameAndPasswordMatch.hasMatch()) {
                            QString userName = usernameAndPasswordMatch.captured(1);
                            QString password = usernameAndPasswordMatch.captured(2);

                            containerConfig.insert(config_key::userName, userName);
                            containerConfig.insert(config_key::password, password);
                        }
                    }

                    config.insert(config_key::container, ContainerProps::containerToString(container));
                }
                config.insert(ProtocolProps::protoToString(protocol), containerConfig);
            }
            installedContainers.insert(container, config);
        }
        const static QRegularExpression torOrDnsRegExp("(amnezia-(?:torwebsite|dns)).*?([0-9]*)/(udp|tcp).*");
        QRegularExpressionMatch torOrDnsRegMatch = torOrDnsRegExp.match(containerInfo);
        if (torOrDnsRegMatch.hasMatch()) {
            QString name = torOrDnsRegMatch.captured(1);
            QString port = torOrDnsRegMatch.captured(2);
            QString transportProto = torOrDnsRegMatch.captured(3);
            DockerContainer container = ContainerProps::containerFromString(name);

            QJsonObject config;
            Proto mainProto = ContainerProps::defaultProtocol(container);
            for (auto protocol : ContainerProps::protocolsForContainer(container)) {
                QJsonObject containerConfig;
                if (protocol == mainProto) {
                    containerConfig.insert(config_key::port, port);
                    containerConfig.insert(config_key::transport_proto, transportProto);

                    if (protocol == Proto::TorWebSite) {
                        stdOut.clear();
                        script = QString("sudo docker exec -i %1 sh -c 'cat /var/lib/tor/hidden_service/hostname'").arg(name);

                        ErrorCode errorCode = serverController->runScript(credentials, script, cbReadStdOut, cbReadStdErr);
                        if (errorCode != ErrorCode::NoError) {
                            return errorCode;
                        }

                        if (stdOut.isEmpty()) {
                            logger.error() << "Key parameters for the tor container are missing";
                            continue;
                        }

                        QString onion = stdOut;
                        onion.replace("\n", "");
                        containerConfig.insert(config_key::site, onion);
                    }

                    config.insert(config_key::container, ContainerProps::containerToString(container));
                }
                config.insert(ProtocolProps::protoToString(protocol), containerConfig);
            }
            installedContainers.insert(container, config);
        }
    }

    return ErrorCode::NoError;
}

void InstallController::updateContainer(QJsonObject config)
{
    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials serverCredentials =
            qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));

    const DockerContainer container = ContainerProps::containerFromString(config.value(config_key::container).toString());
    QJsonObject oldContainerConfig = m_containersModel->getContainerConfig(container);
    ErrorCode errorCode = ErrorCode::NoError;

    if (isUpdateDockerContainerRequired(container, oldContainerConfig, config)) {
        QSharedPointer<ServerController> serverController(new ServerController(m_settings));
        connect(serverController.get(), &ServerController::serverIsBusy, this, &InstallController::serverIsBusy);
        connect(this, &InstallController::cancelInstallation, serverController.get(), &ServerController::cancelInstallation);

        errorCode = serverController->updateContainer(serverCredentials, container, oldContainerConfig, config);
        clearCachedProfile(serverController);
    }

    if (errorCode == ErrorCode::NoError) {
        m_serversModel->updateContainerConfig(container, config);
        m_protocolModel->updateModel(config);

        auto defaultContainer = qvariant_cast<DockerContainer>(m_serversModel->data(serverIndex, ServersModel::Roles::DefaultContainerRole));
        if ((serverIndex == m_serversModel->getDefaultServerIndex()) && (container == defaultContainer)) {
            emit currentContainerUpdated();
        } else {
            emit updateContainerFinished(tr("Settings updated successfully"));
        }

        return;
    }

    emit installationErrorOccurred(errorCode);
}

void InstallController::rebootProcessedServer()
{
    int serverIndex = m_serversModel->getProcessedServerIndex();
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    const auto errorCode = m_serversModel->rebootServer(serverController);
    if (errorCode == ErrorCode::NoError) {
        emit rebootProcessedServerFinished(tr("Server '%1' was rebooted").arg(serverName));
    } else {
        emit installationErrorOccurred(errorCode);
    }
}

void InstallController::removeProcessedServer()
{
    int serverIndex = m_serversModel->getProcessedServerIndex();
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();

    m_serversModel->removeServer();
    emit removeProcessedServerFinished(tr("Server '%1' was removed").arg(serverName));
}

void InstallController::removeAllContainers()
{
    int serverIndex = m_serversModel->getProcessedServerIndex();
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    ErrorCode errorCode = m_serversModel->removeAllContainers(serverController);
    if (errorCode == ErrorCode::NoError) {
        emit removeAllContainersFinished(tr("All containers from server '%1' have been removed").arg(serverName));
        return;
    }
    emit installationErrorOccurred(errorCode);
}

void InstallController::removeProcessedContainer()
{
    int serverIndex = m_serversModel->getProcessedServerIndex();
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();

    int container = m_containersModel->getProcessedContainerIndex();
    QString containerName = m_containersModel->getProcessedContainerName();

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    ErrorCode errorCode = m_serversModel->removeContainer(serverController, container);
    if (errorCode == ErrorCode::NoError) {

        emit removeProcessedContainerFinished(tr("%1 has been removed from the server '%2'").arg(containerName, serverName));
        return;
    }
    emit installationErrorOccurred(errorCode);
}

void InstallController::removeApiConfig(const int serverIndex)
{
    m_serversModel->removeApiConfig(serverIndex);
    emit apiConfigRemoved(tr("Api config removed"));
}

void InstallController::clearCachedProfile(QSharedPointer<ServerController> serverController)
{
    if (serverController.isNull()) {
        serverController.reset(new ServerController(m_settings));
    }

    int serverIndex = m_serversModel->getProcessedServerIndex();
    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getProcessedContainerIndex());
    if (ContainerProps::containerService(container) == ServiceType::Other) {
        return;
    }

    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    ServerCredentials serverCredentials =
            qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));

    m_serversModel->clearCachedProfile(container);
    m_clientManagementModel->revokeClient(containerConfig, container, serverCredentials, serverIndex, serverController);

    emit cachedProfileCleared(tr("%1 cached profile cleared").arg(ContainerProps::containerHumanNames().value(container)));
}

QRegularExpression InstallController::ipAddressPortRegExp()
{
    return NetworkUtilities::ipAddressPortRegExp();
}

QRegularExpression InstallController::ipAddressRegExp()
{
    return NetworkUtilities::ipAddressRegExp();
}

void InstallController::setProcessedServerCredentials(const QString &hostName, const QString &userName, const QString &secretData)
{
    m_processedServerCredentials.hostName = hostName;
    if (m_processedServerCredentials.hostName.contains(":")) {
        m_processedServerCredentials.port = m_processedServerCredentials.hostName.split(":").at(1).toInt();
        m_processedServerCredentials.hostName = m_processedServerCredentials.hostName.split(":").at(0);
    }
    m_processedServerCredentials.userName = userName;
    m_processedServerCredentials.secretData = secretData;
}

void InstallController::setShouldCreateServer(bool shouldCreateServer)
{
    m_shouldCreateServer = shouldCreateServer;
}

void InstallController::mountSftpDrive(const QString &port, const QString &password, const QString &username)
{
    QString mountPath;
    QString cmd;

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials serverCredentials =
            qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));
    QString hostname = serverCredentials.hostName;

#ifdef Q_OS_WINDOWS
    mountPath = Utils::getNextDriverLetter() + ":";
    //    QString cmd = QString("net use \\\\sshfs\\%1@x.x.x.x!%2 /USER:%1 %3")
    //            .arg(labelTftpUserNameText())
    //            .arg(labelTftpPortText())
    //            .arg(labelTftpPasswordText());

    cmd = "C:\\Program Files\\SSHFS-Win\\bin\\sshfs.exe";
#elif defined AMNEZIA_DESKTOP
    mountPath = QString("%1/sftp:%2:%3").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), hostname, port);
    QDir dir(mountPath);
    if (!dir.exists()) {
        dir.mkpath(mountPath);
    }

    cmd = "/usr/local/bin/sshfs";
#endif

#ifdef AMNEZIA_DESKTOP
    QSharedPointer<QProcess> process;
    process.reset(new QProcess());
    m_sftpMountProcesses.append(process);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process.get(), &QProcess::readyRead, this, [this, process, mountPath]() {
        QString s = process->readAll();
        if (s.contains("The service sshfs has been started")) {
            QDesktopServices::openUrl(QUrl("file:///" + mountPath));
        }
        qDebug() << s;
    });

    process->setProgram(cmd);

    QString args = QString("%1@%2:/ %3 "
                           "-o port=%4 "
                           "-f "
                           "-o reconnect "
                           "-o rellinks "
                           "-o fstypename=SSHFS "
                           "-o ssh_command=/usr/bin/ssh.exe "
                           "-o UserKnownHostsFile=/dev/null "
                           "-o StrictHostKeyChecking=no "
                           "-o password_stdin")
                           .arg(username, hostname, mountPath, port);

    //    args.replace("\n", " ");
    //    args.replace("\r", " ");
    // #ifndef Q_OS_WIN
    //    args.replace("reconnect-orellinks", "");
    // #endif
    process->setArguments(args.split(" ", Qt::SkipEmptyParts));
    process->start();
    process->waitForStarted(50);
    if (process->state() != QProcess::Running) {
        qDebug() << "onPushButtonSftpMountDriveClicked process not started";
        qDebug() << args;
    } else {
        process->write((password + "\n").toUtf8());
    }

#endif
}

bool InstallController::checkSshConnection(QSharedPointer<ServerController> serverController)
{
    if (serverController.isNull()) {
        serverController.reset(new ServerController(m_settings));
    }

    ErrorCode errorCode = ErrorCode::NoError;
    m_privateKeyPassphrase = "";

    if (m_processedServerCredentials.secretData.contains("BEGIN") && m_processedServerCredentials.secretData.contains("PRIVATE KEY")) {
        auto passphraseCallback = [this]() {
            emit passphraseRequestStarted();
            QEventLoop loop;
            QObject::connect(this, &InstallController::passphraseRequestFinished, &loop, &QEventLoop::quit);
            loop.exec();

            return m_privateKeyPassphrase;
        };

        QString decryptedPrivateKey;
        errorCode = serverController->getDecryptedPrivateKey(m_processedServerCredentials, decryptedPrivateKey, passphraseCallback);
        if (errorCode == ErrorCode::NoError) {
            m_processedServerCredentials.secretData = decryptedPrivateKey;
        } else {
            emit installationErrorOccurred(errorCode);
            return false;
        }
    }

    QString output;
    output = serverController->checkSshConnection(m_processedServerCredentials, errorCode);

    if (errorCode != ErrorCode::NoError) {
        emit installationErrorOccurred(errorCode);
        return false;
    } else {
        if (output.contains(tr("Please login as the user"))) {
            output.replace("\n", "");
            emit wrongInstallationUser(output);
            return false;
        }
    }
    return true;
}

void InstallController::setEncryptedPassphrase(QString passphrase)
{
    m_privateKeyPassphrase = passphrase;
    emit passphraseRequestFinished();
}

void InstallController::addEmptyServer()
{
    QJsonObject server;
    server.insert(config_key::hostName, m_processedServerCredentials.hostName);
    server.insert(config_key::userName, m_processedServerCredentials.userName);
    server.insert(config_key::password, m_processedServerCredentials.secretData);
    server.insert(config_key::port, m_processedServerCredentials.port);
    server.insert(config_key::description, m_settings->nextAvailableServerName());

    server.insert(config_key::defaultContainer, ContainerProps::containerToString(DockerContainer::None));

    m_serversModel->addServer(server);

    emit installServerFinished(tr("Server added successfully"));
}

bool InstallController::fillAvailableServices()
{
    ApiController apiController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv());

    QByteArray responseBody;
    ErrorCode errorCode = apiController.getServicesList(responseBody);
    if (errorCode != ErrorCode::NoError) {
        emit installationErrorOccurred(errorCode);
        return false;
    }

    QJsonObject data = QJsonDocument::fromJson(responseBody).object();
    m_apiServicesModel->updateModel(data);
    return true;
}

bool InstallController::installServiceFromApi()
{
    if (m_serversModel->isServerFromApiAlreadyExists(m_apiServicesModel->getCountryCode(), m_apiServicesModel->getSelectedServiceType(),
                                                     m_apiServicesModel->getSelectedServiceProtocol())) {
        emit installationErrorOccurred(ErrorCode::ApiConfigAlreadyAdded);
        return false;
    }

    ApiController apiController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv());
    QJsonObject serverConfig;

    ErrorCode errorCode = apiController.getConfigForService(m_settings->getInstallationUuid(true), m_apiServicesModel->getCountryCode(),
                                                            m_apiServicesModel->getSelectedServiceType(),
                                                            m_apiServicesModel->getSelectedServiceProtocol(), "", QJsonObject(), serverConfig);
    if (errorCode != ErrorCode::NoError) {
        emit installationErrorOccurred(errorCode);
        return false;
    }

    auto serviceInfo = m_apiServicesModel->getSelectedServiceInfo();
    QJsonObject apiConfig = serverConfig.value(configKey::apiConfig).toObject();
    apiConfig.insert(configKey::serviceInfo, serviceInfo);
    apiConfig.insert(configKey::userCountryCode, m_apiServicesModel->getCountryCode());
    apiConfig.insert(configKey::serviceType, m_apiServicesModel->getSelectedServiceType());
    apiConfig.insert(configKey::serviceProtocol, m_apiServicesModel->getSelectedServiceProtocol());

    serverConfig.insert(configKey::apiConfig, apiConfig);

    m_serversModel->addServer(serverConfig);
    emit installServerFromApiFinished(tr("%1 installed successfully.").arg(m_apiServicesModel->getSelectedServiceName()));
    return true;
}

bool InstallController::updateServiceFromApi(const int serverIndex, const QString &newCountryCode, const QString &newCountryName,
                                             bool reloadServiceConfig)
{
    ApiController apiController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv());

    auto serverConfig = m_serversModel->getServerConfig(serverIndex);
    auto apiConfig = serverConfig.value(configKey::apiConfig).toObject();
    auto authData = serverConfig.value(configKey::authData).toObject();

    QJsonObject newServerConfig;
    ErrorCode errorCode = apiController.getConfigForService(
            m_settings->getInstallationUuid(true), apiConfig.value(configKey::userCountryCode).toString(),
            apiConfig.value(configKey::serviceType).toString(), apiConfig.value(configKey::serviceProtocol).toString(), newCountryCode,
            authData, newServerConfig);
    if (errorCode != ErrorCode::NoError) {
        emit installationErrorOccurred(errorCode);
        return false;
    }

    QJsonObject newApiConfig = newServerConfig.value(configKey::apiConfig).toObject();
    newApiConfig.insert(configKey::userCountryCode, apiConfig.value(configKey::userCountryCode));
    newApiConfig.insert(configKey::serviceType, apiConfig.value(configKey::serviceType));
    newApiConfig.insert(configKey::serviceProtocol, apiConfig.value(configKey::serviceProtocol));

    newServerConfig.insert(configKey::apiConfig, newApiConfig);
    newServerConfig.insert(configKey::authData, authData);
    newServerConfig.insert(config_key::crc, serverConfig.value(config_key::crc));
    m_serversModel->editServer(newServerConfig, serverIndex);

    if (reloadServiceConfig) {
        emit reloadServerFromApiFinished(tr("API config reloaded"));
    } else if (newCountryName.isEmpty()) {
        emit updateServerFromApiFinished();
    } else {
        emit changeApiCountryFinished(tr("Successfully changed the country of connection to %1").arg(newCountryName));
    }
    return true;
}

void InstallController::updateServiceFromTelegram(const int serverIndex)
{
    ApiController *apiController = new ApiController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv());

    auto serverConfig = m_serversModel->getServerConfig(serverIndex);

    apiController->updateServerConfigFromApi(m_settings->getInstallationUuid(true), serverIndex, serverConfig);
    connect(apiController, &ApiController::finished, this, [this, apiController](const QJsonObject &config, const int serverIndex) {
        m_serversModel->editServer(config, serverIndex);
        emit updateServerFromApiFinished();
        apiController->deleteLater();
    });
    connect(apiController, &ApiController::errorOccurred, this, [this, apiController](ErrorCode errorCode) {
        emit installationErrorOccurred(errorCode);
        apiController->deleteLater();
    });
}

bool InstallController::isUpdateDockerContainerRequired(const DockerContainer container, const QJsonObject &oldConfig,
                                                        const QJsonObject &newConfig)
{
    Proto mainProto = ContainerProps::defaultProtocol(container);

    const QJsonObject &oldProtoConfig = oldConfig.value(ProtocolProps::protoToString(mainProto)).toObject();
    const QJsonObject &newProtoConfig = newConfig.value(ProtocolProps::protoToString(mainProto)).toObject();

    if (container == DockerContainer::Awg) {
        const AwgConfig oldConfig(oldProtoConfig);
        const AwgConfig newConfig(newProtoConfig);

        if (oldConfig.hasEqualServerSettings(newConfig)) {
            return false;
        }
    } else if (container == DockerContainer::WireGuard) {
        const WgConfig oldConfig(oldProtoConfig);
        const WgConfig newConfig(newProtoConfig);

        if (oldConfig.hasEqualServerSettings(newConfig)) {
            return false;
        }
    }

    return true;
}
