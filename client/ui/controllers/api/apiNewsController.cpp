#include "apiNewsController.h"

#include <QJsonDocument>
#include <QJsonObject>
#include "core/api/apiUtils.h"

namespace
{
    namespace configKey
    {
        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serviceType[] = "service_type";
    }
}

ApiNewsController::ApiNewsController(const QSharedPointer<NewsModel> &newsModel,
                                     const std::shared_ptr<Settings> &settings,
                                     QObject *parent)
    : QObject(parent), m_newsModel(newsModel), m_settings(settings)
{
}

void ApiNewsController::fetchNews()
{
    const QJsonArray servers = m_settings->serversArray();
    bool hasGatewayApiServer = false;
    QJsonArray userCountryCodes;
    QJsonArray serviceTypes;

    for (const QJsonValue &v : servers) {
        if (!v.isObject())
            continue;
        const QJsonObject serverObj = v.toObject();
        if (!apiUtils::isServerFromApi(serverObj) || apiUtils::getConfigSource(serverObj) == apiDefs::ConfigSource::Telegram)
            continue;
        hasGatewayApiServer = true;

        const QJsonObject apiConfig = serverObj.value(apiDefs::key::apiConfig).toObject();
        const QString uc = apiConfig.value(configKey::userCountryCode).toString();
        const QString st = apiConfig.value(configKey::serviceType).toString();
        if (!uc.isEmpty() && !userCountryCodes.contains(uc)) {
            userCountryCodes.append(uc);
        }
        if (!st.isEmpty() && !serviceTypes.contains(st)) {
            serviceTypes.append(st);
        }
    }
    if (!hasGatewayApiServer) {
        qDebug() << "No Gateway API servers found, disabling news fetching";
        return;
    }
    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(),
                                        apiDefs::requestTimeoutMsecs, m_settings->isStrictKillSwitchEnabled());
    QByteArray responseBody;
    QJsonObject payload;
    payload.insert("locale", m_settings->getAppLanguage().name().split("_").first());

    if (!userCountryCodes.isEmpty()) payload.insert(configKey::userCountryCode, userCountryCodes);
    if (!serviceTypes.isEmpty()) payload.insert(configKey::serviceType, serviceTypes);

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