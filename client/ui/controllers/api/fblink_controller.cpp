#include "fblink_controller.h"

#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QUrl>
#include <QDate>
#include <QDateTime>
#include <QTimer>
#include <QVariantList>

// Backend API URL
const QString BACKEND_URL = "https://srv.frakebit.com/api/v1";

namespace
{
bool stringListContainsInsensitive(const QStringList &values, const QString &expected)
{
    for (const QString &value : values) {
        if (value.compare(expected, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

bool isFBLinkServer(const QJsonObject &server)
{
    const QString description = server.value("description").toString();
    const QString name = server.value("name").toString();
    return description.startsWith("FBLink VPN")
            || name.startsWith("FBLink VPN")
            || server.value("fblink_server").toBool();
}

bool serverHasContainer(const QJsonObject &server, const QString &containerName)
{
    const QJsonArray containers = server.value("containers").toArray();
    for (const QJsonValue &containerValue : containers) {
        const QJsonObject containerObject = containerValue.toObject();
        if (containerObject.value("container").toString().compare(containerName, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

int findFBLinkServerIndexByHostName(const QJsonArray &servers, const QString &hostName)
{
    if (hostName.isEmpty()) {
        return -1;
    }

    for (int i = 0; i < servers.size(); ++i) {
        const QJsonObject server = servers.at(i).toObject();
        if (!isFBLinkServer(server)) {
            continue;
        }
        if (server.value("hostName").toString().compare(hostName, Qt::CaseInsensitive) == 0) {
            return i;
        }
    }

    return -1;
}

void reconcileServerDefaultContainer(ServersModel *serversModel, const std::shared_ptr<Settings> &settings,
                                     int serverIndex, const QString &preferredContainer,
                                     const QString &fallbackContainer)
{
    if (!serversModel || !settings || serverIndex < 0) {
        return;
    }

    const QJsonArray syncedServers = settings->serversArray();
    if (serverIndex >= syncedServers.size()) {
        return;
    }

    QJsonObject server = syncedServers.at(serverIndex).toObject();
    QString containerToApply;
    if (!preferredContainer.isEmpty() && serverHasContainer(server, preferredContainer)) {
        containerToApply = preferredContainer;
    } else if (!fallbackContainer.isEmpty() && serverHasContainer(server, fallbackContainer)) {
        containerToApply = fallbackContainer;
    }

    if (containerToApply.isEmpty()) {
        return;
    }

    if (server.value("defaultContainer").toString().compare(containerToApply, Qt::CaseInsensitive) == 0) {
        return;
    }

    server["defaultContainer"] = containerToApply;
    serversModel->editServer(server, serverIndex);
}

QString networkOperationToString(QNetworkAccessManager::Operation operation)
{
    switch (operation) {
    case QNetworkAccessManager::HeadOperation:
        return "HEAD";
    case QNetworkAccessManager::GetOperation:
        return "GET";
    case QNetworkAccessManager::PutOperation:
        return "PUT";
    case QNetworkAccessManager::PostOperation:
        return "POST";
    case QNetworkAccessManager::DeleteOperation:
        return "DELETE";
    case QNetworkAccessManager::CustomOperation:
        return "CUSTOM";
    default:
        return "UNKNOWN";
    }
}
} // namespace

FBLinkController::FBLinkController(ImportController *importController,
                                     const std::shared_ptr<Settings> &settings,
                                     ServersModel *serversModel, QObject *parent)
    : QObject(parent),
      m_importController(importController),
      m_settings(settings),
      m_serversModel(serversModel)
{
    m_nam = new QNetworkAccessManager(this);
    m_nam->setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    m_apiUrl = BACKEND_URL;

    // Lazy sync at startup: если пользователь уже залогинен — сразу идём в bootstrap
    // через текущий access token. Refresh произойдёт только при реальном 401 внутри
    // fetchSubscription()/fetchConfig(), что убирает лишний round-trip на каждый запуск.
    if (isLoggedIn()) {
        setLoadingState(true);
        QTimer::singleShot(0, this, [this]() {
            beginSessionSync();
        });
    }
}

void FBLinkController::beginSessionSync()
{
    m_fetchConfigAfterSubscription = true;
    fetchSubscription(true);
}

void FBLinkController::syncAll()
{
    if (isLoggedIn()) {
        setLoadingState(true);
        beginSessionSync();
    }
}

QNetworkRequest FBLinkController::createApiRequest(const QString &path, bool isJsonRequest, bool authorized) const
{
    QNetworkRequest request(QUrl(m_apiUrl + path));
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    if (isJsonRequest) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    }

    if (authorized) {
        const QString token = getJwtToken();
        if (!token.isEmpty()) {
            request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
        }
    }

    return request;
}

void FBLinkController::logApiFailure(const QString &operationName, QNetworkReply *reply) const
{
    if (!reply) {
        return;
    }

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qWarning().noquote()
        << QString("[FBLink API] %1 failed: %2 %3, http=%4, error=%5 (%6)")
              .arg(operationName,
                   networkOperationToString(reply->operation()),
                   reply->request().url().toString(),
                   QString::number(httpStatus),
                   QString::number(reply->error()),
                   reply->errorString());
}

bool FBLinkController::shouldRefreshToken(QNetworkReply *reply) const
{
    if (!reply) {
        return false;
    }

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return reply->error() == QNetworkReply::AuthenticationRequiredError
           || reply->error() == QNetworkReply::ContentAccessDenied
           || httpStatus == 401;
}

void FBLinkController::setLoadingState(bool isLoading)
{
    if (m_isLoading == isLoading) {
        return;
    }

    m_isLoading = isLoading;
    emit loadingChanged();
}

void FBLinkController::login(const QString &email, const QString &password)
{
    if (email.isEmpty() || password.isEmpty()) {
        emit loginError(tr("Email и пароль не могут быть пустыми"));
        return;
    }

    QNetworkRequest request = createApiRequest("/auth/login", true, false);

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
                setLoadingState(true);
                emit loginSuccess();
                emit loginStateChanged();
                emit subscriptionChanged();

                beginSessionSync();
            } else {
                setLoadingState(false);
                emit loginError(tr("Некорректный формат ответа сервера"));
            }
        } else {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QString errStr = tr("Ошибка сети: ") + reply->errorString();
            if(!doc.isNull() && doc.object().contains("error")) {
                errStr = doc.object()["error"].toString();
            }
            logApiFailure("login", reply);
            setLoadingState(false);
            emit loginError(errStr);
        }
    });
}

void FBLinkController::registerUser(const QString &email, const QString &password)
{
    QNetworkRequest request = createApiRequest("/auth/register", true, false);

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
            logApiFailure("register", reply);
            emit registerError(errStr);
        }
    });
}

