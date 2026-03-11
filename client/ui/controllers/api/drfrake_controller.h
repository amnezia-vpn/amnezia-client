#ifndef DRFRAKECONTROLLER_H
#define DRFRAKECONTROLLER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <memory>

#include "ui/controllers/importController.h"
#include "settings.h"
#include "ui/models/servers_model.h"

class ImportController;
class Settings;

class DrFrakeController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY loginStateChanged)
    Q_PROPERTY(bool isSubscribed READ isSubscribed NOTIFY subscriptionChanged)
    Q_PROPERTY(QString subscriptionPlan READ subscriptionPlan NOTIFY subscriptionChanged)
    Q_PROPERTY(QString subscriptionEndDate READ subscriptionEndDate NOTIFY subscriptionChanged)

public:
    explicit DrFrakeController(ImportController *importController, const std::shared_ptr<Settings> &settings,
                               ServersModel *serversModel = nullptr, QObject *parent = nullptr);

    Q_INVOKABLE void login(const QString &email, const QString &password);
    Q_INVOKABLE void loginWithToken(const QString &token);
    Q_INVOKABLE void fetchConfig();
    Q_INVOKABLE void fetchSubscription();
    Q_INVOKABLE void createPayment(const QString &plan);
    Q_INVOKABLE void logout();

    bool isLoggedIn() const;
    bool isSubscribed() const;
    QString subscriptionPlan() const;
    QString subscriptionEndDate() const;

signals:
    void loginSuccess();
    void loginError(const QString &errorMessage);
    void configFetched();
    void configError(const QString &errorMessage);
    void loginStateChanged();
    void subscriptionChanged();
    void subscriptionFetched();
    void subscriptionError(const QString &errorMessage);
    void paymentCreated(const QString &confirmationUrl);
    void paymentError(const QString &errorMessage);

private:
    QNetworkAccessManager *m_nam;
    ImportController *m_importController;
    std::shared_ptr<Settings> m_settings;
    ServersModel *m_serversModel;

    QString m_apiUrl;

    void saveJwtToken(const QString &token);
    QString getJwtToken() const;
    void saveSubscriptionInfo(const QString &status, const QString &plan, const QString &endDate);
    void clearExistingDrFrakeServers();
};

#endif // DRFRAKECONTROLLER_H
