#include "updateController.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVersionNumber>
#include <QtConcurrent>

#include "amnezia_application.h"
#include "core/errorstrings.h"
#include "version.h"

namespace {
#ifdef Q_OS_MACOS
    const QString installerPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/AmneziaVPN.dmg";
#elif defined Q_OS_WINDOWS
    const QString installerPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/AmneziaVPN.exe";
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    const QString installerPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/AmneziaVPN.tar.zip";
#endif
}

UpdateController::UpdateController(const std::shared_ptr<Settings> &settings, QObject *parent) : QObject(parent), m_settings(settings)
{
}

QString UpdateController::getHeaderText()
{
    return tr("New version released: %1 (%2)").arg(m_version, m_releaseDate);
}

QString UpdateController::getChangelogText()
{
    return m_changelogText;
}

void UpdateController::checkForUpdates()
{
    QNetworkRequest request;
    request.setTransferTimeout(7000);
    QString endpoint = "https://api.github.com/repos/amnezia-vpn/amnezia-client/releases/latest";
    request.setUrl(endpoint);

    QNetworkReply *reply = amnApp->manager()->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString contents = QString::fromUtf8(reply->readAll());
            QJsonObject data = QJsonDocument::fromJson(contents.toUtf8()).object();
            m_version = data.value("tag_name").toString();

            auto currentVersion = QVersionNumber::fromString(QString(APP_VERSION));
            qDebug() << currentVersion;
            auto newVersion = QVersionNumber::fromString(m_version);
            if (newVersion > currentVersion) {
                m_changelogText = data.value("body").toString();

                QString dateString = data.value("published_at").toString();
                QDateTime dateTime = QDateTime::fromString(dateString, "yyyy-MM-ddTHH:mm:ssZ");
                m_releaseDate = dateTime.toString("MMM dd yyyy");

                QJsonArray assets = data.value("assets").toArray();

                for (auto asset : assets) {
                    QJsonObject assetObject = asset.toObject();
                    if (assetObject.value("name").toString().contains(".dmg")) {
                        m_downloadUrl = assetObject.value("browser_download_url").toString();
                    }
                }

                emit updateFound();
            }
        } else {
            if (reply->error() == QNetworkReply::NetworkError::OperationCanceledError
                || reply->error() == QNetworkReply::NetworkError::TimeoutError) {
                qDebug() << errorString(ErrorCode::ApiConfigTimeoutError);
            } else {
                QString err = reply->errorString();
                qDebug() << QString::fromUtf8(reply->readAll());
                qDebug() << reply->error();
                qDebug() << err;
                qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                qDebug() << errorString(ErrorCode::ApiConfigDownloadError);
            }
        }

        reply->deleteLater();
    });

    QObject::connect(reply, &QNetworkReply::errorOccurred,
                     [this, reply](QNetworkReply::NetworkError error) { qDebug() << reply->errorString() << error; });
    connect(reply, &QNetworkReply::sslErrors, [this, reply](const QList<QSslError> &errors) {
        qDebug().noquote() << errors;
        qDebug() << errorString(ErrorCode::ApiConfigSslError);
    });
}

void UpdateController::runInstaller()
{
    QNetworkRequest request;
    request.setTransferTimeout(7000);
    request.setUrl(m_downloadUrl);

    QNetworkReply *reply = amnApp->manager()->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QFile file(installerPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();

                QFutureWatcher<int> watcher;
                QFuture<int> future = QtConcurrent::run([this]() {
                    QString t = installerPath;
                    QRemoteObjectPendingReply<int> ipcReply = IpcClient::Interface()->mountDmg(t, true);
                    ipcReply.waitForFinished();
                    QProcess::execute("/Volumes/AmneziaVPN/AmneziaVPN.app/Contents/MacOS/AmneziaVPN");
                    ipcReply = IpcClient::Interface()->mountDmg(t, false);
                    ipcReply.waitForFinished();
                    return ipcReply.returnValue();
                });

                QEventLoop wait;
                connect(&watcher, &QFutureWatcher<ErrorCode>::finished, &wait, &QEventLoop::quit);
                watcher.setFuture(future);
                wait.exec();

                qDebug() <<  future.result();

//                emit errorOccured("");
            }
        } else {
            if (reply->error() == QNetworkReply::NetworkError::OperationCanceledError
                || reply->error() == QNetworkReply::NetworkError::TimeoutError) {
                qDebug() << errorString(ErrorCode::ApiConfigTimeoutError);
            } else {
                QString err = reply->errorString();
                qDebug() << QString::fromUtf8(reply->readAll());
                qDebug() << reply->error();
                qDebug() << err;
                qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                qDebug() << errorString(ErrorCode::ApiConfigDownloadError);
            }
        }

        reply->deleteLater();
    });

}
