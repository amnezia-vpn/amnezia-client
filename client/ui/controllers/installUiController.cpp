#include "installUiController.h"

#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QtConcurrent>

#include "core/utils/api/apiUtils.h"
#include "core/controllers/installController.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/networkUtilities.h"
#include "logger.h"
#include "protocols/protocols_defs.h"
#include "ui/models/protocols/awgConfigModel.h"
#include "ui/models/protocols/wireguardConfigModel.h"
#include "core/utils/utilities.h"

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

InstallUiController::InstallUiController(InstallController *installController, QServersRepository *serversRepository,
                                         ServersController *serversController, ServersModel *serversModel, ContainersModel *containersModel,
                                         ProtocolsModel *protocolsModel, ClientManagementController *clientManagementController,
                                         QAppSettingsRepository *appSettingsRepository, const std::shared_ptr<Settings> &settings,
                                         QObject *parent)
    : QObject(parent),
      m_installController(installController),
      m_serversRepository(serversRepository),
      m_serversController(serversController),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_protocolModel(protocolsModel),
      m_clientManagementController(clientManagementController),
      m_appSettingsRepository(appSettingsRepository),
      m_settings(settings)
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

        int serverIndex = m_serversRepository->serversCount() - 1;
        QJsonObject serverConfig = m_serversRepository->server(serverIndex);
        QJsonArray containers = serverConfig.value(config_key::containers).toArray();
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
        QJsonObject serverConfig = m_serversRepository->server(serverIndex);
        QJsonArray containers = serverConfig.value(config_key::containers).toArray();
        int containersCount = containers.size();

        bool wasContainerInstalled = false;
        errorCode = m_installController->installContainer(serverCredentials, container, port, transportProto, serverIndex,
                                                          wasContainerInstalled);
        if (errorCode) {
            emit installationErrorOccurred(errorCode);
            return;
        }

        QJsonObject newServerConfig = m_serversRepository->server(serverIndex);
        QJsonArray newContainers = newServerConfig.value(config_key::containers).toArray();
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
    QJsonObject serverBefore = m_serversRepository->server(serverIndex);
    QJsonArray containersBefore = serverBefore.value(config_key::containers).toArray();
    int containersCountBefore = containersBefore.size();

    ErrorCode errorCode = m_installController->scanServerForInstalledContainers(serverIndex);

    if (errorCode == ErrorCode::NoError) {
        QJsonObject serverAfter = m_serversRepository->server(serverIndex);
        QJsonArray containersAfter = serverAfter.value(config_key::containers).toArray();
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
    QJsonObject oldContainerConfig = m_containersModel->getContainerConfig(container);

    ErrorCode errorCode = m_installController->updateContainer(serverIndex, container, oldContainerConfig, config);

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

void InstallUiController::removeApiConfig(const int serverIndex)
{
    m_serversController->removeApiConfig(serverIndex);
    emit apiConfigRemoved(tr("Api config removed"));
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
    QJsonObject server;
    server.insert(config_key::hostName, m_processedServerCredentials.hostName);
    server.insert(config_key::userName, m_processedServerCredentials.userName);
    server.insert(config_key::password, m_processedServerCredentials.secretData);
    server.insert(config_key::port, m_processedServerCredentials.port);
    server.insert(config_key::description, m_appSettingsRepository->nextAvailableServerName());

    server.insert(config_key::defaultContainer, ContainerProps::containerToString(DockerContainer::None));

    m_serversController->addServer(server);
    emit installServerFinished(tr("Server added successfully"));
}

bool InstallUiController::isConfigValid()
{
    int serverIndex = m_serversController->getDefaultServerIndex();
    QJsonObject serverConfigObject = m_serversController->getServerConfig(serverIndex);

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