void FBLinkController::verifyEmail(const QString &email, const QString &code)
{
    QNetworkRequest request = createApiRequest("/auth/verify", true, false);

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
                setLoadingState(true);
                emit verifySuccess();
                emit loginStateChanged();
                emit subscriptionChanged();
                beginSessionSync();
            } else {
                setLoadingState(false);
                emit verifyError(tr("Неверный ответ сервера"));
            }
        } else {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QString errStr = tr("Неверный код");
            if (!doc.isNull() && doc.object().contains("error"))
                errStr = doc.object()["error"].toString();
            logApiFailure("verify", reply);
            setLoadingState(false);
            emit verifyError(errStr);
        }
    });
}

void FBLinkController::forgotPassword(const QString &email)
{
    QNetworkRequest request = createApiRequest("/auth/forgot-password", true, false);

    QJsonObject json;
    json["email"] = email;

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit forgotPasswordSent();
        } else {
            logApiFailure("forgot-password", reply);
            emit forgotPasswordError(tr("Ошибка отправки кода"));
        }
    });
}

void FBLinkController::resetPassword(const QString &email, const QString &code, const QString &newPassword)
{
    QNetworkRequest request = createApiRequest("/auth/reset-password", true, false);

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
            logApiFailure("reset-password", reply);
            emit resetPasswordError(errStr);
        }
    });
}

