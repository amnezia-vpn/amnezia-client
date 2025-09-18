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

void ApiNewsController::fetchNews()
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
    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_settings->isStrictKillSwitchEnabled());
    QByteArray responseBody;
    QJsonObject payload;
    payload.insert("locale", m_settings->getAppLanguage().name().split("_").first());

    const QJsonObject stacksJson = stacks.toJson();
    if (stacksJson.contains(configKey::userCountryCode)) {
        payload.insert(configKey::userCountryCode, stacksJson.value(configKey::userCountryCode));
    }
    if (stacksJson.contains(configKey::serviceType)) {
        payload.insert(configKey::serviceType, stacksJson.value(configKey::serviceType));
    }

    ErrorCode errorCode = gatewayController.post(QString("%1v1/news"), payload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
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
}
