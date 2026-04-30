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
#include <QSysInfo>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QGuiApplication>
#include <QClipboard>
#include <QLocale>
#include <memory>

// Backend API URL
const QString BACKEND_URL = "https://srv.frakebit.com/api/v1";

namespace
{
constexpr char kLastSelectedFBLinkHostNameKey[] = "Conf/lastSelectedFBLinkHostName";
constexpr char kLastSelectedServerIdKey[] = "Conf/lastSelectedServerId";
constexpr char kVIPEnabledProfilesCountKey[] = "Conf/vipEnabledProfilesCount";
constexpr char kVIPLastActiveProfileIDKey[] = "Conf/vipLastActiveRoutingProfileId";
constexpr char kVIPLastKnownRoutingHashKey[] = "Conf/vipLastKnownGoodRoutingRulesHash";
constexpr char kLastActiveRoutingProfileIdKey[] = "Conf/lastActiveRoutingProfileId";
constexpr char kLastKnownGoodRoutingRulesHashKey[] = "Conf/lastKnownGoodRoutingRulesHash";
constexpr char kVIPAdBlockStatusKey[] = "subscriptionVIPAdBlockStatus";
constexpr char kVIPAdBlockDnsSourceKey[] = "subscriptionVIPAdBlockDnsSource";
constexpr char kVIPAdBlockDegradeReasonKey[] = "subscriptionVIPAdBlockDegradeReason";
constexpr char kUserEmailKey[] = "accountEmail";
constexpr char kSafeModeUntilEpochKey[] = "Conf/safeModeUntilEpochSec";
constexpr char kShowNewFeaturesGuideKey[] = "Conf/showNewFeaturesGuide";
constexpr int kPendingRoutingSyncRetryDelayMs = 1500;
constexpr int kPendingRoutingSyncMaxFetchFailures = 3;
constexpr int kPendingRoutingSyncMaxBlockMs = 30000;

QSettings appSettings()
{
    return QSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
}

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

QString safeTrimmedReason(const QString &reason)
{
    QString normalized = reason.trimmed();
    if (normalized == "none" || normalized == "ok") {
        normalized.clear();
    }
    return normalized;
}

QString adBlockStatusLabel(const QString &rawStatus, bool enabled)
{
    if (!enabled) {
        return QObject::tr("Выключен");
    }

    const QString normalized = rawStatus.trimmed().toLower();
    if (normalized == "applied") {
        return QObject::tr("Работает");
    }
    if (normalized == "degraded" || normalized == "unavailable") {
        return QObject::tr("Временно недоступен");
    }

    // Optimistic UI: until fetchConfig() confirms degradation, keep AdBlock as active.
    return QObject::tr("Работает");
}

QString sanitizeSensitiveTokens(const QString &input)
{
    QString sanitized = input;
    static const QRegularExpression bearerRegex(QStringLiteral("(Bearer\\s+)[A-Za-z0-9\\-_.]+"),
                                                 QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression jwtRegex(QStringLiteral("([A-Za-z0-9\\-_]+\\.[A-Za-z0-9\\-_]+\\.[A-Za-z0-9\\-_]+)"));
    static const QRegularExpression emailRegex(QStringLiteral("([A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,})"),
                                               QRegularExpression::CaseInsensitiveOption);

    sanitized.replace(bearerRegex, QStringLiteral("\\1***"));
    sanitized.replace(jwtRegex, QStringLiteral("***"));
    sanitized.replace(emailRegex, QStringLiteral("***@***"));
    return sanitized;
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
    if (!getJwtToken().isEmpty()) {
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
    if (isLoading) {
        m_loadingOperationsCount++;
    } else {
        m_loadingOperationsCount--;
        if (m_loadingOperationsCount < 0) {
            m_loadingOperationsCount = 0;
        }
    }
    
    bool newIsLoading = (m_loadingOperationsCount > 0);
    if (m_isLoading != newIsLoading) {
        m_isLoading = newIsLoading;
        emit loadingChanged();
    }
}

void FBLinkController::setConfigSyncState(bool isSyncing)
{
    if (isSyncing) {
        m_configSyncOperationsCount++;
    } else {
        m_configSyncOperationsCount--;
        if (m_configSyncOperationsCount < 0) {
            m_configSyncOperationsCount = 0;
        }
    }
    
    bool currentlySyncing = m_configSyncOperationsCount > 0;
    if (m_isConfigSyncing != currentlySyncing) {
        m_isConfigSyncing = currentlySyncing;
        emit configSyncChanged();
    }
}

void FBLinkController::setPendingRoutingSync(bool pending)
{
    if (pending) {
        m_pendingRoutingSyncSinceMs = QDateTime::currentMSecsSinceEpoch();
        m_pendingRoutingSyncFetchFailures = 0;
    } else {
        m_pendingRoutingSyncSinceMs = 0;
        m_pendingRoutingSyncFetchFailures = 0;
    }

    if (m_hasPendingRoutingSync == pending) {
        return;
    }

    m_hasPendingRoutingSync = pending;
    emit routingSyncPendingChanged();
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

    connect(reply, &QNetworkReply::finished, this, [this, reply, email]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QJsonObject obj = doc.object();

            if (obj.contains("access_token")) {
                saveJwtToken(obj["access_token"].toString());
                if (obj.contains("refresh_token"))
                    saveRefreshToken(obj["refresh_token"].toString());
                setUserEmail(email);
                saveSubscriptionInfo("", "", "");  // Clear stale data from previous user
                emit loginSuccess();
                emit loginStateChanged();
                emit subscriptionChanged();

                beginSessionSync();
            } else {
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

    connect(reply, &QNetworkReply::finished, this, [this, reply, email]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject obj = doc.object();

            if (obj.contains("access_token")) {
                saveJwtToken(obj["access_token"].toString());
                if (obj.contains("refresh_token"))
                    saveRefreshToken(obj["refresh_token"].toString());
                setUserEmail(email);
                saveSubscriptionInfo("", "", "");
                emit verifySuccess();
                emit loginStateChanged();
                emit subscriptionChanged();
                beginSessionSync();
            } else {
                emit verifyError(tr("Неверный ответ сервера"));
            }
        } else {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QString errStr = tr("Неверный код");
            if (!doc.isNull() && doc.object().contains("error"))
                errStr = doc.object()["error"].toString();
            logApiFailure("verify", reply);
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
    setUserEmail("");
    saveSubscriptionInfo("", "", "");
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

    if (m_isFetchingConfig) {
        m_pendingConfigFetch = true;
        m_pendingConfigAllowRefreshRetry = m_pendingConfigAllowRefreshRetry || allowRefreshRetry;
        qDebug() << "[FBLink API] fetch-config already in progress, coalescing request";
        return;
    }

    m_isFetchingConfig = true;

    setConfigSyncState(true);
    QNetworkRequest request = createApiRequest("/me/config", false, true);

    QNetworkReply *reply = m_nam->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, allowRefreshRetry]() {
        reply->deleteLater();
        m_isFetchingConfig = false;
        std::shared_ptr<void> guard(nullptr, [this](void*){ setConfigSyncState(false); });
        const bool rerunFetch = m_pendingConfigFetch;
        const bool rerunAllowRefreshRetry = m_pendingConfigAllowRefreshRetry;
        m_pendingConfigFetch = false;
        m_pendingConfigAllowRefreshRetry = true;

        const auto rerunCoalescedFetch = [this, rerunFetch, rerunAllowRefreshRetry]() {
            if (!rerunFetch) {
                return;
            }
            QTimer::singleShot(0, this, [this, rerunAllowRefreshRetry]() {
                fetchConfig(rerunAllowRefreshRetry);
            });
        };

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
                const QString vipAdBlockStatus = obj.value("vip_ad_block_status").toString().trimmed().toLower();
                const QString vipAdBlockDnsSource = obj.value("vip_ad_block_dns_source").toString();
                QString vipAdBlockDegradeReason = safeTrimmedReason(obj.value("vip_ad_block_degrade_reason").toString());
                if (vipAdBlockDegradeReason.isEmpty()) {
                    vipAdBlockDegradeReason = safeTrimmedReason(obj.value("degrade_reason").toString());
                }

                qDebug() << "[FBLink] fetchConfig: vip_ad_block_requested =" << vipAdBlockRequested
                         << "vip_ad_block_applied =" << vipAdBlockApplied
                         << "vip_ad_block_status =" << vipAdBlockStatus
                         << "vip_ad_block_dns_source =" << vipAdBlockDnsSource
                         << "vip_ad_block_degrade_reason =" << vipAdBlockDegradeReason;

                {
                    QSettings qSettings = appSettings();
                    qSettings.setValue(kVIPAdBlockStatusKey, vipAdBlockStatus);
                    qSettings.setValue(kVIPAdBlockDnsSourceKey, vipAdBlockDnsSource);
                    qSettings.setValue(kVIPAdBlockDegradeReasonKey, vipAdBlockDegradeReason);

                    bool hasManagedRoutingSnapshot = false;
                    for (const QString &configData : configStrings) {
                        if (configData.contains("\"routing\"") && configData.contains("\"rules\"")) {
                            hasManagedRoutingSnapshot = true;
                            break;
                        }
                    }
                    if (hasManagedRoutingSnapshot) {
                        const QString joinedConfigs = configStrings.join('\n');
                        const QString configHash = QString::fromLatin1(
                                QCryptographicHash::hash(joinedConfigs.toUtf8(), QCryptographicHash::Sha1).toHex());
                        qSettings.setValue(kVIPLastKnownRoutingHashKey, configHash);
                        qSettings.setValue(kLastKnownGoodRoutingRulesHashKey, configHash);
                    }
                    qSettings.sync();
                }
                emit subscriptionChanged();

                if (m_importController && m_settings && m_serversModel) {
                    QJsonArray servers = m_settings->serversArray();
                    const int currentDefaultServerIndex = m_serversModel->getDefaultServerIndex();
                    QSettings qSettings = appSettings();
                    const QString persistedSelectedHost = qSettings.value(kLastSelectedFBLinkHostNameKey, "").toString();
                    QString selectedFBLinkHostName;
                    QString selectedFBLinkContainer;
                    if (currentDefaultServerIndex >= 0 && currentDefaultServerIndex < servers.size()) {
                        const QJsonObject currentDefaultServer = servers.at(currentDefaultServerIndex).toObject();
                        if (isFBLinkServer(currentDefaultServer)) {
                            selectedFBLinkHostName = currentDefaultServer.value("hostName").toString();
                            selectedFBLinkContainer = currentDefaultServer.value("defaultContainer").toString();
                            if (!selectedFBLinkHostName.isEmpty()) {
                                qSettings.setValue(kLastSelectedFBLinkHostNameKey, selectedFBLinkHostName);
                                const QJsonValue serverIdValue = currentDefaultServer.value("id");
                                if (serverIdValue.isDouble()) {
                                    qSettings.setValue(kLastSelectedServerIdKey, serverIdValue.toInt());
                                } else if (serverIdValue.isString()) {
                                    qSettings.setValue(kLastSelectedServerIdKey, serverIdValue.toString());
                                } else {
                                    qSettings.setValue(kLastSelectedServerIdKey, selectedFBLinkHostName);
                                }
                            }
                        }
                    }
                    if (selectedFBLinkHostName.isEmpty()) {
                        selectedFBLinkHostName = persistedSelectedHost;
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

                    // Use pinned-host fast-path only when we already have a local FBLink server list.
                    // After reinstall, Android Auto Backup may restore selected host in settings,
                    // while local servers are empty. In that bootstrap case we must import full list.
                    const bool hasLocalFBLinkSnapshot = !existingFBLinkServerIndices.isEmpty();
                    const bool hasPinnedSelectedHost =
                            hasLocalFBLinkSnapshot && !selectedFBLinkHostName.isEmpty();
                    bool selectedConfigSeenInResponse = false;

                    for (const QString &configData : configStrings) {
                        if (m_importController->extractConfigFromData(configData)) {
                            QJsonObject newConfig = QJsonDocument::fromJson(m_importController->getConfig().toUtf8()).object();
                            QString newHostName = newConfig.value("hostName").toString();

                            // Fast-path sync mode: when user already selected a server,
                            // apply only that server config and ignore the rest.
                            if (hasPinnedSelectedHost
                                && newHostName.compare(selectedFBLinkHostName, Qt::CaseInsensitive) != 0) {
                                m_importController->clearConfigFileName();
                                continue;
                            }
                            if (hasPinnedSelectedHost
                                && newHostName.compare(selectedFBLinkHostName, Qt::CaseInsensitive) == 0) {
                                selectedConfigSeenInResponse = true;
                            }

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

                    // Full cleanup is safe only when there is no pinned selected server.
                    // With pinned selection we keep old servers untouched if backend
                    // returned only a partial config set.
                    if (!hasPinnedSelectedHost) {
                        // Remove any FBLink VPN servers that were not in the new config
                        // Iterate backwards so indices don't shift during removal
                        for (int i = existingFBLinkServerIndices.size() - 1; i >= 0; --i) {
                            int serverIndex = existingFBLinkServerIndices.at(i);
                            if (!updatedIndices.contains(serverIndex)) {
                                m_serversModel->removeServer(serverIndex);
                            }
                        }
                    } else if (!selectedConfigSeenInResponse) {
                        qWarning() << "[FBLink] fetchConfig: selected server config was not returned, keeping current local snapshot:"
                                   << selectedFBLinkHostName;
                    }

                    // Partial-response fallback:
                    // if selected server config arrived, routing sync is considered complete
                    // and connection can proceed immediately without waiting for other servers.
                    if (hasPinnedSelectedHost && selectedConfigSeenInResponse) {
                        setPendingRoutingSync(false);
                    } else if (!m_isLoading) {
                        setPendingRoutingSync(false);
                    } else {
                        qDebug() << "[FBLink] fetchConfig: keep pending routing sync because mutation request is still in progress";
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

                        const QJsonArray finalServers = m_settings->serversArray();
                        if (finalServerIndex >= 0 && finalServerIndex < finalServers.size()) {
                            const QJsonObject finalServer = finalServers.at(finalServerIndex).toObject();
                            if (isFBLinkServer(finalServer)) {
                                const QString finalHost = finalServer.value("hostName").toString();
                                if (!finalHost.isEmpty()) {
                                    qSettings.setValue(kLastSelectedFBLinkHostNameKey, finalHost);
                                    const QJsonValue serverIdValue = finalServer.value("id");
                                    if (serverIdValue.isDouble()) {
                                        qSettings.setValue(kLastSelectedServerIdKey, serverIdValue.toInt());
                                    } else if (serverIdValue.isString()) {
                                        qSettings.setValue(kLastSelectedServerIdKey, serverIdValue.toString());
                                    } else {
                                        qSettings.setValue(kLastSelectedServerIdKey, finalHost);
                                    }
                                }
                            }
                        }
                    }

                    emit configFetched();
                } else {
                    emit configError(tr("Внутренняя ошибка: Контроллеры не инициализированы"));
                }
            } else {
                emit configError(tr("Сервер не вернул конфигурацию"));
            }
            rerunCoalescedFetch();
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

             const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
             const bool authFailure = (httpStatus == 401)
                                      || reply->error() == QNetworkReply::AuthenticationRequiredError
                                      || reply->error() == QNetworkReply::ContentAccessDenied;
             if (!authFailure && m_hasPendingRoutingSync && !m_isLoading) {
                 m_pendingRoutingSyncFetchFailures++;
                 const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
                 const qint64 pendingAgeMs =
                         m_pendingRoutingSyncSinceMs > 0 ? (nowMs - m_pendingRoutingSyncSinceMs) : 0;
                 const bool shouldDropPending =
                         m_pendingRoutingSyncFetchFailures >= kPendingRoutingSyncMaxFetchFailures
                         || pendingAgeMs >= kPendingRoutingSyncMaxBlockMs;

                 qWarning() << "[FBLink] fetchConfig: pending routing sync fetch failed, attempt"
                            << m_pendingRoutingSyncFetchFailures
                            << "pending_age_ms=" << pendingAgeMs
                            << "http=" << httpStatus
                            << "network_error=" << reply->error();

                 if (shouldDropPending) {
                     qWarning() << "[FBLink] fetchConfig: dropping pending routing sync after repeated failures;"
                                   " using last known config snapshot";
                     setPendingRoutingSync(false);
                 } else {
                     QTimer::singleShot(kPendingRoutingSyncRetryDelayMs, this, [this]() {
                         if (!m_hasPendingRoutingSync || m_isFetchingConfig || m_isLoading) {
                             return;
                         }
                         qDebug() << "[FBLink] fetchConfig: retrying config fetch for pending routing sync";
                         fetchConfig(false);
                     });
                 }
             }

             emit configError(errStr);
             rerunCoalescedFetch();
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
    createPayment(plan, QString(), true);
}

void FBLinkController::createPaymentWithPromo(const QString &plan, const QString &promoCode)
{
    createPayment(plan, promoCode, true);
}

void FBLinkController::createPayment(const QString &plan, const QString &promoCode, bool allowRefreshRetry)
{
    QString token = getJwtToken();
    if (token.isEmpty()) {
        emit paymentError(tr("Необходимо войти в аккаунт"));
        return;
    }

    QNetworkRequest request = createApiRequest("/payments/create", true, true);

    QJsonObject json;
    json["plan"] = plan;
    const QString normalizedPromoCode = promoCode.trimmed().toUpper();
    if (!normalizedPromoCode.isEmpty()) {
        json["promo_code"] = normalizedPromoCode;
    }

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, plan, normalizedPromoCode, allowRefreshRetry]() {
        reply->deleteLater();

        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject obj = doc.object();

        if (reply->error() == QNetworkReply::NoError) {
            QString confirmUrl = obj.value("confirmation_url").toString();
            QString status = obj.value("status").toString();
            if (confirmUrl.isEmpty() && status == "succeeded") {
                fetchSubscription();
                emit paymentActivated();
            } else {
                emit paymentCreated(confirmUrl);
            }
        } else {
            logApiFailure("create-payment", reply);

            if (allowRefreshRetry && shouldRefreshToken(reply)) {
                refreshAccessToken([this, plan, normalizedPromoCode]() {
                    createPayment(plan, normalizedPromoCode, false);
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

    if (m_isFetchingSubscription) {
        m_pendingSubscriptionFetch = true;
        m_pendingSubscriptionAllowRefreshRetry = m_pendingSubscriptionAllowRefreshRetry || allowRefreshRetry;
        qDebug() << "[FBLink API] fetch-subscription already in progress, coalescing request";
        return;
    }

    m_isFetchingSubscription = true;

    QNetworkRequest request = createApiRequest("/me/subscription", false, true);

    QNetworkReply *reply = m_nam->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, allowRefreshRetry]() {
        reply->deleteLater();
        m_isFetchingSubscription = false;
        const bool rerunFetch = m_pendingSubscriptionFetch;
        const bool rerunAllowRefreshRetry = m_pendingSubscriptionAllowRefreshRetry;
        m_pendingSubscriptionFetch = false;
        m_pendingSubscriptionAllowRefreshRetry = true;

        const auto rerunCoalescedFetch = [this, rerunFetch, rerunAllowRefreshRetry]() {
            if (!rerunFetch) {
                return;
            }
            QTimer::singleShot(0, this, [this, rerunAllowRefreshRetry]() {
                fetchSubscription(rerunAllowRefreshRetry);
            });
        };

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
            QString accountEmail = obj.value("email").toString().trimmed();
            if (accountEmail.isEmpty()) {
                accountEmail = obj.value("user_email").toString().trimmed();
            }
            if (accountEmail.isEmpty()) {
                accountEmail = obj.value("account_email").toString().trimmed();
            }
            if (accountEmail.isEmpty()) {
                accountEmail = obj.value("user").toObject().value("email").toString().trimmed();
            }
            if (!accountEmail.isEmpty()) {
                setUserEmail(accountEmail);
            }

            saveSubscriptionInfo(status, plan, endDate, autoRenew, cardSaved, trialAvailable,
                                 allowedProtocols, canUseSiteSplitTunneling, canUseAppSplitTunneling,
                                 canManageRoutingProfiles, canUseAdBlock, vipAdBlockEnabled);
            // Сервер подтвердил статус — записываем время верификации
            m_lastSubscriptionVerifiedAt = QDateTime::currentSecsSinceEpoch();
            const bool shouldFetchConfig = m_fetchConfigAfterSubscription;
            m_fetchConfigAfterSubscription = false;
            emit subscriptionChanged();
            emit subscriptionFetched();
            if (shouldFetchConfig) {
                if (isSubscribed()) {
                    fetchConfig(true);
                } else {
                    clearExistingFBLinkServers();
                }
            }
            rerunCoalescedFetch();
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
            emit subscriptionError(errStr);
            rerunCoalescedFetch();
        }
    });
}

void FBLinkController::logout()
{
    m_pendingRefreshCallbacks.clear();
    m_isRefreshing = false;
    setLoadingState(false);
    setPendingRoutingSync(false);
    {
        QSettings qSettings = appSettings();
        qSettings.setValue(kVIPAdBlockStatusKey, "");
        qSettings.setValue(kVIPAdBlockDnsSourceKey, "");
        qSettings.setValue(kVIPAdBlockDegradeReasonKey, "");
        qSettings.setValue(kVIPEnabledProfilesCountKey, 0);
        qSettings.setValue(kVIPLastActiveProfileIDKey, -1);
        qSettings.setValue(kLastActiveRoutingProfileIdKey, -1);
        qSettings.setValue(kLastKnownGoodRoutingRulesHashKey, "");
        qSettings.setValue(kShowNewFeaturesGuideKey, false);
        qSettings.sync();
    }
    saveJwtToken("");
    saveRefreshToken("");
    setUserEmail("");
    saveSubscriptionInfo("", "", "");
    emit loginStateChanged();
    emit newFeaturesGuideChanged();
    emit subscriptionChanged();
}

bool FBLinkController::isLoggedIn() const
{
    return !getJwtToken().isEmpty();
}

QString FBLinkController::userEmail() const
{
    QSettings qSettings = appSettings();
    return qSettings.value(kUserEmailKey, "").toString();
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
    const bool featureEnabled = qSettings.value("subscriptionCanManageRoutingProfiles", false).toBool();
    const QString plan = qSettings.value("subscriptionPlan", "").toString().trimmed();
    return featureEnabled && plan.compare("vip", Qt::CaseInsensitive) == 0;
}

bool FBLinkController::canUseAdBlock() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    const bool featureEnabled = qSettings.value("subscriptionCanUseAdBlock", false).toBool();
    const QString plan = qSettings.value("subscriptionPlan", "").toString().trimmed();
    return featureEnabled && plan.compare("vip", Qt::CaseInsensitive) == 0;
}

bool FBLinkController::vipAdBlockEnabled() const
{
    QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
    return qSettings.value("subscriptionVIPAdBlockEnabled", false).toBool();
}

QString FBLinkController::vipAdBlockStatus() const
{
    QSettings qSettings = appSettings();
    const QString status = qSettings.value(kVIPAdBlockStatusKey, "").toString().trimmed().toLower();
    return status;
}

QString FBLinkController::vipAdBlockStatusLabel() const
{
    return adBlockStatusLabel(vipAdBlockStatus(), vipAdBlockEnabled());
}

QString FBLinkController::vipAdBlockDegradeReason() const
{
    QSettings qSettings = appSettings();
    return safeTrimmedReason(qSettings.value(kVIPAdBlockDegradeReasonKey, "").toString());
}

bool FBLinkController::safeModeActive() const
{
    QSettings qSettings = appSettings();
    const qint64 untilEpoch = qSettings.value(kSafeModeUntilEpochKey, 0).toLongLong();
    return untilEpoch > QDateTime::currentSecsSinceEpoch();
}

QString FBLinkController::safeModeUntilText() const
{
    QSettings qSettings = appSettings();
    const qint64 untilEpoch = qSettings.value(kSafeModeUntilEpochKey, 0).toLongLong();
    if (untilEpoch <= QDateTime::currentSecsSinceEpoch()) {
        return {};
    }
    return QLocale::system().toString(QDateTime::fromSecsSinceEpoch(untilEpoch), QLocale::ShortFormat);
}

bool FBLinkController::showNewFeaturesGuide() const
{
    QSettings qSettings = appSettings();
    const bool requested = qSettings.value(kShowNewFeaturesGuideKey, false).toBool();
    if (!requested) {
        return false;
    }
    const QString plan = qSettings.value("subscriptionPlan", "").toString().trimmed();
    const bool canManageProfiles = qSettings.value("subscriptionCanManageRoutingProfiles", false).toBool();
    return plan.compare("vip", Qt::CaseInsensitive) == 0 && canManageProfiles;
}

bool FBLinkController::isLoading() const
{
    return m_isLoading;
}

bool FBLinkController::isConfigSyncing() const
{
    return m_isConfigSyncing;
}

bool FBLinkController::hasPendingRoutingSync() const
{
    return m_hasPendingRoutingSync;
}

void FBLinkController::setAutoRenew(bool enabled)
{
    setAutoRenew(enabled, true);
}

void FBLinkController::setVipAdBlockEnabled(bool enabled)
{
    setVipAdBlockEnabled(enabled, true);
}

void FBLinkController::submitBugReport(const QString &note)
{
    submitBugReport(note, true);
}

void FBLinkController::exitSafeMode()
{
    QSettings qSettings = appSettings();
    qSettings.setValue(kSafeModeUntilEpochKey, 0);
    qSettings.sync();
    emit subscriptionChanged();
}

void FBLinkController::armNewFeaturesGuide()
{
    QSettings qSettings = appSettings();
    const QString plan = qSettings.value("subscriptionPlan", "").toString().trimmed();
    const bool canManageProfiles = qSettings.value("subscriptionCanManageRoutingProfiles", false).toBool();
    qSettings.setValue(kShowNewFeaturesGuideKey, plan.compare("vip", Qt::CaseInsensitive) == 0 && canManageProfiles);
    qSettings.sync();
    emit newFeaturesGuideChanged();
}

void FBLinkController::dismissNewFeaturesGuide()
{
    QSettings qSettings = appSettings();
    qSettings.setValue(kShowNewFeaturesGuideKey, false);
    qSettings.sync();
    emit newFeaturesGuideChanged();
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

    setPendingRoutingSync(true);
    setLoadingState(true);
    QNetworkRequest request = createApiRequest("/me/subscription/ad-block", true, true);

    QJsonObject json;
    json["enabled"] = enabled;

    QNetworkReply *reply = m_nam->sendCustomRequest(request, "PATCH", QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, enabled, allowRefreshRetry]() {
        reply->deleteLater();
        std::shared_ptr<void> guard(nullptr, [this](void*){ setLoadingState(false); });
        if (reply->error() == QNetworkReply::NoError) {
            QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
            qSettings.setValue("subscriptionVIPAdBlockEnabled", enabled);
            if (enabled) {
                // Optimistic state: show active immediately, then reconcile with /me/config.
                qSettings.setValue(kVIPAdBlockStatusKey, "applied");
                qSettings.setValue(kVIPAdBlockDegradeReasonKey, "");
            } else {
                qSettings.setValue(kVIPAdBlockStatusKey, "");
                qSettings.setValue(kVIPAdBlockDnsSourceKey, "");
                qSettings.setValue(kVIPAdBlockDegradeReasonKey, "");
            }
            qSettings.sync();
            emit vipAdBlockChanged(enabled);
            emit subscriptionChanged();
            if (isSubscribed()) {
                // Ensure mutation loading-state is dropped before config reconciliation.
                QTimer::singleShot(0, this, [this]() { fetchConfig(true); });
            } else {
                setPendingRoutingSync(false);
            }
        } else {
            logApiFailure("set-vip-ad-block", reply);
            if (allowRefreshRetry && shouldRefreshToken(reply)) {
                refreshAccessToken([this, enabled]() {
                    setVipAdBlockEnabled(enabled, false);
                });
                return;
            }

            setPendingRoutingSync(false);

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QString errStr = doc.object().value("error").toString(reply->errorString());
            emit requestError(errStr);
        }
    });
}

void FBLinkController::submitBugReport(const QString &note, bool allowRefreshRetry)
{
    QString token = getJwtToken();
    if (token.isEmpty()) {
        emit requestError(tr("Необходимо войти в аккаунт"));
        return;
    }

    QJsonObject diagnostics;
    diagnostics.insert("platform", QSysInfo::prettyProductName());
    diagnostics.insert("architecture", QSysInfo::currentCpuArchitecture());
    diagnostics.insert("subscription_plan", subscriptionPlan());
    diagnostics.insert("vip_ad_block_enabled", vipAdBlockEnabled());
    diagnostics.insert("vip_ad_block_status", vipAdBlockStatus());
    diagnostics.insert("vip_ad_block_dns_source", appSettings().value(kVIPAdBlockDnsSourceKey, "").toString());
    diagnostics.insert("vip_ad_block_degrade_reason", vipAdBlockDegradeReason());
    diagnostics.insert("safe_mode_active", safeModeActive());
    diagnostics.insert("safe_mode_until", safeModeUntilText());

    QSettings qSettings = appSettings();
    diagnostics.insert("default_server_index", qSettings.value("Servers/defaultServerIndex", -1).toInt());
    diagnostics.insert("last_selected_host", qSettings.value(kLastSelectedFBLinkHostNameKey, "").toString());
    diagnostics.insert("enabled_profiles_count", qSettings.value(kVIPEnabledProfilesCountKey, 0).toInt());
    diagnostics.insert("active_profile_id", qSettings.value(kVIPLastActiveProfileIDKey, -1).toInt());
    diagnostics.insert("routing_rules_hash", qSettings.value(kVIPLastKnownRoutingHashKey, "").toString());

    if (m_serversModel) {
        const int defaultIndex = m_serversModel->getDefaultServerIndex();
        if (defaultIndex >= 0) {
            const QJsonObject server = m_serversModel->getServerConfig(defaultIndex);
            diagnostics.insert("default_server_name", server.value("description").toString());
            diagnostics.insert("default_server_host", server.value("hostName").toString());
            diagnostics.insert("default_server_country", server.value("server_country_code").toString());
            diagnostics.insert("default_server_container", server.value("defaultContainer").toString());
        }
    }

    QJsonObject payload;
    payload.insert("note", sanitizeSensitiveTokens(note));
    payload.insert("diagnostics", diagnostics);

    QNetworkRequest request = createApiRequest("/me/support/bug-report", true, true);
    QNetworkReply *reply = m_nam->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, note, allowRefreshRetry, payload]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            const QString ticketId = obj.value("ticket_id").toString(
                    QString::number(QDateTime::currentSecsSinceEpoch()));
            emit bugReportSubmitted(ticketId);
            return;
        }

        logApiFailure("submit-bug-report", reply);
        if (allowRefreshRetry && shouldRefreshToken(reply)) {
            refreshAccessToken([this, note]() {
                submitBugReport(note, false);
            });
            return;
        }

        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        if (auto *clipboard = QGuiApplication::clipboard()) {
            clipboard->setText(QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact)));
        }
        const QString fallbackMessage = obj.value("error").toString(reply->errorString());
        emit requestError(tr("Не удалось отправить отчёт. Диагностика скопирована в буфер обмена. %1")
                                  .arg(fallbackMessage));
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
    if (!canManageRoutingProfiles()) {
        emit routingProfilesError(tr("Функция доступна только для VIP"));
        return;
    }

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
            const QJsonArray profilesArray = obj.value("profiles").toArray();
            int enabledProfilesCount = 0;
            int activeProfileId = -1;
            for (const QJsonValue &profileValue : profilesArray) {
                const QJsonObject profile = profileValue.toObject();
                if (!profile.value("enabled").toBool(false)) {
                    continue;
                }
                ++enabledProfilesCount;
                if (activeProfileId < 0) {
                    activeProfileId = profile.value("id").toInt(-1);
                }
            }

            QSettings qSettings = appSettings();
            qSettings.setValue(kVIPEnabledProfilesCountKey, enabledProfilesCount);
            qSettings.setValue(kVIPLastActiveProfileIDKey, activeProfileId);
            qSettings.setValue(kLastActiveRoutingProfileIdKey, activeProfileId);
            qSettings.sync();

            emit routingProfilesFetched(profilesArray.toVariantList());
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
    if (!canManageRoutingProfiles()) {
        emit routingProfilesError(tr("Функция доступна только для VIP"));
        return;
    }

    QString token = getJwtToken();
    if (token.isEmpty()) {
        emit routingProfilesError(tr("Необходимо войти в аккаунт"));
        return;
    }

    const int id = profile.value("id").toInt();
    const bool hasId = id > 0;

    setPendingRoutingSync(true);
    setLoadingState(true);
    QNetworkRequest request = createApiRequest(hasId ? QString("/me/routing-profiles/%1").arg(id)
                                                     : "/me/routing-profiles",
                                               true, true);

    QJsonObject payload = QJsonObject::fromVariantMap(profile);
    QNetworkReply *reply = hasId
        ? m_nam->put(request, QJsonDocument(payload).toJson())
        : m_nam->post(request, QJsonDocument(payload).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, profile, allowRefreshRetry]() {
        reply->deleteLater();
        std::shared_ptr<void> guard(nullptr, [this](void*){ setLoadingState(false); });
        if (reply->error() == QNetworkReply::NoError) {
            emit routingProfileSaved();
            fetchRoutingProfiles(true);
            if (isSubscribed()) {
                // Ensure mutation loading-state is dropped before config reconciliation.
                QTimer::singleShot(0, this, [this]() { fetchConfig(true); });
            } else {
                setPendingRoutingSync(false);
            }
        } else {
            logApiFailure("save-routing-profile", reply);
            if (allowRefreshRetry && shouldRefreshToken(reply)) {
                refreshAccessToken([this, profile]() {
                    saveRoutingProfile(profile, false);
                });
                return;
            }

            setPendingRoutingSync(false);
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
    if (!canManageRoutingProfiles()) {
        emit routingProfilesError(tr("Функция доступна только для VIP"));
        return;
    }

    QString token = getJwtToken();
    if (token.isEmpty()) {
        emit routingProfilesError(tr("Необходимо войти в аккаунт"));
        return;
    }

    setPendingRoutingSync(true);
    setLoadingState(true);
    QNetworkRequest request = createApiRequest(QString("/me/routing-profiles/%1").arg(id), false, true);

    QNetworkReply *reply = m_nam->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, id, allowRefreshRetry]() {
        reply->deleteLater();
        std::shared_ptr<void> guard(nullptr, [this](void*){ setLoadingState(false); });
        if (reply->error() == QNetworkReply::NoError) {
            emit routingProfileDeleted();
            fetchRoutingProfiles(true);
            if (isSubscribed()) {
                // Ensure mutation loading-state is dropped before config reconciliation.
                QTimer::singleShot(0, this, [this]() { fetchConfig(true); });
            } else {
                setPendingRoutingSync(false);
            }
        } else {
            logApiFailure("delete-routing-profile", reply);
            if (allowRefreshRetry && shouldRefreshToken(reply)) {
                refreshAccessToken([this, id]() {
                    deleteRoutingProfile(id, false);
                });
                return;
            }

            setPendingRoutingSync(false);
            QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            emit routingProfilesError(obj.value("error").toString(reply->errorString()));
        }
    });
}

void FBLinkController::copySystemRoutingProfile(const QString &code)
{
    copySystemRoutingProfile(code, true);
}

void FBLinkController::copySystemRoutingProfile(const QString &code, bool allowRefreshRetry)
{
    if (!canManageRoutingProfiles()) {
        emit routingProfilesError(tr("Функция доступна только для VIP"));
        return;
    }

    const QString normalizedCode = code.trimmed();
    if (normalizedCode.isEmpty()) {
        emit routingProfilesError(tr("Код системного пресета не задан"));
        return;
    }

    QString token = getJwtToken();
    if (token.isEmpty()) {
        emit routingProfilesError(tr("Необходимо войти в аккаунт"));
        return;
    }

    setPendingRoutingSync(true);
    setLoadingState(true);
    QNetworkRequest request = createApiRequest(QString("/me/routing-profiles/system/%1/copy").arg(normalizedCode), true, true);
    QNetworkReply *reply = m_nam->post(request, QByteArrayLiteral("{}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply, normalizedCode, allowRefreshRetry]() {
        reply->deleteLater();
        std::shared_ptr<void> guard(nullptr, [this](void*){ setLoadingState(false); });
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            const QVariantMap profile = obj.value("profile").toObject().toVariantMap();
            const bool created = obj.value("created").toBool(false);
            emit routingSystemProfileCopied(profile, created);
            fetchRoutingProfiles(true);
            if (isSubscribed()) {
                // Ensure mutation loading-state is dropped before config reconciliation.
                QTimer::singleShot(0, this, [this]() { fetchConfig(true); });
            } else {
                setPendingRoutingSync(false);
            }
        } else {
            logApiFailure("copy-system-routing-profile", reply);
            if (allowRefreshRetry && shouldRefreshToken(reply)) {
                refreshAccessToken([this, normalizedCode]() {
                    copySystemRoutingProfile(normalizedCode, false);
                });
                return;
            }

            setPendingRoutingSync(false);
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

void FBLinkController::setUserEmail(const QString &email)
{
    const QString normalized = email.trimmed();
    QSettings qSettings = appSettings();
    const QString current = qSettings.value(kUserEmailKey, "").toString().trimmed();
    if (current.compare(normalized, Qt::CaseInsensitive) == 0) {
        return;
    }

    // Email changed => switch account context.
    // Drop pinned server selection so fetchConfig() does not sync only one host.
    qSettings.remove(kLastSelectedFBLinkHostNameKey);
    qSettings.remove(kLastSelectedServerIdKey);
    qSettings.setValue(kUserEmailKey, normalized);
    qSettings.sync();

    // Prevent mixing stale servers from another account until fresh config arrives.
    clearExistingFBLinkServers();
    emit userEmailChanged();
}