void FBLinkController::loginWithToken(const QString &token)
{
    saveJwtToken(token);
    saveSubscriptionInfo("", "", "");
    setLoadingState(true);
    emit loginStateChanged();
    emit subscriptionChanged();
    beginSessionSync();
}

void FBLinkController::fetchConfig()
{
    fetchConfig(true);
}

void FBLinkController::fetchConfig(bool allowRefreshRetry)
{
    QString token = getJwtToken();
    if (token.isEmpty()) {
        emit configError(tr("Не выполнен вход в систему"));
        return;
    }

    QNetworkRequest request = createApiRequest("/me/config", false, true);

    QNetworkReply *reply = m_nam->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, allowRefreshRetry]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QJsonObject obj = doc.object();

            if (obj.contains("configs") || obj.contains("config")) {
                QStringList configStrings;
                if (obj.contains("configs") && obj["configs"].isArray()) {
                    QJsonArray configsArr = obj["configs"].toArray();
                    for (const QJsonValue &val : configsArr) {
                        if (val.isString()) configStrings.append(val.toString());
                        else if (val.isObject()) {
                            // Support array of objects { config: "vpn://...", region: "NL" }
                            QJsonObject cObj = val.toObject();
                            if (cObj.contains("config")) configStrings.append(cObj["config"].toString());
                        }
                    }
                } else if (obj.contains("config")) {
                    QString configDataStr = obj["config"].toString();
                    configStrings = configDataStr.split('\n', Qt::SkipEmptyParts);
                }
                QString region = obj["region"].toString();
                const bool vipAdBlockRequested = obj.value("vip_ad_block_requested").toBool(false);
                const bool vipAdBlockApplied = obj.value("vip_ad_block_applied").toBool(false);
                const QString vipAdBlockStatus = obj.value("vip_ad_block_status").toString();
                const QString vipAdBlockDnsSource = obj.value("vip_ad_block_dns_source").toString();

                qDebug() << "[FBLink] fetchConfig: vip_ad_block_requested =" << vipAdBlockRequested
                         << "vip_ad_block_applied =" << vipAdBlockApplied
                         << "vip_ad_block_status =" << vipAdBlockStatus
                         << "vip_ad_block_dns_source =" << vipAdBlockDnsSource;

                if (m_importController && m_settings && m_serversModel) {
                    QJsonArray servers = m_settings->serversArray();
                    const int currentDefaultServerIndex = m_serversModel->getDefaultServerIndex();
                    QString selectedFBLinkHostName;
                    QString selectedFBLinkContainer;
                    if (currentDefaultServerIndex >= 0 && currentDefaultServerIndex < servers.size()) {
                        const QJsonObject currentDefaultServer = servers.at(currentDefaultServerIndex).toObject();
                        if (isFBLinkServer(currentDefaultServer)) {
                            selectedFBLinkHostName = currentDefaultServer.value("hostName").toString();
                            selectedFBLinkContainer = currentDefaultServer.value("defaultContainer").toString();
                        }
                    }

                    QList<int> existingFBLinkServerIndices;

                    // Locate all existing FBLink VPN servers by description prefix or fblink_server marker
                    for (int i = 0; i < servers.size(); ++i) {
                        QJsonObject server = servers.at(i).toObject();
                        if (isFBLinkServer(server)) {
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

                    const QStringList subscriptionProtocols = allowedProtocols();
                    QString preferredContainer = "fblink-awg";
                    if (stringListContainsInsensitive(subscriptionProtocols, "vless")
                        || stringListContainsInsensitive(subscriptionProtocols, "xray")
                        || subscriptionPlan().compare("vip", Qt::CaseInsensitive) == 0) {
                        preferredContainer = "fblink-xray";
                    }

                    const QJsonArray syncedServers = m_settings->serversArray();
                    const int preservedServerIndex = findFBLinkServerIndexByHostName(syncedServers, selectedFBLinkHostName);
                    int preferredServerIndex = -1;
                    for (int i = 0; i < syncedServers.size(); ++i) {
                        QJsonObject server = syncedServers.at(i).toObject();
                        if (!isFBLinkServer(server)) {
                            continue;
                        }

                        const QString defaultContainer = server.value("defaultContainer").toString();
                        if (defaultContainer.compare(preferredContainer, Qt::CaseInsensitive) == 0) {
                            preferredServerIndex = i;
                            break;
                        }

                        if (serverHasContainer(server, preferredContainer)) {
                            server["defaultContainer"] = preferredContainer;
                            m_serversModel->editServer(server, i);
                            preferredServerIndex = i;
                            break;
                        }
                    }

                    const int finalServerIndex = preservedServerIndex >= 0 ? preservedServerIndex : preferredServerIndex;
                    if (finalServerIndex >= 0) {
                        if (preservedServerIndex >= 0) {
                            reconcileServerDefaultContainer(m_serversModel, m_settings, preservedServerIndex,
                                                            preferredContainer, selectedFBLinkContainer);
                        }
                        m_serversModel->setDefaultServerIndex(finalServerIndex);
                        m_serversModel->setProcessedServerIndex(finalServerIndex);
                    }

                    emit configFetched();
                } else {
                    setLoadingState(false);
                    emit configError(tr("Внутренняя ошибка: Контроллеры не инициализированы"));
                }
            } else {
                setLoadingState(false);
                emit configError(tr("Сервер не вернул конфигурацию"));
            }
        } else {
             QByteArray responseData = reply->readAll();
             QJsonDocument doc = QJsonDocument::fromJson(responseData);
             QString errStr = tr("Ошибка сети: ") + reply->errorString();
             if(!doc.isNull() && doc.object().contains("error")) {
                 errStr = doc.object()["error"].toString();
             }
             logApiFailure("fetch-config", reply);

             if (allowRefreshRetry && shouldRefreshToken(reply)) {
                 refreshAccessToken([this]() {
                     fetchConfig(false);
                 });
                 return;
             }

             setLoadingState(false);
             emit configError(errStr);
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
    createPayment(plan, true);
}

void FBLinkController::createPayment(const QString &plan, bool allowRefreshRetry)
{
    QString token = getJwtToken();
    if (token.isEmpty()) {
        emit paymentError(tr("Необходимо войти в аккаунт"));
        return;
    }

    QNetworkRequest request = createApiRequest("/payments/create", true, true);

    QJsonObject json;
    json["plan"] = plan;

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, plan, allowRefreshRetry]() {
        reply->deleteLater();

        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject obj = doc.object();

        if (reply->error() == QNetworkReply::NoError) {
            QString confirmUrl = obj.value("confirmation_url").toString();
            emit paymentCreated(confirmUrl);
        } else {
            logApiFailure("create-payment", reply);

            if (allowRefreshRetry && shouldRefreshToken(reply)) {
                refreshAccessToken([this, plan]() {
                    createPayment(plan, false);
                });
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
    fetchSubscription(true);
}

void FBLinkController::fetchSubscription(bool allowRefreshRetry)
{
    QString token = getJwtToken();
    if (token.isEmpty()) {
        emit subscriptionError(tr("Не выполнен вход в систему"));
        return;
    }

    QNetworkRequest request = createApiRequest("/me/subscription", false, true);

    QNetworkReply *reply = m_nam->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, allowRefreshRetry]() {
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
            QStringList allowedProtocols;
            for (const QJsonValue &item : obj.value("allowed_protocols").toArray()) {
                allowedProtocols.append(item.toString());
            }
            const bool canUseSiteSplitTunneling = obj.value("can_use_site_split_tunneling").toBool(false);
            const bool canUseAppSplitTunneling = obj.value("can_use_app_split_tunneling").toBool(false);
            const bool canManageRoutingProfiles = obj.value("can_manage_routing_profiles").toBool(false);
            const bool canUseAdBlock = obj.value("can_use_ad_block").toBool(false);
            const bool vipAdBlockEnabled = obj.value("vip_ad_block_enabled").toBool(false);

            saveSubscriptionInfo(status, plan, endDate, autoRenew, cardSaved, trialAvailable,
                                 allowedProtocols, canUseSiteSplitTunneling, canUseAppSplitTunneling,
                                 canManageRoutingProfiles, canUseAdBlock, vipAdBlockEnabled);
            // Сервер подтвердил статус — записываем время верификации
            m_lastSubscriptionVerifiedAt = QDateTime::currentSecsSinceEpoch();
            const bool shouldFetchConfig = m_fetchConfigAfterSubscription;
            m_fetchConfigAfterSubscription = false;
            setLoadingState(false);
            emit subscriptionChanged();
            emit subscriptionFetched();
            if (shouldFetchConfig) {
                if (isSubscribed()) {
                    fetchConfig(true);
                } else {
                    clearExistingFBLinkServers();
                }
            }
        } else {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QString errStr = tr("Ошибка сети: ") + reply->errorString();
            if (!doc.isNull() && doc.object().contains("error")) {
                errStr = doc.object()["error"].toString();
            }
            logApiFailure("fetch-subscription", reply);

            if (allowRefreshRetry && shouldRefreshToken(reply)) {
                refreshAccessToken([this]() {
                    fetchSubscription(false);
                });
                return;
            }

            m_fetchConfigAfterSubscription = false;
            setLoadingState(false);
            emit subscriptionError(errStr);
        }
    });
}

