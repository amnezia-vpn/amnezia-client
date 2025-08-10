#ifndef APIPREMV1MIGRATIONUICONTROLLER_H
#define APIPREMV1MIGRATIONUICONTROLLER_H

#include <QObject>
#include <QSharedPointer>

#include "core/defs.h"
#include "ui/models/servers_model.h"

class ApiPremV1MigrationController;

class ApiPremV1MigrationUIController : public QObject
{
    Q_OBJECT
public:
    explicit ApiPremV1MigrationUIController(const QSharedPointer<ServersModel> &serversModel,
                                            const QSharedPointer<ApiPremV1MigrationController> &coreController,
                                            QObject *parent = nullptr);

    Q_PROPERTY(QJsonArray subscriptionsModel READ getSubscriptionModel NOTIFY subscriptionsModelChanged)

public slots:
    bool hasConfigsToMigration();
    void getSubscriptionList(const QString &email);
    QJsonArray getSubscriptionModel();
    void sendMigrationCode(const int subscriptionIndex);
    void migrate(const QString &migrationCode);

    bool isPremV1MigrationReminderActive();
    void disablePremV1MigrationReminder();

signals:
    void subscriptionsModelChanged();
    void otpSuccessfullySent();
    void importPremiumV2VpnKey(const QString &vpnKey);
    void errorOccurred(amnezia::ErrorCode errorCode);
    void showMigrationDrawer();
    void migrationFinished();
    void noSubscriptionToMigrate();

private:
    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ApiPremV1MigrationController> m_coreController;
    
};

#endif // APIPREMV1MIGRATIONUICONTROLLER_H


