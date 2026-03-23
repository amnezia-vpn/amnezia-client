#include "installUiController.h"

#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QStandardPaths>

#include "core/utils/api/apiUtils.h"
#include "core/controllers/selfhosted/installController.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/networkUtilities.h"
#include "logger.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "ui/models/protocols/awgConfigModel.h"
#include "ui/models/protocols/wireguardConfigModel.h"
#include "ui/models/protocols/openvpnConfigModel.h"
#include "ui/models/protocols/xrayConfigModel.h"
#ifdef Q_OS_WINDOWS
#include "ui/models/protocols/ikev2ConfigModel.h"
#endif
#include "ui/models/services/sftpConfigModel.h"
#include "ui/models/services/socks5ProxyConfigModel.h"
#include "ui/models/services/torConfigModel.h"
#include "core/utils/utilities.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"
#include "core/models/protocols/openVpnProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"

namespace
{
    Logger logger("InstallUiController");

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

InstallUiController::InstallUiController(InstallController *installController,
                                         ServersController *serversController,
                                         SettingsController *settingsController,
                                         ProtocolsModel *protocolsModel,
                                         UsersController *usersController,
                                         AwgConfigModel *awgConfigModel,
                                         WireGuardConfigModel *wireGuardConfigModel,
                                         OpenVpnConfigModel *openVpnConfigModel,
                                         XrayConfigModel *xrayConfigModel,
                                         TorConfigModel *torConfigModel,
#ifdef Q_OS_WINDOWS
                                         Ikev2ConfigModel *ikev2ConfigModel,
#endif
                                         SftpConfigModel *sftpConfigModel,
                                         Socks5ProxyConfigModel *socks5ConfigModel,
                                         QObject *parent)
    : QObject(parent),
      m_installController(installController),
      m_serversController(serversController),
      m_settingsController(settingsController),
      m_protocolModel(protocolsModel),
      m_usersController(usersController),
      m_awgConfigModel(awgConfigModel),
      m_wireGuardConfigModel(wireGuardConfigModel),
      m_openVpnConfigModel(openVpnConfigModel),
      m_xrayConfigModel(xrayConfigModel),
      m_torConfigModel(torConfigModel),
#ifdef Q_OS_WINDOWS
      m_ikev2ConfigModel(ikev2ConfigModel),
#endif
      m_sftpConfigModel(sftpConfigModel),
      m_socks5ConfigModel(socks5ConfigModel)
{
    connect(m_installController, &InstallController::configValidated, this, &InstallUiController::configValidated);
    connect(m_installController, &InstallController::validationErrorOccurred, this, [this](ErrorCode errorCode) {
        if (errorCode == ErrorCode::NoInstalledContainersError) {
            emit noInstalledContainers();
        } else {
            emit installationErrorOccurred(errorCode);
        }
    });
}

InstallUiController::~InstallUiController()
{
}

void InstallUiController::install(DockerContainer container, int port, TransportProto transportProto, int serverIndex)
{
    const bool isNewServer = serverIndex < 0;
    
    ServerCredentials serverCredentials;
    if (isNewServer) {
        serverCredentials = m_processedServerCredentials;
    } else {
        serverCredentials = m_serversController->getServerCredentials(serverIndex);
        m_processedServerCredentials = ServerCredentials();
    }

    QMap<DockerContainer, QJsonObject> preparedContainers;
    QString finishMessage;
    ErrorCode errorCode;

    if (isNewServer) {
        int existingServerIndex = -1;
        if (m_installController->isServerAlreadyExists(serverCredentials, existingServerIndex)) {
            emit serverAlreadyExists(existingServerIndex);
            return;
        }

        bool wasContainerInstalled = false;
        errorCode = m_installController->installServer(serverCredentials, container, port, transportProto, wasContainerInstalled);
        if (errorCode) {
            emit installationErrorOccurred(errorCode);
            return;
        }

        int serverIndex = m_serversController->getServersCount() - 1;
        ServerConfig serverConfig = m_serversController->getServerConfig(serverIndex);
        QMap<DockerContainer, ContainerConfig> containers = serverConfig.containers();
        int containersCount = containers.size();

        if (wasContainerInstalled) {
            finishMessage = tr("%1 installed successfully. ").arg(ContainerUtils::containerHumanNames().value(container));
        } else {
            finishMessage = tr("%1 is already installed on the server. ").arg(ContainerUtils::containerHumanNames().value(container));
        }

        if (containersCount > 1) {
            finishMessage += tr("\nAdded containers that were already installed on the server");
        }

        emit installServerFinished(finishMessage);
    } else {
        ServerConfig serverConfig = m_serversController->getServerConfig(serverIndex);
        QMap<DockerContainer, ContainerConfig> containers = serverConfig.containers();
        int containersCount = containers.size();

        bool wasContainerInstalled = false;
        errorCode = m_installController->installContainer(serverIndex, container, port, transportProto,
                                                          wasContainerInstalled);
        if (errorCode) {
            emit installationErrorOccurred(errorCode);
            return;
        }

        ServerConfig newServerConfig = m_serversController->getServerConfig(serverIndex);
        QMap<DockerContainer, ContainerConfig> newContainers = newServerConfig.containers();
        int newContainersCount = newContainers.size();

        bool hasNewContainers = (newContainersCount - containersCount) > (wasContainerInstalled ? 1 : 0);

        if (wasContainerInstalled) {
            finishMessage = tr("%1 installed successfully. ").arg(ContainerUtils::containerHumanNames().value(container));
        } else {
            finishMessage = tr("%1 is already installed on the server. ").arg(ContainerUtils::containerHumanNames().value(container));
        }

        if (hasNewContainers) {
            finishMessage += tr("\nAlready installed containers were found on the server. "
                                "All installed containers have been added to the application");
        }

        emit installContainerFinished(finishMessage, ContainerUtils::containerService(container) == ServiceType::Other);
    }
}

void InstallUiController::scanServerForInstalledContainers(int serverIndex)
{
    ServerConfig serverBefore = m_serversController->getServerConfig(serverIndex);
    QMap<DockerContainer, ContainerConfig> containersBefore = serverBefore.containers();
    int containersCountBefore = containersBefore.size();

    ErrorCode errorCode = m_installController->scanServerForInstalledContainers(serverIndex);

    if (errorCode == ErrorCode::NoError) {
        ServerConfig serverAfter = m_serversController->getServerConfig(serverIndex);
        QMap<DockerContainer, ContainerConfig> containersAfter = serverAfter.containers();
        int containersCountAfter = containersAfter.size();

        bool isInstalledContainerAdded = containersCountAfter > containersCountBefore;
        emit scanServerFinished(isInstalledContainerAdded);
        return;
    }

    emit installationErrorOccurred(errorCode);
}

void InstallUiController::updateContainer(int serverIndex, int containerIndex, int protocolIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    
    Proto protocolType = static_cast<Proto>(protocolIndex);
    
    ContainerConfig containerConfig;
    containerConfig.container = container;
    
    switch (protocolType) {
    case Proto::Awg: {
        containerConfig.protocolConfig = m_awgConfigModel->getProtocolConfig();
        break;
    }
    case Proto::WireGuard: {
        containerConfig.protocolConfig = m_wireGuardConfigModel->getProtocolConfig();
        break;
    }
    case Proto::OpenVpn: {
        containerConfig.protocolConfig = m_openVpnConfigModel->getProtocolConfig();
        break;
    }
    case Proto::Xray:
    case Proto::SSXray: {
        containerConfig.protocolConfig = m_xrayConfigModel->getProtocolConfig();
        break;
    }
    case Proto::TorWebSite: {
        containerConfig.protocolConfig = m_torConfigModel->getProtocolConfig();
        break;
    }
    case Proto::Sftp: {
        containerConfig.protocolConfig = m_sftpConfigModel->getProtocolConfig();
        break;
    }
    case Proto::Socks5Proxy: {
        containerConfig.protocolConfig = m_socks5ConfigModel->getProtocolConfig();
        break;
    }
#ifdef Q_OS_WINDOWS
    case Proto::Ikev2: {
        containerConfig.protocolConfig = m_ikev2ConfigModel->getProtocolConfig();
        break;
    }
#endif
    default:
        return;
    }
    ContainerConfig oldContainerConfig = m_serversController->getContainerConfig(serverIndex, container);

    ErrorCode errorCode = m_installController->updateContainer(serverIndex, container, oldContainerConfig, containerConfig);

    if (errorCode == ErrorCode::NoError) {
        ContainerConfig updatedConfig = m_serversController->getContainerConfig(serverIndex, container);
        m_protocolModel->updateModel(updatedConfig);

        auto defaultContainer = m_serversController->getServerConfig(serverIndex).defaultContainer();
        if ((serverIndex == m_serversController->getDefaultServerIndex()) && (container == defaultContainer)) {
            emit currentContainerUpdated();
        } else {
            emit updateContainerFinished(tr("Settings updated successfully"));
        }

        return;
    }

    emit installationErrorOccurred(errorCode);
}

void InstallUiController::rebootServer(int serverIndex)
{
    QString serverName = m_serversController->getServerConfig(serverIndex).displayName();

    const auto errorCode = m_installController->rebootServer(serverIndex);
    if (errorCode == ErrorCode::NoError) {
        emit rebootServerFinished(tr("Server '%1' was rebooted").arg(serverName));
    } else {
        emit installationErrorOccurred(errorCode);
    }
}

void InstallUiController::removeServer(int serverIndex)
{
    QString serverName = m_serversController->getServerConfig(serverIndex).displayName();

    m_serversController->removeServer(serverIndex);
    emit removeServerFinished(tr("Server '%1' was removed").arg(serverName));
}

void InstallUiController::removeAllContainers(int serverIndex)
{
    QString serverName = m_serversController->getServerConfig(serverIndex).displayName();

    ErrorCode errorCode = m_installController->removeAllContainers(serverIndex);
    if (errorCode == ErrorCode::NoError) {
        emit removeAllContainersFinished(tr("All containers from server '%1' have been removed").arg(serverName));
        return;
    }
    emit installationErrorOccurred(errorCode);
}

void InstallUiController::removeContainer(int serverIndex, int containerIndex)
{
    QString serverName = m_serversController->getServerConfig(serverIndex).displayName();

    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    QString containerName = ContainerUtils::containerHumanNames().value(container);

    ErrorCode errorCode = m_installController->removeContainer(serverIndex, container);
    if (errorCode == ErrorCode::NoError) {

        emit removeContainerFinished(tr("%1 has been removed from the server '%2'").arg(containerName, serverName));
        return;
    }
    emit installationErrorOccurred(errorCode);
}

void InstallUiController::clearCachedProfile(int serverIndex, int containerIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    if (ContainerUtils::containerService(container) == ServiceType::Other) {
        return;
    }

    m_installController->clearCachedProfile(serverIndex, container);

    emit cachedProfileCleared(tr("%1 cached profile cleared").arg(ContainerUtils::containerHumanNames().value(container)));
    ContainerConfig updatedConfig = m_serversController->getContainerConfig(serverIndex, container);
    m_protocolModel->updateModel(updatedConfig);
}

QRegularExpression InstallUiController::ipAddressRegExp()
{
    return NetworkUtilities::ipAddressRegExp();
}

void InstallUiController::clearProcessedServerCredentials()
{
    m_processedServerCredentials = ServerCredentials();
}

void InstallUiController::setProcessedServerCredentials(const QString &hostName, const QString &userName, const QString &secretData)
{
    m_processedServerCredentials.hostName = hostName;
    if (m_processedServerCredentials.hostName.contains(":")) {
        m_processedServerCredentials.port = m_processedServerCredentials.hostName.split(":").at(1).toInt();
        m_processedServerCredentials.hostName = m_processedServerCredentials.hostName.split(":").at(0);
    }
    m_processedServerCredentials.userName = userName;
    m_processedServerCredentials.secretData = secretData;
}

void InstallUiController::mountSftpDrive(int serverIndex, const QString &port, const QString &password, const QString &username)
{
    ServerCredentials serverCredentials = m_serversController->getServerCredentials(serverIndex);
    ErrorCode errorCode = m_installController->mountSftpDrive(serverCredentials, port, password, username);
    if (errorCode != ErrorCode::NoError) {
        emit installationErrorOccurred(errorCode);
    }
}

bool InstallUiController::checkSshConnection()
{
    m_privateKeyPassphrase = "";

    auto passphraseCallback = [this]() {
        emit passphraseRequestStarted();
        QEventLoop loop;
        QObject::connect(this, &InstallUiController::passphraseRequestFinished, &loop, &QEventLoop::quit);
        loop.exec();
        return m_privateKeyPassphrase;
    };

    QString output;
    ErrorCode errorCode = m_installController->checkSshConnection(m_processedServerCredentials, output, passphraseCallback);

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

void InstallUiController::setEncryptedPassphrase(QString passphrase)
{
    m_privateKeyPassphrase = passphrase;
    emit passphraseRequestFinished();
}

void InstallUiController::addEmptyServer()
{
    SelfHostedServerConfig serverConfig;
    serverConfig.hostName = m_processedServerCredentials.hostName;
    serverConfig.userName = m_processedServerCredentials.userName;
    serverConfig.password = m_processedServerCredentials.secretData;
    serverConfig.port = m_processedServerCredentials.port;
    serverConfig.description = m_settingsController->nextAvailableServerName();
    serverConfig.defaultContainer = DockerContainer::None;

    m_serversController->addServer(ServerConfig(serverConfig));
    emit installServerFinished(tr("Server added successfully"));
}

void InstallUiController::validateConfig()
{
    int serverIndex = m_serversController->getDefaultServerIndex();
    m_installController->validateConfig(serverIndex);
}

void InstallUiController::updateProtocols(int serverIndex, int containerIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    ContainerConfig containerConfig = m_serversController->getContainerConfig(serverIndex, container);
    containerConfig.container = container;
    m_protocolModel->updateModel(containerConfig);
}

void InstallUiController::openServerSettings(int serverIndex, int containerIndex, int protocolIndex)
{
    updateProtocolConfigModel(serverIndex, containerIndex, protocolIndex);
}

void InstallUiController::openClientSettings(int serverIndex, int containerIndex, int protocolIndex)
{
    updateProtocolConfigModel(serverIndex, containerIndex, protocolIndex);
}

int InstallUiController::defaultPort(int protocolIndex)
{
    Proto proto = static_cast<Proto>(protocolIndex);
    return ProtocolUtils::defaultPort(proto);
}

int InstallUiController::getPortForInstall(int protocolIndex)
{
    Proto proto = static_cast<Proto>(protocolIndex);
    return ProtocolUtils::getPortForInstall(proto);
}

int InstallUiController::defaultTransportProto(int protocolIndex)
{
    Proto proto = static_cast<Proto>(protocolIndex);
    return static_cast<int>(ProtocolUtils::defaultTransportProto(proto));
}

bool InstallUiController::defaultPortChangeable(int protocolIndex)
{
    Proto proto = static_cast<Proto>(protocolIndex);
    return ProtocolUtils::defaultPortChangeable(proto);
}

bool InstallUiController::defaultTransportProtoChangeable(int protocolIndex)
{
    Proto proto = static_cast<Proto>(protocolIndex);
    return ProtocolUtils::defaultTransportProtoChangeable(proto);
}

void InstallUiController::updateProtocolConfigModel(int serverIndex, int containerIndex, int protocolIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    ContainerConfig containerConfig = m_serversController->getContainerConfig(serverIndex, container);
    containerConfig.container = container;
    Proto protocolType = static_cast<Proto>(protocolIndex);

    auto updateIfPresent = [&](auto* model, auto* config) {
        if (model && config) model->updateModel(container, *config);
    };

    switch (protocolType) {
    case Proto::Awg: updateIfPresent(m_awgConfigModel, containerConfig.getAwgProtocolConfig()); break;
    case Proto::WireGuard: updateIfPresent(m_wireGuardConfigModel, containerConfig.getWireGuardProtocolConfig()); break;
    case Proto::OpenVpn: updateIfPresent(m_openVpnConfigModel, containerConfig.getOpenVpnProtocolConfig()); break;
    case Proto::Xray: updateIfPresent(m_xrayConfigModel, containerConfig.getXrayProtocolConfig()); break;
    case Proto::TorWebSite: updateIfPresent(m_torConfigModel, containerConfig.getTorProtocolConfig()); break;
    case Proto::Sftp: updateIfPresent(m_sftpConfigModel, containerConfig.getSftpProtocolConfig()); break;
    case Proto::Socks5Proxy: updateIfPresent(m_socks5ConfigModel, containerConfig.getSocks5ProxyProtocolConfig()); break;
#ifdef Q_OS_WINDOWS
    case Proto::Ikev2: updateIfPresent(m_ikev2ConfigModel, containerConfig.getIkev2ProtocolConfig()); break;
#endif
    default: break;
    }
}