void FBLinkController::logout()
{
    m_pendingRefreshCallbacks.clear();
    m_isRefreshing = false;
    setLoadingState(false);
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

QStringList FBLinkController::allowedProtocols() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("subscriptionAllowedProtocols").toStringList();
}

QString FBLinkController::subscriptionEndDate() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("subscriptionEndDate", "").toString();
}

void FBLinkController::saveSubscriptionInfo(const QString &status, const QString &plan, const QString &endDate,
                                             bool autoRenew, bool cardSaved, bool trialAvailable,
                                             const QStringList &allowedProtocols, bool canUseSiteSplitTunneling,
                                             bool canUseAppSplitTunneling, bool canManageRoutingProfiles,
                                             bool canUseAdBlock, bool vipAdBlockEnabled)
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    qSettings.setValue("subscriptionStatus", status);
    qSettings.setValue("subscriptionPlan", plan);
    qSettings.setValue("subscriptionEndDate", endDate);
    qSettings.setValue("subscriptionAutoRenew", autoRenew);
    qSettings.setValue("subscriptionCardSaved", cardSaved);
    qSettings.setValue("subscriptionTrialAvailable", trialAvailable);
    qSettings.setValue("subscriptionAllowedProtocols", allowedProtocols);
    qSettings.setValue("subscriptionCanUseSiteSplitTunneling", canUseSiteSplitTunneling);
    qSettings.setValue("subscriptionCanUseAppSplitTunneling", canUseAppSplitTunneling);
    qSettings.setValue("subscriptionCanManageRoutingProfiles", canManageRoutingProfiles);
    qSettings.setValue("subscriptionCanUseAdBlock", canUseAdBlock);
    qSettings.setValue("subscriptionVIPAdBlockEnabled", vipAdBlockEnabled);
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

