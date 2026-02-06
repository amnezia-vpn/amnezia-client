#ifndef SERVERSBACKUPCONTROLLER_H
#define SERVERSBACKUPCONTROLLER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>

class QTemporaryFile;
class ServersModel;

#include "core/controllers/serverController.h"
#include "core/defs.h"
#include "containers/containers_defs.h"

using namespace amnezia;

/**
 * @brief Controller for managing Amnezia VPN configuration backups
 * 
 * Uses existing ServerController and libssh::Client from Amnezia
 * Bash scripts are embedded directly in C++ code
 * Supports direct container backup via docker cp
 * 
 * Fully cross-platform: Windows, macOS, Linux, iOS, Android
 */
class ServersBackupController : public QObject
{
    Q_OBJECT

public:
    explicit ServersBackupController(std::shared_ptr<Settings> settings, ServersModel *serversModel, QObject *parent = nullptr);
    ~ServersBackupController();

    /**
     * @brief Backup information
     */
    struct BackupInfo {
        QString filename;
        QString fullPath;
        QDateTime createdAt;
        qint64 size;
        bool isValid;
        QStringList containers;
    };

    enum BackupStatus {
        Idle,
        InProgress,
        Success,
        Failed
    };
    Q_ENUM(BackupStatus)

public slots:
    /**
     * @brief Create backup on server (all containers)
     * @param credentials Server credentials
     */
    void createBackup(const ServerCredentials &credentials);
    
    /**
     * @brief Create backup and automatically download to device (for QML)
     * @param downloadToDevice Download to device after creation?
     * @param deleteFromServer Delete from server after download?
     */
    Q_INVOKABLE void createBackupWithDownload(bool downloadToDevice = true, 
                                               bool deleteFromServer = true);

    /**
     * @brief Create backup of specific container
     * @param credentials Server credentials
     * @param container Container type for backup
     */
    void createContainerBackup(const ServerCredentials &credentials, DockerContainer container);
    
    /**
     * @brief Create backup of specific container by name
     * @param credentials Server credentials
     * @param containerName Container name (e.g. "amnezia-awg")
     */
    void createBackupByName(const ServerCredentials &credentials, const QString &containerName);

    /**
     * @brief Create backup of multiple containers
     * @param credentials Server credentials
     * @param containers List of containers for backup
     */
    void createContainersBackup(const ServerCredentials &credentials, const QList<DockerContainer> &containers);

    /**
     * @brief Get list of backups from server
     * @param credentials Server credentials
     */
    void fetchBackupList(const ServerCredentials &credentials);

    /**
     * @brief Restore from backup
     * @param credentials Server credentials
     * @param backupFilename Backup file name
     * @param containers List of containers (empty = all)
     * @param replaceMode If true - clears container first, then restores. If false - adds data on top of existing
     */
    void restoreBackup(const ServerCredentials &credentials, 
                       const QString &backupFilename, 
                       const QStringList &containers = QStringList(),
                       bool replaceMode = false);

    /**
     * @brief Check backup status on server
     * @param credentials Server credentials
     */
    void checkBackupStatus(const ServerCredentials &credentials);

    /**
     * @brief Download backup to local machine
     * @param credentials Server credentials
     * @param backupFilename Backup file name
     * @param localPath Save path
     */
    void downloadBackup(const ServerCredentials &credentials,
                        const QString &backupFilename, 
                        const QString &localPath);

    /**
     * @brief Upload backup to server
     * @param credentials Server credentials
     * @param localPath Path to local file
     * @param replaceMode Restore mode (true = replace, false = add). Saved for later use in restoreBackup
     */
    void uploadBackup(const ServerCredentials &credentials,
                      const QString &localPath,
                      bool replaceMode = false);
    
    // Overloaded method for setup wizard with separate credential parameters
    Q_INVOKABLE void uploadBackupWithStrings(const QString &hostname,
                                             const QString &username,
                                             const QString &secretData,
                                             const QString &localPath,
                                             bool replaceMode = false);
    
    /**
     * @brief Universal method to start restore (from QML)
     * Automatically selects correct path depending on parameters
     * @param isFromSetupWizard Restore from setup wizard?
     * @param backupFilePath Path to local backup file
     * @param replaceMode Restore mode (true = replace, false = add)
     * @param wizardHostname Hostname for setup wizard (optional)
     * @param wizardUsername Username for setup wizard (optional)
     * @param wizardSecretData Secret data for setup wizard (optional)
     */
    Q_INVOKABLE void startRestore(bool isFromSetupWizard,
                                   const QString &backupFilePath,
                                   bool replaceMode,
                                   const QString &wizardHostname = QString(),
                                   const QString &wizardUsername = QString(),
                                   const QString &wizardSecretData = QString());
    
    /**
     * @brief Prepare restore information from backup file
     * Parses filename, extracts IP, prepares metadata
     * @param backupFilePath Path to backup file
     * @return QVariantMap with keys: fileName, serverIp
     */
    Q_INVOKABLE QVariantMap getBackupFileInfo(const QString &backupFilePath);
    
    /**
     * @brief Scan backup file and determine which containers it contains
     * @param localPath Path to local backup file
     * @return List of container names found in backup
     */
    Q_INVOKABLE QStringList scanBackupForContainers(const QString &localPath);
    
    /**
     * @brief Set default server and container after restore (for setup wizard)
     * @param isFromSetupWizard Was restore called from setup wizard
     * @return true if successful, false if no servers or containers
     */
    Q_INVOKABLE bool setDefaultServerAfterRestore(bool isFromSetupWizard);
    
