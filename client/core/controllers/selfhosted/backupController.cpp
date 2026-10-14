#include "backupController.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QUrl>
#if !defined(Q_OS_IOS)
#include <QProcess>
#endif
#ifdef Q_OS_ANDROID
#include <QJniObject>
#include "platforms/android/android_controller.h"
#endif

#include "core/utils/containers/containerUtils.h"
#include "core/utils/errorStrings.h"
#include "core/utils/networkUtilities.h"
#include "core/utils/utilities.h"

namespace
{
    // Backup file name produced by the scripts: "IP_ADDRESS - DD-MM-YYYY_HH-MM-SS.tgz"
    const char *kBackupNamePattern = "([\\d_]+)\\s+-\\s+(\\d{2}-\\d{2}-\\d{4})_(\\d{2}-\\d{2}-\\d{2})\\.tgz";
}

BackupController::BackupController(QObject *parent) : QObject(parent), m_backupDir("/var/backups/amnezia")
{
}

void BackupController::setBackupDirectory(const QString &directory)
{
    m_backupDir = directory;
}

QString BackupController::backupDirectory() const
{
    return m_backupDir;
}

QString BackupController::lastError() const
{
    return m_lastError;
}

ErrorCode BackupController::createBackup(const ServerCredentials &credentials, QString &backupFilename)
{
    m_sshSession.resetConnection();
    emit progressChanged(0, tr("Starting backup creation..."));
    return runBackupScript(credentials, backupScript(ipForFilename(credentials.hostName)), backupFilename);
}

ErrorCode BackupController::createContainersBackup(const ServerCredentials &credentials,
                                                   const QList<DockerContainer> &containers, QString &backupFilename)
{
    m_sshSession.resetConnection();
    emit progressChanged(0, tr("Starting backup for %1 containers...").arg(containers.size()));
    return runBackupScript(credentials, containersBackupScript(containers, ipForFilename(credentials.hostName)),
                           backupFilename);
}

ErrorCode BackupController::runBackupScript(const ServerCredentials &credentials, const QString &script,
                                            QString &backupFilename)
{
    ErrorCode error = runHostScript(credentials, script);
    if (error != ErrorCode::NoError) {
        m_lastError = m_error.isEmpty() ? errorString(error) : m_error;
        return error;
    }

    QRegularExpressionMatch match = QRegularExpression(kBackupNamePattern).match(m_output);
    if (!match.hasMatch()) {
        m_lastError = tr("Failed to parse backup filename from output: %1").arg(m_output.left(200));
        return ErrorCode::InternalError;
    }

    backupFilename = match.captured(1) + " - " + match.captured(2) + "_" + match.captured(3) + ".tgz";
    emit progressChanged(100, tr("Backup created successfully"));
    return ErrorCode::NoError;
}

ErrorCode BackupController::restoreBackup(const ServerCredentials &credentials, const QString &backupFilename,
                                          bool replaceMode)
{
    emit progressChanged(0, tr("Starting restore from %1...").arg(backupFilename));

    ErrorCode error = runHostScript(credentials, restoreScript(backupFilename, replaceMode));

    // The script may exit with 0 and still report a failure, so inspect its output too
    const bool hasError = m_output.contains("[ERROR]") || m_error.contains("[ERROR]")
            || m_output.contains("Failed to extract backup") || m_error.contains("Failed to extract backup");
    if (error != ErrorCode::NoError || hasError) {
        m_lastError = hasError ? m_output + "\n" + m_error : (m_error.isEmpty() ? errorString(error) : m_error);
        return error != ErrorCode::NoError ? error : ErrorCode::InternalError;
    }

    if (!m_output.contains("Restore completed successfully")) {
        m_lastError = tr("Backup restore did not complete successfully. Output: %1").arg(m_output);
        return ErrorCode::InternalError;
    }

    emit progressChanged(100, tr("Backup restored successfully"));
    return ErrorCode::NoError;
}

