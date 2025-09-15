#ifndef APIV1SERVERCONFIG_H
#define APIV1SERVERCONFIG_H

#include "serverConfig.h"

class ApiV1ServerConfig : public ServerConfig
{
public:
    ApiV1ServerConfig();
    ApiV1ServerConfig(const QJsonObject &serverConfigObject);
    ApiV1ServerConfig(const ApiV1ServerConfig &other);

    QJsonObject toJson() const override;

    QString name;
    QString description;

    QString accessToken;
    QString endpoint;
    QString serviceProtocol;
};

#endif // APIV1SERVERCONFIG_H
