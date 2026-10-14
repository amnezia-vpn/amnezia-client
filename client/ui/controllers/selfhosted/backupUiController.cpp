#include "backupUiController.h"

#include <QDebug>
#include <QTimer>

#include "core/controllers/serversController.h"
#include "ui/controllers/serversUiController.h"
#ifdef Q_OS_IOS
#include "platforms/ios/ios_controller.h"
#endif

BackupUiController::BackupUiController(BackupController *backupController, ServersController *serversController,
                                       ServersUiController *serversUiController, QObject *parent)
    : QObject(parent),
      m_backupController(backupController),
      m_serversController(serversController),
      m_serversUiController(serversUiController)
{
    connect(m_backupController, &BackupController::progressChanged, this, &BackupUiController::progressChanged);
}

void BackupUiController::createBackupWithDownload(bool downloadToDevice, bool deleteFromServer)
{
    ServerCredentials credentials;
    if (!serverCredentials(false, credentials)) {
        return;
    }

    QString backupFilename;
    ErrorCode error = m_backupController->createBackup(credentials, backupFilename);
    if (error != ErrorCode::NoError) {
        emit errorOccurred(tr("Failed to create backup: %1").arg(m_backupController->lastError()));
        return;
    }

    if (!downloadToDevice) {
        emit backupCreated(backupFilename);
        return;
    }

    const QString localPath = BackupController::defaultDownloadPath(backupFilename);
    error = m_backupController->downloadBackup(credentials, backupFilename, localPath);
    if (error != ErrorCode::NoError) {
        emit errorOccurred(tr("Failed to download backup: %1").arg(m_backupController->lastError()));
        return;
    }

#ifdef Q_OS_IOS
    IosController::Instance()->shareText(QStringList { localPath });
#endif

    if (deleteFromServer && m_backupController->deleteBackup(credentials, backupFilename) != ErrorCode::NoError) {
        qWarning() << "Failed to delete backup from server:" << m_backupController->lastError();
    }

    emit backupDownloaded(localPath);
}

void BackupUiController::startRestore(bool isFromSetupWizard, const QString &backupFilePath, bool replaceMode,
                                      const QString &wizardHostname, const QString &wizardUsername,
                                      const QString &wizardSecretData)
{
    ServerCredentials credentials;
    if (isFromSetupWizard && !wizardHostname.isEmpty()) {
        credentials.hostName = wizardHostname;
        credentials.userName = wizardUsername;
        credentials.secretData = wizardSecretData;
    } else if (!serverCredentials(true, credentials)) {
        return;
    }

    QString remoteFilename;
    ErrorCode error = m_backupController->uploadBackup(credentials, backupFilePath, remoteFilename);
    if (error != ErrorCode::NoError) {
        emit errorOccurred(tr("Failed to upload backup: %1").arg(m_backupController->lastError()));
        return;
    }

    error = m_backupController->restoreBackup(credentials, remoteFilename, replaceMode);
    if (error != ErrorCode::NoError) {
        emit errorOccurred(tr("Failed to restore backup: %1").arg(m_backupController->lastError()));
        return;
    }

    emit backupRestored();
}

QStringList BackupUiController::scanBackupForContainers(const QString &localPath)
{
    return m_backupController->scanBackupForContainers(localPath);
}

QVariantMap BackupUiController::getBackupFileInfo(const QString &backupFilePath)
{
    const BackupController::BackupFileInfo info = BackupController::backupFileInfo(backupFilePath);
    return { { "fileName", info.fileName }, { "serverIp", info.serverIp } };
}

bool BackupUiController::setDefaultServerAfterRestore(bool isFromSetupWizard)
{
    if (!isFromSetupWizard || m_serversController->getServersCount() == 0) {
        return false;
    }

    const int serverIndex = m_serversController->getServersCount() - 1;
    m_serversUiController->setDefaultServerAtIndex(serverIndex);
    m_serversUiController->setProcessedServerId(m_serversController->getServerId(serverIndex));

    m_containerRetryCount = 0;
    QTimer::singleShot(500, this, &BackupUiController::trySetDefaultContainer);
    return true;
}

bool BackupUiController::serverCredentials(bool fallbackToDefault, ServerCredentials &credentials)
{
    QString serverId = m_serversUiController->getProcessedServerId();
    if (m_serversController->indexOfServerId(serverId) < 0 && fallbackToDefault) {
        serverId = m_serversController->getDefaultServerId();
    }

    if (m_serversController->indexOfServerId(serverId) < 0) {
        emit errorOccurred(tr("No server selected"));
        return false;
    }

    credentials = m_serversController->getServerCredentials(serverId);
    return true;
}

void BackupUiController::trySetDefaultContainer()
{
    const int serverIndex = m_serversController->getServersCount() - 1;
    const QString serverId = m_serversController->getServerId(serverIndex);
    const auto containers = m_serversController->getServerContainersMap(serverId);

    for (auto it = containers.cbegin(); it != containers.cend(); ++it) {
        if (it.key() != DockerContainer::None) {
            m_serversUiController->setDefaultContainerAtIndex(serverIndex, static_cast<int>(it.key()));
            break;
        }
    }

    if (!containers.isEmpty()) {
        emit defaultServerAndContainerSet();
        return;
    }

    // The containers map is filled asynchronously after restore, poll a few times before giving up
    m_containerRetryCount++;
    if (m_containerRetryCount < m_maxContainerRetries) {
        QTimer::singleShot(500, this, &BackupUiController::trySetDefaultContainer);
    } else {
        m_containerRetryCount = 0;
        emit defaultServerAndContainerSet();
    }
}