bool FBLinkController::canUseSiteSplitTunneling() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("subscriptionCanUseSiteSplitTunneling", false).toBool();
}

bool FBLinkController::canUseAppSplitTunneling() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("subscriptionCanUseAppSplitTunneling", false).toBool();
}

bool FBLinkController::canManageRoutingProfiles() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("subscriptionCanManageRoutingProfiles", false).toBool();
}

bool FBLinkController::canUseAdBlock() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("subscriptionCanUseAdBlock", false).toBool();
}

bool FBLinkController::vipAdBlockEnabled() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("subscriptionVIPAdBlockEnabled", false).toBool();
}

bool FBLinkController::isLoading() const
{
    return m_isLoading;
}

void FBLinkController::setAutoRenew(bool enabled)
{
    setAutoRenew(enabled, true);
}

void FBLinkController::setVipAdBlockEnabled(bool enabled)
{
    setVipAdBlockEnabled(enabled, true);
}

void FBLinkController::setAutoRenew(bool enabled, bool allowRefreshRetry)
{
    QString token = getJwtToken();
    if (token.isEmpty()) return;

    QNetworkRequest request = createApiRequest("/me/subscription/auto-renew", true, true);

    QJsonObject json;
    json["enabled"] = enabled;

    QNetworkReply *reply = m_nam->sendCustomRequest(request, "PATCH", QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, enabled, allowRefreshRetry]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
            qSettings.setValue("subscriptionAutoRenew", enabled);
            qSettings.sync();
            emit autoRenewChanged(enabled);
            emit subscriptionChanged();
        } else {
            logApiFailure("set-auto-renew", reply);
            if (allowRefreshRetry && shouldRefreshToken(reply)) {
                refreshAccessToken([this, enabled]() {
                    setAutoRenew(enabled, false);
                });
                return;
            }

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QString errStr = doc.object().value("error").toString(reply->errorString());
            emit requestError(errStr);
        }
    });
}

