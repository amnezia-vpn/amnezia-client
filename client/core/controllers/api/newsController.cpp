#include "newsController.h"

#include "core/controllers/gatewayController.h"
#include "core/utils/api/apiEnums.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/constants/configKeys.h"
#include <QtConcurrent/QtConcurrent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSharedPointer>

using namespace amnezia;

NewsController::NewsController(SecureAppSettingsRepository* appSettingsRepository,
                               ServersController* serversController)
    : m_appSettingsRepository(appSettingsRepository), m_serversController(serversController)
{
}

QFuture<QPair<ErrorCode, QJsonArray>> NewsController::fetchNews()
{
    if (!m_serversController) {
        qWarning() << "ServersController is null, skip fetchNews";
        return QtFuture::makeReadyFuture(qMakePair(ErrorCode::InternalError, QJsonArray()));
    }
    
    const auto stacks = m_serversController->gatewayStacks();
    if (stacks.isEmpty()) {
        qDebug() << "No Gateway stacks, skip fetchNews";
        return QtFuture::makeReadyFuture(qMakePair(ErrorCode::NoError, QJsonArray()));
    }

    auto gatewayController = QSharedPointer<GatewayController>::create(
        m_appSettingsRepository->getGatewayEndpoint(),
        m_appSettingsRepository->isDevGatewayEnv(),
        apiDefs::requestTimeoutMsecs,
        m_appSettingsRepository->isStrictKillSwitchEnabled());
    
    QJsonObject payload;
    payload.insert("locale", m_appSettingsRepository->getAppLanguage().name().split("_").first());

    const QJsonObject stacksJson = stacks.toJson();
    if (stacksJson.contains(apiDefs::key::userCountryCode)) {
        payload.insert(apiDefs::key::userCountryCode, stacksJson.value(apiDefs::key::userCountryCode));
    }
    if (stacksJson.contains(apiDefs::key::serviceType)) {
        payload.insert(apiDefs::key::serviceType, stacksJson.value(apiDefs::key::serviceType));
    }

    auto future = gatewayController->postAsync(QString("%1v1/news"), payload);
    return future.then([gatewayController](QPair<ErrorCode, QByteArray> result) -> QPair<ErrorCode, QJsonArray> {
        auto [errorCode, responseBody] = result;
        if (errorCode != ErrorCode::NoError) {
            return qMakePair(errorCode, QJsonArray());
        }

        QJsonDocument doc = QJsonDocument::fromJson(responseBody);
        QJsonArray newsArray;
        if (doc.isArray()) {
            newsArray = doc.array();
        } else if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.value("news").isArray()) {
                newsArray = obj.value("news").toArray();
            }
        }

        return qMakePair(ErrorCode::NoError, newsArray);
    });
}

