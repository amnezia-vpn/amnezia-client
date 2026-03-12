#include "drfrake_controller.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QUrl>
#include <QDate>
#include <QDateTime>

// Backend API URL
const QString BACKEND_URL = "https://srv.frakebit.com/api/v1";

DrFrakeController::DrFrakeController(ImportController *importController,
                                     const std::shared_ptr<Settings> &settings, 
                                     ServersModel *serversModel, QObject *parent)
    : QObject(parent), 
      m_importController(importController), 
      m_settings(settings), 
      m_serversModel(serversModel)
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
                saveSubscriptionInfo("", "", "");  // Clear stale data from previous user
                emit loginSuccess();
                emit loginStateChanged();
                emit subscriptionChanged();

                // Fetch VPN config and subscription status right after successful login
                fetchConfig();
                fetchSubscription();
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

void DrFrakeController::loginWithToken(const QString &token)
{
    saveJwtToken(token);
    saveSubscriptionInfo("", "", "");
    emit loginStateChanged();
    emit subscriptionChanged();
    fetchConfig();
    fetchSubscription();
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
                QString configDataStr = obj["config"].toString();
                QStringList configStrings = configDataStr.split('\n', Qt::SkipEmptyParts);

                if (m_importController && m_settings && m_serversModel) {
                    QJsonArray servers = m_settings->serversArray();
                    QList<int> existingDrFrakeServerIndices;

                    // Locate all existing Dr.Frake VPN servers
                    for (int i = 0; i < servers.size(); ++i) {
                        QJsonObject server = servers.at(i).toObject();
                        QString desc = server.value("description").toString();
                        QString name = server.value("name").toString();
                        if (desc.startsWith("Dr.Frake VPN") || name.startsWith("Dr.Frake VPN")) {
                            existingDrFrakeServerIndices.append(i);
                        }
                    }

                    // For each new config, we'll try to find an exact match to update in-place
                    QList<int> updatedIndices;

                    for (const QString &configData : configStrings) {
                        if (m_importController->extractConfigFromData(configData)) {
                            QJsonObject newConfig = QJsonDocument::fromJson(m_importController->getConfig().toUtf8()).object();
                            QString newHostName = newConfig.value("hostName").toString();

                            bool found = false;
                            for (int i : existingDrFrakeServerIndices) {
                                if (updatedIndices.contains(i)) continue; // Already updated this one

                                QJsonObject existingServer = servers.at(i).toObject();
                                if (existingServer.value("hostName").toString() == newHostName) {
                                    // Found a match! Update it in-place
                                    m_serversModel->editServer(newConfig, i);
                                    updatedIndices.append(i);
                                    found = true;
                                    break;
                                }
                            }

                            if (!found) {
                                // Not found, so add it as a new server
                                m_importController->importConfig();
                            } else {
                                // We handled it manually, clear the import controller's pending state
                                m_importController->clearConfigFileName();
                                // We can't clear getConfig() directly but it's safe to ignore it until next extract
                            }
                        }
                    }

                    // Remove any Dr.Frake VPN servers that were not in the new config
                    // Iterate backwards so indices don't shift during removal
                    for (int i = existingDrFrakeServerIndices.size() - 1; i >= 0; --i) {
                        int serverIndex = existingDrFrakeServerIndices.at(i);
                        if (!updatedIndices.contains(serverIndex)) {
                            m_serversModel->removeServer(serverIndex);
                        }
                    }

                    emit configFetched();
                } else {
                    emit configError(tr("Внутренняя ошибка: Контроллеры не инициализированы"));
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
}

void DrFrakeController::createPayment(const QString &plan)
{
    QString token = getJwtToken();
    if (token.isEmpty()) {
        emit paymentError(tr("Необходимо войти в аккаунт"));
        return;
    }

    QUrl url(m_apiUrl + "/payments/create");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QJsonObject json;
    json["plan"] = plan;

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject obj = doc.object();

        if (reply->error() == QNetworkReply::NoError) {
            QString confirmUrl = obj.value("confirmation_url").toString();
            emit paymentCreated(confirmUrl);
        } else {
            int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (httpStatus == 401) {
                logout();
                emit paymentError(tr("Сессия истекла, войдите снова"));
                return;
            }
            QString errStr = obj.contains("error") ? obj["error"].toString()
                                                    : tr("Ошибка создания платежа: ") + reply->errorString();
            emit paymentError(errStr);
        }
    });
}

