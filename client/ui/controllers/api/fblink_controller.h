#ifndef FBLINKCONTROLLER_H
#define FBLINKCONTROLLER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVector>
#include <functional>
#include <memory>

#include "ui/controllers/importController.h"
#include "settings.h"
#include "ui/models/servers_model.h"

class ImportController;
class Settings;

class FBLinkController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY loginStateChanged)
    Q_PROPERTY(QString userEmail READ userEmail NOTIFY userEmailChanged)
    Q_PROPERTY(bool isSubscribed READ isSubscribed NOTIFY subscriptionChanged)
    Q_PROPERTY(QString subscriptionPlan READ subscriptionPlan NOTIFY subscriptionChanged)
    Q_PROPERTY(QStringList allowedProtocols READ allowedProtocols NOTIFY subscriptionChanged)
    Q_PROPERTY(QString subscriptionEndDate READ subscriptionEndDate NOTIFY subscriptionChanged)
    Q_PROPERTY(bool autoRenew READ autoRenew NOTIFY subscriptionChanged)
    Q_PROPERTY(bool cardSaved READ cardSaved NOTIFY subscriptionChanged)
    Q_PROPERTY(bool trialAvailable READ trialAvailable NOTIFY subscriptionChanged)
    Q_PROPERTY(bool canUseSiteSplitTunneling READ canUseSiteSplitTunneling NOTIFY subscriptionChanged)
    Q_PROPERTY(bool canUseAppSplitTunneling READ canUseAppSplitTunneling NOTIFY subscriptionChanged)
    Q_PROPERTY(bool canManageRoutingProfiles READ canManageRoutingProfiles NOTIFY subscriptionChanged)
    Q_PROPERTY(bool canUseAdBlock READ canUseAdBlock NOTIFY subscriptionChanged)
    Q_PROPERTY(bool vipAdBlockEnabled READ vipAdBlockEnabled NOTIFY subscriptionChanged)
    Q_PROPERTY(QString vipAdBlockStatus READ vipAdBlockStatus NOTIFY subscriptionChanged)
    Q_PROPERTY(QString vipAdBlockStatusLabel READ vipAdBlockStatusLabel NOTIFY subscriptionChanged)
    Q_PROPERTY(QString vipAdBlockDegradeReason READ vipAdBlockDegradeReason NOTIFY subscriptionChanged)
    Q_PROPERTY(bool safeModeActive READ safeModeActive NOTIFY subscriptionChanged)
    Q_PROPERTY(QString safeModeUntilText READ safeModeUntilText NOTIFY subscriptionChanged)
    Q_PROPERTY(bool showNewFeaturesGuide READ showNewFeaturesGuide NOTIFY newFeaturesGuideChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool isConfigSyncing READ isConfigSyncing NOTIFY configSyncChanged)
    Q_PROPERTY(bool hasPendingRoutingSync READ hasPendingRoutingSync NOTIFY routingSyncPendingChanged)

public:
    explicit FBLinkController(ImportController *importController, const std::shared_ptr<Settings> &settings,
                               ServersModel *serversModel = nullptr, QObject *parent = nullptr);

    Q_INVOKABLE void login(const QString &email, const QString &password);
    Q_INVOKABLE void registerUser(const QString &email, const QString &password);
    Q_INVOKABLE void verifyEmail(const QString &email, const QString &code);
    Q_INVOKABLE void forgotPassword(const QString &email);
    Q_INVOKABLE void resetPassword(const QString &email, const QString &code, const QString &newPassword);
    Q_INVOKABLE void loginWithToken(const QString &token);
    Q_INVOKABLE void fetchConfig();
    Q_INVOKABLE void fetchSubscription();
    Q_INVOKABLE void syncAll();
    Q_INVOKABLE void createPayment(const QString &plan);
    Q_INVOKABLE void createPaymentWithPromo(const QString &plan, const QString &promoCode);
    Q_INVOKABLE void setAutoRenew(bool enabled);
    Q_INVOKABLE void setVipAdBlockEnabled(bool enabled);
    Q_INVOKABLE void deleteCard();
    Q_INVOKABLE void fetchRoutingProfiles();
    Q_INVOKABLE void saveRoutingProfile(const QVariantMap &profile);
    Q_INVOKABLE void deleteRoutingProfile(int id);
    Q_INVOKABLE void copySystemRoutingProfile(const QString &code);
    Q_INVOKABLE void submitBugReport(const QString &note = QString());
    Q_INVOKABLE void exitSafeMode();
    Q_INVOKABLE void armNewFeaturesGuide();
    Q_INVOKABLE void dismissNewFeaturesGuide();
    Q_INVOKABLE void logout();

    bool isLoggedIn() const;
    QString userEmail() const;
    bool isSubscribed() const;
    QString subscriptionPlan() const;
    QStringList allowedProtocols() const;
    QString subscriptionEndDate() const;
    bool autoRenew() const;
    bool cardSaved() const;
    bool trialAvailable() const;
    bool canUseSiteSplitTunneling() const;
    bool canUseAppSplitTunneling() const;
    bool canManageRoutingProfiles() const;
    bool canUseAdBlock() const;
    bool vipAdBlockEnabled() const;
    QString vipAdBlockStatus() const;
    QString vipAdBlockStatusLabel() const;
    QString vipAdBlockDegradeReason() const;
    bool safeModeActive() const;
    QString safeModeUntilText() const;
    bool showNewFeaturesGuide() const;
    bool isLoading() const;
    bool isConfigSyncing() const;
    bool hasPendingRoutingSync() const;

