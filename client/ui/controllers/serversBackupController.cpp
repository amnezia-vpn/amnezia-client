#include "serversBackupController.h"
#include <QDebug>
#include <QDir>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QUrl>
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

ServersBackupController::ServersBackupController(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_serverController(new ServerController(settings, this))
    , m_status(Idle)
    , m_backupDir("/var/backups/amnezia")
    , m_restoreReplaceMode(false)
    , m_tempUploadFile(nullptr)
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

    // Получаем IP адрес сервера
    QString serverIp = NetworkUtilities::getIPAddress(credentials.hostName);
    if (serverIp.isEmpty()) {
        serverIp = credentials.hostName;
    }
    // Форматируем IP: заменяем точки на подчеркивания
    QString ipFormatted = serverIp;
    ipFormatted.replace(".", "_");

    // Получаем bash скрипт для backup с IP адресом
    QString script = getBackupScript(ipFormatted);

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
        // Парсим имя созданного backup из вывода: формат "IP_ADDRESS - DD-MM-YYYY_HH-MM-SS.tgz"
        QRegularExpression re("([\\d_]+)\\s+-\\s+(\\d{2}-\\d{2}-\\d{4})_(\\d{2}-\\d{2}-\\d{2})\\.tgz");
        QRegularExpressionMatch match = re.match(m_currentOutput);
        
        if (match.hasMatch()) {
            QString backupFilename = match.captured(1) + " - " + match.captured(2) + "_" + match.captured(3) + ".tgz";
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

    // Получаем IP адрес сервера
    QString serverIp = NetworkUtilities::getIPAddress(credentials.hostName);
    if (serverIp.isEmpty()) {
        serverIp = credentials.hostName;
    }
    // Форматируем IP: заменяем точки на подчеркивания
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
        // Парсим имя созданного backup из вывода: формат "IP_ADDRESS - DD-MM-YYYY_HH-MM-SS.tgz"
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

    // Получаем IP адрес сервера
    QString serverIp = NetworkUtilities::getIPAddress(credentials.hostName);
    if (serverIp.isEmpty()) {
        serverIp = credentials.hostName;
    }
    // Форматируем IP: заменяем точки на подчеркивания
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
        // Парсим имя созданного backup из вывода: формат "IP_ADDRESS - DD-MM-YYYY_HH-MM-SS.tgz"
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

    // Проверяем вывод на наличие ошибок, даже если скрипт завершился с кодом 0
    bool hasError = m_currentOutput.contains("[ERROR]") || 
                    m_currentOutput.contains("Failed to extract backup") ||
                    m_currentError.contains("[ERROR]") ||
                    m_currentError.contains("Failed to extract backup");

    if (error == ErrorCode::NoError && !hasError) {
        // Проверяем, что восстановление действительно завершилось успешно
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
                                          const QString &localPath,
                                          bool replaceMode)
{
    if (m_status == InProgress) {
        emit errorOccurred("Another operation is in progress", ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    // Сохраняем режим восстановления для последующего использования
    m_restoreReplaceMode = replaceMode;

    QString actualLocalPath = localPath;
    QString filename;
    
#ifdef Q_OS_ANDROID
    // Для Android URI нужно получить имя файла и использовать файловый дескриптор
    if (localPath.startsWith("content://")) {
        // Получаем имя файла из URI
        filename = AndroidController::instance()->getFileName(localPath);
        if (filename.isEmpty()) {
            // Fallback: извлекаем имя из URI
            QStringList parts = localPath.split('/');
            if (!parts.isEmpty()) {
                filename = parts.last();
                // Декодируем URL-кодированные символы
                if (filename.contains('%')) {
                    filename = QUrl::fromPercentEncoding(filename.toUtf8());
                }
            }
        }
        
        // Для Android URI используем файловый дескриптор через SystemController::readFile
        // Но scpFileCopy требует путь к файлу, поэтому нужно скопировать файл во временную директорию
        QByteArray fileData;
        qDebug() << "Reading Android URI:" << localPath;
        if (!SystemController::readFile(localPath, fileData)) {
            qDebug() << "Failed to read from Android URI";
            emit errorOccurred(tr("Failed to read backup file from Android storage"), ErrorCode::ReadError);
            return;
        }
        qDebug() << "Read" << fileData.size() << "bytes from Android URI";
        
        // Удаляем предыдущий временный файл, если он существует
        if (m_tempUploadFile) {
            delete m_tempUploadFile;
            m_tempUploadFile = nullptr;
        }
        
        // Создаем временный файл (сохраняем в член класса, чтобы не удалялся)
        // Используем setAutoRemove(false) чтобы файл не удалялся автоматически
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
        // НЕ закрываем файл - он должен оставаться открытым для SCP
        // m_tempUploadFile->close();
        actualLocalPath = m_tempUploadFile->fileName();
        
        qDebug() << "Created temp file:" << actualLocalPath << "written:" << written << "bytes, size:" << QFileInfo(actualLocalPath).size();
        
        // Проверяем, что файл существует и доступен для чтения
        QFileInfo tempFileInfo(actualLocalPath);
        if (!tempFileInfo.exists()) {
            qDebug() << "Temp file does not exist after creation!";
            emit errorOccurred(tr("Failed to create temporary file"), ErrorCode::OpenError);
            delete m_tempUploadFile;
            m_tempUploadFile = nullptr;
            return;
        }
        
        // Если имя файла пустое, используем имя из временного файла
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
    // Для других платформ используем обычную проверку
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
        
        // Удаляем временный файл после успешной загрузки
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
        
        // Удаляем временный файл при ошибке
        if (m_tempUploadFile) {
            m_tempUploadFile->remove();
            delete m_tempUploadFile;
            m_tempUploadFile = nullptr;
        }
    }
}

// Перегруженный метод для setup wizard с отдельными параметрами credentials
void ServersBackupController::uploadBackupWithStrings(const QString &hostname,
                                                     const QString &username,
                                                     const QString &secretData,
                                                     const QString &localPath,
                                                     bool replaceMode)
{
    // Создаем ServerCredentials из строк
    ServerCredentials credentials;
    credentials.hostName = hostname;
    credentials.userName = username;
    credentials.secretData = secretData;
    credentials.port = 22; // Default SSH port
    
    qDebug() << "uploadBackupWithStrings called with hostname:" << hostname << "username:" << username;
    
    // Вызываем основной метод
    uploadBackup(credentials, localPath, replaceMode);
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

    // Экранируем имя файла для безопасного использования в bash
    QString escapedFilename = backupFilename;
    escapedFilename.replace("'", "'\\''"); // Экранируем одинарные кавычки
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
// ВСТРОЕННЫЕ BASH СКРИПТЫ
// ============================================================================

QString ServersBackupController::getBackupScript(const QString &ipAddress) const
{
    // Упрощенная версия bash скрипта, встроенная в C++
    // Формат имени файла: IP_ADDRESS - DD-MM-YYYY_HH-MM-SS.tgz
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
tar -czf "$BACKUP_FILENAME" -C "$BACKUP_SUBDIR" . 2>/dev/null
rm -rf "$BACKUP_SUBDIR"

echo "[INFO] Backup created: $BACKUP_FILENAME"
)").arg(m_backupDir, ipAddress);
}

QString ServersBackupController::getContainerBackupScript(DockerContainer container, const QString &ipAddress) const
{
    QString containerName = ContainerProps::containerToString(container);
    
    // Backup конкретного контейнера напрямую через docker cp
    // Формат имени файла: IP_ADDRESS - DD-MM-YYYY_HH-MM-SS.tgz
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
    
    // Формат имени файла: IP_ADDRESS - DD-MM-YYYY_HH-MM-SS.tgz
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

# Создание директории
mkdir -p "$BACKUP_SUBDIR"

# Список контейнеров для backup
CONTAINERS=(%3)

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
tar -czf "$BACKUP_FILENAME" -C "$BACKUP_SUBDIR" . 2>/dev/null
rm -rf "$BACKUP_SUBDIR"

echo "[INFO] Backup created: $BACKUP_FILENAME"
)").arg(m_backupDir, ipAddress, containersList.trimmed());
}

QString ServersBackupController::getRestoreScript(const QString &backupFilename, 
                                                  const QStringList &containers,
                                                  bool replaceMode) const
{
    Q_UNUSED(containers); // TODO: Использовать для выборочного восстановления
    
    // Экранируем имя файла для безопасного использования в bash
    QString escapedFilename = backupFilename;
    escapedFilename.replace("'", "'\\''"); // Экранируем одинарные кавычки
    
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

# Проверка существования файла backup
if [ ! -f "$BACKUP_DIR/$BACKUP_FILE" ]; then
    echo "[ERROR] Backup file not found: $BACKUP_DIR/$BACKUP_FILE"
    exit 1
fi

# Извлечение backup
mkdir -p "$TEMP_DIR"
EXTRACT_OUTPUT=$(tar -xzf "$BACKUP_DIR/$BACKUP_FILE" -C "$TEMP_DIR" 2>&1)
EXTRACT_EXIT_CODE=$?
if [ $EXTRACT_EXIT_CODE -ne 0 ]; then
    echo "[ERROR] Failed to extract backup archive: $EXTRACT_OUTPUT"
    rm -rf "$TEMP_DIR"
    exit 1
fi

# Ищем директорию с контейнерами (может быть backup_temp_* или просто контейнеры напрямую)
BACKUP_SUBDIR=$(ls -d "$TEMP_DIR"/backup_* 2>/dev/null | head -1)

# Если не нашли backup_*, проверяем, есть ли директории контейнеров напрямую
if [ -z "$BACKUP_SUBDIR" ]; then
    # Проверяем, есть ли директории контейнеров (amnezia-*) напрямую в TEMP_DIR
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
        
        # Режим замены: очистка контейнера перед восстановлением
        if [ "$REPLACE_MODE" = "1" ]; then
            echo "[INFO] Clearing container $container_name before restore..."
            # Создаем пустую директорию для очистки /opt/amnezia
            TEMP_CLEAR_DIR="/tmp/clear_amnezia_$$"
            mkdir -p "$TEMP_CLEAR_DIR/amnezia"
            # Копируем пустую директорию, что удалит старое содержимое
            sudo docker cp "$TEMP_CLEAR_DIR/amnezia" "$container_name:/opt/" 2>/dev/null || true
            # Удаляем временную директорию
            rm -rf "$TEMP_CLEAR_DIR"
        fi
        
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
)").arg(m_backupDir).arg(escapedFilename).arg(replaceMode ? "1" : "0");
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
    
    // Проверяем на ошибки в выводе
    if (data.contains("[ERROR]") || data.contains("ERROR")) {
        // Ошибка обнаружена в stdout, но это не критично для handleStdOut
        // Основная проверка будет в restoreBackup после выполнения скрипта
    }
    
    // Обновляем прогресс на основе вывода
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

