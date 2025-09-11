#include "installController.h"

#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QtConcurrent>

#include "core/api/apiUtils.h"
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
                                     const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_protocolModel(protocolsModel),
      m_clientManagementModel(clientManagementModel),
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
                // int s3 = QRandomGenerator::global()->bounded(15, 150);
                // int s4 = QRandomGenerator::global()->bounded(15, 150);

                // Ensure all values are unique and don't create equal packet sizes
                QSet<int> usedValues;
                usedValues.insert(s1);

                while (usedValues.contains(s2) || s1 + AwgConstant::messageInitiationSize == s2 + AwgConstant::messageResponseSize) {
                    s2 = QRandomGenerator::global()->bounded(15, 150);
                }
                usedValues.insert(s2);

                // while (usedValues.contains(s3)
                //        || s1 + AwgConstant::messageInitiationSize == s3 + AwgConstant::messageCookieReplySize
                //        || s2 + AwgConstant::messageResponseSize == s3 + AwgConstant::messageCookieReplySize) {
                //     s3 = QRandomGenerator::global()->bounded(15, 150);
                // }
                // usedValues.insert(s3);

                // while (usedValues.contains(s4)
                //        || s1 + AwgConstant::messageInitiationSize == s4 + AwgConstant::messageTransportSize
                //        || s2 + AwgConstant::messageResponseSize == s4 + AwgConstant::messageTransportSize
                //        || s3 + AwgConstant::messageCookieReplySize == s4 + AwgConstant::messageTransportSize) {
                //     s4 = QRandomGenerator::global()->bounded(15, 150);
                // }

                QString initPacketJunkSize = QString::number(s1);
                QString responsePacketJunkSize = QString::number(s2);
                // QString cookieReplyPacketJunkSize = QString::number(s3);
                // QString transportPacketJunkSize = QString::number(s4);

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

                // TODO:
                // containerConfig[config_key::cookieReplyPacketJunkSize] = cookieReplyPacketJunkSize;
                // containerConfig[config_key::transportPacketJunkSize] = transportPacketJunkSize;

                // containerConfig[config_key::specialJunk1] = specialJunk1;
                // containerConfig[config_key::specialJunk2] = specialJunk2;
                // containerConfig[config_key::specialJunk3] = specialJunk3;
                // containerConfig[config_key::specialJunk4] = specialJunk4;
                // containerConfig[config_key::specialJunk5] = specialJunk5;
                // containerConfig[config_key::controlledJunk1] = controlledJunk1;
                // containerConfig[config_key::controlledJunk2] = controlledJunk2;
                // containerConfig[config_key::controlledJunk3] = controlledJunk3;
                // containerConfig[config_key::specialHandshakeTimeout] = specialHandshakeTimeout;

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
            const auto &protocols = ContainerProps::protocolsForContainer(container);
            
            for (const auto &protocol : protocols) {
                QJsonObject containerConfig;
                
                // for Multiprotocols (OpenVPN over SS, OpenVPN over Cloak)
                bool shouldProcessProtocol = false;
                if (container == DockerContainer::ShadowSocks || container == DockerContainer::Cloak) {
                    shouldProcessProtocol = true;
                } else {
                    shouldProcessProtocol = (protocol == mainProto);
                }
                
                if (shouldProcessProtocol) {
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

                        containerConfig[config_key::subnet_address] = serverConfigMap.value("Address").remove("/24");
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

                        // containerConfig[config_key::cookieReplyPacketJunkSize] = serverConfigMap.value(config_key::cookieReplyPacketJunkSize);
                        // containerConfig[config_key::transportPacketJunkSize] = serverConfigMap.value(config_key::transportPacketJunkSize);

                        // containerConfig[config_key::specialJunk1] = serverConfigMap.value(config_key::specialJunk1);
                        // containerConfig[config_key::specialJunk2] = serverConfigMap.value(config_key::specialJunk2);
                        // containerConfig[config_key::specialJunk3] = serverConfigMap.value(config_key::specialJunk3);
                        // containerConfig[config_key::specialJunk4] = serverConfigMap.value(config_key::specialJunk4);
                        // containerConfig[config_key::specialJunk5] = serverConfigMap.value(config_key::specialJunk5);
                        // containerConfig[config_key::controlledJunk1] = serverConfigMap.value(config_key::controlledJunk1);
                        // containerConfig[config_key::controlledJunk2] = serverConfigMap.value(config_key::controlledJunk2);
                        // containerConfig[config_key::controlledJunk3] = serverConfigMap.value(config_key::controlledJunk3);
                        // containerConfig[config_key::specialHandshakeTimeout] = serverConfigMap.value(config_key::specialHandshakeTimeout);

                    } else if (protocol == Proto::WireGuard) {
                        QString serverConfig = serverController->getTextFileFromContainer(container, credentials,
                                                                                          protocols::wireguard::serverConfigPath, errorCode);

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
                        containerConfig[config_key::subnet_address] = serverConfigMap.value("Address").remove("/24");
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
                    } else if (protocol == Proto::Xray) {
                        QString currentConfig = serverController->getTextFileFromContainer(
                                container, credentials, amnezia::protocols::xray::serverConfigPath, errorCode);

                        QJsonDocument doc = QJsonDocument::fromJson(currentConfig.toUtf8());
                        qDebug() << doc;
                        if (doc.isNull() || !doc.isObject()) {
                            logger.error() << "Failed to parse server config JSON";
                            errorCode = ErrorCode::InternalError;
                            return errorCode;
                        }
                        QJsonObject serverConfig = doc.object();

                        if (!serverConfig.contains("inbounds")) {
                            logger.error() << "Server config missing 'inbounds' field";
                            errorCode = ErrorCode::InternalError;
                            return errorCode;
                        }

                        QJsonArray inbounds = serverConfig["inbounds"].toArray();
                        if (inbounds.isEmpty()) {
                            logger.error() << "Server config has empty 'inbounds' array";
                            errorCode = ErrorCode::InternalError;
                            return errorCode;
                        }

                        QJsonObject inbound = inbounds[0].toObject();
                        if (!inbound.contains("streamSettings")) {
                            logger.error() << "Inbound missing 'streamSettings' field";
                            errorCode = ErrorCode::InternalError;
                            return errorCode;
                        }

                        QJsonObject streamSettings = inbound["streamSettings"].toObject();
                        QJsonObject realitySettings = streamSettings["realitySettings"].toObject();
                        if (!realitySettings.contains("serverNames")) {
                            logger.error() << "Settings missing 'clients' field";
                            errorCode = ErrorCode::InternalError;
                            return errorCode;
                        }

                        QString siteName = realitySettings["serverNames"][0].toString();
                        qDebug() << siteName;

                        containerConfig.insert(config_key::site, siteName);
                    } else if (protocol == Proto::OpenVpn) {
                        QString serverConfig = serverController->getTextFileFromContainer(container, credentials,
                                                                                          protocols::openvpn::serverConfigPath, errorCode);

                        QMap<QString, QString> serverConfigMap;
                        auto serverConfigLines = serverConfig.split("\n");
                        for (auto &line : serverConfigLines) {
                            auto trimmedLine = line.trimmed();
                            if (trimmedLine.startsWith("#") || trimmedLine.isEmpty()) {
                                continue;
                            } else {
                                QStringList parts = trimmedLine.split(" ");
                                if (parts.count() >= 2) {
                                    QString key = parts[0];
                                    QString value = parts.mid(1).join(" ");
                                    serverConfigMap.insert(key, value);
                                }
                            }
                        }

                        QString serverValue = serverConfigMap.value("server");

                        if (!serverValue.isEmpty()) {
                            QStringList serverParts = serverValue.split(" ");
                            if (serverParts.count() >= 1) {
                                containerConfig[config_key::subnet_address] = serverParts[0];
                            }
                        }

                        bool ncpDisable = serverConfig.contains("ncp-disable");
                        containerConfig[config_key::ncp_disable] = ncpDisable;

                        bool tlsAuth = serverConfig.contains("tls-auth");
                        containerConfig[config_key::tls_auth] = tlsAuth;

                        bool blockOutsideDns = serverConfig.contains("block-outside-dns");
                        
                        containerConfig[config_key::block_outside_dns] = blockOutsideDns;

                        QString cipher = serverConfigMap.value("cipher");
                        if (!cipher.isEmpty()) {
                            containerConfig[config_key::cipher] = cipher;
                        }

                        QString hash = serverConfigMap.value("auth");
                        if (!hash.isEmpty()) {
                            containerConfig[config_key::hash] = hash;
                        }
                    } else if (protocol == Proto::Cloak) {
                        QString cloakConfig = serverController->getTextFileFromContainer(container, credentials,
                                                                                         "/opt/amnezia/cloak/ck-config.json", errorCode);

                        QJsonDocument doc = QJsonDocument::fromJson(cloakConfig.toUtf8());
                        
                        if (!doc.isNull() && doc.isObject()) {
                            QJsonObject cloakConfigObj = doc.object();
                            
                            QString site = cloakConfigObj.value("RedirAddr").toString();
                            if (!site.isEmpty()) {
                                containerConfig[config_key::site] = site;
                            }
                        } else {
                            qDebug() << "Failed to parse main loop Cloak JSON config";
                        }
                        
                    } else if (protocol == Proto::ShadowSocks) {
                        QString shadowsocksConfig = serverController->getTextFileFromContainer(container, credentials,
                                                                                               "/opt/amnezia/shadowsocks/ss-config.json", errorCode);

                        QJsonDocument doc = QJsonDocument::fromJson(shadowsocksConfig.toUtf8());
                        
                        if (!doc.isNull() && doc.isObject()) {
                            QJsonObject ssConfigObj = doc.object();
                            QString cipher = ssConfigObj.value("method").toString();
                            if (!cipher.isEmpty()) {
                                containerConfig[config_key::cipher] = cipher;
                            }
                        } else {
                            qDebug() << "Failed to parse main loop Shadowsocks JSON config";
                        }
                    }

                    config.insert(config_key::container, ContainerProps::containerToString(container));
                }
                if (shouldProcessProtocol) {
                    config.insert(ProtocolProps::protoToString(protocol), containerConfig);
                }
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
    QJsonObject updatedConfig = m_settings->containerConfig(serverIndex, container);
    emit profileCleared(updatedConfig);
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

