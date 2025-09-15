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
#include "core/models/containers/containerConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/cloakProtocolConfig.h"
#include "core/models/protocols/ipsecProtocolConfig.h"
#include "core/models/protocols/openvpnProtocolConfig.h"
#include "core/models/protocols/sftpProtocolConfig.h"
#include "core/models/protocols/shadowsocksProtocolConfig.h"
#include "core/models/protocols/socks5ProxyProtocolConfig.h"
#include "core/models/protocols/torProtocolConfig.h"
#include "core/models/protocols/wireguardProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/models/servers/selfHostedServerConfig.h"
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
    ContainerConfig containerConfig;
    containerConfig.containerName = ContainerProps::containerToString(container);
    containerConfig.containerType = container;

    auto mainProto = ContainerProps::defaultProtocol(container);
    for (auto protocol : ContainerProps::protocolsForContainer(container)) {
        if (protocol == mainProto) {
            QString protocolName = ProtocolProps::protoToString(protocol);

            switch (protocol) {
            case Proto::Awg: {
                auto protocolConfig = QSharedPointer<AwgProtocolConfig>::create(protocolName, port, transportProto);
                containerConfig.protocolConfigs.insert(protocolName, protocolConfig);
                break;
            }
            case Proto::WireGuard: {
                auto protocolConfig = QSharedPointer<WireGuardProtocolConfig>::create(protocolName, port, transportProto);
                containerConfig.protocolConfigs.insert(protocolName, protocolConfig);
                break;
            }
            case Proto::OpenVpn: {
                auto protocolConfig = QSharedPointer<OpenVpnProtocolConfig>::create(protocolName, port, transportProto);
                containerConfig.protocolConfigs.insert(protocolName, protocolConfig);
                break;
            }
            case Proto::ShadowSocks: {
                auto protocolConfig = QSharedPointer<ShadowsocksProtocolConfig>::create(protocolName, port);
                containerConfig.protocolConfigs.insert(protocolName, protocolConfig);
                break;
            }
            case Proto::Cloak: {
                auto protocolConfig = QSharedPointer<CloakProtocolConfig>::create(protocolName, port);
                containerConfig.protocolConfigs.insert(protocolName, protocolConfig);
                break;
            }
            case Proto::Xray: {
                auto protocolConfig = QSharedPointer<XrayProtocolConfig>::create(protocolName, port, transportProto);
                containerConfig.protocolConfigs.insert(protocolName, protocolConfig);
                break;
            }
            case Proto::TorWebSite: {
                auto protocolConfig = QSharedPointer<TorProtocolConfig>::create(protocolName);
                containerConfig.protocolConfigs.insert(protocolName, protocolConfig);
                break;
            }
            case Proto::Ikev2: {
                auto protocolConfig = QSharedPointer<IpsecProtocolConfig>::create(protocolName);
                containerConfig.protocolConfigs.insert(protocolName, protocolConfig);
                break;
            }
            case Proto::Sftp: {
                auto protocolConfig = QSharedPointer<SftpProtocolConfig>::create(protocolName, port);
                containerConfig.protocolConfigs.insert(protocolName, protocolConfig);
                break;
            }
            case Proto::Socks5Proxy: {
                auto protocolConfig = QSharedPointer<Socks5ProxyProtocolConfig>::create(protocolName, port);
                containerConfig.protocolConfigs.insert(protocolName, protocolConfig);
                break;
            }
            default: break;
            }
        }
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

    QMap<QString, ContainerConfig> installedContainers;
    ErrorCode errorCode = getAlreadyInstalledContainers(serverCredentials, serverController, installedContainers);
    if (errorCode) {
        emit installationErrorOccurred(errorCode);
        return;
    }

    QString finishMessage = "";

    bool isContainerInstalled = false;
    for (const auto &containerConfig : installedContainers) {
        if (containerConfig.containerType == container) {
            isContainerInstalled = true;
            break;
        }
    }

    if (!isContainerInstalled) {
        errorCode =
                serverController->setupContainer(installedContainers.value(ContainerProps::containerToString(container)), serverCredentials);
        if (errorCode) {
            emit installationErrorOccurred(errorCode);
            return;
        }

        // todo should process this function after install
        // VpnConfigurationsController::updateContainerConfigAfterInstallation(containerConfig.containerType, containerConfig, stdOut);

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

void InstallController::installServer(const DockerContainer containerType, QMap<QString, ContainerConfig> installedContainers,
                                      const ServerCredentials &serverCredentials, const QSharedPointer<ServerController> &serverController,
                                      QString &finishMessage)
{
    if (installedContainers.size() > 1) {
        finishMessage += tr("\nAdded containers that were already installed on the server");
    }

    auto serverConfig = QSharedPointer<SelfHostedServerConfig>::create();
    serverConfig->serverCredentials = m_processedServerCredentials;
    serverConfig->name = m_settings->nextAvailableServerName();

    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);
    for (auto &containerConfig : installedContainers) {
        if (ContainerProps::isSupportedByCurrentPlatform(containerConfig.containerType)) {
            auto errorCode = vpnConfigurationController.createClientProtocolConfigs(serverCredentials, containerConfig);
            if (errorCode) {
                emit installationErrorOccurred(errorCode);
                return;
            }

            // errorCode = m_clientManagementModel->appendClient(iterator.key(), serverCredentials, containerConfig,
            //                                                   QString("Admin [%1]").arg(QSysInfo::prettyProductName()), serverController);
            if (errorCode) {
                emit installationErrorOccurred(errorCode);
                return;
            }
        }
    }

    serverConfig->containerConfigs = installedContainers;
    serverConfig->defaultContainerType = containerType;
    serverConfig->defaultContainerName = ContainerProps::containerToString(containerType);

    m_serversModel->addServer(serverConfig);

    emit installServerFinished(finishMessage);
}

void InstallController::installContainer(const DockerContainer containerType, QMap<QString, ContainerConfig> installedContainers,
                                         const ServerCredentials &serverCredentials,
                                         const QSharedPointer<ServerController> &serverController, QString &finishMessage)
{
    bool isInstalledContainerAddedToGui = false;

    auto processedIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfig = m_serversModel->getServerConfig(processedIndex);
    auto selfHostedServerConfig = qSharedPointerCast<SelfHostedServerConfig>(serverConfig);

    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);
    for (auto &containerConfig : installedContainers) {
        if (selfHostedServerConfig->containerConfigs.contains(containerConfig.containerName)) {
            continue;
        }

        if (ContainerProps::isSupportedByCurrentPlatform(containerConfig.containerType)) {
            auto errorCode = vpnConfigurationController.createClientProtocolConfigs(serverCredentials, containerConfig);
            if (errorCode) {
                emit installationErrorOccurred(errorCode);
                return;
            }

            // errorCode = m_clientManagementModel->appendClient(iterator.key(), serverCredentials, containerConfig,
            //                                                   QString("Admin [%1]").arg(QSysInfo::prettyProductName()), serverController);
            if (errorCode) {
                emit installationErrorOccurred(errorCode);
                return;
            }
        }

        if (containerType != containerConfig.containerType) { // skip the newly installed container
            isInstalledContainerAddedToGui = true;
        }
    }
    selfHostedServerConfig->containerConfigs = installedContainers;
    m_serversModel->editServer(selfHostedServerConfig, processedIndex);

    if (isInstalledContainerAddedToGui) {
        finishMessage += tr("\nAlready installed containers were found on the server. "
                            "All installed containers have been added to the application");
    }

    emit installContainerFinished(finishMessage, ContainerProps::containerService(containerType) == ServiceType::Other);
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
    auto processedIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfig = m_serversModel->getServerConfig(processedIndex);
    auto selfHostedServerConfig = qSharedPointerCast<SelfHostedServerConfig>(serverConfig);
    auto serverCredentials = selfHostedServerConfig->serverCredentials;

    QMap<QString, ContainerConfig> installedContainers;
    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    ErrorCode errorCode = getAlreadyInstalledContainers(selfHostedServerConfig->serverCredentials, serverController, installedContainers);

    if (errorCode == ErrorCode::NoError) {
        bool isInstalledContainerAddedToGui = false;
        VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

        for (auto &containerConfig : installedContainers) {
            if (selfHostedServerConfig->containerConfigs.contains(containerConfig.containerName)) {
                continue;
            }

            if (ContainerProps::isSupportedByCurrentPlatform(containerConfig.containerType)) {
                auto errorCode = vpnConfigurationController.createClientProtocolConfigs(serverCredentials, containerConfig);
                if (errorCode) {
                    emit installationErrorOccurred(errorCode);
                    return;
                }

                // errorCode = m_clientManagementModel->appendClient(iterator.key(), serverCredentials, containerConfig,
                //                                                   QString("Admin [%1]").arg(QSysInfo::prettyProductName()), serverController);
                if (errorCode) {
                    emit installationErrorOccurred(errorCode);
                    return;
                }
            }

            selfHostedServerConfig->containerConfigs.insert(containerConfig.containerName, containerConfig);
            isInstalledContainerAddedToGui = true;
        }

        m_serversModel->editServer(selfHostedServerConfig, processedIndex);

        emit scanServerFinished(isInstalledContainerAddedToGui);
        return;
    }

    emit installationErrorOccurred(errorCode);
}

