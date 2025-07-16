#include "apiNewsController.h"

#include <QJsonDocument>
#include <QJsonObject>

ApiNewsController::ApiNewsController(const QSharedPointer<NewsModel> &newsModel,
                                     const std::shared_ptr<Settings> &settings,
                                     QObject *parent)
    : QObject(parent), m_newsModel(newsModel), m_settings(settings)
{
}

void ApiNewsController::fetchNews()
{
    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(),
                                        apiDefs::requestTimeoutMsecs, m_settings->isStrictKillSwitchEnabled());
    QByteArray responseBody;
    QJsonObject payload;
    payload.insert("locale", m_settings->getAppLanguage().name().split("_").first());

    ErrorCode errorCode = gatewayController.post(QString("%1v1/news"), payload, responseBody);
    qDebug() << "fetchNews" << errorCode;
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