void FBLinkController::setVipAdBlockEnabled(bool enabled, bool allowRefreshRetry)
{
    QString token = getJwtToken();
    if (token.isEmpty()) return;

    QNetworkRequest request = createApiRequest("/me/subscription/ad-block", true, true);

    QJsonObject json;
    json["enabled"] = enabled;

    QNetworkReply *reply = m_nam->sendCustomRequest(request, "PATCH", QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, enabled, allowRefreshRetry]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
            qSettings.setValue("subscriptionVIPAdBlockEnabled", enabled);
            qSettings.sync();
            emit vipAdBlockChanged(enabled);
            emit subscriptionChanged();
            if (isSubscribed()) {
                fetchConfig(true);
            }
        } else {
            logApiFailure("set-vip-ad-block", reply);
            if (allowRefreshRetry && shouldRefreshToken(reply)) {
                refreshAccessToken([this, enabled]() {
                    setVipAdBlockEnabled(enabled, false);
                });
                return;
            }

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QString errStr = doc.object().value("error").toString(reply->errorString());
            emit requestError(errStr);
        }
    });
}

void FBLinkController::deleteCard()
{
    deleteCard(true);
}

void FBLinkController::deleteCard(bool allowRefreshRetry)
{
    QString token = getJwtToken();
    if (token.isEmpty()) return;

    QNetworkRequest request = createApiRequest("/me/card", false, true);

    QNetworkReply *reply = m_nam->deleteResource(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, allowRefreshRetry]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
            qSettings.setValue("subscriptionCardSaved", false);
            qSettings.setValue("subscriptionAutoRenew", false);
            qSettings.sync();
            emit cardDeleted();
            emit subscriptionChanged();
        } else {
            logApiFailure("delete-card", reply);
            if (allowRefreshRetry && shouldRefreshToken(reply)) {
                refreshAccessToken([this]() {
                    deleteCard(false);
                });
                return;
            }

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QString errStr = doc.object().value("error").toString(reply->errorString());
            emit requestError(errStr);
        }
    });
}

void FBLinkController::fetchRoutingProfiles()
{
    fetchRoutingProfiles(true);
}

void FBLinkController::fetchRoutingProfiles(bool allowRefreshRetry)
{
    QString token = getJwtToken();
    if (token.isEmpty()) {
        emit routingProfilesError(tr("Необходимо войти в аккаунт"));
        return;
    }

    QNetworkRequest request = createApiRequest("/me/routing-profiles", false, true);

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, allowRefreshRetry]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            emit routingProfilesFetched(obj.value("profiles").toArray().toVariantList());
        } else {
            logApiFailure("fetch-routing-profiles", reply);
            if (allowRefreshRetry && shouldRefreshToken(reply)) {
                refreshAccessToken([this]() {
                    fetchRoutingProfiles(false);
                });
                return;
            }

            QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            emit routingProfilesError(obj.value("error").toString(reply->errorString()));
        }
    });
}