ErrorCode BackupController::downloadBackup(const ServerCredentials &credentials, const QString &backupFilename,
                                           const QString &localPath)
{
    if (backupFilename.isEmpty()) {
        m_lastError = tr("Backup filename is empty");
        return ErrorCode::InternalError;
    }

    emit progressChanged(0, tr("Downloading backup..."));

    QDir localDir = QFileInfo(localPath).dir();
    if (!localDir.exists() && !localDir.mkpath(".")) {
        m_lastError = tr("Failed to create local directory: %1").arg(localDir.path());
        return ErrorCode::InternalError;
    }

    libssh::Client sshClient;
    ErrorCode error = sshClient.connectToHost(credentials);
    if (error == ErrorCode::NoError) {
        error = sshClient.scpFileDownload(QString("%1/%2").arg(m_backupDir, backupFilename), localPath);
        sshClient.disconnectFromHost();
    }

    if (error != ErrorCode::NoError) {
        m_lastError = errorString(error);
        return error;
    }

    emit progressChanged(100, tr("Backup downloaded successfully"));
    return ErrorCode::NoError;
}

ErrorCode BackupController::uploadBackup(const ServerCredentials &credentials, const QString &localPath,
                                         QString &remoteFilename)
{
    m_sshSession.resetConnection();

    QByteArray data;
    ErrorCode error = readLocalFile(localPath, data);
    if (error != ErrorCode::NoError) {
        return error;
    }

    QString filename = fileNameFromPath(localPath);
    if (filename.isEmpty()) {
        filename = "backup.tgz";
    }
    // libssh passes the scp path to the remote shell verbatim, spaces would split the argument
    filename.replace(' ', '_');

    emit progressChanged(0, tr("Uploading backup..."));

    // The backup directory is root-owned, so upload to /tmp first and move with sudo
    const QString tmpRemotePath = QString("/tmp/amnezia_restore_%1.tgz").arg(Utils::getRandomString(8));
    const QString remotePath = QString("%1/%2").arg(m_backupDir, filename);

    error = m_sshSession.uploadFileToHost(credentials, data, tmpRemotePath);
    if (error != ErrorCode::NoError) {
        m_lastError = errorString(error);
        return error;
    }

    error = runHostScript(credentials,
                          QString("mkdir -p \"%1\" && mv \"%2\" \"%3\"").arg(m_backupDir, tmpRemotePath, remotePath));
    if (error != ErrorCode::NoError) {
        const QString moveError = m_error;
        runHostScript(credentials, QString("rm -f \"%1\"").arg(tmpRemotePath));
        m_lastError = moveError.isEmpty() ? errorString(error) : moveError;
        return error;
    }

    remoteFilename = filename;
    emit progressChanged(100, tr("Backup uploaded successfully"));
    return ErrorCode::NoError;
}

ErrorCode BackupController::deleteBackup(const ServerCredentials &credentials, const QString &backupFilename)
{
    emit progressChanged(0, tr("Deleting backup..."));

    QString escapedFilename = backupFilename;
    escapedFilename.replace("'", "'\\''");

    ErrorCode error = runHostScript(credentials, QString("sudo rm -f '%1/%2'").arg(m_backupDir, escapedFilename));
    if (error != ErrorCode::NoError) {
        m_lastError = m_error.isEmpty() ? errorString(error) : m_error;
        return error;
    }

    emit progressChanged(100, tr("Backup deleted"));
    return ErrorCode::NoError;
}

