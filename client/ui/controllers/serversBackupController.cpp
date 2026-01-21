#include "serversBackupController.h"
#include <QDebug>
#include <QDir>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QStandardPaths>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif
#ifdef Q_OS_IOS
#include "platforms/ios/ios_controller.h"
#endif
#include "containers/containers_defs.h"

ServersBackupController::ServersBackupController(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_serverController(new ServerController(settings, this))
    , m_status(Idle)
    , m_backupDir("/var/backups/amnezia")
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

    // Получаем bash скрипт для backup
    QString script = getBackupScript();

    // Callback для обработки stdout
    auto cbStdOut = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdOut(data, m_currentOutput);
    };

    // Callback для обработки stderr
    auto cbStdErr = [this](const QString &data, libssh::Client &client) -> ErrorCode {
        Q_UNUSED(client);
        return handleStdErr(data, m_currentError);
    };

    // Запускаем скрипт на сервере
    ErrorCode error = m_serverController->runHostScript(credentials, script, cbStdOut, cbStdErr);

    if (error == ErrorCode::NoError) {
        // Парсим имя созданного backup из вывода
        QRegularExpression re("backup_(\\d{8}_\\d{6})\\.tar\\.gz");
        QRegularExpressionMatch match = re.match(m_currentOutput);
        
        if (match.hasMatch()) {
            QString backupFilename = "backup_" + match.captured(1) + ".tar.gz";
            setStatus(Success);
            setProgress(100, tr("Backup created successfully"));
            emit backupCreated(backupFilename);
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

    QString script = getContainerBackupScript(container);

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
        QRegularExpression re("backup_(\\d{8}_\\d{6})\\.tar\\.gz");
        QRegularExpressionMatch match = re.match(m_currentOutput);
        
        if (match.hasMatch()) {
            QString backupFilename = "backup_" + match.captured(1) + ".tar.gz";
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

    QString script = getContainersBackupScript(containers);

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
        QRegularExpression re("backup_(\\d{8}_\\d{6})\\.tar\\.gz");
        QRegularExpressionMatch match = re.match(m_currentOutput);
        
        if (match.hasMatch()) {
            QString backupFilename = "backup_" + match.captured(1) + ".tar.gz";
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
                                           const QStringList &containers)
{
    if (m_status == InProgress) {
        emit errorOccurred("Another operation is in progress", ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    setStatus(InProgress);
    setProgress(0, tr("Starting restore from %1...").arg(backupFilename));

    m_currentOutput.clear();
    m_currentError.clear();

    QString script = getRestoreScript(backupFilename, containers);

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
        setProgress(100, tr("Backup restored successfully"));
        emit backupRestored();
    } else {
        setStatus(Failed);
        emit errorOccurred(tr("Failed to restore backup: %1").arg(m_currentError), error);
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
        // На Android используем публичную папку Download (через JNI)
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
        // На Desktop используем Documents (как обычный backup)
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
        emit backupDownloaded(actualLocalPath);
    } else {
        setStatus(Failed);
        emit errorOccurred(tr("Failed to download backup: error code %1").arg(static_cast<int>(error)), error);
    }
}

void ServersBackupController::uploadBackup(const ServerCredentials &credentials,
                                          const QString &localPath)
{
    if (m_status == InProgress) {
        emit errorOccurred("Another operation is in progress", ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    // Check if local file exists
    QFileInfo localFileInfo(localPath);
    if (!localFileInfo.exists()) {
        emit errorOccurred(tr("Local file does not exist: %1").arg(localPath), ErrorCode::InternalError);
        return;
    }

    if (!localFileInfo.isFile()) {
        emit errorOccurred(tr("Path is not a file: %1").arg(localPath), ErrorCode::InternalError);
        return;
    }

    setStatus(InProgress);
    setProgress(0, tr("Uploading backup..."));

    // Construct remote file path with filename from local path
    QString filename = localFileInfo.fileName();
    QString remotePath = QString("%1/%2").arg(m_backupDir, filename);

    setProgress(25, tr("Starting file transfer..."));

    ErrorCode error = m_serverController->uploadFileToHostPublic(credentials, localPath, remotePath,
                                                                  libssh::ScpOverwriteMode::ScpOverwriteExisting);

    if (error == ErrorCode::NoError) {
        setStatus(Success);
        setProgress(100, tr("Backup uploaded successfully"));
        emit backupUploaded(remotePath);
    } else {
        setStatus(Failed);
        emit errorOccurred(tr("Failed to upload backup: error code %1").arg(static_cast<int>(error)), error);
    }
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

    QString script = QString("sudo rm -f %1/%2").arg(m_backupDir).arg(backupFilename);

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
// ВСТРОЕННЫЕ BASH СКРИПТЫ
// ============================================================================

QString ServersBackupController::getBackupScript() const
{
    // Упрощенная версия bash скрипта, встроенная в C++
    return QString(R"(
#!/bin/bash
set -e

BACKUP_DIR=%1
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_SUBDIR="$BACKUP_DIR/backup_$TIMESTAMP"

echo "[INFO] Starting backup..."

# Создание директории
mkdir -p "$BACKUP_SUBDIR"

# Список контейнеров Amnezia
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

# Backup каждого контейнера (включая остановленные)
for container in "${CONTAINERS[@]}"; do
    if sudo docker ps -a --format '{{.Names}}' | grep -q "^$container$"; then
        echo "[INFO] Backing up $container..."
        mkdir -p "$BACKUP_SUBDIR/$container"
        
        # Копируем /opt/amnezia
        sudo docker cp "$container:/opt/amnezia" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
        
        # Сохраняем метаданные
        sudo docker inspect "$container" > "$BACKUP_SUBDIR/$container/container_inspect.json" 2>/dev/null || true
    fi
done

# Создание архива
cd "$BACKUP_DIR"
tar -czf "backup_$TIMESTAMP.tar.gz" "backup_$TIMESTAMP" 2>/dev/null
rm -rf "$BACKUP_SUBDIR"

echo "[INFO] Backup created: backup_$TIMESTAMP.tar.gz"
)").arg(m_backupDir);
}

QString ServersBackupController::getContainerBackupScript(DockerContainer container) const
{
    QString containerName = ContainerProps::containerToString(container);
    
    // Backup конкретного контейнера напрямую через docker cp
    return QString(R"(
#!/bin/bash
set -e

BACKUP_DIR=%1
CONTAINER_NAME=%2
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_SUBDIR="$BACKUP_DIR/backup_$TIMESTAMP"

echo "[INFO] Starting backup for container: $CONTAINER_NAME..."

# Проверка существования контейнера
if ! sudo docker ps -a --format '{{.Names}}' | grep -q "^$CONTAINER_NAME$"; then
    echo "[ERROR] Container $CONTAINER_NAME does not exist"
    exit 1
fi

# Создание директории
mkdir -p "$BACKUP_SUBDIR/$CONTAINER_NAME"

# Backup конфигураций из контейнера напрямую
echo "[INFO] Copying /opt/amnezia from container..."
sudo docker cp "$CONTAINER_NAME:/opt/amnezia" "$BACKUP_SUBDIR/$CONTAINER_NAME/" 2>/dev/null || {
    echo "[WARN] Failed to copy /opt/amnezia, trying alternative paths..."
    # Альтернативные пути для разных типов контейнеров
    sudo docker cp "$CONTAINER_NAME:/etc/openvpn" "$BACKUP_SUBDIR/$CONTAINER_NAME/" 2>/dev/null || true
    sudo docker cp "$CONTAINER_NAME:/etc/wireguard" "$BACKUP_SUBDIR/$CONTAINER_NAME/" 2>/dev/null || true
    sudo docker cp "$CONTAINER_NAME:/etc/ipsec.d" "$BACKUP_SUBDIR/$CONTAINER_NAME/" 2>/dev/null || true
    sudo docker cp "$CONTAINER_NAME:/etc/xray" "$BACKUP_SUBDIR/$CONTAINER_NAME/" 2>/dev/null || true
}

# Сохранение метаданных контейнера
echo "[INFO] Saving container metadata..."
sudo docker inspect "$CONTAINER_NAME" > "$BACKUP_SUBDIR/$CONTAINER_NAME/container_inspect.json" 2>/dev/null || true

# Сохранение конфигурации сети
sudo docker network inspect $(sudo docker inspect -f '{{range $k, $v := .NetworkSettings.Networks}}{{$k}} {{end}}' "$CONTAINER_NAME" | awk '{print $1}') \
    > "$BACKUP_SUBDIR/$CONTAINER_NAME/network_config.json" 2>/dev/null || true

# Создание архива
cd "$BACKUP_DIR"
tar -czf "backup_$TIMESTAMP.tar.gz" "backup_$TIMESTAMP" 2>/dev/null
rm -rf "$BACKUP_SUBDIR"

echo "[INFO] Backup created: backup_$TIMESTAMP.tar.gz"
)").arg(m_backupDir).arg(containerName);
}

QString ServersBackupController::getContainersBackupScript(const QList<DockerContainer> &containers) const
{
    QString containersList;
    for (const DockerContainer &container : containers) {
        QString containerName = ContainerProps::containerToString(container);
        containersList += QString("\"%1\" ").arg(containerName);
    }
    
    return QString(R"(
#!/bin/bash
set -e

BACKUP_DIR=%1
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_SUBDIR="$BACKUP_DIR/backup_$TIMESTAMP"

echo "[INFO] Starting backup for containers..."

# Создание директории
mkdir -p "$BACKUP_SUBDIR"

# Список контейнеров для backup
CONTAINERS=(%2)

# Backup каждого контейнера
for container in "${CONTAINERS[@]}"; do
    if sudo docker ps -a --format '{{.Names}}' | grep -q "^$container$"; then
        echo "[INFO] Backing up $container..."
        mkdir -p "$BACKUP_SUBDIR/$container"
        
        # Копируем /opt/amnezia напрямую из контейнера
        sudo docker cp "$container:/opt/amnezia" "$BACKUP_SUBDIR/$container/" 2>/dev/null || {
            echo "[WARN] Failed to copy /opt/amnezia from $container, trying alternative paths..."
            sudo docker cp "$container:/etc/openvpn" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
            sudo docker cp "$container:/etc/wireguard" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
            sudo docker cp "$container:/etc/ipsec.d" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
            sudo docker cp "$container:/etc/xray" "$BACKUP_SUBDIR/$container/" 2>/dev/null || true
        }
        
        # Сохраняем метаданные
        sudo docker inspect "$container" > "$BACKUP_SUBDIR/$container/container_inspect.json" 2>/dev/null || true
    else
        echo "[WARN] Container $container does not exist, skipping..."
    fi
done

# Создание архива
cd "$BACKUP_DIR"
tar -czf "backup_$TIMESTAMP.tar.gz" "backup_$TIMESTAMP" 2>/dev/null
rm -rf "$BACKUP_SUBDIR"

echo "[INFO] Backup created: backup_$TIMESTAMP.tar.gz"
)").arg(m_backupDir).arg(containersList.trimmed());
}

QString ServersBackupController::getRestoreScript(const QString &backupFilename, 
                                                  const QStringList &containers) const
{
    Q_UNUSED(containers); // TODO: Использовать для выборочного восстановления
    
    return QString(R"(
#!/bin/bash
set -e

BACKUP_DIR=%1
BACKUP_FILE=%2
TEMP_DIR="/tmp/amnezia_restore_$$"

echo "[INFO] Starting restore from $BACKUP_FILE..."

# Извлечение backup
mkdir -p "$TEMP_DIR"
tar -xzf "$BACKUP_DIR/$BACKUP_FILE" -C "$TEMP_DIR" 2>/dev/null

BACKUP_SUBDIR=$(ls -d "$TEMP_DIR"/backup_* 2>/dev/null | head -1)

if [ -z "$BACKUP_SUBDIR" ]; then
    echo "[ERROR] Failed to extract backup"
    exit 1
fi

# Восстановление каждого контейнера
for container_dir in "$BACKUP_SUBDIR"/*; do
    if [ ! -d "$container_dir" ]; then
        continue
    fi
    
    container_name=$(basename "$container_dir")
    
    if sudo docker ps -a --format '{{.Names}}' | grep -q "^$container_name$"; then
        echo "[INFO] Restoring $container_name..."
        
        # Остановка контейнера
        sudo docker stop "$container_name" 2>/dev/null || true
        
        # Восстановление /opt/amnezia
        if [ -d "$container_dir/amnezia" ]; then
            sudo docker cp "$container_dir/amnezia" "$container_name:/opt/" 2>/dev/null || true
        fi
        
        # Запуск контейнера
        sudo docker start "$container_name" 2>/dev/null || true
        
        echo "[INFO] $container_name restored"
    fi
done

# Очистка
rm -rf "$TEMP_DIR"

echo "[INFO] Restore completed successfully"
)").arg(m_backupDir).arg(backupFilename);
}

QString ServersBackupController::getCheckStatusScript() const
{
    return QString(R"SCRIPT(
#!/bin/bash

BACKUP_DIR=%1

echo "Backup directory: $BACKUP_DIR"

# Проверка наличия backup
BACKUPS=$(ls -t "$BACKUP_DIR"/backup_*.tar.gz 2>/dev/null | wc -l)
echo "Total backups: $BACKUPS"

if [ "$BACKUPS" -gt 0 ]; then
    # Информация о последнем backup
    LATEST=$(ls -t "$BACKUP_DIR"/backup_*.tar.gz 2>/dev/null | head -1)
    if [ -n "$LATEST" ]; then
    echo "Latest backup: $(basename "$LATEST")"
    echo "Size: $(du -h "$LATEST" | cut -f1)"
        echo "Modified: $(stat -c %y "$LATEST" 2>/dev/null | cut -d'.' -f1)"
    fi
fi

# Проверка запущенных контейнеров
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

// ============================================================================
// ПАРСИНГ ВЫВОДА
// ============================================================================

QList<ServersBackupController::BackupInfo> ServersBackupController::parseBackupList(const QString &output)
{
    QList<BackupInfo> backups;
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    // Парсим вывод ls -lht
    QRegularExpression re("^-.*\\s+(\\d+)\\s+\\w+\\s+\\d+\\s+([\\d:]+)\\s+(.+backup_(\\d{8}_\\d{6})\\.tar\\.gz)$");
    
    for (const QString &line : lines) {
        QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
            BackupInfo info;
            info.size = match.captured(1).toLongLong();
            info.filename = QFileInfo(match.captured(3)).fileName();
            info.fullPath = match.captured(3);
            
            // Парсим дату из имени файла
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
    
    // Парсим текстовый вывод
    status["raw_output"] = output;
    status["has_backups"] = output.contains("Total backups:");
    
    // Извлекаем количество backup
    QRegularExpression reTotal("Total backups: (\\d+)");
    QRegularExpressionMatch matchTotal = reTotal.match(output);
    if (matchTotal.hasMatch()) {
        status["total_backups"] = matchTotal.captured(1).toInt();
    }
    
    // Извлекаем информацию о последнем backup
    QRegularExpression reLatest("Latest backup: (.+)");
    QRegularExpressionMatch matchLatest = reLatest.match(output);
    if (matchLatest.hasMatch()) {
        status["latest_backup"] = matchLatest.captured(1);
    }
    
    return status;
}

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
// ============================================================================

ErrorCode ServersBackupController::handleStdOut(const QString &data, QString &output)
{
    output += data;
    qDebug().noquote() << "[BACKUP]" << data;
    
    // Обновляем прогресс на основе вывода
    if (data.contains("Starting backup")) {
        setProgress(10, tr("Starting backup..."));
    } else if (data.contains("Backing up")) {
        setProgress(50, tr("Backing up containers..."));
    } else if (data.contains("Backup created")) {
        setProgress(90, tr("Finalizing..."));
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

