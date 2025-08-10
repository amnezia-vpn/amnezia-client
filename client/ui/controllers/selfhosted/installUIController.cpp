#include "installUIController.h"

#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QtConcurrent>

#include "core/api/apiUtils.h"
#include "core/controllers/selfhosted/serverController.h"
#include "core/controllers/selfhosted/installController.h"
#include "core/controllers/vpnConfigurationController.h"
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

InstallUIController::InstallUIController(const QSharedPointer<ServersModel> &serversModel, const QSharedPointer<ContainersModel> &containersModel,
                                        const QSharedPointer<ProtocolsModel> &protocolsModel,
                                        const QSharedPointer<ClientManagementModel> &clientManagementModel,
                                        QSharedPointer<InstallController> coreInstallController,
                                        QSharedPointer<ApiConfigsController> apiConfigsCoreController,
                                        QSharedPointer<ClientManagementController> clientManagementController,
                                        QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_protocolModel(protocolsModel),
      m_clientManagementModel(clientManagementModel),
      m_coreInstallController(coreInstallController),
      m_apiConfigsCoreController(apiConfigsCoreController),
      m_clientManagementController(clientManagementController)
{
}

InstallUIController::~InstallUIController()
{
#ifdef Q_OS_WINDOWS
    for (QSharedPointer<QProcess> process : m_sftpMountProcesses) {
        Utils::signalCtrl(process->processId(), CTRL_C_EVENT);
        process->kill();
        process->waitForFinished();
    }
#endif
}

void InstallUIController::install(DockerContainer container, int port, TransportProto transportProto)
{
    ServerCredentials serverCredentials;
    if (m_shouldCreateServer) {
        serverCredentials = m_processedServerCredentials;
    } else {
        int serverIndex = m_serversModel->getProcessedServerIndex();
        serverCredentials = qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));
    }

    InstallResult result = m_coreInstallController->installContainer(container, port, transportProto,
                                                                    serverCredentials, m_shouldCreateServer,
                                                                    m_privateKeyPassphrase);
    
    if (result.errorCode != ErrorCode::NoError) {
        emit installationErrorOccurred(result.errorCode);
        return;
    }

    if (result.isServiceInstall) {
        emit installContainerFinished(result.message, true);
    } else {
        if (m_shouldCreateServer) {
            emit installServerFinished(result.message);
        } else {
            emit installContainerFinished(result.message, false);
        }
    }
}





bool InstallUIController::isServerAlreadyExists()
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

void InstallUIController::scanServerForInstalledContainers()
{
    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials serverCredentials =
            qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));

    auto result = m_coreInstallController->scanServerForInstalledContainers(serverCredentials);
    
    if (result.errorCode != ErrorCode::NoError) {
        emit installationErrorOccurred(result.errorCode);
        return;
    }

    if (result.isInstalledContainerFound) {
        emit scanServerFinished(true);
    } else {
        emit scanServerFinished(false);
    }
}


void InstallUIController::updateContainer()
{
    int serverIndex = m_serversModel->getProcessedServerIndex();
    const DockerContainer container = static_cast<DockerContainer>(m_containersModel->getProcessedContainerIndex());
    const QString containerName = m_containersModel->getProcessedContainerName();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);

    auto result = m_coreInstallController->updateContainer(container, credentials, containerConfig);
    
    if (result.errorCode != ErrorCode::NoError) {
        emit installationErrorOccurred(result.errorCode);
        return;
    }

    emit updateContainerFinished(tr("Settings updated successfully"));
}

void InstallUIController::rebootProcessedServer()
{
    int serverIndex = m_serversModel->getProcessedServerIndex();
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);

    ErrorCode errorCode = m_coreInstallController->rebootProcessedServer(credentials);
    if (errorCode == ErrorCode::NoError) {
        emit rebootProcessedServerFinished(tr("Server '%1' was rebooted").arg(serverName));
    } else {
        emit installationErrorOccurred(errorCode);
    }
}

void InstallUIController::removeProcessedServer()
{
    int serverIndex = m_serversModel->getProcessedServerIndex();
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();

    m_serversModel->removeProcessedServer();
    emit removeProcessedServerFinished(tr("Server '%1' was removed").arg(serverName));
}

void InstallUIController::removeAllContainers()
{
    int serverIndex = m_serversModel->getProcessedServerIndex();
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);

    ErrorCode errorCode = m_coreInstallController->removeAllContainers(credentials);
    if (errorCode == ErrorCode::NoError) {
        emit removeAllContainersFinished(tr("All containers from server '%1' have been removed").arg(serverName));
        return;
    }
    emit installationErrorOccurred(errorCode);
}

