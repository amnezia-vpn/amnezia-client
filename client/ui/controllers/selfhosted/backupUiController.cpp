#include "backupUiController.h"

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
    connect(m_backupController, &BackupController::backupCreated, this, &BackupUiController::backupCreated);
    connect(m_backupController, &BackupController::backupDownloaded, this, &BackupUiController::onBackupDownloaded);
    connect(m_backupController, &BackupController::backupRestored, this, &BackupUiController::backupRestored);
    connect(m_backupController, &BackupController::defaultServerAndContainerSet, this,
            &BackupUiController::defaultServerAndContainerSet);
    connect(m_backupController, &BackupController::progressChanged, this, &BackupUiController::progressChanged);
    connect(m_backupController, &BackupController::errorOccurred, this, &BackupUiController::errorOccurred);
}

void BackupUiController::createBackupWithDownload(bool downloadToDevice, bool deleteFromServer)
{
    ServerCredentials credentials;
    if (serverCredentials(false, credentials)) {
        m_backupController->createAndDownloadBackup(credentials, downloadToDevice, deleteFromServer);
    }
}

void BackupUiController::startRestore(const QString &backupFilePath, bool replaceMode)
{
    ServerCredentials credentials;
    if (serverCredentials(true, credentials)) {
        m_backupController->uploadAndRestore(credentials, backupFilePath, replaceMode);
    }
}

void BackupUiController::setDefaultServerAfterRestore()
{
    const int serversCount = m_serversUiController->getServersCount();
    if (serversCount > 0) {
        m_serversUiController->setProcessedServerId(m_serversUiController->getServerId(serversCount - 1));
    }
    m_backupController->setDefaultServerAfterRestore();
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

void BackupUiController::onBackupDownloaded(const QString &localPath)
{
#ifdef Q_OS_IOS
    IosController::Instance()->shareText(QStringList { localPath });
#endif
    emit backupDownloaded(localPath);
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