bool InstallController::isConfigValid()
{
    int serverIndex = m_serversModel->getDefaultServerIndex();
    QJsonObject serverConfigObject = m_serversModel->getServerConfig(serverIndex);

    if (apiUtils::isServerFromApi(serverConfigObject)) {
        return true;
    }

    if (!m_serversModel->data(serverIndex, ServersModel::Roles::HasInstalledContainers).toBool()) {
        emit noInstalledContainers();
        return false;
    }

    DockerContainer container = qvariant_cast<DockerContainer>(m_serversModel->data(serverIndex, ServersModel::Roles::DefaultContainerRole));

    if (container == DockerContainer::None) {
        emit installationErrorOccurred(ErrorCode::NoInstalledContainersError);
        return false;
    }

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);

    QFutureWatcher<ErrorCode> watcher;

    QFuture<ErrorCode> future = QtConcurrent::run([this, container, &credentials, &containerConfig, &serverController]() {
        ErrorCode errorCode = ErrorCode::NoError;

        auto isProtocolConfigExists = [](const QJsonObject &containerConfig, const DockerContainer container) {
            for (Proto protocol : ContainerProps::protocolsForContainer(container)) {
                QString protocolConfig =
                        containerConfig.value(ProtocolProps::protoToString(protocol)).toObject().value(config_key::last_config).toString();

                if (protocolConfig.isEmpty()) {
                    return false;
                }
            }
            return true;
        };

        if (!isProtocolConfigExists(containerConfig, container)) {
            VpnConfigurationsController vpnConfigurationController(m_settings, serverController);
            errorCode = vpnConfigurationController.createProtocolConfigForContainer(credentials, container, containerConfig);
            if (errorCode != ErrorCode::NoError) {
                return errorCode;
            }
            m_serversModel->updateContainerConfig(container, containerConfig);

            errorCode = m_clientManagementModel->appendClient(container, credentials, containerConfig,
                                                              QString("Admin [%1]").arg(QSysInfo::prettyProductName()), serverController);
            if (errorCode != ErrorCode::NoError) {
                return errorCode;
            }
        }
        return errorCode;
    });

    QEventLoop wait;
    connect(&watcher, &QFutureWatcher<ErrorCode>::finished, &wait, &QEventLoop::quit);
    watcher.setFuture(future);
    wait.exec();

    ErrorCode errorCode = watcher.result();

    if (errorCode != ErrorCode::NoError) {
        emit installationErrorOccurred(errorCode);
        return false;
    }
    return true;
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
