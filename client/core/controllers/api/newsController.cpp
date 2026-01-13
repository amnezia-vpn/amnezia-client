#include "newsController.h"

#include "core/controllers/gatewayController.h"
#include "core/utils/api/apiDefs.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace
{
    namespace configKey
    {
        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serviceType[] = "service_type";
    }
}

NewsController::NewsController(AppSettingsRepository* appSettingsRepository,
                               ServersController* serversController)
    : m_appSettingsRepository(appSettingsRepository), m_serversController(serversController)
{
}

ErrorCode NewsController::fetchNews(QJsonArray &newsArray)
{
    if (!m_serversController) {
        qWarning() << "ServersController is null, skip fetchNews";
        return ErrorCode::InternalError;
    }
    
    const auto stacks = m_serversController->gatewayStacks();
    if (stacks.isEmpty()) {
        qDebug() << "No Gateway stacks, skip fetchNews";
        return ErrorCode::NoError;
    }

    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), 
                                       m_appSettingsRepository->isDevGatewayEnv(),
                                       apiDefs::requestTimeoutMsecs, 
                                       m_appSettingsRepository->isStrictKillSwitchEnabled());
    
    QJsonObject payload;
    payload.insert("locale", m_appSettingsRepository->getAppLanguage().name().split("_").first());

    const QJsonObject stacksJson = stacks.toJson();
    if (stacksJson.contains(configKey::userCountryCode)) {
        payload.insert(configKey::userCountryCode, stacksJson.value(configKey::userCountryCode));
    }
    if (stacksJson.contains(configKey::serviceType)) {
        payload.insert(configKey::serviceType, stacksJson.value(configKey::serviceType));
    }

    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/news"), payload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonDocument doc = QJsonDocument::fromJson(responseBody);
    if (doc.isArray()) {
        newsArray = doc.array();
    } else if (doc.isObject()) {
        QJsonObject obj = doc.object();
        if (obj.value("news").isArray()) {
            newsArray = obj.value("news").toArray();
        }
    }

    return ErrorCode::NoError;
}