    /**
     * @brief Install containers from backup on empty server (for setup wizard)
     * Scans backup, adds empty server and sends signal to install containers
     * @param backupFilePath Path to local backup file
     * @param hostname Server hostname
     * @param username Username for SSH
     * @param secretData Password/key for SSH
     */
    Q_INVOKABLE void prepareRestoreFromBackup(const QString &backupFilePath,
                                               const QString &hostname,
                                               const QString &username,
                                               const QString &secretData);

    /**
     * @brief Delete backup from server
     * @param credentials Server credentials
     * @param backupFilename Backup file name
     */
    void deleteBackup(const ServerCredentials &credentials,
                      const QString &backupFilename);

    /**
     * @brief Set backup directory on server
     */
    void setBackupDirectory(const QString &directory);

    /**
     * @brief Get backup directory
     */
    QString backupDirectory() const { return m_backupDir; }

signals:
    /**
     * @brief Operation status changed
     */
    void statusChanged(BackupStatus status);

    /**
     * @brief Operation progress (0-100)
     */
    void progressChanged(int percent, const QString &message);

    /**
     * @brief Backup list received
     */
    void backupListReceived(const QList<BackupInfo> &backups);

    /**
     * @brief Backup created successfully
     */
    void backupCreated(const QString &backupFilename);

    /**
     * @brief Backup restored successfully
     */
    void backupRestored();
    
    /**
     * @brief Need to set default server and container (for setup wizard)
     * This signal is sent after backupRestored() if restore was from setup wizard
     */
    void needSetDefaultServer();
    
    /**
     * @brief Default server and container successfully set
     * Can navigate to result page
     */
    void defaultServerAndContainerSet();
    
    /**
     * @brief All containers from backup installed
     * Can proceed to data restore
     * @param backupFilePath Path to backup file
     * @param hostname Hostname
     * @param username Username  
     * @param secretData Secret data
     * @param serverIp IP address (for display)
     * @param fileName File name (for display)
     */
    void readyForRestore(const QString &backupFilePath,
                        const QString &hostname,
                        const QString &username,
                        const QString &secretData,
                        const QString &serverIp,
                        const QString &fileName);

    /**
     * @brief Backup downloaded
     */
    void backupDownloaded(const QString &localPath);

    /**
     * @brief Backup uploaded to server
     */
    void backupUploaded(const QString &serverPath);

    /**
     * @brief Backup status information received
     */
    void backupStatusReceived(const QJsonObject &status);

    /**
     * @brief Error occurred
     */
    void errorOccurred(const QString &errorMessage, ErrorCode errorCode);

private:
    /**
     * @brief Get bash script for creating backup of all containers
     * @param ipAddress Server IP address in underscored format (e.g. "192_119_110_11")
     */
    QString getBackupScript(const QString &ipAddress) const;

    /**
     * @brief Get bash script for creating backup of specific container
     * @param container Container type
     * @param ipAddress Server IP address in underscored format (e.g. "192_119_110_11")
     */
    QString getContainerBackupScript(DockerContainer container, const QString &ipAddress) const;

    /**
     * @brief Get bash script for creating backup of multiple containers
     * @param containers List of containers
     * @param ipAddress Server IP address in underscored format (e.g. "192_119_110_11")
     */
    QString getContainersBackupScript(const QList<DockerContainer> &containers, const QString &ipAddress) const;

    /**
     * @brief Get bash script for restore
     * @param backupFilename Backup file name
     * @param containers List of containers
     * @param replaceMode If true - clears container first, then restores
     */
    QString getRestoreScript(const QString &backupFilename, const QStringList &containers, bool replaceMode = false) const;

    /**
     * @brief Get bash script for status check
     */
    QString getCheckStatusScript() const;

    /**
     * @brief Get bash script for backup list
     */
    QString getListBackupsScript() const;

    /**
     * @brief Parse backup list from output
     */
    QList<BackupInfo> parseBackupList(const QString &output);

    /**
     * @brief Parse status from output
     */
    QJsonObject parseBackupStatus(const QString &output);

    /**
     * @brief Handle standard output
     */
    ErrorCode handleStdOut(const QString &data, QString &output);

    /**
     * @brief Handle error output
     */
    ErrorCode handleStdErr(const QString &data, QString &error);

    /**
     * @brief Set status
     */
    void setStatus(BackupStatus status);

    /**
     * @brief Set progress
     */
    void setProgress(int percent, const QString &message);
    
    /**
     * @brief Attempt to set default container (called from timer)
     */
    void trySetDefaultContainer();

private:
    std::shared_ptr<Settings> m_settings;
    ServersModel *m_serversModel;
    ServerController *m_serverController;
    BackupStatus m_status;
    QString m_backupDir;
    QString m_currentOutput;
    QString m_currentError;
    bool m_restoreReplaceMode; // Save restore mode for use after uploadBackup
    QTemporaryFile *m_tempUploadFile; // Temp file for Android URI (to prevent deletion before upload completes)
    
    // For setting default container
    int m_containerRetryCount;
    static constexpr int m_maxContainerRetries = 3;
    
    // For automatic restore after upload
    ServerCredentials m_pendingRestoreCredentials;
    bool m_autoRestoreAfterUpload;
    
    // For automatic backup download/delete
    bool m_autoDownloadAfterCreate;
    bool m_autoDeleteAfterDownload;
    QString m_lastCreatedBackupFilename;
};

#endif // SERVERSBACKUPCONTROLLER_H

