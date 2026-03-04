#include "drfrake_controller.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QUrl>

// Backend API URL
const QString BACKEND_URL = "http://31.135.65.188:8081/api/v1";

DrFrakeController::DrFrakeController(ImportController *importController,
                                     const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent), m_importController(importController), m_settings(settings)
{
    m_nam = new QNetworkAccessManager(this);
    m_apiUrl = BACKEND_URL;
}

void DrFrakeController::login(const QString &email, const QString &password)
{
    if (email.isEmpty() || password.isEmpty()) {
        emit loginError(tr("Email and password cannot be empty"));
        return;
    }

    QUrl url(m_apiUrl + "/auth/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["email"] = email;
    json["password"] = password;

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QJsonObject obj = doc.object();

            if (obj.contains("access_token")) {
                saveJwtToken(obj["access_token"].toString());
                emit loginSuccess();
                emit loginStateChanged();
                
                // Fetch VPN config right after successful login
                fetchConfig();
            } else {
                emit loginError(tr("Invalid response format"));
            }
        } else {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QString errStr = tr("Network Error: ") + reply->errorString();
            if(!doc.isNull() && doc.object().contains("error")) {
                errStr = doc.object()["error"].toString();
            }
            emit loginError(errStr);
        }
    });
}

void DrFrakeController::fetchConfig()
{
    QString token = getJwtToken();
    if (token.isEmpty()) {
        emit configError(tr("Не выполнен вход в систему"));
        return;
    }

    QUrl url(m_apiUrl + "/me/config");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QNetworkReply *reply = m_nam->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QJsonObject obj = doc.object();

            if (obj.contains("config")) {
                QString configData = obj["config"].toString();

                if (m_importController) {
                    // Remove all existing Dr.Frake VPN servers to avoid duplicates
                    clearExistingDrFrakeServers();

                    if (m_importController->extractConfigFromData(configData)) {
                        m_importController->importConfig();
                        emit configFetched();
                    } else {
                        emit configError(tr("Не удалось прочитать конфигурацию"));
                    }
                } else {
                    emit configError(tr("Внутренняя ошибка: ImportController не инициализирован"));
                }
            } else {
                emit configError(tr("Сервер не вернул конфигурацию"));
            }
        } else {
             QByteArray responseData = reply->readAll();
             QJsonDocument doc = QJsonDocument::fromJson(responseData);
             QString errStr = tr("Ошибка сети: ") + reply->errorString();
             if(!doc.isNull() && doc.object().contains("error")) {
                 errStr = doc.object()["error"].toString();
             }
             emit configError(errStr);
             
             // If Unauthorized (401/403), logout user
             if (reply->error() == QNetworkReply::AuthenticationRequiredError || 
                 reply->error() == QNetworkReply::ContentAccessDenied) {
                 logout();
             }
        }
    });
}

void DrFrakeController::clearExistingDrFrakeServers()
{
    if (!m_settings) return;

    QJsonArray servers = m_settings->serversArray();
    // Iterate in reverse to safely remove by index
    for (int i = servers.size() - 1; i >= 0; --i) {
        QJsonObject server = servers.at(i).toObject();
        QString desc = server.value("description").toString();
        QString name = server.value("name").toString();
        // Match any server that starts with "Dr.Frake VPN" (including "Dr.Frake VPN - Netherlands" etc.)
        if (desc.startsWith("Dr.Frake VPN") || name.startsWith("Dr.Frake VPN")) {
            m_settings->removeServer(i);
        }
    }
}

void DrFrakeController::logout()
{
    saveJwtToken("");
    emit loginStateChanged();
}

bool DrFrakeController::isLoggedIn() const
{
    return !getJwtToken().isEmpty();
}

void DrFrakeController::saveJwtToken(const QString &token)
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "DrFrakeVPN", "DrFrakeVPN");
    qSettings.setValue("authToken", token);
    qSettings.sync();
}

QString DrFrakeController::getJwtToken() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "DrFrakeVPN", "DrFrakeVPN");
    return qSettings.value("authToken", "").toString();
}