void DrFrakeController::fetchSubscription()
{
    QString token = getJwtToken();
    if (token.isEmpty()) {
        emit subscriptionError(tr("Не выполнен вход в систему"));
        return;
    }

    QUrl url(m_apiUrl + "/me/subscription");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QNetworkReply *reply = m_nam->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QJsonObject obj = doc.object();

            QString status        = obj.value("status").toString("inactive");
            QString plan          = obj.value("plan").toString("");
            QString endDate       = obj.value("expires_at").toString("");
            bool autoRenew        = obj.value("auto_renew").toBool(true);
            bool cardSaved        = obj.value("card_saved").toBool(false);
            bool trialAvailable   = obj.value("trial_available").toBool(false);

            saveSubscriptionInfo(status, plan, endDate, autoRenew, cardSaved, trialAvailable);
            emit subscriptionChanged();
            emit subscriptionFetched();
        } else {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QString errStr = tr("Ошибка сети: ") + reply->errorString();
            if (!doc.isNull() && doc.object().contains("error")) {
                errStr = doc.object()["error"].toString();
            }
            emit subscriptionError(errStr);
        }
    });
}

void DrFrakeController::logout()
{
    saveJwtToken("");
    saveSubscriptionInfo("", "", "");
    emit loginStateChanged();
    emit subscriptionChanged();
}

bool DrFrakeController::isLoggedIn() const
{
    return !getJwtToken().isEmpty();
}

bool DrFrakeController::isSubscribed() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "DrFrakeVPN", "DrFrakeVPN");
    QString status = qSettings.value("subscriptionStatus", "").toString();
    if (status != "active") return false;

    // Free plan is not a paid subscription
    QString plan = qSettings.value("subscriptionPlan", "").toString();
    if (plan == "free" || plan.isEmpty()) return false;

    // Check expiry date — backend may return full ISO datetime or date-only
    QString endDateStr = qSettings.value("subscriptionEndDate", "").toString();
    if (endDateStr.isEmpty()) return false;

    // Try date-only first (YYYY-MM-DD), then full datetime
    QDate endDate = QDate::fromString(endDateStr, Qt::ISODate);
    if (!endDate.isValid()) {
        QDateTime dt = QDateTime::fromString(endDateStr, Qt::ISODateWithMs);
        if (!dt.isValid())
            dt = QDateTime::fromString(endDateStr, Qt::ISODate);
        endDate = dt.date();
    }
    if (!endDate.isValid()) return false;
    return QDate::currentDate() <= endDate;
}

QString DrFrakeController::subscriptionPlan() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "DrFrakeVPN", "DrFrakeVPN");
    return qSettings.value("subscriptionPlan", "").toString();
}

QString DrFrakeController::subscriptionEndDate() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "DrFrakeVPN", "DrFrakeVPN");
    return qSettings.value("subscriptionEndDate", "").toString();
}

void DrFrakeController::saveSubscriptionInfo(const QString &status, const QString &plan, const QString &endDate,
                                             bool autoRenew, bool cardSaved, bool trialAvailable)
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "DrFrakeVPN", "DrFrakeVPN");
    qSettings.setValue("subscriptionStatus", status);
    qSettings.setValue("subscriptionPlan", plan);
    qSettings.setValue("subscriptionEndDate", endDate);
    qSettings.setValue("subscriptionAutoRenew", autoRenew);
    qSettings.setValue("subscriptionCardSaved", cardSaved);
    qSettings.setValue("subscriptionTrialAvailable", trialAvailable);
    qSettings.sync();
}

bool DrFrakeController::autoRenew() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "DrFrakeVPN", "DrFrakeVPN");
    return qSettings.value("subscriptionAutoRenew", true).toBool();
}

bool DrFrakeController::cardSaved() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "DrFrakeVPN", "DrFrakeVPN");
    return qSettings.value("subscriptionCardSaved", false).toBool();
}

bool DrFrakeController::trialAvailable() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "DrFrakeVPN", "DrFrakeVPN");
    return qSettings.value("subscriptionTrialAvailable", false).toBool();
}

void DrFrakeController::setAutoRenew(bool enabled)
{
    QString token = getJwtToken();
    if (token.isEmpty()) return;

    QUrl url(m_apiUrl + "/me/subscription/auto-renew");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QJsonObject json;
    json["enabled"] = enabled;

    QNetworkReply *reply = m_nam->sendCustomRequest(request, "PATCH", QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, enabled]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "DrFrakeVPN", "DrFrakeVPN");
            qSettings.setValue("subscriptionAutoRenew", enabled);
            qSettings.sync();
            emit autoRenewChanged(enabled);
            emit subscriptionChanged();
        } else {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QString errStr = doc.object().value("error").toString(reply->errorString());
            emit requestError(errStr);
        }
    });
}

void DrFrakeController::deleteCard()
{
    QString token = getJwtToken();
    if (token.isEmpty()) return;

    QUrl url(m_apiUrl + "/me/card");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

    QNetworkReply *reply = m_nam->deleteResource(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "DrFrakeVPN", "DrFrakeVPN");
            qSettings.setValue("subscriptionCardSaved", false);
            qSettings.setValue("subscriptionAutoRenew", false);
            qSettings.sync();
            emit cardDeleted();
            emit subscriptionChanged();
        } else {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QString errStr = doc.object().value("error").toString(reply->errorString());
            emit requestError(errStr);
        }
    });
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
