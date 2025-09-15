#ifndef APIV2SERVERCONFIG_H
#define APIV2SERVERCONFIG_H

#include "serverConfig.h"

namespace apiv2
{
    struct Country
    {
        QString code;
        QString name;
    };

    struct PublicKey
    {
        QString expiresAt;
    };

    struct Subscription
    {
        QString endDate;
    };

    struct AuthData
    {
        QString apiKey;

        QJsonObject toJson();
    };

    struct ApiConfig
    {
        QVector<Country> availableCountries;
        QStringList supportedProtocols;

        Subscription subscription;
        PublicKey publicKey;

        AuthData authData;

        QString serverCountryCode;
        QString serverCountryName;

        QString serviceProtocol;
        QString serviceType;

        QString userCountryCode;

        QString vpnKey;
    };
}

class ApiV2ServerConfig : public ServerConfig
{
public:
    ApiV2ServerConfig();
    ApiV2ServerConfig(const QJsonObject &serverConfigObject);
    ApiV2ServerConfig(const ApiV2ServerConfig &other);

    QJsonObject toJson() const override;

    bool isPremium();

    QString name;
    QString description;

    apiv2::ApiConfig apiConfig;
};

#endif // APIV2SERVERCONFIG_H
