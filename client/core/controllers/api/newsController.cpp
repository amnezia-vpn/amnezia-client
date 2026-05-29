#include "newsController.h"

#include "core/controllers/gatewayController.h"
#include "core/repositories/secureServersRepository.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include <QtConcurrent/QtConcurrent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QSharedPointer>

using namespace amnezia;

NewsController::NewsController(SecureAppSettingsRepository *appSettingsRepository,
                               SecureServersRepository *serversRepository)
    : m_appSettingsRepository(appSettingsRepository),
      m_serversRepository(serversRepository)
{
}

QJsonObject NewsController::getServicesList() const
{
    if (!m_serversRepository) {
        return {};
    }
    QSet<QString> userCountryCodes;
    QSet<QString> serviceTypes;
    const QVector<QString> ids = m_serversRepository->orderedServerIds();
    for (const QString &id : ids) {
        const auto apiV2 = m_serversRepository->apiV2Config(id);
        if (!apiV2.has_value()) {
            continue;
        }
        if (!apiV2->apiConfig.userCountryCode.isEmpty()) {
            userCountryCodes.insert(apiV2->apiConfig.userCountryCode);
        }
        const QString serviceType = apiV2->serviceType();
        if (!serviceType.isEmpty()) {
            serviceTypes.insert(serviceType);
        }
    }
    if (userCountryCodes.isEmpty() && serviceTypes.isEmpty()) {
        return {};
    }
    QJsonObject json;

    QJsonArray userCountryCodesArray;
    for (const QString &code : userCountryCodes) {
        userCountryCodesArray.append(code);
    }
    json[apiDefs::key::userCountryCode] = userCountryCodesArray;

    QJsonArray serviceTypesArray;
    for (const QString &type : serviceTypes) {
        serviceTypesArray.append(type);
    }
    json[apiDefs::key::serviceType] = serviceTypesArray;

    return json;
}

QFuture<QPair<ErrorCode, QJsonArray>> NewsController::fetchNews()
{
    if (!m_serversRepository) {
        qWarning() << "SecureServersRepository is null, skip fetchNews";
        return QtFuture::makeReadyFuture(qMakePair(ErrorCode::InternalError, QJsonArray()));
    }

    const QJsonObject services = getServicesList();
    if (services.isEmpty()) {
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

    if (services.contains(apiDefs::key::userCountryCode)) {
        payload.insert(apiDefs::key::userCountryCode, services.value(apiDefs::key::userCountryCode));
    }
    if (services.contains(apiDefs::key::serviceType)) {
        payload.insert(apiDefs::key::serviceType, services.value(apiDefs::key::serviceType));
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
