#include "updateController.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVersionNumber>
#include <QtConcurrent>
#include <QUrl>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>

#include "amnezia_application.h"
#include "core/api/apiDefs.h"
#include "core/errorstrings.h"
#include "core/scripts_registry.h"
#include "logger.h"
#include "version.h"
#include "core/controllers/gatewayController.h"

namespace
{
    Logger logger("UpdateController");

#if defined(Q_OS_WINDOWS)
    const QLatin1String kInstallerRemoteFileNamePattern("AmneziaVPN_%1_x64.exe");
    const QString kInstallerLocalPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/AmneziaVPN_installer.exe";
#elif defined(Q_OS_MACOS)
    const QLatin1String kInstallerRemoteFileNamePattern("AmneziaVPN_%1_macos.zip");
    const QString kInstallerLocalPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/AmneziaVPN.zip";
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    const QLatin1String kInstallerRemoteFileNamePattern("AmneziaVPN_%1_linux_x64.tar.zip");
    const QString kInstallerLocalPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/AmneziaVPN.tar.zip";
#endif
}

UpdateController::UpdateController(const std::shared_ptr<Settings> &settings, QObject *parent) : QObject(parent), m_settings(settings)
{
}

QString UpdateController::getHeaderText()
{
    if (m_releaseDate.trimmed().isEmpty()) {
        return tr("New version released: %1").arg(m_version);
    } else {
        return tr("New version released: %1 (%2)").arg(m_version, m_releaseDate);
    }
}

QString UpdateController::getChangelogText()
{
    QStringList lines = m_changelogText.split("\n");
    QStringList filteredChangeLogText;
    bool add = false;
    QString osSection;

#ifdef Q_OS_WINDOWS
    osSection = "### Windows";
#elif defined(Q_OS_MACOS)
    osSection = "### macOS";
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    osSection = "### Linux";
#endif

    for (const QString &line : lines) {
        if (line.startsWith("### General")) {
            add = true;
        } else if (line.startsWith("### ") && line != osSection) {
            add = false;
        } else if (line == osSection) {
            add = true;
        }

        if (add) {
            filteredChangeLogText.append(line);
        }
    }

    return filteredChangeLogText.join("\n");
}

QString UpdateController::getVersion() const
{
    return m_version;
}

void UpdateController::checkForUpdates()
{
    qDebug() << "checkForUpdates";
    if (!fetchGatewayUrl()) return;
    if (!fetchVersionInfo()) return;
    if (!isNewVersionAvailable()) return;
    if (!fetchChangelog()) return;
    if (!fetchReleaseDate()) return;

    m_downloadUrl = composeDownloadUrl();
    emit updateFound();
}

bool UpdateController::fetchGatewayUrl()
{
    // Workaround: wait before contacting gateway to avoid rate limit triggered by other requests (news etc.)
    {
        QEventLoop wait;
        QTimer::singleShot(1000, &wait, &QEventLoop::quit);
        wait.exec(QEventLoop::ExcludeUserInputEvents);
    }

    GatewayController gatewayController(m_settings->getGatewayEndpoint(),
                                        m_settings->isDevGatewayEnv(),
                                        7000,
                                        m_settings->isStrictKillSwitchEnabled());

    QJsonObject apiPayload;
    apiPayload[apiDefs::key::cliVersion] = QString(APP_VERSION);
    apiPayload[apiDefs::key::osVersion] = QSysInfo::productType();
    apiPayload[apiDefs::key::installationUuid] = m_settings->getInstallationUuid(true);
    
    QByteArray gatewayResponse;
    
    auto err = gatewayController.post(QStringLiteral("%1v1/updater_endpoint"), apiPayload, gatewayResponse);
    if (err != ErrorCode::NoError) {
        logger.error() << errorString(err);
        return false;
    }
    
    QJsonObject gatewayData = QJsonDocument::fromJson(gatewayResponse).object();
    
    QString baseUrl = gatewayData.value("url").toString();
    if (baseUrl.endsWith('/')) {
        baseUrl.chop(1);
    }
    
    m_baseUrl = baseUrl;
    return true;
}

