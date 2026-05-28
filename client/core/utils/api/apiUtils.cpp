#include "apiUtils.h"

#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/configKeys.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

using namespace amnezia;

namespace
{
    const QByteArray AMNEZIA_CONFIG_SIGNATURE = QByteArray::fromHex("000000ff");

    constexpr QLatin1String unprocessableSubscriptionMessage("Failed to retrieve subscription information. Is it activated?");
    constexpr QLatin1String trialAlreadyUsedMessage("trial subscription already used");

    QDateTime subscriptionEndUtcFromString(const QString &subscriptionEndDate)
    {
        if (subscriptionEndDate.isEmpty()) {
            return {};
        }
        QDateTime endDate = QDateTime::fromString(subscriptionEndDate, Qt::ISODateWithMs).toUTC();
        if (!endDate.isValid()) {
            endDate = QDateTime::fromString(subscriptionEndDate, Qt::ISODate).toUTC();
        }
        return endDate;
    }

    QString apiErrorMessageFromJson(const QJsonObject &jsonObj)
    {
        const QJsonValue value = jsonObj.value(QStringLiteral("message"));
        return value.isString() ? value.toString().trimmed() : QString();
    }

    QString escapeUnicode(const QString &input)
    {
        QString output;
        for (QChar c : input) {
            if (c.unicode() < 0x20 || c.unicode() > 0x7E) {
                output += QString("\\u%1").arg(QString::number(c.unicode(), 16).rightJustified(4, '0'));
            } else {
                output += c;
            }
        }
        return output;
    }
}

bool apiUtils::isSubscriptionExpired(const QString &subscriptionEndDate)
{
    if (subscriptionEndDate.isEmpty()) {
        return false;
    }
    const QDateTime endDate = subscriptionEndUtcFromString(subscriptionEndDate);
    if (!endDate.isValid()) {
        return false;
    }
    return endDate <= QDateTime::currentDateTimeUtc();
}

bool apiUtils::isSubscriptionExpiringSoon(const QString &subscriptionEndDate, int withinDays)
{
    if (subscriptionEndDate.isEmpty()) {
        return false;
    }
    const QDateTime endDate = subscriptionEndUtcFromString(subscriptionEndDate);
    if (!endDate.isValid()) {
        return false;
    }
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    if (endDate <= nowUtc) {
        return false;
    }
    return endDate <= nowUtc.addDays(withinDays);
}

