#ifndef BACKUPCONTROLLER_H
#define BACKUPCONTROLLER_H

#include <QObject>
#include <QStringList>

#include "core/utils/commonStructs.h"
#include "core/utils/containerEnum.h"
#include "core/utils/errorCodes.h"
#include "core/utils/selfhosted/sshSession.h"

using namespace amnezia;

class BackupController : public QObject
{
    Q_OBJECT
public:
    struct BackupFileInfo
    {
        QString fileName;
        QString serverIp;
    };

    explicit BackupController(QObject *parent = nullptr);

    void setBackupDirectory(const QString &directory);
    QString backupDirectory() const;

    ErrorCode createBackup(const ServerCredentials &credentials, QString &backupFilename);
    ErrorCode createContainersBackup(const ServerCredentials &credentials, const QList<DockerContainer> &containers,
                                     QString &backupFilename);
    ErrorCode restoreBackup(const ServerCredentials &credentials, const QString &backupFilename, bool replaceMode);
    ErrorCode downloadBackup(const ServerCredentials &credentials, const QString &backupFilename, const QString &localPath);
    ErrorCode uploadBackup(const ServerCredentials &credentials, const QString &localPath, QString &remoteFilename);
    ErrorCode deleteBackup(const ServerCredentials &credentials, const QString &backupFilename);

    QStringList scanBackupForContainers(const QString &localPath);

    static BackupFileInfo backupFileInfo(const QString &backupFilePath);
    static QString defaultDownloadPath(const QString &backupFilename);

    QString lastError() const;

signals:
    void progressChanged(int percent, const QString &message);

private:
    ErrorCode runBackupScript(const ServerCredentials &credentials, const QString &script, QString &backupFilename);
    ErrorCode runHostScript(const ServerCredentials &credentials, const QString &script);
    ErrorCode readLocalFile(const QString &localPath, QByteArray &data);

    static QString fileNameFromPath(const QString &path);
    static QString ipForFilename(const QString &hostName);

    QString backupScript(const QString &ipAddress) const;
    QString containersBackupScript(const QList<DockerContainer> &containers, const QString &ipAddress) const;
    QString restoreScript(const QString &backupFilename, bool replaceMode) const;

    SshSession m_sshSession;
    QString m_backupDir;
    QString m_output;
    QString m_error;
    QString m_lastError;
};

#endif // BACKUPCONTROLLER_H