ErrorCode InstallController::getAlreadyInstalledContainers(const ServerCredentials &credentials,
                                                           const QSharedPointer<ServerController> &serverController,
                                                           QMap<QString, ContainerConfig> &containerConfigs)
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
            DockerContainer containerType = ContainerProps::containerFromString(containerName);

            ContainerConfig containerConfig;
            containerConfig.containerName = containerName;
            containerConfig.containerType = containerType;
            Proto mainProto = ContainerProps::defaultProtocol(containerType);
            const auto &protocols = ContainerProps::protocolsForContainer(containerType);

            for (const auto &protocol : protocols) {
                // for Multiprotocols (OpenVPN over SS, OpenVPN over Cloak)
                bool shouldProcessProtocol = false;
                if (containerType == DockerContainer::ShadowSocks || containerType == DockerContainer::Cloak) {
                    shouldProcessProtocol = true;
                } else {
                    shouldProcessProtocol = (protocol == mainProto);
                }

                if (shouldProcessProtocol) {
                    QString protocolName = ProtocolProps::protoToString(protocol);

                    if (protocol == Proto::Awg || protocol == Proto::WireGuard) {
                        QString serverConfig = serverController->getTextFileFromContainer(
                                containerType, credentials,
                                protocol == Proto::Awg ? protocols::awg::serverConfigPath : protocols::wireguard::serverConfigPath,
                                errorCode);

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

                        if (protocol == Proto::Awg) {
                            auto awgProtocolConfig = QSharedPointer<AwgProtocolConfig>::create(
                                    protocolName, port.toInt(), ProtocolProps::transportProtoFromString(transportProto));
                            awgProtocolConfig->serverProtocolConfig.subnetAddress = serverConfigMap.value("Address").remove("/24");
                            awgProtocolConfig->serverProtocolConfig.awgData.junkPacketCount =
                                    serverConfigMap.value(config_key::junkPacketCount);
                            awgProtocolConfig->serverProtocolConfig.awgData.junkPacketMinSize =
                                    serverConfigMap.value(config_key::junkPacketMinSize);
                            awgProtocolConfig->serverProtocolConfig.awgData.junkPacketMaxSize =
                                    serverConfigMap.value(config_key::junkPacketMaxSize);
                            awgProtocolConfig->serverProtocolConfig.awgData.initPacketJunkSize =
                                    serverConfigMap.value(config_key::initPacketJunkSize);
                            awgProtocolConfig->serverProtocolConfig.awgData.responsePacketJunkSize =
                                    serverConfigMap.value(config_key::responsePacketJunkSize);
                            awgProtocolConfig->serverProtocolConfig.awgData.initPacketMagicHeader =
                                    serverConfigMap.value(config_key::initPacketMagicHeader);
                            awgProtocolConfig->serverProtocolConfig.awgData.responsePacketMagicHeader =
                                    serverConfigMap.value(config_key::responsePacketMagicHeader);
                            awgProtocolConfig->serverProtocolConfig.awgData.underloadPacketMagicHeader =
                                    serverConfigMap.value(config_key::underloadPacketMagicHeader);
                            awgProtocolConfig->serverProtocolConfig.awgData.transportPacketMagicHeader =
                                    serverConfigMap.value(config_key::transportPacketMagicHeader);

                            containerConfig.protocolConfigs.insert(protocolName, awgProtocolConfig);

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
                            auto wireguardProtocolConfig = QSharedPointer<WireGuardProtocolConfig>::create(
                                    protocolName, port.toInt(), ProtocolProps::transportProtoFromString(transportProto));
                            wireguardProtocolConfig->serverProtocolConfig.subnetAddress = serverConfigMap.value("Address").remove("/24");

                            containerConfig.protocolConfigs.insert(protocolName, wireguardProtocolConfig);
                        }
                    } else if (protocol == Proto::Sftp) {
                        stdOut.clear();
                        script = QString("sudo docker inspect --format '{{.Config.Cmd}}' %1").arg(containerName);

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

                        auto sftpProtocolConfig = QSharedPointer<SftpProtocolConfig>::create(protocolName, port.toInt());
                        sftpProtocolConfig->serverProtocolConfig.userName = userName;
                        sftpProtocolConfig->serverProtocolConfig.password = password;

                        containerConfig.protocolConfigs.insert(protocolName, sftpProtocolConfig);
                    } else if (protocol == Proto::Socks5Proxy) {
                        QString proxyConfig = serverController->getTextFileFromContainer(containerType, credentials,
                                                                                         protocols::socks5Proxy::proxyConfigPath, errorCode);

                        const static QRegularExpression usernameAndPasswordRegExp("users (\\w+):CL:(\\w+)");
                        QRegularExpressionMatch usernameAndPasswordMatch = usernameAndPasswordRegExp.match(proxyConfig);

                        if (usernameAndPasswordMatch.hasMatch()) {
                            QString userName = usernameAndPasswordMatch.captured(1);
                            QString password = usernameAndPasswordMatch.captured(2);

                            auto socks5ProxyProtocolConfig = QSharedPointer<Socks5ProxyProtocolConfig>::create(protocolName, port.toInt());
                            socks5ProxyProtocolConfig->serverProtocolConfig.userName = userName;
                            socks5ProxyProtocolConfig->serverProtocolConfig.password = password;

                            containerConfig.protocolConfigs.insert(protocolName, socks5ProxyProtocolConfig);
                        }
                    } else if (protocol == Proto::Xray) {
                        QString currentConfig = serverController->getTextFileFromContainer(
                                containerType, credentials, amnezia::protocols::xray::serverConfigPath, errorCode);

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

                        auto xrayProtocolConfig = QSharedPointer<XrayProtocolConfig>::create(
                                protocolName, port.toInt(), ProtocolProps::transportProtoFromString(transportProto));
                        xrayProtocolConfig->serverProtocolConfig.site = siteName;

                        containerConfig.protocolConfigs.insert(protocolName, xrayProtocolConfig);
                    } else if (protocol == Proto::OpenVpn) {
                        QString serverConfig = serverController->getTextFileFromContainer(containerType, credentials,
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

                        auto openVpnProtocolConfig = QSharedPointer<OpenVpnProtocolConfig>::create(
                                protocolName, port.toInt(), ProtocolProps::transportProtoFromString(transportProto));

                        if (!serverValue.isEmpty()) {
                            QStringList serverParts = serverValue.split(" ");
                            if (serverParts.count() >= 1) {
                                openVpnProtocolConfig->serverProtocolConfig.subnetAddress = serverParts[0];
                            }
                        }

                        bool ncpDisable = serverConfig.contains("ncp-disable");
                        openVpnProtocolConfig->serverProtocolConfig.ncpDisable = ncpDisable;

                        bool tlsAuth = serverConfig.contains("tls-auth");
                        openVpnProtocolConfig->serverProtocolConfig.tlsAuth = tlsAuth;

                        bool blockOutsideDns = serverConfig.contains("block-outside-dns");
                        openVpnProtocolConfig->serverProtocolConfig.blockOutsideDns = blockOutsideDns;

                        QString cipher = serverConfigMap.value("cipher");
                        if (!cipher.isEmpty()) {
                            openVpnProtocolConfig->serverProtocolConfig.cipher = cipher;
                        }

                        QString hash = serverConfigMap.value("auth");
                        if (!hash.isEmpty()) {
                            openVpnProtocolConfig->serverProtocolConfig.hash = hash;
                        }

                        containerConfig.protocolConfigs.insert(protocolName, openVpnProtocolConfig);
                    } else if (protocol == Proto::Cloak) {
                        QString cloakConfig = serverController->getTextFileFromContainer(containerType, credentials,
                                                                                         "/opt/amnezia/cloak/ck-config.json", errorCode);

                        QJsonDocument doc = QJsonDocument::fromJson(cloakConfig.toUtf8());

                        auto cloakProtocolConfig = QSharedPointer<CloakProtocolConfig>::create(protocolName, port.toInt());

                        if (!doc.isNull() && doc.isObject()) {
                            QJsonObject cloakConfigObj = doc.object();

                            QString site = cloakConfigObj.value("RedirAddr").toString();
                            if (!site.isEmpty()) {
                                cloakProtocolConfig->serverProtocolConfig.site = site;
                            }
                        } else {
                            qDebug() << "Failed to parse main loop Cloak JSON config";
                        }

                        containerConfig.protocolConfigs.insert(protocolName, cloakProtocolConfig);

                    } else if (protocol == Proto::ShadowSocks) {
                        QString shadowsocksConfig = serverController->getTextFileFromContainer(
                                containerType, credentials, "/opt/amnezia/shadowsocks/ss-config.json", errorCode);

                        QJsonDocument doc = QJsonDocument::fromJson(shadowsocksConfig.toUtf8());

                        auto shadowsocksProtocolConfig = QSharedPointer<ShadowsocksProtocolConfig>::create(protocolName, port.toInt());

                        if (!doc.isNull() && doc.isObject()) {
                            QJsonObject ssConfigObj = doc.object();
                            QString cipher = ssConfigObj.value("method").toString();
                            if (!cipher.isEmpty()) {
                                shadowsocksProtocolConfig->serverProtocolConfig.cipher = cipher;
                            }
                        } else {
                            qDebug() << "Failed to parse main loop Shadowsocks JSON config";
                        }

                        containerConfig.protocolConfigs.insert(protocolName, shadowsocksProtocolConfig);
                    }
                }
            }
            containerConfigs.insert(containerName, containerConfig);
        }

        const static QRegularExpression torOrDnsRegExp("(amnezia-(?:torwebsite|dns)).*?([0-9]*)/(udp|tcp).*");
        QRegularExpressionMatch torOrDnsRegMatch = torOrDnsRegExp.match(containerInfo);
        if (torOrDnsRegMatch.hasMatch()) {
            QString containerName = torOrDnsRegMatch.captured(1);
            QString port = torOrDnsRegMatch.captured(2);
            QString transportProto = torOrDnsRegMatch.captured(3);
            DockerContainer containerType = ContainerProps::containerFromString(containerName);

            ContainerConfig containerConfig;
            containerConfig.containerName = containerName;
            containerConfig.containerType = containerType;
            Proto mainProto = ContainerProps::defaultProtocol(containerType);
            for (auto protocol : ContainerProps::protocolsForContainer(containerType)) {
                if (protocol == mainProto) {
                    QString protocolName = ProtocolProps::protoToString(protocol);

                    if (protocol == Proto::TorWebSite) {
                        stdOut.clear();
                        script = QString("sudo docker exec -i %1 sh -c 'cat /var/lib/tor/hidden_service/hostname'").arg(containerName);

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

                        auto torProtocolConfig = QSharedPointer<TorProtocolConfig>::create(protocolName);
                        torProtocolConfig->serverProtocolConfig.site = onion;

                        containerConfig.protocolConfigs.insert(protocolName, torProtocolConfig);
                    }
                }
            }
            containerConfigs.insert(containerName, containerConfig);
        }
    }

    return ErrorCode::NoError;
}