amnezia::ErrorCode apiUtils::checkNetworkReplyErrors(const QList<QSslError> &sslErrors, const QString &replyErrorString,
                                                     const QNetworkReply::NetworkError &replyError, const int httpStatusCode,
                                                     const QByteArray &responseBody)
{
    const int httpStatusCodeConflict = 409;
    const int httpStatusCodeNotFound = 404;
    const int httpStatusCodeNotImplemented = 501;
    const int httpStatusCodePaymentRequired = 402;
    const int httpStatusCodeTooManyRequests = 429;
    const int httpStatusCodeRequestTimeout = 408;
    const int httpStatusCodeUnprocessableEntity = 422;

    if (!sslErrors.empty()) {
        qDebug().noquote() << sslErrors;
        return amnezia::ErrorCode::ApiConfigSslError;
    }
    if (replyError == QNetworkReply::NetworkError::OperationCanceledError
        || replyError == QNetworkReply::NetworkError::TimeoutError) {
        qDebug() << replyError;
        return amnezia::ErrorCode::ApiConfigTimeoutError;
    }
    if (replyError == QNetworkReply::NetworkError::OperationNotImplementedError) {
        qDebug() << replyError;
        return amnezia::ErrorCode::ApiUpdateRequestError;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseBody);
    if (jsonDoc.isObject()) {
        QJsonObject jsonObj = jsonDoc.object();
        const int httpStatusFromBody = jsonObj.value(QStringLiteral("http_status")).toInt(-1);

        if (httpStatusFromBody == httpStatusCodeTooManyRequests) {
            return amnezia::ErrorCode::ApiRateLimitError;
        }
        if (httpStatusFromBody == httpStatusCodeConflict) {
            if (apiErrorMessageFromJson(jsonObj).contains(trialAlreadyUsedMessage, Qt::CaseInsensitive)) {
                return amnezia::ErrorCode::ApiTrialAlreadyUsedError;
            }
            return amnezia::ErrorCode::ApiConfigLimitError;
        }
        if (httpStatusFromBody == httpStatusCodeNotFound) {
            return amnezia::ErrorCode::ApiNotFoundError;
        }
        if (httpStatusFromBody == httpStatusCodeRequestTimeout) {
            return amnezia::ErrorCode::ApiConfigTimeoutError;
        }
        if (httpStatusFromBody == httpStatusCodeNotImplemented) {
            return amnezia::ErrorCode::ApiUpdateRequestError;
        }
        if (httpStatusFromBody == httpStatusCodeUnprocessableEntity) {
            if (apiErrorMessageFromJson(jsonObj) == unprocessableSubscriptionMessage) {
                return amnezia::ErrorCode::ApiSubscriptionExpiredError;
            }
            return amnezia::ErrorCode::ApiConfigDownloadError;
        }
        if (httpStatusFromBody == httpStatusCodePaymentRequired) {
            const QString message = apiErrorMessageFromJson(jsonObj);
            if (message.contains(QLatin1String("refresh_captcha"), Qt::CaseInsensitive)) {
                return amnezia::ErrorCode::ApiCaptchaRefreshError;
            }
            if (message.contains(QLatin1String("invalid_captcha"), Qt::CaseInsensitive)) {
                return amnezia::ErrorCode::ApiCaptchaInvalidError;
            }
            if (jsonObj.contains(QStringLiteral("captcha_id")) || jsonObj.contains(QStringLiteral("captcha_image"))
                || message.compare(QLatin1String("rate_limit_exceeded"), Qt::CaseInsensitive) == 0
                || message.contains(QLatin1String("rate_limit_exceeded"), Qt::CaseInsensitive)) {
                return amnezia::ErrorCode::ApiCaptchaRequiredError;
            }
            return amnezia::ErrorCode::ApiSubscriptionNotActiveError;
        }

        if (httpStatusFromBody >= 300) {
            return amnezia::ErrorCode::ApiConfigDownloadError;
        }
    }

    if (replyError == QNetworkReply::NoError) {
        return amnezia::ErrorCode::NoError;
    }

    qDebug() << "something went wrong";
    return amnezia::ErrorCode::ApiConfigDownloadError;
}

bool apiUtils::isPremiumServer(const QJsonObject &serverConfigObject)
{
    static const QSet<serverConfigUtils::ConfigType> premiumTypes = { serverConfigUtils::ConfigType::AmneziaPremiumV1, serverConfigUtils::ConfigType::AmneziaPremiumV2,
                                                            serverConfigUtils::ConfigType::ExternalPremium };
    return premiumTypes.contains(serverConfigUtils::configTypeFromJson(serverConfigObject));
}

