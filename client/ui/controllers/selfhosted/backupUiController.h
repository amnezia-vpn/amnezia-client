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
    void startRestore(const QString &backupFilePath, bool replaceMode);
    void setDefaultServerAfterRestore();

    QStringList scanBackupForContainers(const QString &localPath);
    QVariantMap getBackupFileInfo(const QString &backupFilePath);

signals:
    void backupCreated(const QString &backupFilename);
    void backupDownloaded(const QString &localPath);
    void backupRestored();
    void defaultServerAndContainerSet();
    void progressChanged(int percent, const QString &message);
    void errorOccurred(const QString &errorMessage);

private:
    void onBackupDownloaded(const QString &localPath);
    bool serverCredentials(bool fallbackToDefault, ServerCredentials &credentials);

    BackupController *m_backupController;
    ServersController *m_serversController;
    ServersUiController *m_serversUiController;
};

#endif // BACKUPUICONTROLLER_H
