#include "apiUtils.h"

#include <QDateTime>
#include <QJsonObject>

bool apiUtils::isSubscriptionExpired(const QString &subscriptionEndDate)
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime endDate = QDateTime::fromString(subscriptionEndDate, Qt::ISODateWithMs);
    return endDate < now;
}

bool apiUtils::isServerFromApi(const QJsonObject &serverConfigObject)
{
    auto configVersion = serverConfigObject.value(apiDefs::key::configVersion).toInt();
    switch (configVersion) {
    case apiDefs::ConfigSource::Telegram: return true;
    case apiDefs::ConfigSource::AmneziaGateway: return true;
    default: return false;
    }
}

apiDefs::ConfigType apiUtils::getConfigType(const QJsonObject &serverConfigObject)
{
    auto configVersion = serverConfigObject.value(apiDefs::key::configVersion).toInt();
    switch (configVersion) {
    case apiDefs::ConfigSource::Telegram: {
    };
    case apiDefs::ConfigSource::AmneziaGateway: {
        constexpr QLatin1String stackPremium("prem");
        constexpr QLatin1String stackFree("free");

        constexpr QLatin1String servicePremium("amnezia-premium");
        constexpr QLatin1String serviceFree("amnezia-free");

        auto apiConfigObject = serverConfigObject.value(apiDefs::key::apiConfig).toObject();
        auto stackType = apiConfigObject.value(apiDefs::key::stackType).toString();
        auto serviceType = apiConfigObject.value(apiDefs::key::serviceType).toString();

        if (serviceType == servicePremium || stackType == stackPremium) {
            return apiDefs::ConfigType::AmneziaPremiumV2;
        } else if (serviceType == serviceFree || stackType == stackFree) {
            return apiDefs::ConfigType::AmneziaFreeV3;
        }
    }
    default: {
        return apiDefs::ConfigType::SelfHosted;
    }
    };
}

apiDefs::ConfigSource apiUtils::getConfigSource(const QJsonObject &serverConfigObject)
{
    return static_cast<apiDefs::ConfigSource>(serverConfigObject.value(apiDefs::key::configVersion).toInt());
}

amnezia::ErrorCode apiUtils::checkNetworkReplyErrors(const QList<QSslError> &sslErrors, QNetworkReply *reply)
{
    const int httpStatusCodeConflict = 409;
    const int httpStatusCodeNotFound = 404;

    if (!sslErrors.empty()) {
        qDebug().noquote() << sslErrors;
        return amnezia::ErrorCode::ApiConfigSslError;
    } else if (reply->error() == QNetworkReply::NoError) {
        return amnezia::ErrorCode::NoError;
    } else if (reply->error() == QNetworkReply::NetworkError::OperationCanceledError
               || reply->error() == QNetworkReply::NetworkError::TimeoutError) {
        return amnezia::ErrorCode::ApiConfigTimeoutError;
    } else {
        QString err = reply->errorString();
        int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << QString::fromUtf8(reply->readAll());
        qDebug() << reply->error();
        qDebug() << err;
        qDebug() << httpStatusCode;
        if (httpStatusCode == httpStatusCodeConflict) {
            return amnezia::ErrorCode::ApiConfigLimitError;
        } else if (httpStatusCode == httpStatusCodeNotFound) {
            return amnezia::ErrorCode::ApiNotFoundError;
        }
        return amnezia::ErrorCode::ApiConfigDownloadError;
    }

    qDebug() << "something went wrong";
    return amnezia::ErrorCode::InternalError;
}