bool UpdateController::fetchVersionInfo()
{
    QByteArray data;
    if (!doSyncGet("/VERSION", data)) {
        return false;
    }
    m_version = QString::fromUtf8(data).trimmed();
    return true;
}

bool UpdateController::isNewVersionAvailable()
{
    auto currentVersion = QVersionNumber::fromString(QString(APP_VERSION));
    auto newVersion = QVersionNumber::fromString(m_version);
    return newVersion > currentVersion;
}

bool UpdateController::fetchChangelog()
{
    QByteArray data;
    if (!doSyncGet("/CHANGELOG", data)) {
        m_changelogText = tr("Failed to load changelog text");
    } else {
        m_changelogText = QString::fromUtf8(data);
    }
    return true;
}

bool UpdateController::fetchReleaseDate()
{
    QByteArray data;
    if (doSyncGet("/RELEASE_DATE", data)) {
        m_releaseDate = QString::fromUtf8(data).trimmed();
    } else {
        m_releaseDate = QString();
    }
    return true;
}

void UpdateController::setupNetworkErrorHandling(QNetworkReply* reply, const QString& operation)
{
    QObject::connect(reply, &QNetworkReply::errorOccurred, [this, reply, operation](QNetworkReply::NetworkError error) {
        logger.error() << QString("Network error occurred while fetching %1: %2 %3")
                          .arg(operation, reply->errorString(), QString::number(error));
    });
    
    QObject::connect(reply, &QNetworkReply::sslErrors, [this, reply, operation](const QList<QSslError> &errors) {
        QStringList errorStrings;
        for (const QSslError &err : errors) {
            errorStrings << err.errorString();
        }
        logger.error() << QString("SSL errors while fetching %1: %2").arg(operation, errorStrings.join("; "));
    });
}

void UpdateController::handleNetworkError(QNetworkReply* reply, const QString& operation)
{
    if (reply->error() == QNetworkReply::NetworkError::OperationCanceledError
        || reply->error() == QNetworkReply::NetworkError::TimeoutError) {
        logger.error() << errorString(ErrorCode::ApiConfigTimeoutError);
    } else {
        QString err = reply->errorString();
        logger.error() << "Network error code:" << QString::number(static_cast<int>(reply->error()));
        logger.error() << "Error message:" << err;
        logger.error() << "HTTP status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        logger.error() << errorString(ErrorCode::ApiConfigDownloadError);
    }
}

QString UpdateController::composeDownloadUrl()
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    const QString fileName = QString(kInstallerRemoteFileNamePattern).arg(m_version);
    return m_baseUrl + "/" + fileName;
#else
    return QString();
#endif
}

void UpdateController::runInstaller()
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    if (m_downloadUrl.isEmpty()) {
        logger.error() << "Download URL is empty";
        return;
    }

    QNetworkRequest request;
    request.setTransferTimeout(30000);
    request.setUrl(m_downloadUrl);

    QNetworkReply *reply = amnApp->networkManager()->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QFile file(kInstallerLocalPath);
            if (!file.open(QIODevice::WriteOnly)) {
                logger.error() << "Failed to open installer file for writing:" << kInstallerLocalPath << "Error:" << file.errorString();
                reply->deleteLater();
                return;
            }

            if (file.write(reply->readAll()) == -1) {
                logger.error() << "Failed to write installer data to file:" << kInstallerLocalPath << "Error:" << file.errorString();
                file.close();
                reply->deleteLater();
                return;
            }

            file.close();

    #if defined(Q_OS_WINDOWS)
            runWindowsInstaller(kInstallerLocalPath);
    #elif defined(Q_OS_MACOS)
            runMacInstaller(kInstallerLocalPath);
    #elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
            runLinuxInstaller(kInstallerLocalPath);
    #endif
        } else {
            if (reply->error() == QNetworkReply::NetworkError::OperationCanceledError
                || reply->error() == QNetworkReply::NetworkError::TimeoutError) {
                logger.error() << errorString(ErrorCode::ApiConfigTimeoutError);
            } else {
                QString err = reply->errorString();
                logger.error() << QString::fromUtf8(reply->readAll());
                logger.error() << "Network error code:" << QString::number(static_cast<int>(reply->error()));
                logger.error() << "Error message:" << err;
                logger.error() << "HTTP status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                logger.error() << errorString(ErrorCode::ApiConfigDownloadError);
            }
        }
        reply->deleteLater();
    });
