#include "installUiController.h"

#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QFutureWatcher>
#include <QtConcurrent>

#include "core/utils/api/apiUtils.h"
#include "core/controllers/selfhosted/installController.h"
#include "core/controllers/connectionController.h"
#include "core/utils/networkUtilities.h"
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
#include "core/models/containerConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"
#include "core/models/protocols/openVpnProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"

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
                                         MtProxyConfigModel* mtConfigModel,
                                         TelemtConfigModel *telemtConfigModel,
                                         ConnectionController *connectionController,
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
      m_socks5ConfigModel(socks5ConfigModel),
      m_mtProxyConfigModel(mtConfigModel),
      m_telemtConfigModel(telemtConfigModel),
      m_connectionController(connectionController)
{
    connect(m_installController, &InstallController::configValidated, this, &InstallUiController::configValidated);
    connect(m_installController, &InstallController::validationErrorOccurred, this, &InstallUiController::installationErrorOccurred);
}

InstallUiController::~InstallUiController()
{
}

void InstallUiController::install(DockerContainer container, int port, TransportProto transportProto, const QString &serverId)
{
    const bool isNewServer = serverId.isEmpty();
    
    ServerCredentials serverCredentials;
    if (isNewServer) {
        serverCredentials = m_processedServerCredentials;
    } else {
        serverCredentials = m_serversController->getServerCredentials(serverId);
        m_processedServerCredentials = ServerCredentials();
    }

    if (isNewServer) {
        int existingServerIndex = -1;
        if (m_installController->isServerAlreadyExists(serverCredentials, existingServerIndex)) {
            emit serverAlreadyExists(existingServerIndex);
            return;
        }

        auto *watcher = new QFutureWatcher<QPair<ErrorCode, bool>>(this);
        connect(watcher, &QFutureWatcher<QPair<ErrorCode, bool>>::finished, this, [=, this]() {
            auto result = watcher->result();
            ErrorCode errorCode = result.first;
            bool wasContainerInstalled = result.second;

            if (errorCode) {
                emit installationErrorOccurred(errorCode);
                watcher->deleteLater();
                return;
            }

            const QString newServerId = m_serversController->getServerId(m_serversController->getServersCount() - 1);
            const auto admin = m_serversController->selfHostedAdminConfig(newServerId);
            if (!admin.has_value()) {
                emit installationErrorOccurred(ErrorCode::InternalError);
                watcher->deleteLater();
                return;
            }

            QString finishMessage;
            QMap<DockerContainer, ContainerConfig> containers = admin->containers;
            int containersCount = containers.size();

            if (wasContainerInstalled) {
                finishMessage = tr("%1 installed successfully. ").arg(ContainerUtils::containerHumanNames().value(container));
            } else {
                finishMessage = tr("%1 is already installed on the server. ").arg(ContainerUtils::containerHumanNames().value(container));
            }

            if (containersCount > 1) {
                finishMessage += tr("\nAdded containers that were already installed on the server");
            }

            if (!m_connectionController->isConnected()) {
                m_serversController->setDefaultServer(newServerId);
            }

            if (m_isDoubleVpnFlow) {
                bool success = setupMultihopEntryNode(newServerId, m_doubleVpnEntryCredentials.hostName, m_doubleVpnEntryCredentials.userName, m_doubleVpnEntryCredentials.secretData, port);
                m_isDoubleVpnFlow = false;
                if (success) {
                    finishMessage += tr("\nMultihop entry node successfully configured!");
                } else {
                    watcher->deleteLater();
                    return; // Early return to avoid showing success message if entry node failed
                }
            }

            emit installServerFinished(finishMessage);
            watcher->deleteLater();
        });

        QFuture<QPair<ErrorCode, bool>> future = QtConcurrent::run([=, this]() {
            bool installed = false;
            ErrorCode err = m_installController->installServer(serverCredentials, container, port, transportProto, installed);
            return qMakePair(err, installed);
        });
        watcher->setFuture(future);
    } else {
        const auto adminBefore = m_serversController->selfHostedAdminConfig(serverId);
        if (!adminBefore.has_value()) {
            emit installationErrorOccurred(ErrorCode::InternalError);
            return;
        }
        QMap<DockerContainer, ContainerConfig> containers = adminBefore->containers;
        int containersCount = containers.size();

        auto *watcher = new QFutureWatcher<QPair<ErrorCode, bool>>(this);
        connect(watcher, &QFutureWatcher<QPair<ErrorCode, bool>>::finished, this, [=, this]() {
            auto result = watcher->result();
            ErrorCode errorCode = result.first;
            bool wasContainerInstalled = result.second;

            if (errorCode) {
                emit installationErrorOccurred(errorCode);
                watcher->deleteLater();
                return;
            }

            const auto adminAfter = m_serversController->selfHostedAdminConfig(serverId);
            if (!adminAfter.has_value()) {
                emit installationErrorOccurred(ErrorCode::InternalError);
                watcher->deleteLater();
                return;
            }
            QMap<DockerContainer, ContainerConfig> newContainers = adminAfter->containers;
            int newContainersCount = newContainers.size();

            bool hasNewContainers = (newContainersCount - containersCount) > (wasContainerInstalled ? 1 : 0);
            QString finishMessage;

            if (wasContainerInstalled) {
                finishMessage = tr("%1 installed successfully. ").arg(ContainerUtils::containerHumanNames().value(container));
            } else {
                finishMessage = tr("%1 is already installed on the server. ").arg(ContainerUtils::containerHumanNames().value(container));
            }

            if (hasNewContainers) {
                finishMessage += tr("\nAlready installed containers were found on the server. "
                                    "All installed containers have been added to the application");
            }

            const bool isServiceInstall = ContainerUtils::containerService(container) == ServiceType::Other;
            if (!m_connectionController->isConnected() && !isServiceInstall) {
                m_serversController->setDefaultContainer(serverId, container);
            }

            emit installContainerFinished(finishMessage, isServiceInstall);
            watcher->deleteLater();
        });

        QFuture<QPair<ErrorCode, bool>> future = QtConcurrent::run([=, this]() {
            bool installed = false;
            ErrorCode err = m_installController->installContainer(serverId, container, port, transportProto, installed);
            return qMakePair(err, installed);
        });
        watcher->setFuture(future);
    }
}

