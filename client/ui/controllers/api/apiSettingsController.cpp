#include "apiSettingsController.h"

#include <QEventLoop>
#include <QJsonDocument>
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

ApiSettingsController::ApiSettingsController(const QSharedPointer<ServersModel> &serversModel,
                                             const QSharedPointer<ApiAccountInfoModel> &apiAccountInfoModel,
                                             const QSharedPointer<ApiCountryModel> &apiCountryModel,
                                             const QSharedPointer<ApiDevicesModel> &apiDevicesModel,
                                             const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_apiAccountInfoModel(apiAccountInfoModel),
      m_apiCountryModel(apiCountryModel),
      m_apiDevicesModel(apiDevicesModel),
      m_settings(settings)
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
    auto serverConfig = m_serversModel->getServerConfig(processedIndex);
    auto apiConfig = serverConfig.value(configKey::apiConfig).toObject();
    auto authData = serverConfig.value(configKey::authData).toObject();

    bool isTestPurchase = apiConfig.value(apiDefs::key::isTestPurchase).toBool(false);
    GatewayController gatewayController(m_settings->getGatewayEndpoint(isTestPurchase), m_settings->isDevGatewayEnv(isTestPurchase),
                                        requestTimeoutMsecs, m_settings->isStrictKillSwitchEnabled());

    QJsonObject apiPayload;
    apiPayload[configKey::userCountryCode] = apiConfig.value(configKey::userCountryCode).toString();
    apiPayload[configKey::serviceType] = apiConfig.value(configKey::serviceType).toString();
    apiPayload[configKey::authData] = authData;
    apiPayload[apiDefs::key::cliVersion] = QString(APP_VERSION);
    apiPayload[apiDefs::key::appLanguage] = m_settings->getAppLanguage().name().split("_").first();

    QByteArray responseBody;

    ErrorCode errorCode = gatewayController.post(QString("%1v1/account_info"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    QJsonObject accountInfo = QJsonDocument::fromJson(responseBody).object();
    m_apiAccountInfoModel->updateModel(accountInfo, serverConfig);

    QString subscriptionEndDate = accountInfo.value(apiDefs::key::subscriptionEndDate).toString();
    if (!subscriptionEndDate.isEmpty()) {
        apiConfig.insert(apiDefs::key::subscriptionEndDate, subscriptionEndDate);
        serverConfig.insert(configKey::apiConfig, apiConfig);
        m_serversModel->editServer(serverConfig, processedIndex);
    }

    if (reload) {
        updateApiCountryModel();
        updateApiDevicesModel();
    }

    return true;
}

void ApiSettingsController::getRenewalLink()
{
    auto processedIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfig = m_serversModel->getServerConfig(processedIndex);
    auto apiConfig = serverConfig.value(configKey::apiConfig).toObject();
    auto authData = serverConfig.value(configKey::authData).toObject();

    bool isTestPurchase = apiConfig.value(apiDefs::key::isTestPurchase).toBool(false);
    auto gatewayController = QSharedPointer<GatewayController>::create(m_settings->getGatewayEndpoint(isTestPurchase),
                                                                       m_settings->isDevGatewayEnv(isTestPurchase),
                                                                       requestTimeoutMsecs,
                                                                       m_settings->isStrictKillSwitchEnabled());

    QJsonObject apiPayload;
    apiPayload[configKey::userCountryCode] = apiConfig.value(configKey::userCountryCode).toString();
    apiPayload[configKey::serviceType] = apiConfig.value(configKey::serviceType).toString();
    apiPayload[configKey::authData] = authData;
    apiPayload[apiDefs::key::cliVersion] = QString(APP_VERSION);
    apiPayload[apiDefs::key::appLanguage] = m_settings->getAppLanguage().name().split("_").first();

    auto future = gatewayController->postAsync(QString("%1v1/renewal_link"), apiPayload);
    future.then(this, [this, gatewayController](QPair<ErrorCode, QByteArray> result) {
        auto [errorCode, responseBody] = result;
        if (errorCode != ErrorCode::NoError) {
            emit errorOccurred(errorCode);
            return;
        }

        QJsonObject responseJson = QJsonDocument::fromJson(responseBody).object();
        QString url = responseJson.value("renewal_url").toString();
        if (!url.isEmpty()) {
            emit renewalLinkReceived(url);
        }
    });
}

void ApiSettingsController::updateApiCountryModel()
{
    m_apiCountryModel->updateModel(m_apiAccountInfoModel->getAvailableCountries(), "");
    m_apiCountryModel->updateIssuedConfigsInfo(m_apiAccountInfoModel->getIssuedConfigsInfo());
}

void ApiSettingsController::updateApiDevicesModel()
{
    m_apiDevicesModel->updateModel(m_apiAccountInfoModel->getIssuedConfigsInfo());
}