void FBLinkController::saveRoutingProfile(const QVariantMap &profile)
{
    saveRoutingProfile(profile, true);
}

void FBLinkController::saveRoutingProfile(const QVariantMap &profile, bool allowRefreshRetry)
{
    QString token = getJwtToken();
    if (token.isEmpty()) {
        emit routingProfilesError(tr("Необходимо войти в аккаунт"));
        return;
    }

    const int id = profile.value("id").toInt();
    const bool hasId = id > 0;

    QNetworkRequest request = createApiRequest(hasId ? QString("/me/routing-profiles/%1").arg(id)
                                                     : "/me/routing-profiles",
                                               true, true);

    QJsonObject payload = QJsonObject::fromVariantMap(profile);
    QNetworkReply *reply = hasId
        ? m_nam->put(request, QJsonDocument(payload).toJson())
        : m_nam->post(request, QJsonDocument(payload).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, profile, allowRefreshRetry]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit routingProfileSaved();
            fetchRoutingProfiles(true);
            if (isSubscribed()) {
                fetchConfig(true);
            }
        } else {
            logApiFailure("save-routing-profile", reply);
            if (allowRefreshRetry && shouldRefreshToken(reply)) {
                refreshAccessToken([this, profile]() {
                    saveRoutingProfile(profile, false);
                });
                return;
            }

            QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            emit routingProfilesError(obj.value("error").toString(reply->errorString()));
        }
    });
}

void FBLinkController::deleteRoutingProfile(int id)
{
    deleteRoutingProfile(id, true);
}

void FBLinkController::deleteRoutingProfile(int id, bool allowRefreshRetry)
{
    QString token = getJwtToken();
    if (token.isEmpty()) {
        emit routingProfilesError(tr("Необходимо войти в аккаунт"));
        return;
    }

    QNetworkRequest request = createApiRequest(QString("/me/routing-profiles/%1").arg(id), false, true);

    QNetworkReply *reply = m_nam->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, id, allowRefreshRetry]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit routingProfileDeleted();
            fetchRoutingProfiles(true);
            if (isSubscribed()) {
                fetchConfig(true);
            }
        } else {
            logApiFailure("delete-routing-profile", reply);
            if (allowRefreshRetry && shouldRefreshToken(reply)) {
                refreshAccessToken([this, id]() {
                    deleteRoutingProfile(id, false);
                });
                return;
            }

            QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            emit routingProfilesError(obj.value("error").toString(reply->errorString()));
        }
    });
}

// Обновить пару токенов через refresh_token.
// Если refresh провалится — автоматически вызывается logout().
// onSuccess вызывается после успешного обновления.
void FBLinkController::refreshAccessToken(std::function<void()> onSuccess)
{
    if (onSuccess) {
        m_pendingRefreshCallbacks.append(onSuccess);
    }

    if (m_isRefreshing) {
        qDebug() << "[FBLink API] refresh already in progress, queued callback";
        return;
    }

    QString refreshToken = getRefreshToken();
    if (refreshToken.isEmpty()) {
        // Нет refresh-токена — сессия истекла
        m_pendingRefreshCallbacks.clear();
        logout();
        return;
    }

    m_isRefreshing = true;

    QNetworkRequest request = createApiRequest("/auth/refresh", true, false);

    QJsonObject json;
    json["refresh_token"] = refreshToken;

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_isRefreshing = false;
        const auto callbacks = m_pendingRefreshCallbacks;
        m_pendingRefreshCallbacks.clear();

        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            if (obj.contains("access_token")) {
                saveJwtToken(obj["access_token"].toString());
                if (obj.contains("refresh_token"))
                    saveRefreshToken(obj["refresh_token"].toString());
                emit loginStateChanged();
                for (const auto &callback : callbacks) {
                    if (callback) {
                        callback();
                    }
                }
            } else {
                qWarning() << "[FBLink API] refresh succeeded without access_token";
                logout();
            }
        } else {
            // refresh провалился — токен истёк или недействителен
            logApiFailure("refresh-token", reply);
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
