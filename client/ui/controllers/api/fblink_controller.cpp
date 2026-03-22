#include "fblink_controller.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QUrl>
#include <QDate>
#include <QDateTime>
#include <QTimer>

// Backend API URL
const QString BACKEND_URL = "https://srv.frakebit.com/api/v1";

FBLinkController::FBLinkController(ImportController *importController,
                                     const std::shared_ptr<Settings> &settings,
                                     ServersModel *serversModel, QObject *parent)
    : QObject(parent),
      m_importController(importController),
      m_settings(settings),
      m_serversModel(serversModel)
{
    m_nam = new QNetworkAccessManager(this);
    m_apiUrl = BACKEND_URL;

    // Lazy sync at startup: если пользователь уже залогинен — сначала обновляем токен (sliding window),
    // потом синхронизируем конфиги. Если refresh провалится — logout().
    if (isLoggedIn()) {
        m_isLoading = true;
        emit loadingChanged();
        QTimer::singleShot(0, this, [this]() {
            refreshAccessToken([this]() {
                fetchConfig();
                fetchSubscription();
            });
        });
    }
}

void FBLinkController::login(const QString &email, const QString &password)
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
                if (obj.contains("refresh_token"))
                    saveRefreshToken(obj["refresh_token"].toString());
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

void FBLinkController::registerUser(const QString &email, const QString &password)
{
    QUrl url(m_apiUrl + "/auth/register");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["email"] = email;
    json["password"] = password;

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit registerCodeSent();
        } else {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QString errStr = tr("Ошибка регистрации");
            if (!doc.isNull() && doc.object().contains("error"))
                errStr = doc.object()["error"].toString();
            emit registerError(errStr);
        }
    });
}

void FBLinkController::verifyEmail(const QString &email, const QString &code)
{
    QUrl url(m_apiUrl + "/auth/verify");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["email"] = email;
    json["code"] = code;

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject obj = doc.object();

            if (obj.contains("access_token")) {
                saveJwtToken(obj["access_token"].toString());
                if (obj.contains("refresh_token"))
                    saveRefreshToken(obj["refresh_token"].toString());
                saveSubscriptionInfo("", "", "");
                emit verifySuccess();
                emit loginStateChanged();
                emit subscriptionChanged();
                fetchConfig();
                fetchSubscription();
            } else {
                emit verifyError(tr("Неверный ответ сервера"));
            }
        } else {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QString errStr = tr("Неверный код");
            if (!doc.isNull() && doc.object().contains("error"))
                errStr = doc.object()["error"].toString();
            emit verifyError(errStr);
        }
    });
}

void FBLinkController::forgotPassword(const QString &email)
{
    QUrl url(m_apiUrl + "/auth/forgot-password");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["email"] = email;

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit forgotPasswordSent();
        } else {
            emit forgotPasswordError(tr("Ошибка отправки кода"));
        }
    });
}

void FBLinkController::resetPassword(const QString &email, const QString &code, const QString &newPassword)
{
    QUrl url(m_apiUrl + "/auth/reset-password");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["email"] = email;
    json["code"] = code;
    json["new_password"] = newPassword;

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit resetPasswordSuccess();
        } else {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QString errStr = tr("Ошибка сброса пароля");
            if (!doc.isNull() && doc.object().contains("error"))
                errStr = doc.object()["error"].toString();
            emit resetPasswordError(errStr);
        }
    });
}

void FBLinkController::loginWithToken(const QString &token)
{
    saveJwtToken(token);
    saveSubscriptionInfo("", "", "");
    emit loginStateChanged();
    emit subscriptionChanged();
    fetchConfig();
    fetchSubscription();
}

