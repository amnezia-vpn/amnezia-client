#include "apiNewsController.h"

#include "core/api/apiUtils.h"
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

ApiNewsController::ApiNewsController(NewsModel* newsModel,
                                     QAppSettingsRepository* appSettingsRepository,
                                     ServersController* serversController, QObject *parent)
    : QObject(parent), m_newsModel(newsModel), m_appSettingsRepository(appSettingsRepository), m_serversController(serversController)
{
}

void ApiNewsController::fetchNews(bool showError)
{
    if (!m_serversController) {
        qWarning() << "ServersController is null, skip fetchNews";
        return;
    }
    const auto stacks = m_serversController->gatewayStacks();
    if (stacks.isEmpty()) {
        qDebug() << "No Gateway stacks, skip fetchNews";
        return;
    }

    auto gatewayController = QSharedPointer<GatewayController>::create(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(),
                                                                       apiDefs::requestTimeoutMsecs, m_appSettingsRepository->isStrictKillSwitchEnabled());
    QJsonObject payload;
    payload.insert("locale", m_appSettingsRepository->getAppLanguage().name().split("_").first());

    const QJsonObject stacksJson = stacks.toJson();
    if (stacksJson.contains(configKey::userCountryCode)) {
        payload.insert(configKey::userCountryCode, stacksJson.value(configKey::userCountryCode));
    }
    if (stacksJson.contains(configKey::serviceType)) {
        payload.insert(configKey::serviceType, stacksJson.value(configKey::serviceType));
    }

    auto future = gatewayController->postAsync(QString("%1v1/news"), payload);
    future.then(this, [this, showError, gatewayController](QPair<ErrorCode, QByteArray> result) {
        auto [errorCode, responseBody] = result;
        if (errorCode != ErrorCode::NoError) {
            emit errorOccurred(errorCode, showError);
            return;
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

        m_newsModel->updateModel(newsArray);
        emit fetchNewsFinished();
    });
}