void InstallUiController::scanServerForInstalledContainers(const QString &serverId)
{
    const auto serverBefore = m_serversController->selfHostedAdminConfig(serverId);
    if (!serverBefore.has_value()) {
        emit installationErrorOccurred(ErrorCode::InternalError);
        return;
    }
    QMap<DockerContainer, ContainerConfig> containersBefore = serverBefore->containers;
    int containersCountBefore = containersBefore.size();

    ErrorCode errorCode = m_installController->scanServerForInstalledContainers(serverId);

    if (errorCode == ErrorCode::NoError) {
        const auto serverAfter = m_serversController->selfHostedAdminConfig(serverId);
        if (!serverAfter.has_value()) {
            emit installationErrorOccurred(ErrorCode::InternalError);
            return;
        }
        QMap<DockerContainer, ContainerConfig> containersAfter = serverAfter->containers;
        int containersCountAfter = containersAfter.size();

        bool isInstalledContainerAdded = containersCountAfter > containersCountBefore;
        emit scanServerFinished(isInstalledContainerAdded);
        return;
    }

    emit installationErrorOccurred(errorCode);
}

bool InstallUiController::buildContainerConfigFromModel(int containerIndex, int protocolIndex, ContainerConfig &containerConfig)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    Proto protocolType = static_cast<Proto>(protocolIndex);

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
    case Proto::MtProxy: {
        containerConfig.protocolConfig = m_mtProxyConfigModel->getProtocolConfig();
        break;
    }
    case Proto::Telemt: {
        containerConfig.protocolConfig = m_telemtConfigModel->getProtocolConfig();
        break;
    }
#ifdef Q_OS_WINDOWS
    case Proto::Ikev2: {
        containerConfig.protocolConfig = m_ikev2ConfigModel->getProtocolConfig();
        break;
    }