void InstallController::updateContainer()
{
    int serverIndex = m_serversModel->getProcessedServerIndex();
    auto oldServerConfig = qSharedPointerCast<SelfHostedServerConfig>(m_serversModel->getServerConfig(serverIndex));
    auto newServerConfig = QSharedPointer<SelfHostedServerConfig>::create(*oldServerConfig);

    const DockerContainer container = static_cast<DockerContainer>(m_containersModel->getProcessedContainerIndex());
    const QString containerName = m_containersModel->getProcessedContainerName();
    auto protocolConfigs = m_protocolModel->getProtocolConfigs();
    newServerConfig->updateProtocolConfig(containerName, protocolConfigs);

    auto oldContainerConfig = oldServerConfig->containerConfigs[containerName];
    auto newContainerConfig = newServerConfig->containerConfigs[containerName];

    auto oldProtocolConfigs = oldContainerConfig.protocolConfigs;
    auto newProtocolConfigs = newContainerConfig.protocolConfigs;

    ErrorCode errorCode = ErrorCode::NoError;

    if (isUpdateDockerContainerRequired(container, oldProtocolConfigs, newProtocolConfigs)) {
        QSharedPointer<ServerController> serverController(new ServerController(m_settings));
        connect(serverController.get(), &ServerController::serverIsBusy, this, &InstallController::serverIsBusy);
        connect(this, &InstallController::cancelInstallation, serverController.get(), &ServerController::cancelInstallation);

        errorCode = serverController->updateContainer(newServerConfig->serverCredentials, oldContainerConfig, newContainerConfig);
        clearCachedProfile(serverController);
    }

    if (errorCode == ErrorCode::NoError) {
        m_serversModel->editServer(newServerConfig, serverIndex);
        m_protocolModel->updateModel(protocolConfigs);

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

    m_serversModel->removeProcessedServer();
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

    // QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    // ServerCredentials serverCredentials =
    //         qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));

    // m_serversModel->clearCachedProfile(container);
    // m_clientManagementModel->revokeClient(containerConfig, container, serverCredentials, serverIndex, serverController);

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
    auto serverConfig = QSharedPointer<SelfHostedServerConfig>::create();
    serverConfig->serverCredentials = m_processedServerCredentials;
    serverConfig->name = m_settings->nextAvailableServerName();
    serverConfig->defaultContainerName = ContainerProps::containerToString(DockerContainer::None);
    serverConfig->defaultContainerType = DockerContainer::None;

    m_serversModel->addServer(serverConfig);

    emit installServerFinished(tr("Server added successfully"));
}

bool InstallController::isConfigValid()
{
    int serverIndex = m_serversModel->getDefaultServerIndex();
    auto serverConfig = m_serversModel->getServerConfig(serverIndex);
    auto selfHostedServerConfig = qSharedPointerCast<SelfHostedServerConfig>(serverConfig);
    auto serverCredentials = selfHostedServerConfig->serverCredentials;

    if (serverConfig->type != amnezia::ServerConfigType::SelfHosted) {
        return true;
    }

    if (selfHostedServerConfig->containerConfigs.isEmpty()) {
        emit noInstalledContainers();
        return false;
    }

    auto defaultContainerType = selfHostedServerConfig->defaultContainerType;
    auto defaultContainerName = selfHostedServerConfig->defaultContainerName;

    if (defaultContainerType == DockerContainer::None) {
        emit installationErrorOccurred(ErrorCode::NoInstalledContainersError);
        return false;
    }

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

    auto containerConfig = selfHostedServerConfig->containerConfigs.value(defaultContainerName);

    QFutureWatcher<ErrorCode> watcher;

    QFuture<ErrorCode> future = QtConcurrent::run([this, defaultContainerType, &serverCredentials, &containerConfig, &serverController]() {
        ErrorCode errorCode = ErrorCode::NoError;

        if (containerConfig.isClientProtocolConfigEmpty()) {
            VpnConfigurationsController vpnConfigurationController(m_settings, serverController);
            errorCode = vpnConfigurationController.createClientProtocolConfigs(serverCredentials, containerConfig);
            if (errorCode != ErrorCode::NoError) {
                return errorCode;
            }

            // errorCode = m_clientManagementModel->appendClient(container, credentials, containerConfig,
            //                                                   QString("Admin [%1]").arg(QSysInfo::prettyProductName()), serverController);
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

    selfHostedServerConfig->containerConfigs.insert(defaultContainerName, containerConfig);
    m_serversModel->editServer(selfHostedServerConfig, serverIndex);

    return true;
}

bool InstallController::isUpdateDockerContainerRequired(const DockerContainer container,
                                                        const QMap<QString, QSharedPointer<ProtocolConfig>> &oldProtocolConfigs,
                                                        const QMap<QString, QSharedPointer<ProtocolConfig>> &newProtocolConfigs)
{
    Proto mainProto = ContainerProps::defaultProtocol(container);

    const auto oldProtoConfig = oldProtocolConfigs.value(ProtocolProps::protoToString(mainProto));
    const auto newProtoConfig = newProtocolConfigs.value(ProtocolProps::protoToString(mainProto));

    switch (mainProto) {
    case Proto::Awg: {
        auto newConfig = qSharedPointerCast<AwgProtocolConfig>(oldProtoConfig);
        auto oldConfig = qSharedPointerCast<AwgProtocolConfig>(newProtoConfig);
        return !newConfig->hasEqualServerSettings(*oldConfig.data());
    }
    case Proto::WireGuard: {
        auto newConfig = qSharedPointerCast<WireGuardProtocolConfig>(oldProtoConfig);
        auto oldConfig = qSharedPointerCast<WireGuardProtocolConfig>(newProtoConfig);
        return !newConfig->hasEqualServerSettings(*oldConfig.data());
    }
    default: return true;
    }
}