QStringList BackupController::scanBackupForContainers(const QString &localPath)
{
    QStringList containers;

#ifdef Q_OS_IOS
    Q_UNUSED(localPath);
    // The iOS sandbox cannot spawn tar; the server-side restore only touches what is in the archive anyway
    containers << QStringLiteral("amnezia-awg2") << QStringLiteral("amnezia-wireguard") << QStringLiteral("amnezia-xray")
               << QStringLiteral("amnezia-openvpn-cloak") << QStringLiteral("amnezia-awg");
    return containers;
#else
    QString actualPath = localPath;

#ifdef Q_OS_ANDROID
    if (localPath.startsWith("content://")) {
        QByteArray data;
        if (readLocalFile(localPath, data) != ErrorCode::NoError) {
            return containers;
        }

        actualPath = QDir::temp().filePath("backup_scan_temp.tgz");
        QFile tempFile(actualPath);
        if (!tempFile.open(QIODevice::WriteOnly)) {
            qWarning() << "Failed to create temp file for scanning";
            return containers;
        }
        tempFile.write(data);
        tempFile.close();
    }
#endif

    QProcess process;
    process.start("tar", QStringList() << "-tzf" << actualPath);
    process.waitForFinished(5000);

    if (process.exitCode() != 0) {
        qWarning() << "Failed to read backup archive:" << process.readAllStandardError();
        return containers;
    }

    QSet<QString> foundContainers;
    const QStringList lines = QString(process.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QStringList parts = line.split('/');
        for (const QString &part : parts) {
            if (part.startsWith("amnezia-")) {
                foundContainers.insert(part);
                break;
            }
        }
    }

    containers = foundContainers.values();
    qDebug() << "Found containers in backup:" << containers;
    return containers;
#endif
}

BackupController::BackupFileInfo BackupController::backupFileInfo(const QString &backupFilePath)
{
    BackupFileInfo info;
    info.fileName = fileNameFromPath(backupFilePath);
    if (info.fileName.isEmpty()) {
        info.fileName = "backup.tgz";
    }

    QRegularExpressionMatch match = QRegularExpression("^([\\d_]+)\\s*-").match(info.fileName);
    if (match.hasMatch()) {
        info.serverIp = match.captured(1).replace('_', '.');
    }
    return info;
}

QString BackupController::defaultDownloadPath(const QString &backupFilename)
{
#ifdef Q_OS_ANDROID
    QJniObject mediaDir = QJniObject::callStaticObjectMethod(
            "android/os/Environment", "getExternalStoragePublicDirectory", "(Ljava/lang/String;)Ljava/io/File;",
            QJniObject::getStaticObjectField("android/os/Environment", "DIRECTORY_DOWNLOADS", "Ljava/lang/String;")
                    .object());
    const QString downloadsPath = mediaDir.callObjectMethod("getAbsolutePath", "()Ljava/lang/String;").toString();
    return QDir(downloadsPath).filePath(backupFilename);
#elif defined(Q_OS_IOS)
    return QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(backupFilename);
#else
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (documentsPath.isEmpty()) {
        documentsPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    }
    return QDir(documentsPath).filePath(backupFilename);
#endif
}

ErrorCode BackupController::runHostScript(const ServerCredentials &credentials, const QString &script)
{
    m_output.clear();
    m_error.clear();

    const QString fileName = "/tmp/amnezia_" + Utils::getRandomString(16) + ".sh";
    ErrorCode error = m_sshSession.uploadFileToHost(credentials, script.toUtf8(), fileName);
    if (error != ErrorCode::NoError) {
        return error;
    }

    auto onStdOut = [this](const QString &data, libssh::Client &) -> ErrorCode {
        m_output += data;
        qDebug().noquote() << "[BACKUP]" << data;

        if (data.contains("Starting backup")) {
            emit progressChanged(10, tr("Starting backup..."));
        } else if (data.contains("Backing up")) {
            emit progressChanged(50, tr("Backing up containers..."));
        } else if (data.contains("Backup created")) {
            emit progressChanged(90, tr("Finalizing..."));
        } else if (data.contains("Starting restore")) {
            emit progressChanged(10, tr("Starting restore..."));
        } else if (data.contains("Restoring")) {
            emit progressChanged(50, tr("Restoring containers..."));
        } else if (data.contains("Restore completed successfully")) {
            emit progressChanged(90, tr("Finalizing restore..."));
        }
        return ErrorCode::NoError;
    };

    auto onStdErr = [this](const QString &data, libssh::Client &) -> ErrorCode {
        m_error += data;
        qDebug().noquote() << "[BACKUP ERROR]" << data;
        return ErrorCode::NoError;
    };

    error = m_sshSession.runScript(credentials, QString("sudo bash %1").arg(fileName), onStdOut, onStdErr);
    m_sshSession.runScript(credentials, QString("sudo rm -f %1").arg(fileName));
    return error;
}

