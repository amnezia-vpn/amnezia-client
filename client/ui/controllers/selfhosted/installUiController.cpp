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
#include "protocols/protocols_defs.h"
#include "ui/models/protocols/awgConfigModel.h"
#include "ui/models/protocols/wireguardConfigModel.h"
#include "core/utils/utilities.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"

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
                                         QObject *parent)
    : QObject(parent),
      m_installController(installController),
      m_serversController(serversController),
      m_settingsController(settingsController),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_protocolModel(protocolsModel),
      m_usersController(usersController)
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
            finishMessage = tr("%1 installed successfully. ").arg(ContainerProps::containerHumanNames().value(container));
        } else {
            finishMessage = tr("%1 is already installed on the server. ").arg(ContainerProps::containerHumanNames().value(container));
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
        errorCode = m_installController->installContainer(serverCredentials, container, port, transportProto, serverIndex,
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
            finishMessage = tr("%1 installed successfully. ").arg(ContainerProps::containerHumanNames().value(container));
        } else {
            finishMessage = tr("%1 is already installed on the server. ").arg(ContainerProps::containerHumanNames().value(container));
        }

        if (hasNewContainers) {
            finishMessage += tr("\nAlready installed containers were found on the server. "
                                "All installed containers have been added to the application");
        }

        emit installContainerFinished(finishMessage, ContainerProps::containerService(container) == ServiceType::Other);
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

void InstallUiController::updateContainer(int serverIndex, QJsonObject config)
{
    const DockerContainer container = ContainerProps::containerFromString(config.value(config_key::container).toString());
    QJsonObject oldContainerConfigJson = m_containersModel->getContainerConfig(container);
    ContainerConfig oldContainerConfig = ContainerConfig::fromJson(oldContainerConfigJson);
    ContainerConfig newContainerConfig = ContainerConfig::fromJson(config);

    ErrorCode errorCode = m_installController->updateContainer(serverIndex, container, oldContainerConfig, newContainerConfig);

    if (errorCode == ErrorCode::NoError) {
        m_protocolModel->updateModel(config);

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
    if (ContainerProps::containerService(container) == ServiceType::Other) {
        return;
    }

    m_installController->clearCachedProfile(serverIndex, container);

    emit cachedProfileCleared(tr("%1 cached profile cleared").arg(ContainerProps::containerHumanNames().value(container)));
    QJsonObject updatedConfig = m_serversController->getContainerConfig(serverIndex, container);
    emit profileCleared(updatedConfig);
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
