#include "apiPremV1MigrationUIController.h"

#include "core/controllers/api/apiPremV1MigrationController.h"
#include "settings.h"

ApiPremV1MigrationUIController::ApiPremV1MigrationUIController(const QSharedPointer<ServersModel> &serversModel,
                                                               const QSharedPointer<ApiPremV1MigrationController> &coreController,
                                                               const std::shared_ptr<Settings> &settings,
                                                               QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_coreController(coreController), m_settings(settings)
{
    
}

bool ApiPremV1MigrationUIController::hasConfigsToMigration()
{
    return m_coreController->hasConfigsToMigration();
}

void ApiPremV1MigrationUIController::getSubscriptionList(const QString &email)
{
    m_coreController->getSubscriptionList(email);
}

QJsonArray ApiPremV1MigrationUIController::getSubscriptionModel()
{
    return m_coreController->getSubscriptionModel();
}

void ApiPremV1MigrationUIController::sendMigrationCode(const int subscriptionIndex)
{
    m_coreController->sendMigrationCode(subscriptionIndex);
    emit otpSuccessfullySent();
}

void ApiPremV1MigrationUIController::migrate(const QString &migrationCode)
{
    m_coreController->migrate(migrationCode);
}

bool ApiPremV1MigrationUIController::isPremV1MigrationReminderActive()
{
    return m_coreController->isPremV1MigrationReminderActive();
}

void ApiPremV1MigrationUIController::disablePremV1MigrationReminder()
{
    m_coreController->disablePremV1MigrationReminder();
}


