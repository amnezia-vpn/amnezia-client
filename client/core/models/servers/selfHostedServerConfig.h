#ifndef SELFHOSTEDSERVERCONFIG_H
#define SELFHOSTEDSERVERCONFIG_H

#include "core/defs.h"
#include "serverConfig.h"

class SelfHostedServerConfig : public ServerConfig
{
public:
    SelfHostedServerConfig(const QJsonObject &serverConfigObject);

    QJsonObject toJson() const override;

    QString name;

    amnezia::ServerCredentials serverCredentials;
};

#endif // SELFHOSTEDSERVERCONFIG_H