ErrorCode BackupController::readLocalFile(const QString &localPath, QByteArray &data)
{
#ifdef Q_OS_ANDROID
    if (localPath.startsWith("content://")) {
        const int fd = AndroidController::instance()->getFd(localPath);
        if (fd < 0) {
            m_lastError = tr("Failed to read backup file from Android storage");
            return ErrorCode::ReadError;
        }

        QFile file;
        const bool opened = file.open(fd, QIODevice::ReadOnly);
        if (opened) {
            data = file.readAll();
        }
        AndroidController::instance()->closeFd();

        if (!opened) {
            m_lastError = tr("Failed to read backup file from Android storage");
            return ErrorCode::ReadError;
        }
        return ErrorCode::NoError;
    }
#endif

    QFileInfo fileInfo(localPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        m_lastError = tr("Local file does not exist: %1").arg(localPath);
        return ErrorCode::InternalError;
    }

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = tr("Failed to read backup file: %1").arg(localPath);
        return ErrorCode::ReadError;
    }
    data = file.readAll();
    return ErrorCode::NoError;
}

QString BackupController::fileNameFromPath(const QString &path)
{
    QString fileName;

#ifdef Q_OS_ANDROID
    if (path.startsWith("content://")) {
        fileName = AndroidController::instance()->getFileName(path);
    }
#endif

    if (fileName.isEmpty()) {
        fileName = QFileInfo(path).fileName();
    }

    if (fileName.isEmpty()) {
        const QString localPath = path.startsWith("file://") ? QUrl(path).toLocalFile() : path;
        const QStringList parts = localPath.split('/');
        if (!parts.isEmpty()) {
            fileName = QUrl::fromPercentEncoding(parts.last().toUtf8());
        }
    }

    return fileName;
}

QString BackupController::ipForFilename(const QString &hostName)
{
    QString serverIp = NetworkUtilities::getIPAddress(hostName);
    if (serverIp.isEmpty()) {
        serverIp = hostName;
    }
    return serverIp.replace(".", "_");
}

QString BackupController::backupScript(const QString &ipAddress) const
{
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

mkdir -p "$BACKUP_SUBDIR"

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

for container in "${CONTAINERS[@]}"; do
    if sudo docker ps -a --format '{{.Names}}' | grep -q "^$container$"; then
        echo "[INFO] Backing up $container..."
        mkdir -p "$BACKUP_SUBDIR/$container"
        sudo docker cp "$container:/opt/amnezia" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
        sudo docker inspect "$container" > "$BACKUP_SUBDIR/$container/container_inspect.json" 2>/dev/null || true
    fi
done

cd "$BACKUP_DIR"
tar -czf "$BACKUP_FILENAME" -C "$BACKUP_SUBDIR" . 2>/dev/null
rm -rf "$BACKUP_SUBDIR"

echo "[INFO] Backup created: $BACKUP_FILENAME"
)")
            .arg(m_backupDir, ipAddress);
}

