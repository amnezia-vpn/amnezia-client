#ifndef APIV1SERVERCONFIG_H
#define APIV1SERVERCONFIG_H

#include "serverConfig.h"

class ApiV1ServerConfig : public ServerConfig
{
public:
    ApiV1ServerConfig(const QJsonObject &serverConfigObject);

    QJsonObject toJson() const override;

    QString name;
    QString description;
};

#endif // APIV1SERVERCONFIG_H