#endif
    default:
        return false;
    }
    return true;
}

void InstallUiController::updateClientConfig(const QString &serverId, int containerIndex, int protocolIndex, bool closePage)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    Proto protocolType = static_cast<Proto>(protocolIndex);

    ContainerConfig containerConfig;
    if (!buildContainerConfigFromModel(containerIndex, protocolIndex, containerConfig)) {
        return;
    }

    ErrorCode errorCode = m_installController->updateClientConfig(serverId, container, containerConfig);

    if (errorCode == ErrorCode::NoError) {
        ContainerConfig updatedConfig = m_serversController->getContainerConfig(serverId, container);
        m_protocolModel->updateModel(updatedConfig);
        updateProtocolConfigModel(serverId, static_cast<int>(container), static_cast<int>(protocolType));
        emit updateContainerFinished(tr("Settings updated successfully"), closePage);
        return;
    }

    emit installationErrorOccurred(errorCode);
}

void InstallUiController::updateServerConfig(const QString &serverId, int containerIndex, int protocolIndex, bool closePage)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    Proto protocolType = static_cast<Proto>(protocolIndex);

    ContainerConfig containerConfig;
    if (!buildContainerConfigFromModel(containerIndex, protocolIndex, containerConfig)) {
        return;
    }
    ContainerConfig oldContainerConfig = m_serversController->getContainerConfig(serverId, container);

    const bool asyncUpdate = container == DockerContainer::MtProxy || container == DockerContainer::Telemt
            || container == DockerContainer::Xray || container == DockerContainer::SSXray;

    if (asyncUpdate) {
        emit serverIsBusy(true);
        auto *watcher = new QFutureWatcher<ErrorCode>(this);
        const Proto protocolTypeCopy = protocolType;
        QObject::connect(watcher, &QFutureWatcher<ErrorCode>::finished, this,
                         [this, watcher, serverId, container, closePage, protocolTypeCopy]() {
                             const ErrorCode errorCode = watcher->result();
                             watcher->deleteLater();
                             emit serverIsBusy(false);

                             if (errorCode == ErrorCode::NoError) {
                                 const ContainerConfig updatedConfig =
                                         m_serversController->getContainerConfig(serverId, container);
                                 m_protocolModel->updateModel(updatedConfig);
                                 updateProtocolConfigModel(serverId, static_cast<int>(container), static_cast<int>(protocolTypeCopy));
                                 emit updateContainerFinished(tr("Settings updated successfully"), closePage);
                             } else {
                                 emit installationErrorOccurred(errorCode);
                             }
                         });

        ContainerConfig newConfigCopy = containerConfig;
        ContainerConfig oldConfigCopy = oldContainerConfig;
        InstallController *installController = m_installController;
        QFuture<ErrorCode> future =
                QtConcurrent::run([installController, serverId, container, oldConfigCopy,
                                   newConfigCopy]() mutable -> ErrorCode {
                    return installController->updateServerConfig(serverId, container, oldConfigCopy, newConfigCopy);
                });
        watcher->setFuture(future);
        return;
    }

    ErrorCode errorCode = m_installController->updateServerConfig(serverId, container, oldContainerConfig, containerConfig);

    if (errorCode == ErrorCode::NoError) {
        ContainerConfig updatedConfig = m_serversController->getContainerConfig(serverId, container);
        m_protocolModel->updateModel(updatedConfig);
        updateProtocolConfigModel(serverId, static_cast<int>(container), static_cast<int>(protocolType));
        emit updateContainerFinished(tr("Settings updated successfully"), closePage);
        return;
    }

    emit installationErrorOccurred(errorCode);
}

