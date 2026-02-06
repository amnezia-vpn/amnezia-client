#include "serversBackupController.h"
#include <QDebug>
#include <QDir>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QUrl>
#include <QProcess>
#include <QSet>
#include <QTimer>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#include "platforms/android/android_controller.h"
#endif
#ifdef Q_OS_IOS
#include "platforms/ios/ios_controller.h"
#endif
#include "containers/containers_defs.h"
#include "core/networkUtilities.h"
#include "systemController.h"
#include "ui/models/servers_model.h"
#include "ui/models/containers_model.h"

ServersBackupController::ServersBackupController(std::shared_ptr<Settings> settings, ServersModel *serversModel, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_serversModel(serversModel)
    , m_serverController(new ServerController(settings, this))
    , m_status(Idle)
    , m_backupDir("/var/backups/amnezia")
    , m_restoreReplaceMode(false)
    , m_tempUploadFile(nullptr)
    , m_containerRetryCount(0)
    , m_autoRestoreAfterUpload(false)
    , m_autoDownloadAfterCreate(false)
    , m_autoDeleteAfterDownload(false)
{
}

ServersBackupController::~ServersBackupController()
{
}

void ServersBackupController::setBackupDirectory(const QString &directory)
{
    m_backupDir = directory;
}

void ServersBackupController::createBackup(const ServerCredentials &credentials)
{
    if (m_status == InProgress) {
        emit errorOccurred("Another operation is in progress", ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    setStatus(InProgress);
    setProgress(0, tr("Starting backup creation..."));

    m_currentOutput.clear();
    m_currentError.clear();

    // Get server IP address
    QString serverIp = NetworkUtilities::getIPAddress(credentials.hostName);
    if (serverIp.isEmpty()) {
        serverIp = credentials.hostName;
    }
    // Format IP: replace dots with underscores
    QString ipFormatted = serverIp;
    ipFormatted.replace(".", "_");
    
    // Get bash script for backup with IP address
    QString script = getBackupScript(ipFormatted);

    // Callback for handling stdout
    auto cbStdOut = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdOut(data, m_currentOutput);
    };

    // Callback for handling stderr
    auto cbStdErr = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdErr(data, m_currentError);
    };

    // Run script on server
    ErrorCode error = m_serverController->runHostScript(credentials, script, cbStdOut, cbStdErr);

    if (error == ErrorCode::NoError) {
        // Parse created backup name from output: format "IP_ADDRESS - DD-MM-YYYY_HH-MM-SS.tgz"
        QRegularExpression re("([\\d_]+)\\s+-\\s+(\\d{2}-\\d{2}-\\d{4})_(\\d{2}-\\d{2}-\\d{2})\\.tgz");
        QRegularExpressionMatch match = re.match(m_currentOutput);
        
        if (match.hasMatch()) {
            QString backupFilename = match.captured(1) + " - " + match.captured(2) + "_" + match.captured(3) + ".tgz";
            setStatus(Success);
            setProgress(100, tr("Backup created successfully"));
            
            // Save filename for later deletion
            m_lastCreatedBackupFilename = backupFilename;
            
            // If auto-download enabled, start it
            if (m_autoDownloadAfterCreate) {
                qDebug() << "Auto-downloading backup after creation";
                m_autoDownloadAfterCreate = false; // Reset flag
                downloadBackup(credentials, backupFilename, backupFilename);
            } else {
                emit backupCreated(backupFilename);
            }
        } else {
            setStatus(Failed);
            emit errorOccurred(tr("Failed to parse backup filename from output: %1").arg(m_currentOutput.left(200)), ErrorCode::InternalError);
        }
    } else {
        setStatus(Failed);
        emit errorOccurred(tr("Failed to create backup: %1").arg(m_currentError), error);
    }
}

void ServersBackupController::createBackupByName(const ServerCredentials &credentials, const QString &containerName)
{
    DockerContainer container = ContainerProps::containerFromString(containerName);
    if (container == DockerContainer::None) {
        emit errorOccurred(tr("Unknown container: %1").arg(containerName), ErrorCode::InternalError);
        return;
    }
    createContainerBackup(credentials, container);
}

void ServersBackupController::createContainerBackup(const ServerCredentials &credentials, DockerContainer container)
{
    if (m_status == InProgress) {
        emit errorOccurred("Another operation is in progress", ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    setStatus(InProgress);
    QString containerName = ContainerProps::containerToString(container);
    setProgress(0, tr("Starting backup for container: %1...").arg(containerName));

    m_currentOutput.clear();
    m_currentError.clear();

    // Get server IP address
    QString serverIp = NetworkUtilities::getIPAddress(credentials.hostName);
    if (serverIp.isEmpty()) {
        serverIp = credentials.hostName;
    }
    // Format IP: replace dots with underscores
    QString ipFormatted = serverIp;
    ipFormatted.replace(".", "_");

    QString script = getContainerBackupScript(container, ipFormatted);

    auto cbStdOut = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdOut(data, m_currentOutput);
    };

    auto cbStdErr = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdErr(data, m_currentError);
    };

    ErrorCode error = m_serverController->runHostScript(credentials, script, cbStdOut, cbStdErr);

    if (error == ErrorCode::NoError) {
        // Parse created backup name from output: format "IP_ADDRESS - DD-MM-YYYY_HH-MM-SS.tgz"
        QRegularExpression re("([\\d_]+)\\s+-\\s+(\\d{2}-\\d{2}-\\d{4})_(\\d{2}-\\d{2}-\\d{2})\\.tgz");
        QRegularExpressionMatch match = re.match(m_currentOutput);
        
        if (match.hasMatch()) {
            QString backupFilename = match.captured(1) + " - " + match.captured(2) + "_" + match.captured(3) + ".tgz";
            setStatus(Success);
            setProgress(100, tr("Container backup created successfully"));
            emit backupCreated(backupFilename);
        } else {
            setStatus(Failed);
            emit errorOccurred(tr("Failed to parse backup filename from output: %1").arg(m_currentOutput.left(200)), ErrorCode::InternalError);
        }
    } else {
        setStatus(Failed);
        emit errorOccurred(tr("Failed to create container backup: %1").arg(m_currentError), error);
    }
}

void ServersBackupController::createContainersBackup(const ServerCredentials &credentials, const QList<DockerContainer> &containers)
{
    if (m_status == InProgress) {
        emit errorOccurred("Another operation is in progress", ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    setStatus(InProgress);
    setProgress(0, tr("Starting backup for %1 containers...").arg(containers.size()));

    m_currentOutput.clear();
    m_currentError.clear();

    // Get server IP address
    QString serverIp = NetworkUtilities::getIPAddress(credentials.hostName);
    if (serverIp.isEmpty()) {
        serverIp = credentials.hostName;
    }
    // Format IP: replace dots with underscores
    QString ipFormatted = serverIp;
    ipFormatted.replace(".", "_");

    QString script = getContainersBackupScript(containers, ipFormatted);

    auto cbStdOut = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdOut(data, m_currentOutput);
    };

    auto cbStdErr = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdErr(data, m_currentError);
    };

    ErrorCode error = m_serverController->runHostScript(credentials, script, cbStdOut, cbStdErr);

    if (error == ErrorCode::NoError) {
        // Parse created backup name from output: format "IP_ADDRESS - DD-MM-YYYY_HH-MM-SS.tgz"
        QRegularExpression re("([\\d_]+)\\s+-\\s+(\\d{2}-\\d{2}-\\d{4})_(\\d{2}-\\d{2}-\\d{2})\\.tgz");
        QRegularExpressionMatch match = re.match(m_currentOutput);
        
        if (match.hasMatch()) {
            QString backupFilename = match.captured(1) + " - " + match.captured(2) + "_" + match.captured(3) + ".tgz";
            setStatus(Success);
            setProgress(100, tr("Containers backup created successfully"));
            emit backupCreated(backupFilename);
        } else {
            setStatus(Failed);
            emit errorOccurred(tr("Failed to parse backup filename from output: %1").arg(m_currentOutput.left(200)), ErrorCode::InternalError);
        }
    } else {
        setStatus(Failed);
        emit errorOccurred(tr("Failed to create containers backup: %1").arg(m_currentError), error);
    }
}

void ServersBackupController::fetchBackupList(const ServerCredentials &credentials)
{
    if (m_status == InProgress) {
        emit errorOccurred("Another operation is in progress", ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    setStatus(InProgress);
    setProgress(0, tr("Fetching backup list..."));

    m_currentOutput.clear();
    m_currentError.clear();

    QString script = getListBackupsScript();

    auto cbStdOut = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdOut(data, m_currentOutput);
    };

    auto cbStdErr = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdErr(data, m_currentError);
    };

    ErrorCode error = m_serverController->runHostScript(credentials, script, cbStdOut, cbStdErr);

    if (error == ErrorCode::NoError) {
        QList<BackupInfo> backups = parseBackupList(m_currentOutput);
        setStatus(Success);
        setProgress(100, tr("Backup list received"));
        emit backupListReceived(backups);
    } else {
        setStatus(Failed);
        emit errorOccurred(tr("Failed to fetch backup list: %1").arg(m_currentError), error);
    }
}

void ServersBackupController::restoreBackup(const ServerCredentials &credentials,
                                           const QString &backupFilename,
                                           const QStringList &containers,
                                           bool replaceMode)
{
    if (m_status == InProgress) {
        emit errorOccurred("Another operation is in progress", ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    setStatus(InProgress);
    QString modeText = replaceMode ? tr("replace mode") : tr("add mode");
    setProgress(0, tr("Starting restore from %1 (%2)...").arg(backupFilename, modeText));

    m_currentOutput.clear();
    m_currentError.clear();

    QString script = getRestoreScript(backupFilename, containers, replaceMode);

    auto cbStdOut = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdOut(data, m_currentOutput);
    };

    auto cbStdErr = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdErr(data, m_currentError);
    };

    ErrorCode error = m_serverController->runHostScript(credentials, script, cbStdOut, cbStdErr);

    // Check output for errors, even if script exited with code 0
    bool hasError = m_currentOutput.contains("[ERROR]") || 
                    m_currentOutput.contains("Failed to extract backup") ||
                    m_currentError.contains("[ERROR]") ||
                    m_currentError.contains("Failed to extract backup");

    if (error == ErrorCode::NoError && !hasError) {
        // Check that restore actually completed successfully
        if (m_currentOutput.contains("Restore completed successfully")) {
            setStatus(Success);
            setProgress(100, tr("Backup restored successfully"));
            emit backupRestored();
        } else {
            setStatus(Failed);
            emit errorOccurred(tr("Backup restore did not complete successfully. Output: %1").arg(m_currentOutput), ErrorCode::InternalError);
        }
    } else {
        setStatus(Failed);
        QString errorMessage = hasError ? 
            tr("Failed to restore backup: %1").arg(m_currentOutput + "\n" + m_currentError) :
            tr("Failed to restore backup: %1").arg(m_currentError);
        emit errorOccurred(errorMessage, error);
    }
}

void ServersBackupController::checkBackupStatus(const ServerCredentials &credentials)
{
    if (m_status == InProgress) {
        emit errorOccurred("Another operation is in progress", ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    setStatus(InProgress);
    setProgress(0, tr("Checking backup status..."));

    m_currentOutput.clear();
    m_currentError.clear();

    QString script = getCheckStatusScript();

    auto cbStdOut = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdOut(data, m_currentOutput);
    };

    auto cbStdErr = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdErr(data, m_currentError);
    };

    ErrorCode error = m_serverController->runHostScript(credentials, script, cbStdOut, cbStdErr);

    if (error == ErrorCode::NoError) {
        QJsonObject status = parseBackupStatus(m_currentOutput);
        setStatus(Success);
        setProgress(100, tr("Status received"));
        emit backupStatusReceived(status);
    } else {
        setStatus(Failed);
        emit errorOccurred(tr("Failed to check backup status: %1").arg(m_currentError), error);
    }
}

void ServersBackupController::downloadBackup(const ServerCredentials &credentials,
                                            const QString &backupFilename,
                                            const QString &localPath)
{
    if (m_status == InProgress) {
        emit errorOccurred("Another operation is in progress", ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    setStatus(InProgress);
    setProgress(0, tr("Downloading backup..."));

    // Validate backup filename
    if (backupFilename.isEmpty()) {
        setStatus(Failed);
        emit errorOccurred(tr("Backup filename is empty"), ErrorCode::InternalError);
        return;
    }

    // Construct remote file path
    QString remotePath = QString("%1/%2").arg(m_backupDir, backupFilename);

    // Determine actual local path
    QString actualLocalPath = localPath;
    QFileInfo pathInfo(localPath);
    
    // If only filename provided (no directory), use appropriate folder
    if (!pathInfo.isAbsolute() || pathInfo.dir().path() == ".") {
#ifdef Q_OS_ANDROID
        // On Android use public Download folder (via JNI)
        QJniObject mediaDir = QJniObject::callStaticObjectMethod(
            "android/os/Environment",
            "getExternalStoragePublicDirectory",
            "(Ljava/lang/String;)Ljava/io/File;",
            QJniObject::getStaticObjectField("android/os/Environment", "DIRECTORY_DOWNLOADS", "Ljava/lang/String;").object());
        QString downloadsPath = mediaDir.callObjectMethod("getAbsolutePath", "()Ljava/lang/String;").toString();
        actualLocalPath = QDir(downloadsPath).filePath(backupFilename);
#elif defined(Q_OS_IOS)
        QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        actualLocalPath = QDir(tempPath).filePath(backupFilename);
#else
        // On Desktop use Documents (as regular backup)
        QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        if (documentsPath.isEmpty()) {
            documentsPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        }
        actualLocalPath = QDir(documentsPath).filePath(backupFilename);
#endif
    }

    // Ensure local directory exists
    QFileInfo localFileInfo(actualLocalPath);
    QDir localDir = localFileInfo.dir();
    if (!localDir.exists()) {
        if (!localDir.mkpath(".")) {
            setStatus(Failed);
            emit errorOccurred(tr("Failed to create local directory: %1").arg(localDir.path()), ErrorCode::InternalError);
            return;
        }
    }

    setProgress(25, tr("Starting file transfer..."));

    ErrorCode error = m_serverController->downloadFileFromHost(credentials, remotePath, actualLocalPath);

    if (error == ErrorCode::NoError) {
       // qDebug() << "Backup downloaded to:" << actualLocalPath;
        
#ifdef Q_OS_IOS
        QStringList filesToShare;
        filesToShare.append(actualLocalPath);
        IosController::Instance()->shareText(filesToShare);
#endif
        
        setStatus(Success);
        setProgress(100, tr("Backup downloaded successfully"));
        
        // If auto-delete from server enabled, start it
        if (m_autoDeleteAfterDownload && !m_lastCreatedBackupFilename.isEmpty()) {
            qDebug() << "Auto-deleting backup from server after download:" << m_lastCreatedBackupFilename;
            m_autoDeleteAfterDownload = false; // Reset flag
            deleteBackup(credentials, m_lastCreatedBackupFilename);
            m_lastCreatedBackupFilename.clear();
        }
        
        emit backupDownloaded(actualLocalPath);
    } else {
        setStatus(Failed);
        emit errorOccurred(tr("Failed to download backup: error code %1").arg(static_cast<int>(error)), error);
    }
}

void ServersBackupController::uploadBackup(const ServerCredentials &credentials,
                                          const QString &localPath,
                                          bool replaceMode)
{
    if (m_status == InProgress) {
        emit errorOccurred("Another operation is in progress", ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    // Save restore mode for later use
    m_restoreReplaceMode = replaceMode;

    QString actualLocalPath = localPath;
    QString filename;
    
#ifdef Q_OS_ANDROID
    // For Android URI need to get filename and use file descriptor
    if (localPath.startsWith("content://")) {
        // Get filename from URI
        filename = AndroidController::instance()->getFileName(localPath);
        if (filename.isEmpty()) {
            // Fallback: extract name from URI
            QStringList parts = localPath.split('/');
            if (!parts.isEmpty()) {
                filename = parts.last();
                // Decode URL-encoded characters
                if (filename.contains('%')) {
                    filename = QUrl::fromPercentEncoding(filename.toUtf8());
                }
            }
        }
        
        // For Android URI use file descriptor via SystemController::readFile
        // But scpFileCopy requires file path, so need to copy file to temp directory
        QByteArray fileData;
        qDebug() << "Reading Android URI:" << localPath;
        if (!SystemController::readFile(localPath, fileData)) {
            qDebug() << "Failed to read from Android URI";
            emit errorOccurred(tr("Failed to read backup file from Android storage"), ErrorCode::ReadError);
            return;
        }
        qDebug() << "Read" << fileData.size() << "bytes from Android URI";
        
        // Delete previous temp file if exists
        if (m_tempUploadFile) {
            delete m_tempUploadFile;
            m_tempUploadFile = nullptr;
        }
        
        // Create temp file (save to class member so it doesn't get deleted)
        // Use setAutoRemove(false) so file isn't automatically deleted
        m_tempUploadFile = new QTemporaryFile(this);
        m_tempUploadFile->setAutoRemove(false);
        if (!m_tempUploadFile->open()) {
            qDebug() << "Failed to create temporary file";
            emit errorOccurred(tr("Failed to create temporary file"), ErrorCode::OpenError);
            delete m_tempUploadFile;
            m_tempUploadFile = nullptr;
            return;
        }
        
        qint64 written = m_tempUploadFile->write(fileData);
        m_tempUploadFile->flush();
        // DON'T close file - it must stay open for SCP
        // m_tempUploadFile->close();
        actualLocalPath = m_tempUploadFile->fileName();
        
        qDebug() << "Created temp file:" << actualLocalPath << "written:" << written << "bytes, size:" << QFileInfo(actualLocalPath).size();
        
        // Check that file exists and is readable
        QFileInfo tempFileInfo(actualLocalPath);
        if (!tempFileInfo.exists()) {
            qDebug() << "Temp file does not exist after creation!";
            emit errorOccurred(tr("Failed to create temporary file"), ErrorCode::OpenError);
            delete m_tempUploadFile;
            m_tempUploadFile = nullptr;
            return;
        }
        
        // If filename empty, use name from temp file
        if (filename.isEmpty()) {
            filename = QFileInfo(actualLocalPath).fileName();
        }
    } else {
        QFileInfo localFileInfo(localPath);
        if (!localFileInfo.exists()) {
            emit errorOccurred(tr("Local file does not exist: %1").arg(localPath), ErrorCode::InternalError);
            return;
        }
        filename = localFileInfo.fileName();
    }
#else
    // For other platforms use regular check
    QFileInfo localFileInfo(localPath);
    if (!localFileInfo.exists()) {
        emit errorOccurred(tr("Local file does not exist: %1").arg(localPath), ErrorCode::InternalError);
        return;
    }

    if (!localFileInfo.isFile()) {
        emit errorOccurred(tr("Path is not a file: %1").arg(localPath), ErrorCode::InternalError);
        return;
    }
    
    filename = localFileInfo.fileName();
#endif

    setStatus(InProgress);
    setProgress(0, tr("Uploading backup..."));

    // Construct remote file path with filename
    QString remotePath = QString("%1/%2").arg(m_backupDir, filename);

    setProgress(25, tr("Starting file transfer..."));

    ErrorCode error = m_serverController->uploadFileToHostPublic(credentials, actualLocalPath, remotePath,
                                                                  libssh::ScpOverwriteMode::ScpOverwriteExisting);

    qDebug() << "Upload result, error code:" << static_cast<int>(error);
    
    if (error == ErrorCode::NoError) {
        setStatus(Success);
        setProgress(100, tr("Backup uploaded successfully"));
        emit backupUploaded(remotePath);
        
        // If auto restore enabled, start it
        if (m_autoRestoreAfterUpload) {
            m_autoRestoreAfterUpload = false; // Reset flag
            
            qDebug() << "Auto-starting restore after upload";
            QString backupFilename = remotePath.split('/').last();
            restoreBackup(m_pendingRestoreCredentials, backupFilename, QStringList(), m_restoreReplaceMode);
        }
        
        // Delete temp file after successful upload
        if (m_tempUploadFile) {
            qDebug() << "Removing temp file:" << m_tempUploadFile->fileName();
            m_tempUploadFile->remove();
            delete m_tempUploadFile;
            m_tempUploadFile = nullptr;
        }
    } else {
        setStatus(Failed);
        qDebug() << "Upload failed with error code:" << static_cast<int>(error);
        emit errorOccurred(tr("Failed to upload backup: error code %1").arg(static_cast<int>(error)), error);
        
        // Delete temp file on error
        if (m_tempUploadFile) {
            m_tempUploadFile->remove();
            delete m_tempUploadFile;
            m_tempUploadFile = nullptr;
        }
    }
}

// Overloaded method for setup wizard with separate credential parameters
void ServersBackupController::uploadBackupWithStrings(const QString &hostname,
                                                     const QString &username,
                                                     const QString &secretData,
                                                     const QString &localPath,
                                                     bool replaceMode)
{
    // Create ServerCredentials from strings
    ServerCredentials credentials;
    credentials.hostName = hostname;
    credentials.userName = username;
    credentials.secretData = secretData;
    credentials.port = 22; // Default SSH port
    
    qDebug() << "uploadBackupWithStrings called with hostname:" << hostname << "username:" << username;
    
    // Call main method
    uploadBackup(credentials, localPath, replaceMode);
}

QStringList ServersBackupController::scanBackupForContainers(const QString &localPath)
{
    QStringList containers;
    
    qDebug() << "Scanning backup file for containers:" << localPath;
    
    // For Android URI or regular path use tar to view contents
#ifdef Q_OS_ANDROID
    QString actualPath = localPath;
    if (localPath.startsWith("content://")) {
        // For Android URI need to read file first
        int fd = AndroidController::instance()->getFd(localPath);
        if (fd < 0) {
            qWarning() << "Failed to get file descriptor for Android URI";
            return containers;
        }
        
        QFile file;
        if (!file.open(fd, QIODevice::ReadOnly)) {
            qWarning() << "Failed to open file from descriptor";
            AndroidController::instance()->closeFd();
            return containers;
        }
        
        QByteArray data = file.readAll();
        file.close();
        AndroidController::instance()->closeFd();
        
        // Save to temporary file
        actualPath = QDir::temp().filePath("backup_scan_temp.tgz");
        QFile tempFile(actualPath);
        if (!tempFile.open(QIODevice::WriteOnly)) {
            qWarning() << "Failed to create temp file for scanning";
            return containers;
        }
        tempFile.write(data);
        tempFile.close();
    }
#else
    QString actualPath = localPath;
#endif
    
    // Execute tar command to view contents
    QProcess process;
    process.start("tar", QStringList() << "-tzf" << actualPath);
    process.waitForFinished(5000);
    
    if (process.exitCode() != 0) {
        qWarning() << "Failed to read backup archive:" << process.readAllStandardError();
        return containers;
    }
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    // Find container directories (amnezia-*)
    QSet<QString> foundContainers;
    for (const QString &line : lines) {
        if (line.contains("amnezia-")) {
            // Extract container name from path
            QStringList parts = line.split('/');
            for (const QString &part : parts) {
                if (part.startsWith("amnezia-")) {
                    foundContainers.insert(part);
                    break;
                }
            }
        }
    }
    
    containers = foundContainers.values();
    qDebug() << "Found containers in backup:" << containers;
    
    return containers;
}

void ServersBackupController::deleteBackup(const ServerCredentials &credentials,
                                          const QString &backupFilename)
{
    if (m_status == InProgress) {
        emit errorOccurred("Another operation is in progress", ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    setStatus(InProgress);
    setProgress(0, tr("Deleting backup..."));

    // Escape filename for safe use in bash
    QString escapedFilename = backupFilename;
    escapedFilename.replace("'", "'\\''"); // Escape single quotes
    QString script = QString("sudo rm -f '%1/%2'").arg(m_backupDir).arg(escapedFilename);

    m_currentOutput.clear();
    m_currentError.clear();

    auto cbStdOut = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdOut(data, m_currentOutput);
    };

    auto cbStdErr = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdErr(data, m_currentError);
    };

    ErrorCode error = m_serverController->runHostScript(credentials, script, cbStdOut, cbStdErr);

    if (error == ErrorCode::NoError) {
        setStatus(Success);
        setProgress(100, tr("Backup deleted"));
    } else {
        setStatus(Failed);
        emit errorOccurred(tr("Failed to delete backup: %1").arg(m_currentError), error);
    }
}

// ============================================================================
// EMBEDDED BASH SCRIPTS
// ============================================================================

QString ServersBackupController::getBackupScript(const QString &ipAddress) const
{
    // Simplified bash script version, embedded in C++
    // Filename format: IP_ADDRESS - DD-MM-YYYY_HH-MM-SS.tgz
    return QString(R"(
#!/bin/bash
set -e

BACKUP_DIR=%1
IP_ADDRESS=%2
DATE=$(date +%d-%m-%Y)
TIME=$(date +%H-%M-%S)
BACKUP_FILENAME="$IP_ADDRESS - ${DATE}_${TIME}.tgz"
BACKUP_SUBDIR="$BACKUP_DIR/backup_temp_$$"

echo "[INFO] Starting backup..."

# Create directory
mkdir -p "$BACKUP_SUBDIR"

# List of Amnezia containers
CONTAINERS=(
    "amnezia-awg"
    "amnezia-awg2"
    "amnezia-openvpn"
    "amnezia-xray"
    "amnezia-wireguard"
    "amnezia-ipsec"
    "amnezia-cloak"
    "amnezia-shadowsocks"
)

# Backup each container (including stopped)
for container in "${CONTAINERS[@]}"; do
    if sudo docker ps -a --format '{{.Names}}' | grep -q "^$container$"; then
        echo "[INFO] Backing up $container..."
        mkdir -p "$BACKUP_SUBDIR/$container"
        
        # Copy /opt/amnezia
        sudo docker cp "$container:/opt/amnezia" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
        
        # Save metadata
        sudo docker inspect "$container" > "$BACKUP_SUBDIR/$container/container_inspect.json" 2>/dev/null || true
    fi
done

# Create archive
cd "$BACKUP_DIR"
tar -czf "$BACKUP_FILENAME" -C "$BACKUP_SUBDIR" . 2>/dev/null
rm -rf "$BACKUP_SUBDIR"

echo "[INFO] Backup created: $BACKUP_FILENAME"
)").arg(m_backupDir, ipAddress);
}

QString ServersBackupController::getContainerBackupScript(DockerContainer container, const QString &ipAddress) const
{
    QString containerName = ContainerProps::containerToString(container);
    
    // Backup specific container directly via docker cp
    // Filename format: IP_ADDRESS - DD-MM-YYYY_HH-MM-SS.tgz
    return QString(R"(
#!/bin/bash
set -e

BACKUP_DIR=%1
CONTAINER_NAME=%2
IP_ADDRESS=%3
DATE=$(date +%d-%m-%Y)
TIME=$(date +%H-%M-%S)
BACKUP_FILENAME="$IP_ADDRESS - ${DATE}_${TIME}.tgz"
BACKUP_SUBDIR="$BACKUP_DIR/backup_temp_$$"

echo "[INFO] Starting backup for container: $CONTAINER_NAME..."

# Check container exists
if ! sudo docker ps -a --format '{{.Names}}' | grep -q "^$CONTAINER_NAME$"; then
    echo "[ERROR] Container $CONTAINER_NAME does not exist"
    exit 1
fi

# Create directory
mkdir -p "$BACKUP_SUBDIR/$CONTAINER_NAME"

# Backup configurations from container directly
echo "[INFO] Copying /opt/amnezia from container..."
sudo docker cp "$CONTAINER_NAME:/opt/amnezia" "$BACKUP_SUBDIR/$CONTAINER_NAME/" 2>/dev/null || {
    echo "[WARN] Failed to copy /opt/amnezia, trying alternative paths..."
    # Alternative paths for different container types
    sudo docker cp "$CONTAINER_NAME:/etc/openvpn" "$BACKUP_SUBDIR/$CONTAINER_NAME/" 2>/dev/null || true
    sudo docker cp "$CONTAINER_NAME:/etc/wireguard" "$BACKUP_SUBDIR/$CONTAINER_NAME/" 2>/dev/null || true
    sudo docker cp "$CONTAINER_NAME:/etc/ipsec.d" "$BACKUP_SUBDIR/$CONTAINER_NAME/" 2>/dev/null || true
    sudo docker cp "$CONTAINER_NAME:/etc/xray" "$BACKUP_SUBDIR/$CONTAINER_NAME/" 2>/dev/null || true
}

# Save container metadata
echo "[INFO] Saving container metadata..."
sudo docker inspect "$CONTAINER_NAME" > "$BACKUP_SUBDIR/$CONTAINER_NAME/container_inspect.json" 2>/dev/null || true

# Save network configuration
sudo docker network inspect $(sudo docker inspect -f '{{range $k, $v := .NetworkSettings.Networks}}{{$k}} {{end}}' "$CONTAINER_NAME" | awk '{print $1}') \
    > "$BACKUP_SUBDIR/$CONTAINER_NAME/network_config.json" 2>/dev/null || true

# Create archive
cd "$BACKUP_DIR"
tar -czf "$BACKUP_FILENAME" -C "$BACKUP_SUBDIR" . 2>/dev/null
rm -rf "$BACKUP_SUBDIR"

echo "[INFO] Backup created: $BACKUP_FILENAME"
)").arg(m_backupDir, containerName, ipAddress);
}

QString ServersBackupController::getContainersBackupScript(const QList<DockerContainer> &containers, const QString &ipAddress) const
{
    QString containersList;
    for (const DockerContainer &container : containers) {
        QString containerName = ContainerProps::containerToString(container);
        containersList += QString("\"%1\" ").arg(containerName);
    }
    
    // Filename format: IP_ADDRESS - DD-MM-YYYY_HH-MM-SS.tgz
    return QString(R"(
#!/bin/bash
set -e

BACKUP_DIR=%1
IP_ADDRESS=%2
DATE=$(date +%d-%m-%Y)
TIME=$(date +%H-%M-%S)
BACKUP_FILENAME="$IP_ADDRESS - ${DATE}_${TIME}.tgz"
BACKUP_SUBDIR="$BACKUP_DIR/backup_temp_$$"

echo "[INFO] Starting backup for containers..."

# Create directory
mkdir -p "$BACKUP_SUBDIR"

# List of containers for backup
CONTAINERS=(%3)

# Backup each container
for container in "${CONTAINERS[@]}"; do
    if sudo docker ps -a --format '{{.Names}}' | grep -q "^$container$"; then
        echo "[INFO] Backing up $container..."
        mkdir -p "$BACKUP_SUBDIR/$container"
        
        # Copy /opt/amnezia directly from container
        sudo docker cp "$container:/opt/amnezia" "$BACKUP_SUBDIR/$container/" 2>/dev/null || {
            echo "[WARN] Failed to copy /opt/amnezia from $container, trying alternative paths..."
            sudo docker cp "$container:/etc/openvpn" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
            sudo docker cp "$container:/etc/wireguard" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
            sudo docker cp "$container:/etc/ipsec.d" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
            sudo docker cp "$container:/etc/xray" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
        }
        
        # Save metadata
        sudo docker inspect "$container" > "$BACKUP_SUBDIR/$container/container_inspect.json" 2>/dev/null || true
    else
        echo "[WARN] Container $container does not exist, skipping..."
    fi
done

# Create archive
cd "$BACKUP_DIR"
tar -czf "$BACKUP_FILENAME" -C "$BACKUP_SUBDIR" . 2>/dev/null
rm -rf "$BACKUP_SUBDIR"

echo "[INFO] Backup created: $BACKUP_FILENAME"
)").arg(m_backupDir, ipAddress, containersList.trimmed());
}

QString ServersBackupController::getRestoreScript(const QString &backupFilename, 
                                                  const QStringList &containers,
                                                  bool replaceMode) const
{
    Q_UNUSED(containers); // TODO: Use for selective restore
    
    // Escape filename for safe use in bash
    QString escapedFilename = backupFilename;
    escapedFilename.replace("'", "'\\''"); // Escape single quotes
    
    return QString(R"(
#!/bin/bash
set -e

BACKUP_DIR=%1
BACKUP_FILE='%2'
REPLACE_MODE=%3
TEMP_DIR="/tmp/amnezia_restore_$$"

echo "[INFO] Starting restore from $BACKUP_FILE..."
if [ "$REPLACE_MODE" = "1" ]; then
    echo "[INFO] Using replace mode: containers will be cleared before restore"
else
    echo "[INFO] Using add mode: data will be added to existing containers"
fi

# Check backup file exists
if [ ! -f "$BACKUP_DIR/$BACKUP_FILE" ]; then
    echo "[ERROR] Backup file not found: $BACKUP_DIR/$BACKUP_FILE"
    exit 1
fi

# Extract backup
mkdir -p "$TEMP_DIR"
EXTRACT_OUTPUT=$(tar -xzf "$BACKUP_DIR/$BACKUP_FILE" -C "$TEMP_DIR" 2>&1)
EXTRACT_EXIT_CODE=$?
if [ $EXTRACT_EXIT_CODE -ne 0 ]; then
    echo "[ERROR] Failed to extract backup archive: $EXTRACT_OUTPUT"
    rm -rf "$TEMP_DIR"
    exit 1
fi

# Find directory with containers (may be backup_temp_* or containers directly)
BACKUP_SUBDIR=$(ls -d "$TEMP_DIR"/backup_* 2>/dev/null | head -1)

# If didn't find backup_*, check if container directories exist directly
if [ -z "$BACKUP_SUBDIR" ]; then
    # Check if container directories (amnezia-*) exist directly in TEMP_DIR
    if ls -d "$TEMP_DIR"/amnezia-* 2>/dev/null | head -1 > /dev/null; then
        BACKUP_SUBDIR="$TEMP_DIR"
    else
        echo "[ERROR] Failed to extract backup: backup directory not found in archive"
        echo "[DEBUG] Contents of $TEMP_DIR:"
        ls -la "$TEMP_DIR" || true
        rm -rf "$TEMP_DIR"
        exit 1
    fi
fi

# Restore each container
for container_dir in "$BACKUP_SUBDIR"/*; do
    if [ ! -d "$container_dir" ]; then
        continue
    fi
    
    container_name=$(basename "$container_dir")
    
    if sudo docker ps -a --format '{{.Names}}' | grep -q "^$container_name$"; then
        echo "[INFO] Restoring $container_name..."
        
        # Stop container
        sudo docker stop "$container_name" 2>/dev/null || true
        
        # Replace mode: clear container before restore
        if [ "$REPLACE_MODE" = "1" ]; then
            echo "[INFO] Clearing container $container_name before restore..."
            # Create empty directory to clear /opt/amnezia
            TEMP_CLEAR_DIR="/tmp/clear_amnezia_$$"
            mkdir -p "$TEMP_CLEAR_DIR/amnezia"
            # Copy empty directory, which will delete old contents
            sudo docker cp "$TEMP_CLEAR_DIR/amnezia" "$container_name:/opt/" 2>/dev/null || true
            # Delete temporary directory
            rm -rf "$TEMP_CLEAR_DIR"
        fi
        
        # Restore /opt/amnezia
        if [ -d "$container_dir/amnezia" ]; then
            sudo docker cp "$container_dir/amnezia" "$container_name:/opt/" 2>/dev/null || true
        fi
        
        # Start container
        sudo docker start "$container_name" 2>/dev/null || true
        
        echo "[INFO] $container_name restored"
    fi
done

# Cleanup
rm -rf "$TEMP_DIR"

echo "[INFO] Restore completed successfully"
)").arg(m_backupDir).arg(escapedFilename).arg(replaceMode ? "1" : "0");
}

QString ServersBackupController::getCheckStatusScript() const
{
    return QString(R"SCRIPT(
#!/bin/bash

BACKUP_DIR=%1

echo "Backup directory: $BACKUP_DIR"

# Check backup availability
BACKUPS=$(ls -t "$BACKUP_DIR"/backup_*.tar.gz 2>/dev/null | wc -l)
echo "Total backups: $BACKUPS"

if [ "$BACKUPS" -gt 0 ]; then
    # Last backup information
    LATEST=$(ls -t "$BACKUP_DIR"/backup_*.tar.gz 2>/dev/null | head -1)
    if [ -n "$LATEST" ]; then
    echo "Latest backup: $(basename "$LATEST")"
    echo "Size: $(du -h "$LATEST" | cut -f1)"
        echo "Modified: $(stat -c %y "$LATEST" 2>/dev/null | cut -d'.' -f1)"
    fi
fi

# Check running containers
echo "Running containers:"
sudo docker ps --filter "name=amnezia-" --format '{{.Names}}' 2>/dev/null

echo "Status check completed"
)SCRIPT").arg(m_backupDir);
}

QString ServersBackupController::getListBackupsScript() const
{
    return QString(R"(
#!/bin/bash

BACKUP_DIR=%1

ls -lht "$BACKUP_DIR"/backup_*.tar.gz 2>/dev/null || echo "No backups found"
)").arg(m_backupDir);
}

QList<ServersBackupController::BackupInfo> ServersBackupController::parseBackupList(const QString &output)
{
    QList<BackupInfo> backups;
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    // Parse ls -lht output
    QRegularExpression re("^-.*\\s+(\\d+)\\s+\\w+\\s+\\d+\\s+([\\d:]+)\\s+(.+backup_(\\d{8}_\\d{6})\\.tar\\.gz)$");
    
    for (const QString &line : lines) {
        QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
            BackupInfo info;
            info.size = match.captured(1).toLongLong();
            info.filename = QFileInfo(match.captured(3)).fileName();
            info.fullPath = match.captured(3);
            
            // Parse date from filename
            QString dateStr = match.captured(4);
            info.createdAt = QDateTime::fromString(dateStr, "yyyyMMdd_HHmmss");
            info.isValid = true;
            
            backups.append(info);
        }
    }
    
    return backups;
}

QJsonObject ServersBackupController::parseBackupStatus(const QString &output)
{
    QJsonObject status;
    
    // Parse text output
    status["raw_output"] = output;
    status["has_backups"] = output.contains("Total backups:");
    
    // Extract backup count
    QRegularExpression reTotal("Total backups: (\\d+)");
    QRegularExpressionMatch matchTotal = reTotal.match(output);
    if (matchTotal.hasMatch()) {
        status["total_backups"] = matchTotal.captured(1).toInt();
    }
    
    // Extract last backup information
    QRegularExpression reLatest("Latest backup: (.+)");
    QRegularExpressionMatch matchLatest = reLatest.match(output);
    if (matchLatest.hasMatch()) {
        status["latest_backup"] = matchLatest.captured(1);
    }
    
    return status;
}

// ============================================================================
// HELPER METHODS
// ============================================================================

ErrorCode ServersBackupController::handleStdOut(const QString &data, QString &output)
{
    output += data;
    qDebug().noquote() << "[BACKUP]" << data;
    
    // Check for errors in output
    if (data.contains("[ERROR]") || data.contains("ERROR")) {
        // Error detected in stdout, but not critical for handleStdOut
        // Main check will be in restoreBackup after script execution
    }
    
    // Update progress based on output
    if (data.contains("Starting backup")) {
        setProgress(10, tr("Starting backup..."));
    } else if (data.contains("Backing up")) {
        setProgress(50, tr("Backing up containers..."));
    } else if (data.contains("Backup created")) {
        setProgress(90, tr("Finalizing..."));
    } else if (data.contains("Starting restore")) {
        setProgress(10, tr("Starting restore..."));
    } else if (data.contains("Restoring")) {
        setProgress(50, tr("Restoring containers..."));
    } else if (data.contains("Restore completed successfully")) {
        setProgress(90, tr("Finalizing restore..."));
    }
    
    return ErrorCode::NoError;
}

ErrorCode ServersBackupController::handleStdErr(const QString &data, QString &error)
{
    error += data;
    qDebug().noquote() << "[BACKUP ERROR]" << data;
    return ErrorCode::NoError;
}

void ServersBackupController::setStatus(BackupStatus status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged(status);
    }
}

void ServersBackupController::setProgress(int percent, const QString &message)
{
    emit progressChanged(percent, message);
}

void ServersBackupController::createBackupWithDownload(bool downloadToDevice, bool deleteFromServer)
{
    qDebug() << "createBackupWithDownload: download=" << downloadToDevice << "delete=" << deleteFromServer;
    
    if (!m_serversModel) {
        emit errorOccurred(tr("ServersModel is not available"), ErrorCode::InternalError);
        return;
    }
    
    int serverIndex = m_serversModel->getProcessedServerIndex();
    if (serverIndex < 0) {
        emit errorOccurred(tr("No server selected"), ErrorCode::InternalError);
        return;
    }
    
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    
    // Set flags for automatic download and delete
    m_autoDownloadAfterCreate = downloadToDevice;
    m_autoDeleteAfterDownload = deleteFromServer;
    
    // Create backup
    createBackup(credentials);
}

QVariantMap ServersBackupController::getBackupFileInfo(const QString &backupFilePath)
{
    QVariantMap result;
    
    // Get filename
    QString fileName;
    
#ifdef Q_OS_ANDROID
    if (backupFilePath.startsWith("content://")) {
        fileName = AndroidController::instance()->getFileName(backupFilePath);
    }
#endif
#ifdef Q_OS_IOS
    if (backupFilePath.startsWith("file://")) {
        fileName = IosController::getFileName(backupFilePath);
    }
#endif
    
    if (fileName.isEmpty()) {
        QFileInfo fileInfo(backupFilePath);
        fileName = fileInfo.fileName();
    }
    
    // If filename is empty, use fallback
    if (fileName.isEmpty()) {
        QStringList pathParts = backupFilePath.split('/');
        if (!pathParts.isEmpty()) {
            fileName = pathParts.last();
        }
    }
    
    if (fileName.isEmpty()) {
        fileName = "backup.tgz";
    }
    
    // Extract IP from filename (format: 38_99_23_227 - DD-MM-YYYY_HH-MM-SS.tgz)
    QString serverIp;
    QRegularExpression ipRegex("^([\\d_]+)\\s*-");
    QRegularExpressionMatch match = ipRegex.match(fileName);
    if (match.hasMatch()) {
        serverIp = match.captured(1).replace('_', '.');
    }
    
    // If failed to extract IP, use empty string
    // QML will decide what to use
    
    result["fileName"] = fileName;
    result["serverIp"] = serverIp;
    
    qDebug() << "getBackupFileInfo:" << backupFilePath;
    qDebug() << "  fileName:" << fileName;
    qDebug() << "  serverIp:" << serverIp;
    
    return result;
}

void ServersBackupController::prepareRestoreFromBackup(const QString &backupFilePath,
                                                        const QString &hostname,
                                                        const QString &username,
                                                        const QString &secretData)
{
    qDebug() << "Preparing restore from backup:" << backupFilePath;
    qDebug() << "  hostname:" << hostname;
    qDebug() << "  username:" << username;
    
    // 1. Scan backup to determine containers
    QStringList containers = scanBackupForContainers(backupFilePath);
    
    if (containers.isEmpty()) {
        qWarning() << "No containers found in backup";
        emit errorOccurred(tr("No containers found in backup file"), ErrorCode::InternalError);
        return;
    }
    
    qDebug() << "Found containers in backup:" << containers;
    
    // 2. Extract information from filename
    QString fileName;
#ifdef Q_OS_ANDROID
    if (backupFilePath.startsWith("content://")) {
        fileName = AndroidController::instance()->getFileName(backupFilePath);
    }
#endif
#ifdef Q_OS_IOS
    if (backupFilePath.startsWith("file://")) {
        fileName = IosController::getFileName(backupFilePath);
    }
#endif
    
    if (fileName.isEmpty()) {
        QFileInfo fileInfo(backupFilePath);
        fileName = fileInfo.fileName();
    }
    
    // Extract IP from filename (format: 38_99_23_227 - DD-MM-YYYY_HH-MM-SS.tgz)
    QString serverIp = hostname; // Default to hostname
    QRegularExpression ipRegex("^([\\d_]+)\\s*-");
    QRegularExpressionMatch match = ipRegex.match(fileName);
    if (match.hasMatch()) {
        serverIp = match.captured(1).replace('_', '.');
    }
    
    qDebug() << "Extracted info - fileName:" << fileName << "serverIp:" << serverIp;
    
    // 3. Send signal with data for container installation
    // QML will receive this signal and install containers via InstallController
    emit readyForRestore(backupFilePath, hostname, username, secretData, serverIp, fileName);
}

void ServersBackupController::startRestore(bool isFromSetupWizard,
                                           const QString &backupFilePath,
                                           bool replaceMode,
                                           const QString &wizardHostname,
                                           const QString &wizardUsername,
                                           const QString &wizardSecretData)
{
    qDebug() << "Starting restore: isFromSetupWizard=" << isFromSetupWizard 
             << "replaceMode=" << replaceMode
             << "backupFilePath=" << backupFilePath;
    
    // Enable auto restore flag after upload
    m_autoRestoreAfterUpload = true;
    
    // If this is setup wizard with wizard credentials, use them directly
    if (isFromSetupWizard && !wizardHostname.isEmpty()) {
        qDebug() << "Setup wizard mode, using uploadBackupWithStrings";
        qDebug() << "  hostname:" << wizardHostname;
        qDebug() << "  username:" << wizardUsername;
        
        // Save credentials for subsequent restore
        m_pendingRestoreCredentials.hostName = wizardHostname;
        m_pendingRestoreCredentials.userName = wizardUsername;
        m_pendingRestoreCredentials.secretData = wizardSecretData;
        
        uploadBackupWithStrings(wizardHostname, wizardUsername, wizardSecretData, backupFilePath, replaceMode);
    } else {
        // Regular mode - get credentials from ServersModel
        qDebug() << "Regular mode, getting credentials from ServersModel";
        
        if (!m_serversModel) {
            qWarning() << "ServersModel is null";
            emit errorOccurred(tr("Internal error: ServersModel is not available"), ErrorCode::InternalError);
            return;
        }
        
        int serverIndex = m_serversModel->getProcessedServerIndex();
        qDebug() << "  ProcessedServerIndex:" << serverIndex;
        qDebug() << "  ServersCount:" << m_serversModel->getServersCount();
        
        if (serverIndex < 0) {
            qWarning() << "No processed server selected, trying default server";
            serverIndex = m_serversModel->getDefaultServerIndex();
            qDebug() << "  DefaultServerIndex:" << serverIndex;
            
            if (serverIndex < 0) {
                qWarning() << "No default server either";
                emit errorOccurred(tr("No server selected"), ErrorCode::InternalError);
                return;
            }
        }
        
        qDebug() << "  Using server index:" << serverIndex;
        ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
        
        // Save credentials for subsequent restore
        m_pendingRestoreCredentials = credentials;
        
        uploadBackup(credentials, backupFilePath, replaceMode);
    }
}

bool ServersBackupController::setDefaultServerAfterRestore(bool isFromSetupWizard)
{
    if (!m_serversModel) {
        qWarning() << "ServersModel is null, cannot set default server";
        return false;
    }
    
    if (m_serversModel->getServersCount() == 0) {
        qWarning() << "No servers in model";
        return false;
    }
    
    // For setup wizard, set last added server as default
    if (isFromSetupWizard) {
        int serverIdx = m_serversModel->getServersCount() - 1;
        qDebug() << "Setting default server after restore:" << serverIdx;
        m_serversModel->setDefaultServerIndex(serverIdx);
        m_serversModel->setProcessedServerIndex(serverIdx);
        
        // Reset retry counter
        m_containerRetryCount = 0;
        
        // Start timer to set default container
        QTimer::singleShot(500, this, &ServersBackupController::trySetDefaultContainer);
        
        return true;
    }
    
    return false;
}

void ServersBackupController::trySetDefaultContainer()
{
    if (!m_serversModel) {
        qWarning() << "ServersModel is null";
        emit defaultServerAndContainerSet();
        return;
    }
    
    int serverIdx = m_serversModel->getServersCount() - 1;
    qDebug() << "Timer: Setting default container (attempt" << m_containerRetryCount + 1 << "/" << m_maxContainerRetries << ")";
    
    // Get server configuration
    QJsonObject serverConfig = m_serversModel->getServerConfig(serverIdx);
    QJsonArray containers = serverConfig.value(config_key::containers).toArray();
    
    qDebug() << "  Total containers:" << containers.size();
    
    if (containers.size() > 0) {
        // Find first installed container
        for (int i = 0; i < containers.size(); i++) {
            QJsonObject containerObj = containers.at(i).toObject();
            
            // Check if container has any data (meaning it's installed)
            DockerContainer container = ContainerProps::containerFromString(containerObj.value(config_key::container).toString());
            if (container != DockerContainer::None && !containerObj.isEmpty()) {
                qDebug() << "  Setting default container at index:" << i << "for server:" << serverIdx;
                m_serversModel->setDefaultContainer(serverIdx, i);
                
                // Successfully set - send signal
                qDebug() << "  Default server and container set successfully";
                emit defaultServerAndContainerSet();
                return;
            }
        }
        
        // Containers exist, but none match - proceed anyway
        qDebug() << "  No suitable containers found, but model has data, proceeding";
        emit defaultServerAndContainerSet();
        return;
    }
    
    // Model empty - try again
    m_containerRetryCount++;
    if (m_containerRetryCount < m_maxContainerRetries) {
        qDebug() << "  Model is empty, will retry...";
        QTimer::singleShot(500, this, &ServersBackupController::trySetDefaultContainer);
    } else {
        qDebug() << "  Max retries reached, proceeding anyway";
        m_containerRetryCount = 0;
        emit defaultServerAndContainerSet();
    }
}