void InstallUIController::removeProcessedContainer()
{
    int serverIndex = m_serversModel->getProcessedServerIndex();
    QString serverName = m_serversModel->data(serverIndex, ServersModel::Roles::NameRole).toString();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);

    int containerIndex = m_containersModel->getProcessedContainerIndex();
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    QString containerName = m_containersModel->getProcessedContainerName();

    ErrorCode errorCode = m_coreInstallController->removeProcessedContainer(container, credentials);
    if (errorCode == ErrorCode::NoError) {
        emit removeProcessedContainerFinished(tr("%1 has been removed from the server '%2'").arg(containerName, serverName));
        return;
    }
    emit installationErrorOccurred(errorCode);
}

void InstallUIController::removeApiConfig(const int serverIndex)
{
    if (m_apiConfigsCoreController) {
        m_apiConfigsCoreController->removeApiConfig(serverIndex);
    }
    emit apiConfigRemoved(tr("Api config removed"));
}

void InstallUIController::clearCachedProfile()
{
    int serverIndex = m_serversModel->getProcessedServerIndex();
    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getProcessedContainerIndex());
    if (ContainerProps::containerService(container) == ServiceType::Other) {
        return;
    }

    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);

    ErrorCode errorCode = m_coreInstallController->clearCachedProfile(credentials, container, containerConfig);

    if (errorCode == ErrorCode::NoError) {
        emit cachedProfileCleared(tr("%1 cached profile cleared").arg(ContainerProps::containerHumanNames().value(container)));
    } else {
        emit installationErrorOccurred(errorCode);
    }
}

QRegularExpression InstallUIController::ipAddressPortRegExp()
{
    return NetworkUtilities::ipAddressPortRegExp();
}

QRegularExpression InstallUIController::ipAddressRegExp()
{
    return NetworkUtilities::ipAddressRegExp();
}

void InstallUIController::setProcessedServerCredentials(const QString &hostName, const QString &userName, const QString &secretData)
{
    m_processedServerCredentials.hostName = hostName;
    if (m_processedServerCredentials.hostName.contains(":")) {
        m_processedServerCredentials.port = m_processedServerCredentials.hostName.split(":").at(1).toInt();
        m_processedServerCredentials.hostName = m_processedServerCredentials.hostName.split(":").at(0);
    }
    m_processedServerCredentials.userName = userName;
    m_processedServerCredentials.secretData = secretData;
}

void InstallUIController::setShouldCreateServer(bool shouldCreateServer)
{
    m_shouldCreateServer = shouldCreateServer;
}

void InstallUIController::mountSftpDrive(const QString &port, const QString &password, const QString &username)
{
#ifndef Q_OS_IOS
    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    
    ErrorCode errorCode = m_coreInstallController->mountSftpDrive(credentials, port, password, username);
    if (errorCode != ErrorCode::NoError) {
        emit installationErrorOccurred(errorCode);
    }
#endif
}

bool InstallUIController::checkSshConnection()
{
    ErrorCode errorCode = m_coreInstallController->checkSshConnection(m_processedServerCredentials, m_privateKeyPassphrase);
    
    if (errorCode != ErrorCode::NoError) {
        emit installationErrorOccurred(errorCode);
        return false;
    }
    
    return true;
}

QRegularExpression InstallUIController::ipAddressPortRegExp()
{
    return NetworkUtilities::ipAddressPortRegExp();
}

QRegularExpression InstallUIController::ipAddressRegExp()
{
    return NetworkUtilities::ipAddressRegExp();
}

void InstallUIController::setEncryptedPassphrase(QString passphrase)
{
    m_privateKeyPassphrase = passphrase;
    emit passphraseRequestFinished();
}

void InstallUIController::addEmptyServer()
{
    ErrorCode errorCode = m_coreInstallController->addEmptyServer(m_processedServerCredentials);
    if (errorCode == ErrorCode::NoError) {
        emit installServerFinished(tr("Server added successfully"));
    } else {
        emit installationErrorOccurred(errorCode);
    }
}

bool InstallUIController::isConfigValid()
{
    int serverIndex = m_serversModel->getDefaultServerIndex();
    QJsonObject serverConfigObject = m_serversModel->getServerConfig(serverIndex);

    if (apiUtils::isServerFromApi(serverConfigObject)) {
        return true;
    }

    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    
    bool isValid = m_coreInstallController->isConfigValid(credentials);
    
    if (!isValid) {
        emit installationErrorOccurred(ErrorCode::InternalError);
    }
    
    return isValid;
}