void InstallUiController::setContainerEnabled(const QString &serverId, int containerIndex, bool enabled)
{
    const DockerContainer container = static_cast<DockerContainer>(containerIndex);
    if (container != DockerContainer::MtProxy && container != DockerContainer::Telemt) {
        return;
    }

    emit serverIsBusy(true);
    const ErrorCode errorCode = m_installController->setDockerContainerEnabledState(serverId, container, enabled);
    emit serverIsBusy(false);

    if (errorCode == ErrorCode::NoError) {
        const ContainerConfig currentConfig = m_serversController->getContainerConfig(serverId, container);
        m_protocolModel->updateModel(currentConfig);
        emit setContainerEnabledFinished(enabled);
        return;
    }

    emit installationErrorOccurred(errorCode);
}

void InstallUiController::refreshContainerStatus(const QString &serverId, int containerIndex)
{
    const DockerContainer container = static_cast<DockerContainer>(containerIndex);
    if (container != DockerContainer::MtProxy && container != DockerContainer::Telemt) {
        return;
    }

    int status = 3;
    const ErrorCode errorCode = m_installController->queryDockerContainerStatus(serverId, container, status);
    if (errorCode != ErrorCode::NoError) {
        emit containerStatusRefreshed(3);
        return;
    }
    emit containerStatusRefreshed(status);
}

void InstallUiController::refreshContainerDiagnostics(const QString &serverId, int containerIndex, int port)
{
    const DockerContainer container = static_cast<DockerContainer>(containerIndex);
    if (container != DockerContainer::MtProxy && container != DockerContainer::Telemt) {
        return;
    }

    MtProxyContainerDiagnostics diag;
    const ErrorCode errorCode = m_installController->queryMtProxyDiagnostics(serverId, container, port, diag);
    if (errorCode != ErrorCode::NoError) {
        emit containerDiagnosticsRefreshed(false, false, -1, QString(), QString());
        return;
    }
    emit containerDiagnosticsRefreshed(diag.portReachable, diag.upstreamReachable, diag.clientsConnected,
                                       diag.lastConfigRefresh, diag.statsEndpoint);
}

void InstallUiController::fetchContainerSecret(const QString &serverId, int containerIndex)
{
    const DockerContainer container = static_cast<DockerContainer>(containerIndex);
    if (container != DockerContainer::MtProxy && container != DockerContainer::Telemt) {
        return;
    }

    const QString secret = m_installController->fetchDockerContainerSecret(serverId, container);
    emit containerSecretFetched(secret);
}

void InstallUiController::rebootServer(const QString &serverId)
{
    const QString serverName = m_serversController->notificationDisplayName(serverId);

    const auto errorCode = m_installController->rebootServer(serverId);
    if (errorCode == ErrorCode::NoError) {
        emit rebootServerFinished(tr("Server '%1' was rebooted").arg(serverName));
    } else {
        emit installationErrorOccurred(errorCode);
    }
}

void InstallUiController::removeServer(const QString &serverId)
{
    if (serverId.isEmpty()) {
        return;
    }
    const QString serverName = m_serversController->notificationDisplayName(serverId);

    m_serversController->removeServer(serverId);
    emit removeServerFinished(tr("Server '%1' was removed").arg(serverName));
}

void InstallUiController::removeAllContainers(const QString &serverId)
{
    const QString serverName = m_serversController->notificationDisplayName(serverId);

    ErrorCode errorCode = m_installController->removeAllContainers(serverId);
    if (errorCode == ErrorCode::NoError) {
        emit removeAllContainersFinished(tr("All containers from server '%1' have been removed").arg(serverName));
        return;
    }
    emit installationErrorOccurred(errorCode);
}

