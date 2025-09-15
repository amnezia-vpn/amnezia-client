#ifndef SELFHOSTEDSERVERCONFIG_H
#define SELFHOSTEDSERVERCONFIG_H

#include "core/defs.h"
#include "serverConfig.h"
#include "core/models/protocols/protocolConfig.h"
#include "containers/containers_defs.h"

class SelfHostedServerConfig : public ServerConfig
{
public:
    SelfHostedServerConfig();
    SelfHostedServerConfig(const QJsonObject &serverConfigObject);
    SelfHostedServerConfig(const SelfHostedServerConfig &other);

    QJsonObject toJson() const override;

    QString name;

    amnezia::ServerCredentials serverCredentials;
};

#endif // SELFHOSTEDSERVERCONFIG_H