void FBLinkController::fetchConfig()
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
                QString region = obj["region"].toString();
                QStringList configStrings = configDataStr.split('\n', Qt::SkipEmptyParts);

                if (m_importController && m_settings && m_serversModel) {
                    QJsonArray servers = m_settings->serversArray();
                    QList<int> existingFBLinkServerIndices;

                    // Locate all existing FBLink VPN servers by description prefix or fblink_server marker
                    for (int i = 0; i < servers.size(); ++i) {
                        QJsonObject server = servers.at(i).toObject();
                        QString desc = server.value("description").toString();
                        QString name = server.value("name").toString();
                        if (desc.startsWith("FBLink VPN") || name.startsWith("FBLink VPN")
                            || server.value("fblink_server").toBool()) {
                            existingFBLinkServerIndices.append(i);
                        }
                    }

                    // For each new config, we'll try to find an exact match to update in-place
                    QList<int> updatedIndices;

                    // Build display name: prefer region, fall back to hostname
                    auto makeDescription = [&region](const QString &hostName) -> QString {
                        if (!region.isEmpty())
                            return "FBLink VPN - " + region;
                        return "FBLink VPN - " + hostName;
                    };

                    for (const QString &configData : configStrings) {
                        if (m_importController->extractConfigFromData(configData)) {
                            QJsonObject newConfig = QJsonDocument::fromJson(m_importController->getConfig().toUtf8()).object();
                            QString newHostName = newConfig.value("hostName").toString();
                            // Берём description из самого конфига (содержит region конкретного сервера)
                            // Fallback на makeDescription только если description отсутствует
                            QString description = newConfig.value("description").toString();
                            if (description.isEmpty())
                                description = makeDescription(newHostName);

                            // Код страны для отображения флага
                            QString countryCode = newConfig.value("country_code").toString().toUpper();

                            bool found = false;
                            for (int i : existingFBLinkServerIndices) {
                                if (updatedIndices.contains(i)) continue;

                                QJsonObject existingServer = servers.at(i).toObject();
                                if (existingServer.value("hostName").toString() == newHostName) {
                                    newConfig["description"] = description;
                                    newConfig["fblink_server"] = true;
                                    if (!countryCode.isEmpty())
                                        newConfig["server_country_code"] = countryCode;
                                    m_serversModel->editServer(newConfig, i);
                                    updatedIndices.append(i);
                                    found = true;
                                    break;
                                }
                            }

                            if (!found) {
                                int countBefore = m_serversModel->getServersCount();
                                m_importController->importConfig();
                                // Tag the newly added server so future fetchConfig() calls can find it
                                if (m_serversModel->getServersCount() > countBefore) {
                                    int newIdx = m_serversModel->getServersCount() - 1;
                                    QJsonObject added = m_settings->serversArray().at(newIdx).toObject();
                                    added["description"] = description;
                                    added["fblink_server"] = true;
                                    if (!countryCode.isEmpty())
                                        added["server_country_code"] = countryCode;
                                    m_serversModel->editServer(added, newIdx);
                                }
                            } else {
                                m_importController->clearConfigFileName();
                            }
                        }
                    }

                    // Remove any FBLink VPN servers that were not in the new config
                    // Iterate backwards so indices don't shift during removal
                    for (int i = existingFBLinkServerIndices.size() - 1; i >= 0; --i) {
                        int serverIndex = existingFBLinkServerIndices.at(i);
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

             // If Unauthorized (401), try refresh before logging out
             if (reply->error() == QNetworkReply::AuthenticationRequiredError ||
                 reply->error() == QNetworkReply::ContentAccessDenied) {
                 refreshAccessToken([this]() {
                     fetchConfig();
                     fetchSubscription();
                 });
             }
        }
    });
}

void FBLinkController::clearExistingFBLinkServers()
{
    if (!m_settings || !m_serversModel) return;
    QJsonArray servers = m_settings->serversArray();
    // Идём в обратном порядке — удаление не сдвигает индексы
    for (int i = servers.size() - 1; i >= 0; --i) {
        QJsonObject server = servers.at(i).toObject();
        QString desc = server.value("description").toString();
        QString name = server.value("name").toString();
        if (desc.startsWith("FBLink VPN") || name.startsWith("FBLink VPN")
            || server.value("fblink_server").toBool()) {
            m_serversModel->removeServer(i);
        }
    }
}

void FBLinkController::createPayment(const QString &plan)
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

void FBLinkController::fetchSubscription()
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
            // Сервер подтвердил статус — записываем время верификации
            m_lastSubscriptionVerifiedAt = QDateTime::currentSecsSinceEpoch();
            if (m_isLoading) {
                m_isLoading = false;
                emit loadingChanged();
            }
            emit subscriptionChanged();
            emit subscriptionFetched();
        } else {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QString errStr = tr("Ошибка сети: ") + reply->errorString();
            if (!doc.isNull() && doc.object().contains("error")) {
                errStr = doc.object()["error"].toString();
            }
            // BUG#8: если 401 — пытаемся обновить access token
            if (reply->error() == QNetworkReply::AuthenticationRequiredError ||
                reply->error() == QNetworkReply::ContentAccessDenied) {
                refreshAccessToken([this]() { fetchSubscription(); });
            } else {
                if (m_isLoading) {
                    m_isLoading = false;
                    emit loadingChanged();
                }
                emit subscriptionError(errStr);
            }
        }
    });
}

