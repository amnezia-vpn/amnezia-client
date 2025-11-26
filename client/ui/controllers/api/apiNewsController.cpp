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

ApiNewsController::ApiNewsController(const QSharedPointer<NewsModel> &newsModel, const std::shared_ptr<Settings> &settings,
                                     const QSharedPointer<ServersModel> &serversModel, QObject *parent)
    : QObject(parent), m_newsModel(newsModel), m_settings(settings), m_serversModel(serversModel)
{
}

void ApiNewsController::fetchNews(bool showError)
{
    if (m_serversModel.isNull()) {
        qWarning() << "ServersModel is null, skip fetchNews";
        return;
    }
    const auto stacks = m_serversModel->gatewayStacks();
    if (stacks.isEmpty()) {
        qDebug() << "No Gateway stacks, skip fetchNews";
        return;
    }

    auto gatewayController = QSharedPointer<GatewayController>::create(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(),
                                                                       apiDefs::requestTimeoutMsecs, m_settings->isStrictKillSwitchEnabled());
    QJsonObject payload;
    payload.insert("locale", m_settings->getAppLanguage().name().split("_").first());

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