QString BackupController::containersBackupScript(const QList<DockerContainer> &containers, const QString &ipAddress) const
{
    QStringList names;
    for (const DockerContainer &container : containers) {
        names << QString("\"%1\"").arg(ContainerUtils::containerToString(container));
    }

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

mkdir -p "$BACKUP_SUBDIR"

CONTAINERS=(%3)

for container in "${CONTAINERS[@]}"; do
    if sudo docker ps -a --format '{{.Names}}' | grep -q "^$container$"; then
        echo "[INFO] Backing up $container..."
        mkdir -p "$BACKUP_SUBDIR/$container"
        sudo docker cp "$container:/opt/amnezia" "$BACKUP_SUBDIR/$container/" 2>/dev/null || {
            echo "[WARN] Failed to copy /opt/amnezia from $container, trying alternative paths..."
            sudo docker cp "$container:/etc/openvpn" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
            sudo docker cp "$container:/etc/wireguard" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
            sudo docker cp "$container:/etc/ipsec.d" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
            sudo docker cp "$container:/etc/xray" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
        }
        sudo docker inspect "$container" > "$BACKUP_SUBDIR/$container/container_inspect.json" 2>/dev/null || true
    else
        echo "[WARN] Container $container does not exist, skipping..."
    fi
done

cd "$BACKUP_DIR"
tar -czf "$BACKUP_FILENAME" -C "$BACKUP_SUBDIR" . 2>/dev/null
rm -rf "$BACKUP_SUBDIR"

echo "[INFO] Backup created: $BACKUP_FILENAME"
)")
            .arg(m_backupDir, ipAddress, names.join(' '));
}

QString BackupController::restoreScript(const QString &backupFilename, bool replaceMode) const
{
    QString escapedFilename = backupFilename;
    escapedFilename.replace("'", "'\\''");

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

if [ ! -f "$BACKUP_DIR/$BACKUP_FILE" ]; then
    echo "[ERROR] Backup file not found: $BACKUP_DIR/$BACKUP_FILE"
    exit 1
fi

mkdir -p "$TEMP_DIR"
EXTRACT_OUTPUT=$(tar -xzf "$BACKUP_DIR/$BACKUP_FILE" -C "$TEMP_DIR" 2>&1)
EXTRACT_EXIT_CODE=$?
if [ $EXTRACT_EXIT_CODE -ne 0 ]; then
    echo "[ERROR] Failed to extract backup archive: $EXTRACT_OUTPUT"
    rm -rf "$TEMP_DIR"
    exit 1
fi

# Archive root is either backup_temp_* or the container directories themselves
BACKUP_SUBDIR=$(ls -d "$TEMP_DIR"/backup_* 2>/dev/null | head -1)
if [ -z "$BACKUP_SUBDIR" ]; then
    if ls -d "$TEMP_DIR"/amnezia-* 2>/dev/null | head -1 > /dev/null; then
        BACKUP_SUBDIR="$TEMP_DIR"
    else
        echo "[ERROR] Failed to extract backup: backup directory not found in archive"
        ls -la "$TEMP_DIR" || true
        rm -rf "$TEMP_DIR"
        exit 1
    fi
fi

for container_dir in "$BACKUP_SUBDIR"/*; do
    if [ ! -d "$container_dir" ]; then
        continue
    fi

    container_name=$(basename "$container_dir")

    if sudo docker ps -a --format '{{.Names}}' | grep -q "^$container_name$"; then
        echo "[INFO] Restoring $container_name..."

        sudo docker stop "$container_name" 2>/dev/null || true

        if [ "$REPLACE_MODE" = "1" ]; then
            echo "[INFO] Clearing container $container_name before restore..."
            TEMP_CLEAR_DIR="/tmp/clear_amnezia_$$"
            mkdir -p "$TEMP_CLEAR_DIR/amnezia"
            sudo docker cp "$TEMP_CLEAR_DIR/amnezia" "$container_name:/opt/" 2>/dev/null || true
            rm -rf "$TEMP_CLEAR_DIR"
        fi

        if [ -d "$container_dir/amnezia" ]; then
            sudo docker cp "$container_dir/amnezia" "$container_name:/opt/" 2>/dev/null || true
        fi

        sudo docker start "$container_name" 2>/dev/null || true

        echo "[INFO] $container_name restored"
    fi
done

rm -rf "$TEMP_DIR"

echo "[INFO] Restore completed successfully"
)")
            .arg(m_backupDir, escapedFilename, replaceMode ? "1" : "0");
}
