#ifndef XRAY_CONFIGURATOR_H
#define XRAY_CONFIGURATOR_H

#include <QObject>

#include "configuratorBase.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

class XrayConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    XrayConfigurator(SshSession* sshSession, QObject *parent = nullptr);

    ProtocolConfig createConfig(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &containerConfig,
                                const DnsSettings &dnsSettings,
                                ErrorCode &errorCode) override;

private:
    QString prepareServerConfig(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &containerConfig,
                                const DnsSettings &dnsSettings,
                                ErrorCode &errorCode);
};

#endif // XRAY_CONFIGURATOR_H