#endif
}

#if defined(Q_OS_WINDOWS)
int UpdateController::runWindowsInstaller(const QString &installerPath)
{
    qint64 pid;
    bool success = QProcess::startDetached(installerPath, QStringList(), QString(), &pid);

    if (success) {
        logger.info() << "Installation process started with PID:" << pid;
    } else {
        logger.error() << "Failed to start installation process";
        return -1;
    }

    return 0;
}
#endif

#if defined(Q_OS_MACOS)
int UpdateController::runMacInstaller(const QString &installerPath)
{
    // Create temporary directory for extraction
    QTemporaryDir extractDir;
    extractDir.setAutoRemove(false);
    if (!extractDir.isValid()) {
        logger.error() << "Failed to create temporary directory";
        return -1;
    }
    logger.info() << "Temporary directory created:" << extractDir.path();

    // Create script file in the temporary directory
    QString scriptPath = extractDir.path() + "/mac_installer.sh";
    QFile scriptFile(scriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly)) {
        logger.error() << "Failed to create script file";
        return -1;
    }

    // Get script content from registry
    QString scriptContent = amnezia::scriptData(amnezia::ClientScriptType::mac_installer);
    if (scriptContent.isEmpty()) {
        logger.error() << "macOS installer script content is empty";
        scriptFile.close();
        return -1;
    }

    scriptFile.write(scriptContent.toUtf8());
    scriptFile.close();
    logger.info() << "Script file created:" << scriptPath;

    // Make script executable
    QFile::setPermissions(scriptPath, QFile::permissions(scriptPath) | QFile::ExeUser);

    // Start detached process
    qint64 pid;
    bool success =
            QProcess::startDetached("/bin/bash", QStringList() << scriptPath << extractDir.path() << installerPath, extractDir.path(), &pid);

    if (success) {
        logger.info() << "Installation process started with PID:" << pid;
    } else {
        logger.error() << "Failed to start installation process";
        return -1;
    }

    return 0;
}
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
int UpdateController::runLinuxInstaller(const QString &installerPath)
{
    // Create temporary directory for extraction
    QTemporaryDir extractDir;
    extractDir.setAutoRemove(false);
    if (!extractDir.isValid()) {
        logger.error() << "Failed to create temporary directory";
        return -1;
    }
    logger.info() << "Temporary directory created:" << extractDir.path();

    // Create script file in the temporary directory
    QString scriptPath = extractDir.path() + "/installer.sh";
    QFile scriptFile(scriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly)) {
        logger.error() << "Failed to create script file";
        return -1;
    }

    // Get script content from registry
    QString scriptContent = amnezia::scriptData(amnezia::ClientScriptType::linux_installer);
    scriptFile.write(scriptContent.toUtf8());
    scriptFile.close();
    logger.info() << "Script file created:" << scriptPath;

    // Make script executable
    QFile::setPermissions(scriptPath, QFile::permissions(scriptPath) | QFile::ExeUser);

    // Start detached process
    qint64 pid;
    bool success =
            QProcess::startDetached("/bin/bash", QStringList() << scriptPath << extractDir.path() << installerPath, extractDir.path(), &pid);

    if (success) {
        logger.info() << "Installation process started with PID:" << pid;
    } else {
        logger.error() << "Failed to start installation process";
        return -1;
    }

    return 0;
}
#endif

bool UpdateController::doSyncGet(const QString& endpoint, QByteArray& outData)
{
    QNetworkRequest req;
    req.setTransferTimeout(7000);
    req.setUrl(QUrl(m_baseUrl + endpoint));

    QNetworkReply* reply = amnApp->networkManager()->get(req);
    setupNetworkErrorHandling(reply, endpoint);
    
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool ok = (reply->error() == QNetworkReply::NoError);
    if (ok) {
        outData = reply->readAll();
    } else {
        handleNetworkError(reply, endpoint);
    }
    reply->deleteLater();
    return ok;
}