QString apiUtils::getPremiumV1VpnKey(const QJsonObject &serverConfigObject)
{
    if (serverConfigUtils::configTypeFromJson(serverConfigObject) != serverConfigUtils::ConfigType::AmneziaPremiumV1) {
        return {};
    }

    QList<QPair<QString, QVariant>> orderedFields;
    orderedFields.append(qMakePair(configKey::name, serverConfigObject[configKey::name].toString()));
    orderedFields.append(qMakePair(configKey::description, serverConfigObject[configKey::description].toString()));
    orderedFields.append(qMakePair(configKey::configVersion, serverConfigObject[configKey::configVersion].toDouble()));
    orderedFields.append(qMakePair(apiDefs::key::protocol, serverConfigObject[apiDefs::key::protocol].toString()));
    orderedFields.append(qMakePair(apiDefs::key::apiEndpoint, serverConfigObject[apiDefs::key::apiEndpoint].toString()));
    orderedFields.append(qMakePair(apiDefs::key::apiKey, serverConfigObject[apiDefs::key::apiKey].toString()));

    QString vpnKeyStr = "{";
    for (int i = 0; i < orderedFields.size(); ++i) {
        const auto &pair = orderedFields[i];
        if (pair.second.typeId() == QMetaType::Type::QString) {
            vpnKeyStr += "\"" + pair.first + "\": \"" + pair.second.toString() + "\"";
        } else if (pair.second.typeId() == QMetaType::Type::Double || pair.second.typeId() == QMetaType::Type::Int) {
            vpnKeyStr += "\"" + pair.first + "\": " + QString::number(pair.second.toDouble(), 'f', 1);
        }

        if (i < orderedFields.size() - 1) {
            vpnKeyStr += ", ";
        }
    }
    vpnKeyStr += "}";

    QByteArray vpnKeyCompressed = escapeUnicode(vpnKeyStr).toUtf8();
    vpnKeyCompressed = qCompress(vpnKeyCompressed, 6);
    vpnKeyCompressed = vpnKeyCompressed.mid(4);

    QByteArray signedData = AMNEZIA_CONFIG_SIGNATURE + vpnKeyCompressed;

    return QString("vpn://%1").arg(QString(signedData.toBase64(QByteArray::Base64UrlEncoding)));
}

QString apiUtils::getPremiumV2VpnKey(const QJsonObject &serverConfigObject)
{
    auto configType = serverConfigUtils::configTypeFromJson(serverConfigObject);
    if (configType != serverConfigUtils::ConfigType::AmneziaPremiumV2 && configType != serverConfigUtils::ConfigType::ExternalPremium) {
        return {};
    }

    QString vpnKeyText = "";

    auto apiConfig = serverConfigObject.value(apiDefs::key::apiConfig).toObject();
    auto authData = serverConfigObject.value(QLatin1String("auth_data")).toObject();

    const QString name = serverConfigObject.value(configKey::name).toString();
    const QString description = serverConfigObject.value(configKey::description).toString();
    const double configVersion = serverConfigObject.value(configKey::configVersion).toDouble();

    const QString serviceType = apiConfig.value(apiDefs::key::serviceType).toString();
    const QString serviceProtocol = apiConfig.value(QLatin1String("service_protocol")).toString();
    const QString userCountryCode = apiConfig.value(QLatin1String("user_country_code")).toString();

    const QString apiKey = authData.value(apiDefs::key::apiKey).toString();

    QString vpnKeyStr = "{";
    vpnKeyStr += "\"" + QString(configKey::name) + "\": \"" + name + "\", ";
    vpnKeyStr += "\"" + QString(configKey::description) + "\": \"" + description + "\", ";
    vpnKeyStr += "\"" + QString(configKey::configVersion) + "\": " + QString::number(static_cast<int>(configVersion)) + ", ";

    vpnKeyStr += "\"" + QString(apiDefs::key::apiConfig) + "\": {";
    vpnKeyStr += "\"" + QString(apiDefs::key::serviceType) + "\": \"" + serviceType + "\", ";
    vpnKeyStr += "\"service_protocol\": \"" + serviceProtocol + "\", ";
    vpnKeyStr += "\"user_country_code\": \"" + userCountryCode + "\"";
    vpnKeyStr += "}, ";

    vpnKeyStr += "\"auth_data\": {";
    vpnKeyStr += "\"" + QString(apiDefs::key::apiKey) + "\": \"" + apiKey + "\"";
    vpnKeyStr += "}";

    vpnKeyStr += "}";

    QByteArray vpnKeyCompressed = escapeUnicode(vpnKeyStr).toUtf8();
    vpnKeyCompressed = qCompress(vpnKeyCompressed, 6);
    vpnKeyCompressed = vpnKeyCompressed.mid(4);

    QByteArray signedData = AMNEZIA_CONFIG_SIGNATURE + vpnKeyCompressed;
    vpnKeyText = QString("vpn://%1").arg(QString(signedData.toBase64(QByteArray::Base64UrlEncoding)));

    return vpnKeyText;
}
