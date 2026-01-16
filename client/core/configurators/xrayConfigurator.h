#ifndef XRAY_CONFIGURATOR_H
#define XRAY_CONFIGURATOR_H

#include <QObject>

#include "configuratorBase.h"
#include "core/utils/defs.h"

class XrayConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    XrayConfigurator(AppSettingsRepository* appSettingsRepository, SshSession* sshSession, QObject *parent = nullptr);

    ProtocolConfig createConfig(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &containerConfig,
                                ErrorCode &errorCode);

private:
    QString prepareServerConfig(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &containerConfig,
                                ErrorCode &errorCode);
};

#endif // XRAY_CONFIGURATOR_H
