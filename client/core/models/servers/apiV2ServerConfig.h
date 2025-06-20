#ifndef APIV2SERVERCONFIG_H
#define APIV2SERVERCONFIG_H

#include "serverConfig.h"

namespace apiv2
{
    struct Country {
        QString code;
        QString name;
    };

    struct PublicKey
    {
        QString expiresAt;
    };

    struct Subscription
    {
        QString end_date;
    };

    struct ApiConfig {
        QVector<Country> availableCountries;

        Subscription subscription;
        PublicKey publicKey;

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
    ApiV2ServerConfig(const QJsonObject &serverConfigObject);

    QJsonObject toJson() const override;

    QString name;
    QString description;

    apiv2::ApiConfig apiConfig;
};

#endif // APIV2SERVERCONFIG_H
