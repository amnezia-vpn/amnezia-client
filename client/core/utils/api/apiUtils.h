#ifndef APIUTILS_H
#define APIUTILS_H

#include <QNetworkReply>
#include <QObject>

#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

class SecureAppSettingsRepository;

namespace apiUtils
{
    QString getAppLanguageCode(const SecureAppSettingsRepository *appSettingsRepository);
    QString getDistributionChannel();

    // The gateway may report a country code with a region suffix, e.g. "us-west", while flag
    // resources are named after the ISO 3166-1 alpha-2 code alone. Returns the part before the dash
    // in upper case.
    QString getCountryFlagCode(const QString &serverCountryCode);

    bool isSubscriptionExpired(const QString &subscriptionEndDate);

    bool isSubscriptionExpiringSoon(const QString &subscriptionEndDate, int withinDays = 30);

    bool isPremiumServer(const QJsonObject &serverConfigObject);

    amnezia::ErrorCode checkNetworkReplyErrors(const QList<QSslError> &sslErrors, const QString &replyErrorString,
                                               const QNetworkReply::NetworkError &replyError, const int httpStatusCode,
                                               const QByteArray &responseBody);

    QString getPremiumV2VpnKey(const QJsonObject &serverConfigObject);
}

#endif // APIUTILS_H