signals:
    void loginSuccess();
    void loginError(const QString &errorMessage);
    void registerCodeSent();
    void registerError(const QString &errorMessage);
    void verifySuccess();
    void verifyError(const QString &errorMessage);
    void forgotPasswordSent();
    void forgotPasswordError(const QString &errorMessage);
    void resetPasswordSuccess();
    void resetPasswordError(const QString &errorMessage);
    void configFetched();
    void configError(const QString &errorMessage);
    void loginStateChanged();
    void subscriptionChanged();
    void subscriptionFetched();
    void subscriptionError(const QString &errorMessage);
    void paymentCreated(const QString &confirmationUrl);
    void paymentActivated();
    void paymentError(const QString &errorMessage);
    void autoRenewChanged(bool enabled);
    void vipAdBlockChanged(bool enabled);
    void cardDeleted();
    void routingProfilesFetched(const QVariantList &profiles);
    void routingProfilesError(const QString &errorMessage);
    void routingProfileSaved();
    void routingProfileDeleted();
    void routingSystemProfileCopied(const QVariantMap &profile, bool created);
    void bugReportSubmitted(const QString &ticketId);
    void newFeaturesGuideChanged();
    void requestError(const QString &errorMessage);
    void loadingChanged();
    void configSyncChanged();
    void userEmailChanged();
    void routingSyncPendingChanged();

private:
    QNetworkAccessManager *m_nam;
    ImportController *m_importController;
    std::shared_ptr<Settings> m_settings;
    ServersModel *m_serversModel;

    QString m_apiUrl;

    QNetworkRequest createApiRequest(const QString &path, bool isJsonRequest = false, bool authorized = false) const;
    void logApiFailure(const QString &operationName, QNetworkReply *reply) const;
    bool shouldRefreshToken(QNetworkReply *reply) const;
    void setLoadingState(bool isLoading);
    void setConfigSyncState(bool isSyncing);
    void setPendingRoutingSync(bool pending);

    void fetchConfig(bool allowRefreshRetry);
    void fetchSubscription(bool allowRefreshRetry);
    void fetchRoutingProfiles(bool allowRefreshRetry);
    void saveRoutingProfile(const QVariantMap &profile, bool allowRefreshRetry);
    void deleteRoutingProfile(int id, bool allowRefreshRetry);
    void copySystemRoutingProfile(const QString &code, bool allowRefreshRetry);
    void submitBugReport(const QString &note, bool allowRefreshRetry);
    void createPayment(const QString &plan, const QString &promoCode, bool allowRefreshRetry);
    void setAutoRenew(bool enabled, bool allowRefreshRetry);
    void setVipAdBlockEnabled(bool enabled, bool allowRefreshRetry);
    void deleteCard(bool allowRefreshRetry);

    void saveJwtToken(const QString &token);
    QString getJwtToken() const;
    void saveRefreshToken(const QString &token);
    QString getRefreshToken() const;
    void setUserEmail(const QString &email);
    void refreshAccessToken(std::function<void()> onSuccess = nullptr);
    void beginSessionSync();
    void saveSubscriptionInfo(const QString &status, const QString &plan, const QString &endDate,
                              bool autoRenew = true, bool cardSaved = false, bool trialAvailable = true,
                              const QStringList &allowedProtocols = {}, bool canUseSiteSplitTunneling = false,       
                              bool canUseAppSplitTunneling = false, bool canManageRoutingProfiles = false,
                              bool canUseAdBlock = false, bool vipAdBlockEnabled = false);
    void clearExistingFBLinkServers();

    bool m_isRefreshing = false;
    bool m_isFetchingConfig = false;
    bool m_isFetchingSubscription = false;
    bool m_pendingConfigFetch = false;
    bool m_pendingSubscriptionFetch = false;
    bool m_pendingConfigAllowRefreshRetry = true;
    bool m_pendingSubscriptionAllowRefreshRetry = true;
    bool m_isLoading = false;
    bool m_isConfigSyncing = false;
    bool m_hasPendingRoutingSync = false;
    int m_pendingRoutingSyncFetchFailures = 0;
    qint64 m_pendingRoutingSyncSinceMs = 0;
    int m_loadingOperationsCount = 0;
    int m_configSyncOperationsCount = 0;
    bool m_fetchConfigAfterSubscription = false;
    QVector<std::function<void()>> m_pendingRefreshCallbacks;
    // Защита от обхода подписки: время последней серверной верификации
    // Если прошло > 24ч — при следующем isSubscribed() принудительно обновляем
    qint64 m_lastSubscriptionVerifiedAt = 0;
};

#endif // FBLINKCONTROLLER_H
