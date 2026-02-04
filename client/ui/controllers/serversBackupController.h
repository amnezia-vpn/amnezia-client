#ifndef SERVERSBACKUPCONTROLLER_H
#define SERVERSBACKUPCONTROLLER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>

class QTemporaryFile;

#include "core/controllers/serverController.h"
#include "core/defs.h"
#include "containers/containers_defs.h"

using namespace amnezia;

/**
 * @brief Контроллер для управления backup конфигураций Amnezia VPN
 * 
 * Использует существующий ServerController и libssh::Client из Amnezia
 * Bash скрипты встроены напрямую в C++ код
 * Поддерживает backup конкретных контейнеров напрямую через docker cp
 * 
 * Полностью кроссплатформенный: Windows, macOS, Linux, iOS, Android
 */
class ServersBackupController : public QObject
{
    Q_OBJECT

public:
    explicit ServersBackupController(std::shared_ptr<Settings> settings, QObject *parent = nullptr);
    ~ServersBackupController();

    /**
     * @brief Информация о backup
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
     * @brief Создать backup на сервере (всех контейнеров)
     * @param credentials Учетные данные сервера
     */
    void createBackup(const ServerCredentials &credentials);

    /**
     * @brief Создать backup конкретного контейнера
     * @param credentials Учетные данные сервера
     * @param container Тип контейнера для backup
     */
    void createContainerBackup(const ServerCredentials &credentials, DockerContainer container);
    
    /**
     * @brief Создать backup конкретного контейнера по имени
     * @param credentials Учетные данные сервера
     * @param containerName Имя контейнера (например "amnezia-awg")
     */
    void createBackupByName(const ServerCredentials &credentials, const QString &containerName);

    /**
     * @brief Создать backup нескольких контейнеров
     * @param credentials Учетные данные сервера
     * @param containers Список контейнеров для backup
     */
    void createContainersBackup(const ServerCredentials &credentials, const QList<DockerContainer> &containers);

    /**
     * @brief Получить список backup с сервера
     * @param credentials Учетные данные сервера
     */
    void fetchBackupList(const ServerCredentials &credentials);

    /**
     * @brief Восстановить из backup
     * @param credentials Учетные данные сервера
     * @param backupFilename Имя файла backup
     * @param containers Список контейнеров (пустой = все)
     * @param replaceMode Если true - сначала очищает контейнер, затем восстанавливает. Если false - добавляет данные поверх существующих
     */
    void restoreBackup(const ServerCredentials &credentials, 
                       const QString &backupFilename, 
                       const QStringList &containers = QStringList(),
                       bool replaceMode = false);

    /**
     * @brief Проверить состояние backup на сервере
     * @param credentials Учетные данные сервера
     */
    void checkBackupStatus(const ServerCredentials &credentials);

    /**
     * @brief Скачать backup на локальную машину
     * @param credentials Учетные данные сервера
     * @param backupFilename Имя файла backup
     * @param localPath Путь для сохранения
     */
    void downloadBackup(const ServerCredentials &credentials,
                        const QString &backupFilename, 
                        const QString &localPath);

    /**
     * @brief Загрузить backup на сервер
     * @param credentials Учетные данные сервера
     * @param localPath Путь к локальному файлу
     * @param replaceMode Режим восстановления (true = замена, false = добавление). Сохраняется для последующего использования в restoreBackup
     */
    void uploadBackup(const ServerCredentials &credentials,
                      const QString &localPath,
                      bool replaceMode = false);
    
    // Перегруженный метод для setup wizard с отдельными параметрами credentials
    Q_INVOKABLE void uploadBackupWithStrings(const QString &hostname,
                                             const QString &username,
                                             const QString &secretData,
                                             const QString &localPath,
                                             bool replaceMode = false);

    /**
     * @brief Удалить backup с сервера
     * @param credentials Учетные данные сервера
     * @param backupFilename Имя файла backup
     */
    void deleteBackup(const ServerCredentials &credentials,
                      const QString &backupFilename);

