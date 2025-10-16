#include "apiUtils.h"

#include <QDateTime>
#include <QJsonObject>

namespace
{
    const QByteArray AMNEZIA_CONFIG_SIGNATURE = QByteArray::fromHex("000000ff");

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
    QDateTime now = QDateTime::currentDateTimeUtc();
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
        constexpr QLatin1String freeV2Endpoint(FREE_V2_ENDPOINT);
        constexpr QLatin1String premiumV1Endpoint(PREM_V1_ENDPOINT);

        auto apiEndpoint = serverConfigObject.value(apiDefs::key::apiEndpoint).toString();

        if (apiEndpoint.contains(premiumV1Endpoint)) {
            return apiDefs::ConfigType::AmneziaPremiumV1;
        } else if (apiEndpoint.contains(freeV2Endpoint)) {
            return apiDefs::ConfigType::AmneziaFreeV2;
        }
    };
    case apiDefs::ConfigSource::AmneziaGateway: {
        constexpr QLatin1String servicePremium("amnezia-premium");
        constexpr QLatin1String serviceFree("amnezia-free");
        constexpr QLatin1String serviceExternalPremium("external-premium");

        auto apiConfigObject = serverConfigObject.value(apiDefs::key::apiConfig).toObject();
        auto serviceType = apiConfigObject.value(apiDefs::key::serviceType).toString();

        if (serviceType == servicePremium) {
            return apiDefs::ConfigType::AmneziaPremiumV2;
        } else if (serviceType == serviceFree) {
            return apiDefs::ConfigType::AmneziaFreeV3;
        } else if (serviceType == serviceExternalPremium) {
            return apiDefs::ConfigType::ExternalPremium;
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

amnezia::ErrorCode apiUtils::checkNetworkReplyErrors(const QList<QSslError> &sslErrors, const QString &replyErrorString,
                                                     const QNetworkReply::NetworkError &replyError, const int httpStatusCode,
                                                     const QByteArray &responseBody)
{
    const int httpStatusCodeConflict = 409;
    const int httpStatusCodeNotFound = 404;

    if (!sslErrors.empty()) {
        qDebug().noquote() << sslErrors;
        return amnezia::ErrorCode::ApiConfigSslError;
    } else if (replyError == QNetworkReply::NoError) {
        return amnezia::ErrorCode::NoError;
    } else if (replyError == QNetworkReply::NetworkError::OperationCanceledError
               || replyError == QNetworkReply::NetworkError::TimeoutError) {
        qDebug() << replyError;
        return amnezia::ErrorCode::ApiConfigTimeoutError;
    } else if (replyError == QNetworkReply::NetworkError::OperationNotImplementedError) {
        qDebug() << replyError;
        return amnezia::ErrorCode::ApiUpdateRequestError;
    } else {
        qDebug() << QString::fromUtf8(responseBody);
        qDebug() << replyError;
        qDebug() << replyErrorString;
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

bool apiUtils::isPremiumServer(const QJsonObject &serverConfigObject)
{
    static const QSet<apiDefs::ConfigType> premiumTypes = { apiDefs::ConfigType::AmneziaPremiumV1, apiDefs::ConfigType::AmneziaPremiumV2,
                                                            apiDefs::ConfigType::ExternalPremium };
    return premiumTypes.contains(getConfigType(serverConfigObject));
}

QString apiUtils::getPremiumV1VpnKey(const QJsonObject &serverConfigObject)
{
    if (apiUtils::getConfigType(serverConfigObject) != apiDefs::ConfigType::AmneziaPremiumV1) {
        return {};
    }

    QList<QPair<QString, QVariant>> orderedFields;
    orderedFields.append(qMakePair(apiDefs::key::name, serverConfigObject[apiDefs::key::name].toString()));
    orderedFields.append(qMakePair(apiDefs::key::description, serverConfigObject[apiDefs::key::description].toString()));
    orderedFields.append(qMakePair(apiDefs::key::configVersion, serverConfigObject[apiDefs::key::configVersion].toDouble()));
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
