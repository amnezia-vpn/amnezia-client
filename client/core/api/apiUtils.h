#ifndef APIUTILS_H
#define APIUTILS_H

#include <QNetworkReply>
#include <QObject>

#include "apiDefs.h"
#include "core/defs.h"

namespace apiUtils
{
    bool isServerFromApi(const QJsonObject &serverConfigObject);

    bool isSubscriptionExpired(const QString &subscriptionEndDate);

    bool isPremiumServer(const QJsonObject &serverConfigObject);

    apiDefs::ConfigType getConfigType(const QJsonObject &serverConfigObject);
    apiDefs::ConfigSource getConfigSource(const QJsonObject &serverConfigObject);

    amnezia::ErrorCode checkNetworkReplyErrors(const QList<QSslError> &sslErrors, QNetworkReply *reply);

    QString getPremiumV1VpnKey(const QJsonObject &serverConfigObject);
}

#endif // APIUTILS_H
