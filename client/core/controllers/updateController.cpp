#include "updateController.h"

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
#include <QStringList>

#include "core/defs.h"

UpdateController::UpdateController(QObject *parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished, this, &UpdateController::onVersionCheckFinished);
}

void UpdateController::checkForUpdates()
{
    // Hardcode fallback url for backend API
    QString apiUrl = "https://srv.frakebit.com";


    QUrl url(apiUrl + "/api/v1/client/latest-version");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Start request
    m_nam->get(request);
}

void UpdateController::onVersionCheckFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[UpdateController] Failed to check for updates:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[UpdateController] Invalid JSON received";
        return;
    }

    QJsonObject obj = doc.object();
    QString latestVersion = obj.value("version").toString();
    m_downloadUrl = obj.value("download_url").toString();
    QString releaseNotes = obj.value("release_notes").toString();
    bool isCritical = obj.value("is_critical").toBool(false);

    QString currentVersion = QCoreApplication::applicationVersion();

    // If version is missing or we are in debug build where version might be empty
    if (latestVersion.isEmpty() || currentVersion.isEmpty()) {
        qDebug() << "[UpdateController] Skipping update check, missing version strings.";
        return;
    }

    if (isNewerVersion(currentVersion, latestVersion)) {
        qInfo() << "[UpdateController] Update available! current:" << currentVersion << "latest:" << latestVersion;
        emit updateChecked(true, latestVersion, releaseNotes, isCritical);
    } else {
        qInfo() << "[UpdateController] Client is up to date.";
        emit updateChecked(false, "", "", false);
    }
}

void UpdateController::openDownloadUrl()
{
    if (!m_downloadUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_downloadUrl));
    }
}

bool UpdateController::isNewerVersion(const QString &current, const QString &latest)
{
    // Basic SemVer comparison (e.g. 1.0.4 > 1.0.3)
    QStringList currentParts = current.split('.');
    QStringList latestParts = latest.split('.');

    int maxCount = qMax(currentParts.size(), latestParts.size());

    for (int i = 0; i < maxCount; ++i) {
        int curPart = (i < currentParts.size()) ? currentParts[i].toInt() : 0;
        int latPart = (i < latestParts.size()) ? latestParts[i].toInt() : 0;

        if (latPart > curPart) {
            return true;
        } else if (latPart < curPart) {
            return false;
        }
    }
    return false; // Equal completely
}
