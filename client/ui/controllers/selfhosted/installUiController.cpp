#include "installUiController.h"

#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QtConcurrent>

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
#include "core/utils/utilities.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"
#include "core/models/protocols/openVpnProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "ui/models/containersModel.h"

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
                                         ServersModel *serversModel, ContainersModel *containersModel,
                                         ProtocolsModel *protocolsModel, UsersController *usersController,
                                         AwgConfigModel *awgConfigModel,
                                         WireGuardConfigModel *wireGuardConfigModel,
                                         OpenVpnConfigModel *openVpnConfigModel,
                                         XrayConfigModel *xrayConfigModel,
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
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_protocolModel(protocolsModel),
      m_usersController(usersController),
      m_awgConfigModel(awgConfigModel),
      m_wireGuardConfigModel(wireGuardConfigModel),
      m_openVpnConfigModel(openVpnConfigModel),
      m_xrayConfigModel(xrayConfigModel),
#ifdef Q_OS_WINDOWS
      m_ikev2ConfigModel(ikev2ConfigModel),
#endif
      m_sftpConfigModel(sftpConfigModel),
      m_socks5ConfigModel(socks5ConfigModel)
{
}

InstallUiController::~InstallUiController()
{
}

void InstallUiController::install(DockerContainer container, int port, TransportProto transportProto, int serverIndex)
{
    ServerCredentials serverCredentials;
    if (serverIndex < 0) {
        serverCredentials = m_processedServerCredentials;
    } else {
        serverCredentials = m_serversController->getServerCredentials(serverIndex);
    }

    QMap<DockerContainer, QJsonObject> preparedContainers;
    QString finishMessage;
    ErrorCode errorCode;

    if (serverIndex < 0) {
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
        QMap<DockerContainer, ContainerConfig> containers = ServerConfigUtils::containers(serverConfig);
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
        QMap<DockerContainer, ContainerConfig> containers = ServerConfigUtils::containers(serverConfig);
        int containersCount = containers.size();

        bool wasContainerInstalled = false;
        errorCode = m_installController->installContainer(serverIndex, container, port, transportProto,
                                                          wasContainerInstalled);
        if (errorCode) {
            emit installationErrorOccurred(errorCode);
            return;
        }

        ServerConfig newServerConfig = m_serversController->getServerConfig(serverIndex);
        QMap<DockerContainer, ContainerConfig> newContainers = ServerConfigUtils::containers(newServerConfig);
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
    QMap<DockerContainer, ContainerConfig> containersBefore = ServerConfigUtils::containers(serverBefore);
    int containersCountBefore = containersBefore.size();

    ErrorCode errorCode = m_installController->scanServerForInstalledContainers(serverIndex);

    if (errorCode == ErrorCode::NoError) {
        ServerConfig serverAfter = m_serversController->getServerConfig(serverIndex);
        QMap<DockerContainer, ContainerConfig> containersAfter = ServerConfigUtils::containers(serverAfter);
        int containersCountAfter = containersAfter.size();

        bool isInstalledContainerAdded = containersCountAfter > containersCountBefore;
        emit scanServerFinished(isInstalledContainerAdded);
        return;
    }

    emit installationErrorOccurred(errorCode);
}

void InstallUiController::updateContainer(int serverIndex, int protocolIndex)
{
    int containerIndex = m_containersModel->getProcessedContainerIndex();
    DockerContainer container = qvariant_cast<DockerContainer>(
        m_containersModel->data(containerIndex, ContainersModel::DockerContainerRole));
    
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
    case Proto::Xray: {
        containerConfig.protocolConfig = m_xrayConfigModel->getProtocolConfig();
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
    default:
        return;
    }
    ContainerConfig oldContainerConfig = m_serversController->getContainerConfig(serverIndex, container);

    ErrorCode errorCode = m_installController->updateContainer(serverIndex, container, oldContainerConfig, containerConfig);

    if (errorCode == ErrorCode::NoError) {
        ContainerConfig updatedConfig = m_serversController->getContainerConfig(serverIndex, container);
        m_protocolModel->updateModel(updatedConfig);

        auto defaultContainer = qvariant_cast<DockerContainer>(m_serversModel->data(serverIndex, ServersModel::Roles::DefaultContainerRole));
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
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();

    const auto errorCode = m_installController->rebootServer(serverIndex);
    if (errorCode == ErrorCode::NoError) {
        emit rebootServerFinished(tr("Server '%1' was rebooted").arg(serverName));
    } else {
        emit installationErrorOccurred(errorCode);
    }
}

void InstallUiController::removeServer(int serverIndex)
{
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();

    m_serversController->removeServer(serverIndex);
    emit removeServerFinished(tr("Server '%1' was removed").arg(serverName));
}

void InstallUiController::removeAllContainers(int serverIndex)
{
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();

    ErrorCode errorCode = m_installController->removeAllContainers(serverIndex);
    if (errorCode == ErrorCode::NoError) {
        emit removeAllContainersFinished(tr("All containers from server '%1' have been removed").arg(serverName));
        return;
    }
    emit installationErrorOccurred(errorCode);
}

void InstallUiController::removeContainer(int serverIndex)
{
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();

    int container = m_containersModel->getProcessedContainerIndex();
    QString containerName = m_containersModel->getProcessedContainerName();

    ErrorCode errorCode = m_installController->removeContainer(serverIndex, static_cast<DockerContainer>(container));
    if (errorCode == ErrorCode::NoError) {

        emit removeContainerFinished(tr("%1 has been removed from the server '%2'").arg(containerName, serverName));
        return;
    }
    emit installationErrorOccurred(errorCode);
}

void InstallUiController::clearCachedProfile(int serverIndex)
{
    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getProcessedContainerIndex());
    if (ContainerUtils::containerService(container) == ServiceType::Other) {
        return;
    }

    m_installController->clearCachedProfile(serverIndex, container);

    emit cachedProfileCleared(tr("%1 cached profile cleared").arg(ContainerUtils::containerHumanNames().value(container)));
    ContainerConfig updatedConfig = m_serversController->getContainerConfig(serverIndex, container);
    m_protocolModel->updateModel(updatedConfig);
}

QRegularExpression InstallUiController::ipAddressPortRegExp()
{
    return NetworkUtilities::ipAddressPortRegExp();
}

QRegularExpression InstallUiController::ipAddressRegExp()
{
    return NetworkUtilities::ipAddressRegExp();
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

bool InstallUiController::checkSshConnection(SshSession* sshSession)
{
    Q_UNUSED(sshSession);

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

bool InstallUiController::isConfigValid()
{
    int serverIndex = m_serversController->getDefaultServerIndex();
    ServerConfig serverConfig = m_serversController->getServerConfig(serverIndex);
    QJsonObject serverConfigObject = ServerConfigUtils::toJson(serverConfig);

    if (apiUtils::isServerFromApi(serverConfigObject)) {
        return true;
    }

    if (!m_serversModel->data(serverIndex, ServersModel::Roles::HasInstalledContainers).toBool()) {
        emit noInstalledContainers();
        return false;
    }

    QFutureWatcher<ErrorCode> watcher;
    QFuture<ErrorCode> future = QtConcurrent::run([this, serverIndex]() {
        return m_installController->validateAndPrepareConfig(serverIndex);
    });

    QEventLoop wait;
    connect(&watcher, &QFutureWatcher<ErrorCode>::finished, &wait, &QEventLoop::quit);
    watcher.setFuture(future);
    wait.exec();

    ErrorCode errorCode = watcher.result();

    if (errorCode == ErrorCode::NoInstalledContainersError) {
        emit installationErrorOccurred(errorCode);
        return false;
    }

    if (errorCode != ErrorCode::NoError) {
        emit installationErrorOccurred(errorCode);
        return false;
    }

    return true;
}

void InstallUiController::updateProtocols(const QJsonObject &config)
{
    ContainerConfig containerConfig = ContainerConfig::fromJson(config);
    m_protocolModel->updateModel(containerConfig);
}

void InstallUiController::openServerSettings(int serverIndex, int protocolIndex)
{
    updateProtocolConfigModel(serverIndex, protocolIndex);
}

void InstallUiController::openClientSettings(int serverIndex, int protocolIndex)
{
    updateProtocolConfigModel(serverIndex, protocolIndex);
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

void InstallUiController::updateProtocolConfigModel(int serverIndex, int protocolIndex)
{
    int containerIndex = m_containersModel->getProcessedContainerIndex();
    
    DockerContainer container = qvariant_cast<DockerContainer>(
        m_containersModel->data(containerIndex, ContainersModel::DockerContainerRole));
    
    ContainerConfig containerConfig = m_serversController->getContainerConfig(serverIndex, container);
    
    Proto protocolType = static_cast<Proto>(protocolIndex);
    
    switch (protocolType) {
    case Proto::Awg: {
        if (auto* awgProtocolConfig = containerConfig.getAwgProtocolConfig()) {
            m_awgConfigModel->updateModel(container, *awgProtocolConfig);
        }
        break;
    }
    case Proto::WireGuard: {
        if (auto* wireGuardProtocolConfig = containerConfig.getWireGuardProtocolConfig()) {
            m_wireGuardConfigModel->updateModel(container, *wireGuardProtocolConfig);
        }
        break;
    }
    case Proto::OpenVpn: {
        if (auto* openVpnProtocolConfig = containerConfig.getOpenVpnProtocolConfig()) {
            m_openVpnConfigModel->updateModel(container, *openVpnProtocolConfig);
        }
        break;
    }
    case Proto::Xray: {
        if (auto* xrayProtocolConfig = containerConfig.getXrayProtocolConfig()) {
            m_xrayConfigModel->updateModel(container, *xrayProtocolConfig);
        }
        break;
    }
    case Proto::Sftp: {
        if (auto* sftpProtocolConfig = containerConfig.getSftpProtocolConfig()) {
            m_sftpConfigModel->updateModel(container, *sftpProtocolConfig);
        }
        break;
    }
    case Proto::Socks5Proxy: {
        if (auto* socks5ProxyProtocolConfig = containerConfig.getSocks5ProxyProtocolConfig()) {
            m_socks5ConfigModel->updateModel(container, *socks5ProxyProtocolConfig);
        }
        break;
    }
#ifdef Q_OS_WINDOWS
    case Proto::Ikev2: {
        if (m_ikev2ConfigModel) {
            m_ikev2ConfigModel->updateModel(containerConfig.toJson());
        }
        break;
    }
#endif
    default:
        break;
    }
}

