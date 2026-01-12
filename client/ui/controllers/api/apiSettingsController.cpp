#include "apiSettingsController.h"

#include <QEventLoop>
#include <QTimer>

#include "core/api/apiUtils.h"
#include "core/controllers/gatewayController.h"
#include "platforms/ios/ios_controller.h"
#include "version.h"

namespace
{
    namespace configKey
    {
        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serverCountryCode[] = "server_country_code";
        constexpr char serviceType[] = "service_type";
        constexpr char serviceInfo[] = "service_info";

        constexpr char apiConfig[] = "api_config";
        constexpr char authData[] = "auth_data";
    }

    const int requestTimeoutMsecs = 12 * 1000; // 12 secs
}

ApiSettingsController::ApiSettingsController(ServersController* serversController,
                                             ServersModel* serversModel,
                                             ApiAccountInfoModel* apiAccountInfoModel,
                                             ApiCountryModel* apiCountryModel,
                                             ApiDevicesModel* apiDevicesModel,
                                             QAppSettingsRepository* appSettingsRepository,
                                             QObject *parent)
    : QObject(parent),
      m_serversController(serversController),
      m_serversModel(serversModel),
      m_apiAccountInfoModel(apiAccountInfoModel),
      m_apiCountryModel(apiCountryModel),
      m_apiDevicesModel(apiDevicesModel),
      m_appSettingsRepository(appSettingsRepository)
{
}

ApiSettingsController::~ApiSettingsController()
{
}

bool ApiSettingsController::getAccountInfo(bool reload)
{
    if (reload) {
        QEventLoop wait;
        QTimer::singleShot(1000, &wait, &QEventLoop::quit);
        wait.exec(QEventLoop::ExcludeUserInputEvents);
    }

    auto processedIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfig = m_serversController->getServerConfig(processedIndex);
    auto apiConfig = serverConfig.value(configKey::apiConfig).toObject();
    auto authData = serverConfig.value(configKey::authData).toObject();

    bool isTestPurchase = apiConfig.value(apiDefs::key::isTestPurchase).toBool(false);
    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase), m_appSettingsRepository->isDevGatewayEnv(isTestPurchase), requestTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled());

    QJsonObject apiPayload;
    apiPayload[configKey::userCountryCode] = apiConfig.value(configKey::userCountryCode).toString();
    apiPayload[configKey::serviceType] = apiConfig.value(configKey::serviceType).toString();
    apiPayload[configKey::authData] = authData;
    apiPayload[apiDefs::key::cliVersion] = QString(APP_VERSION);
    apiPayload[apiDefs::key::appLanguage] = m_appSettingsRepository->getAppLanguage().name().split("_").first();

    QByteArray responseBody;

    ErrorCode errorCode = gatewayController.post(QString("%1v1/account_info"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    QJsonObject accountInfo = QJsonDocument::fromJson(responseBody).object();
    m_apiAccountInfoModel->updateModel(accountInfo, serverConfig);

    if (reload) {
        updateApiCountryModel();
        updateApiDevicesModel();
    }

    return true;
}

void ApiSettingsController::updateApiCountryModel()
{
    m_apiCountryModel->updateModel(m_apiAccountInfoModel->getAvailableCountries(), "");
    m_apiCountryModel->updateIssuedConfigsInfo(m_apiAccountInfoModel->getIssuedConfigsInfo());
}

void ApiSettingsController::updateApiDevicesModel()
{
    m_apiDevicesModel->updateModel(m_apiAccountInfoModel->getIssuedConfigsInfo(), m_appSettingsRepository->getInstallationUuid(false));
}
