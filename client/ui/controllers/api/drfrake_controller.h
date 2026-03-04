#ifndef DRFRAKECONTROLLER_H
#define DRFRAKECONTROLLER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <memory>

#include "ui/controllers/importController.h"
#include "settings.h"

class DrFrakeController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY loginStateChanged)

public:
    explicit DrFrakeController(ImportController *importController,
                               const std::shared_ptr<Settings> &settings, QObject *parent = nullptr);

    Q_INVOKABLE void login(const QString &email, const QString &password);
    Q_INVOKABLE void fetchConfig();
    Q_INVOKABLE void logout();
    bool isLoggedIn() const;

signals:
    void loginSuccess();
    void loginError(const QString &errorMessage);
    void configFetched();
    void configError(const QString &errorMessage);
    void loginStateChanged();

private:
    QNetworkAccessManager *m_nam;
    ImportController *m_importController;
    std::shared_ptr<Settings> m_settings;
    
    QString m_apiUrl;

    void saveJwtToken(const QString &token);
    QString getJwtToken() const;
    void clearExistingDrFrakeServers();
};

#endif // DRFRAKECONTROLLER_H