void FBLinkController::logout()
{
    saveJwtToken("");
    saveRefreshToken("");
    saveSubscriptionInfo("", "", "");
    emit loginStateChanged();
    emit subscriptionChanged();
}

bool FBLinkController::isLoggedIn() const
{
    return !getJwtToken().isEmpty();
}

bool FBLinkController::isSubscribed() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    QString status = qSettings.value("subscriptionStatus", "").toString();
    if (status != "active") return false;

    // Free plan — не является платной подпиской
    QString plan = qSettings.value("subscriptionPlan", "").toString();
    if (plan == "free" || plan.isEmpty()) return false;

    // Защита от обхода: если последняя серверная верификация > 24ч назад,
    // принудительно перезапрашиваем fetchSubscription(). Предотвращает bypass
    // через ручное редактирование Windows Registry.
    constexpr qint64 kVerifyIntervalSecs = 24 * 3600;
    qint64 now = QDateTime::currentSecsSinceEpoch();
    if (isLoggedIn() && (now - m_lastSubscriptionVerifiedAt) > kVerifyIntervalSecs) {
        // Несинхронно обновляем — не блокируем UI.
        // Срабатывает сигнал subscriptionChanged() и обновяет QSettings.
        const_cast<FBLinkController*>(this)->fetchSubscription();
    }

    // Проверяем дату — backend возвращает ISO дату с наносекундами
    // Qt не парсит 9-значные наносекунды, преобразуем в YYYY-MM-DD
    QString endDateStr = qSettings.value("subscriptionEndDate", "").toString();
    if (endDateStr.isEmpty()) return false;

    QString dateOnly = endDateStr.contains('T') ? endDateStr.left(10) : endDateStr;
    QDate endDate = QDate::fromString(dateOnly, Qt::ISODate);
    if (!endDate.isValid()) return false;
    return QDate::currentDate() <= endDate;
}

QString FBLinkController::subscriptionPlan() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("subscriptionPlan", "").toString();
}

QString FBLinkController::subscriptionEndDate() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("subscriptionEndDate", "").toString();
}

void FBLinkController::saveSubscriptionInfo(const QString &status, const QString &plan, const QString &endDate,
                                             bool autoRenew, bool cardSaved, bool trialAvailable)
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    qSettings.setValue("subscriptionStatus", status);
    qSettings.setValue("subscriptionPlan", plan);
    qSettings.setValue("subscriptionEndDate", endDate);
    qSettings.setValue("subscriptionAutoRenew", autoRenew);
    qSettings.setValue("subscriptionCardSaved", cardSaved);
    qSettings.setValue("subscriptionTrialAvailable", trialAvailable);
    qSettings.sync();
}

bool FBLinkController::autoRenew() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("subscriptionAutoRenew", true).toBool();
}

bool FBLinkController::cardSaved() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("subscriptionCardSaved", false).toBool();
}

bool FBLinkController::trialAvailable() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("subscriptionTrialAvailable", false).toBool();
}

bool FBLinkController::isLoading() const
{
    return m_isLoading;
}

void FBLinkController::setAutoRenew(bool enabled)
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
            QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
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

void FBLinkController::deleteCard()
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
            QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
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

// Обновить пару токенов через refresh_token.
// Если refresh провалится — автоматически вызывается logout().
// onSuccess вызывается после успешного обновления.
void FBLinkController::refreshAccessToken(std::function<void()> onSuccess)
{
    if (m_isRefreshing) return;

    QString refreshToken = getRefreshToken();
    if (refreshToken.isEmpty()) {
        // Нет refresh-токена — сессия истекла
        logout();
        return;
    }

    m_isRefreshing = true;

    QUrl url(m_apiUrl + "/auth/refresh");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["refresh_token"] = refreshToken;

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess]() {
        reply->deleteLater();
        m_isRefreshing = false;

        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            if (obj.contains("access_token")) {
                saveJwtToken(obj["access_token"].toString());
                if (obj.contains("refresh_token"))
                    saveRefreshToken(obj["refresh_token"].toString());
                emit loginStateChanged();
                if (onSuccess) onSuccess();
            } else {
                logout();
            }
        } else {
            // refresh провалился — токен истёк или недействителен
            logout();
        }
    });
}

void FBLinkController::saveJwtToken(const QString &token)
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    qSettings.setValue("authToken", token);
    qSettings.sync();
}

QString FBLinkController::getJwtToken() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("authToken", "").toString();
}

void FBLinkController::saveRefreshToken(const QString &token)
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    qSettings.setValue("refreshToken", token);
    qSettings.sync();
}

QString FBLinkController::getRefreshToken() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("refreshToken", "").toString();
}