void InstallUiController::removeContainer(const QString &serverId, int containerIndex)
{
    const QString serverName = m_serversController->notificationDisplayName(serverId);

    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    QString containerName = ContainerUtils::containerHumanNames().value(container);

    const bool asyncRemove = container == DockerContainer::Xray || container == DockerContainer::SSXray;

    if (asyncRemove) {
        emit serverIsBusy(true);
        auto *watcher = new QFutureWatcher<ErrorCode>(this);
        QObject::connect(watcher, &QFutureWatcher<ErrorCode>::finished, this,
                         [this, watcher, serverId, container, containerName, serverName]() {
                             const ErrorCode errorCode = watcher->result();
                             watcher->deleteLater();
                             emit serverIsBusy(false);

                             if (errorCode == ErrorCode::NoError) {
                                 emit removeContainerFinished(
                                         tr("%1 has been removed from the server '%2'").arg(containerName, serverName));
                             } else {
                                 emit installationErrorOccurred(errorCode);
                             }
                         });

        InstallController *installController = m_installController;
        QFuture<ErrorCode> future = QtConcurrent::run(
                [installController, serverId, container]() -> ErrorCode {
                    return installController->removeContainer(serverId, container);
                });
        watcher->setFuture(future);
        return;
    }

    ErrorCode errorCode = m_installController->removeContainer(serverId, container);
    if (errorCode == ErrorCode::NoError) {

        emit removeContainerFinished(tr("%1 has been removed from the server '%2'").arg(containerName, serverName));
        return;
    }
    emit installationErrorOccurred(errorCode);
}

void InstallUiController::clearCachedProfile(const QString &serverId, int containerIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    if (ContainerUtils::containerService(container) == ServiceType::Other) {
        return;
    }

    m_installController->clearCachedProfile(serverId, container);

    emit cachedProfileCleared(tr("%1 cached profile cleared").arg(ContainerUtils::containerHumanNames().value(container)));
    ContainerConfig updatedConfig = m_serversController->getContainerConfig(serverId, container);
    m_protocolModel->updateModel(updatedConfig);
}

QRegularExpression InstallUiController::ipAddressRegExp()
{
    return NetworkUtilities::ipAddressRegExp();
}

void InstallUiController::clearProcessedServerCredentials()
{
    m_processedServerCredentials = ServerCredentials();
    m_doubleVpnEntryCredentials = ServerCredentials();
    m_isDoubleVpnFlow = false;
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
    m_doubleVpnEntryCredentials = ServerCredentials();
    m_isDoubleVpnFlow = false;
}

void InstallUiController::setDoubleVpnEntryCredentials(const QString &hostName, const QString &userName, const QString &secretData)
{
    m_doubleVpnEntryCredentials.hostName = hostName;
    if (m_doubleVpnEntryCredentials.hostName.contains(":")) {
        m_doubleVpnEntryCredentials.port = m_doubleVpnEntryCredentials.hostName.split(":").at(1).toInt();
        m_doubleVpnEntryCredentials.hostName = m_doubleVpnEntryCredentials.hostName.split(":").at(0);
    } else {
        m_doubleVpnEntryCredentials.port = 22;
    }
    m_doubleVpnEntryCredentials.userName = userName;
    m_doubleVpnEntryCredentials.secretData = secretData;
    m_isDoubleVpnFlow = true;
}

void InstallUiController::mountSftpDrive(const QString &serverId, const QString &port, const QString &password, const QString &username)
{
    ServerCredentials serverCredentials = m_serversController->getServerCredentials(serverId);
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

bool InstallUiController::checkDoubleVpnEntryConnection()
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
    ErrorCode errorCode = m_installController->checkSshConnection(m_doubleVpnEntryCredentials, output, passphraseCallback);

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
    m_installController->addEmptyServer(m_processedServerCredentials);
    if (!m_connectionController->isConnected()) {
        const QString newServerId = m_serversController->getServerId(m_serversController->getServersCount() - 1);
        if (!newServerId.isEmpty()) {
            m_serversController->setDefaultServer(newServerId);
        }
    }
    emit installServerFinished(tr("Server added successfully"));
}

void InstallUiController::validateConfig()
{
    const QString serverId = m_serversController->getDefaultServerId();
    if (serverId.isEmpty()) {
        return;
    }
    m_installController->validateConfig(serverId);
}

void InstallUiController::updateProtocols(const QString &serverId, int containerIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    ContainerConfig containerConfig = m_serversController->getContainerConfig(serverId, container);
    containerConfig.container = container;
    m_protocolModel->updateModel(containerConfig);
}

void InstallUiController::openServerSettings(const QString &serverId, int containerIndex, int protocolIndex)
{
    updateProtocolConfigModel(serverId, containerIndex, protocolIndex);
}