    /**
     * @brief Установить директорию backup на сервере
     */
    void setBackupDirectory(const QString &directory);

    /**
     * @brief Получить директорию backup
     */
    QString backupDirectory() const { return m_backupDir; }

signals:
    /**
     * @brief Изменился статус операции
     */
    void statusChanged(BackupStatus status);

    /**
     * @brief Прогресс операции (0-100)
     */
    void progressChanged(int percent, const QString &message);

    /**
     * @brief Получен список backup
     */
    void backupListReceived(const QList<BackupInfo> &backups);

    /**
     * @brief Backup создан успешно
     */
    void backupCreated(const QString &backupFilename);

    /**
     * @brief Backup восстановлен успешно
     */
    void backupRestored();

    /**
     * @brief Backup скачан
     */
    void backupDownloaded(const QString &localPath);

    /**
     * @brief Backup загружен на сервер
     */
    void backupUploaded(const QString &serverPath);

    /**
     * @brief Получена информация о состоянии backup
     */
    void backupStatusReceived(const QJsonObject &status);

    /**
     * @brief Произошла ошибка
     */
    void errorOccurred(const QString &errorMessage, ErrorCode errorCode);

private:
    /**
     * @brief Получить bash скрипт для создания backup всех контейнеров
     * @param ipAddress IP адрес сервера в формате с подчеркиваниями (например "192_119_110_11")
     */
    QString getBackupScript(const QString &ipAddress) const;

    /**
     * @brief Получить bash скрипт для создания backup конкретного контейнера
     * @param container Тип контейнера
     * @param ipAddress IP адрес сервера в формате с подчеркиваниями (например "192_119_110_11")
     */
    QString getContainerBackupScript(DockerContainer container, const QString &ipAddress) const;

    /**
     * @brief Получить bash скрипт для создания backup нескольких контейнеров
     * @param containers Список контейнеров
     * @param ipAddress IP адрес сервера в формате с подчеркиваниями (например "192_119_110_11")
     */
    QString getContainersBackupScript(const QList<DockerContainer> &containers, const QString &ipAddress) const;

    /**
     * @brief Получить bash скрипт для восстановления
     * @param backupFilename Имя файла backup
     * @param containers Список контейнеров
     * @param replaceMode Если true - сначала очищает контейнер, затем восстанавливает
     */
    QString getRestoreScript(const QString &backupFilename, const QStringList &containers, bool replaceMode = false) const;

    /**
     * @brief Получить bash скрипт для проверки состояния
     */
    QString getCheckStatusScript() const;

    /**
     * @brief Получить bash скрипт для списка backup
     */
    QString getListBackupsScript() const;

    /**
     * @brief Парсить список backup из вывода
     */
    QList<BackupInfo> parseBackupList(const QString &output);

    /**
     * @brief Парсить статус из вывода
     */
    QJsonObject parseBackupStatus(const QString &output);

    /**
     * @brief Обработать стандартный вывод
     */
    ErrorCode handleStdOut(const QString &data, QString &output);

    /**
     * @brief Обработать вывод ошибок
     */
    ErrorCode handleStdErr(const QString &data, QString &error);

    /**
     * @brief Установить статус
     */
    void setStatus(BackupStatus status);

    /**
     * @brief Установить прогресс
     */
    void setProgress(int percent, const QString &message);

private:
    std::shared_ptr<Settings> m_settings;
    ServerController *m_serverController;
    BackupStatus m_status;
    QString m_backupDir;
    QString m_currentOutput;
    QString m_currentError;
    bool m_restoreReplaceMode; // Сохраняем режим восстановления для использования после uploadBackup
    QTemporaryFile *m_tempUploadFile; // Временный файл для Android URI (чтобы не удалялся до завершения загрузки)
};

#endif // SERVERSBACKUPCONTROLLER_H

