#ifndef BACKUPUICONTROLLER_H
#define BACKUPUICONTROLLER_H

#include <QObject>
#include <QVariantMap>

#include "core/controllers/selfhosted/backupController.h"

class ServersController;
class ServersUiController;

class BackupUiController : public QObject
{
    Q_OBJECT
public:
    explicit BackupUiController(BackupController *backupController, ServersController *serversController,
                                ServersUiController *serversUiController, QObject *parent = nullptr);

public slots:
    void createBackupWithDownload(bool downloadToDevice = true, bool deleteFromServer = true);
    void startRestore(bool isFromSetupWizard, const QString &backupFilePath, bool replaceMode,
                      const QString &wizardHostname = QString(), const QString &wizardUsername = QString(),
                      const QString &wizardSecretData = QString());

    QStringList scanBackupForContainers(const QString &localPath);
    QVariantMap getBackupFileInfo(const QString &backupFilePath);

    bool setDefaultServerAfterRestore(bool isFromSetupWizard);

signals:
    void backupCreated(const QString &backupFilename);
    void backupDownloaded(const QString &localPath);
    void backupRestored();
    void defaultServerAndContainerSet();
    void progressChanged(int percent, const QString &message);
    void errorOccurred(const QString &errorMessage);

private:
    bool serverCredentials(bool fallbackToDefault, ServerCredentials &credentials);
    void trySetDefaultContainer();

    BackupController *m_backupController;
    ServersController *m_serversController;
    ServersUiController *m_serversUiController;

    int m_containerRetryCount = 0;
    static constexpr int m_maxContainerRetries = 3;
};

#endif // BACKUPUICONTROLLER_H