void InstallUiController::openClientSettings(const QString &serverId, int containerIndex, int protocolIndex)
{
    updateProtocolConfigModel(serverId, containerIndex, protocolIndex);
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

void InstallUiController::updateProtocolConfigModel(const QString &serverId, int containerIndex, int protocolIndex)
{
    DockerContainer container = static_cast<DockerContainer>(containerIndex);
    ContainerConfig containerConfig = m_serversController->getContainerConfig(serverId, container);
    containerConfig.container = container;
    Proto protocolType = static_cast<Proto>(protocolIndex);

    auto updateIfPresent = [&](auto* model, auto* config) {
        if (model && config) model->updateModel(container, *config);
    };

    switch (protocolType) {
    case Proto::Awg: updateIfPresent(m_awgConfigModel, containerConfig.getAwgProtocolConfig()); break;
    case Proto::WireGuard: updateIfPresent(m_wireGuardConfigModel, containerConfig.getWireGuardProtocolConfig()); break;
    case Proto::OpenVpn: updateIfPresent(m_openVpnConfigModel, containerConfig.getOpenVpnProtocolConfig()); break;
    case Proto::Xray:
    case Proto::SSXray: updateIfPresent(m_xrayConfigModel, containerConfig.getXrayProtocolConfig()); break;
    case Proto::TorWebSite: updateIfPresent(m_torConfigModel, containerConfig.getTorProtocolConfig()); break;
    case Proto::Sftp: updateIfPresent(m_sftpConfigModel, containerConfig.getSftpProtocolConfig()); break;
    case Proto::Socks5Proxy: updateIfPresent(m_socks5ConfigModel, containerConfig.getSocks5ProxyProtocolConfig()); break;
    case Proto::MtProxy: updateIfPresent(m_mtProxyConfigModel, containerConfig.getMtProxyProtocolConfig()); break;
    case Proto::Telemt: updateIfPresent(m_telemtConfigModel, containerConfig.getTelemtProtocolConfig()); break;
#ifdef Q_OS_WINDOWS
    case Proto::Ikev2: updateIfPresent(m_ikev2ConfigModel, containerConfig.getIkev2ProtocolConfig()); break;
#endif
    default: break;
    }
}

bool InstallUiController::setupMultihopEntryNode(const QString &serverId, const QString &entryIp, const QString &entryUser, const QString &entryPass, int exitPort)
{
    ServerCredentials credentials;
    credentials.hostName = entryIp;
    if (credentials.hostName.contains(":")) {
        credentials.port = credentials.hostName.split(":").at(1).toInt();
        credentials.hostName = credentials.hostName.split(":").at(0);
    } else {
        credentials.port = 22;
    }
    credentials.userName = entryUser;
    credentials.secretData = entryPass;

    auto config = m_serversController->getServerRawConfig(serverId);
    if (!config.has_value()) return false;

    QString exitIp = config.value().value("defaultIpv4").toString();

    QJsonObject obj = config.value();

    int actualExitPort = exitPort;
    if (obj.contains("containers")) {
        QJsonArray containers = obj.value("containers").toArray();
        for (const QJsonValue &c : containers) {
            QJsonObject containerObj = c.toObject();
            QString cName = containerObj.value("container").toString();
            if (cName == "amnezia-awg" || cName == "amnezia-wireguard" || cName == "amnezia-openvpn") {
                if (containerObj.contains("port")) {
                    actualExitPort = containerObj.value("port").toInt();
                    break;
                }
            }
        }
    }

    ErrorCode err = m_installController->setupMultihopEntryNode(credentials, exitIp, actualExitPort);
    if (err != ErrorCode::NoError) {
        emit installationErrorOccurred(err);
        return false;
    }

    // Now update the server config to route through the entry node
    obj.insert("multihop_port", actualExitPort);
    obj.insert("defaultIpv4", entryIp);
    // We update the local settings without ssh
    m_serversController->updateServerRawConfig(serverId, obj);
    return true;
}
