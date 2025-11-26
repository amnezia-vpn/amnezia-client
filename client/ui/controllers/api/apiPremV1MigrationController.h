#ifndef APIPREMV1MIGRATIONCONTROLLER_H
#define APIPREMV1MIGRATIONCONTROLLER_H

#include <QObject>

#include "core/controllers/serversController.h"
#include "core/repositories/qAppSettingsRepository.h"
#include "ui/models/servers_model.h"

class ApiPremV1MigrationController : public QObject
{
    Q_OBJECT
public:
    ApiPremV1MigrationController(ServersController* serversController,
                                 ServersModel* serversModel,
                                 QAppSettingsRepository* appSettingsRepository,
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

    void errorOccurred(ErrorCode errorCode);

    void showMigrationDrawer();
    void migrationFinished();

    void noSubscriptionToMigrate();

private:
    ServersController* m_serversController;
    ServersModel* m_serversModel;
    QAppSettingsRepository* m_appSettingsRepository;

    QJsonArray m_subscriptionsModel;
    int m_subscriptionIndex;
    QString m_email;
};

#endif // APIPREMV1MIGRATIONCONTROLLER_H
