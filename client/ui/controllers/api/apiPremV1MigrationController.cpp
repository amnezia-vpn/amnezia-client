#include "apiPremV1MigrationController.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>

#include "core/utils/api/apiDefs.h"
#include "core/utils/api/apiUtils.h"
#include "core/controllers/gatewayController.h"
#include "settings.h"

ApiPremV1MigrationController::ApiPremV1MigrationController(ServersController* serversController,
                                                           ServersModel* serversModel,
                                                           QAppSettingsRepository* appSettingsRepository,
                                                           QObject *parent)
    : QObject(parent), m_serversController(serversController), m_serversModel(serversModel), m_appSettingsRepository(appSettingsRepository)
{
}

bool ApiPremV1MigrationController::hasConfigsToMigration()
{
    QJsonArray vpnKeys;

    auto serversCount = m_serversModel->getServersCount();
    for (size_t i = 0; i < serversCount; i++) {
        auto serverConfigObject = m_serversController->getServerConfig(i);

        if (apiUtils::getConfigType(serverConfigObject) != apiDefs::ConfigType::AmneziaPremiumV1) {
            continue;
        }

        QString vpnKey = apiUtils::getPremiumV1VpnKey(serverConfigObject);
        vpnKeys.append(vpnKey);
    }

    if (!vpnKeys.isEmpty()) {
        GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                            m_appSettingsRepository->isStrictKillSwitchEnabled());
        QJsonObject apiPayload;

        apiPayload["configs"] = vpnKeys;
        QByteArray responseBody;
        ErrorCode errorCode = gatewayController.post(QString("%1v1/prem-v1/is-active-subscription"), apiPayload, responseBody);

        auto migrationsStatus = QJsonDocument::fromJson(responseBody).object();
        for (const auto &migrationStatus : migrationsStatus) {
            if (migrationStatus == "not_found") {
                return true;
            }
        }
    }

    return false;
}

void ApiPremV1MigrationController::getSubscriptionList(const QString &email)
{
    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled());
    QJsonObject apiPayload;

    apiPayload[apiDefs::key::email] = email;
    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/prem-v1/subscription-list"), apiPayload, responseBody);

    if (errorCode == ErrorCode::NoError) {
        m_email = email;
        m_subscriptionsModel = QJsonDocument::fromJson(responseBody).array();
        if (m_subscriptionsModel.isEmpty()) {
            emit noSubscriptionToMigrate();
            return;
        }

        emit subscriptionsModelChanged();
    } else {
        emit errorOccurred(ErrorCode::ApiMigrationError);
    }
}

QJsonArray ApiPremV1MigrationController::getSubscriptionModel()
{
    return m_subscriptionsModel;
}

void ApiPremV1MigrationController::sendMigrationCode(const int subscriptionIndex)
{
    QEventLoop wait;
    QTimer::singleShot(1000, &wait, &QEventLoop::quit);
    wait.exec(QEventLoop::ExcludeUserInputEvents);

    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled());
    QJsonObject apiPayload;

    apiPayload[apiDefs::key::email] = m_email;
    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/prem-v1/migration-code"), apiPayload, responseBody);

    if (errorCode == ErrorCode::NoError) {
        m_subscriptionIndex = subscriptionIndex;
        emit otpSuccessfullySent();
    } else {
        emit errorOccurred(ErrorCode::ApiMigrationError);
    }
}

void ApiPremV1MigrationController::migrate(const QString &migrationCode)
{
    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled());
    QJsonObject apiPayload;

    apiPayload[apiDefs::key::email] = m_email;
    apiPayload[apiDefs::key::orderId] = m_subscriptionsModel.at(m_subscriptionIndex).toObject().value(apiDefs::key::id).toString();
    apiPayload[apiDefs::key::migrationCode] = migrationCode;
    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/prem-v1/migrate"), apiPayload, responseBody);

    if (errorCode == ErrorCode::NoError) {
        auto responseObject = QJsonDocument::fromJson(responseBody).object();
        QString premiumV2VpnKey = responseObject.value(apiDefs::key::config).toString();

        emit importPremiumV2VpnKey(premiumV2VpnKey);
    } else {
        emit errorOccurred(ErrorCode::ApiMigrationError);
    }
}

bool ApiPremV1MigrationController::isPremV1MigrationReminderActive()
{
    return m_appSettingsRepository->isPremV1MigrationReminderActive();
}

void ApiPremV1MigrationController::disablePremV1MigrationReminder()
{
    m_appSettingsRepository->disablePremV1MigrationReminder();
}
