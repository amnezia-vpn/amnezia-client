#include "dnsttConfigurator.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"

using namespace amnezia;

DnsttConfigurator::DnsttConfigurator(SshSession* sshSession, QObject *parent)
    : ConfiguratorBase(sshSession, parent)
{
}

ProtocolConfig DnsttConfigurator::createConfig(const ServerCredentials &credentials,
                                               DockerContainer container,
                                               const ContainerConfig &containerConfig,
                                               const DnsSettings &dnsSettings,
                                               ErrorCode &errorCode)
{
    Q_UNUSED(credentials)
    Q_UNUSED(container)
    Q_UNUSED(dnsSettings)
    errorCode = ErrorCode::NoError;
    return containerConfig.protocolConfig;
}

ProtocolConfig DnsttConfigurator::processConfigWithLocalSettings(const ConnectionSettings &settings,
                                                                 ProtocolConfig protocolConfig)
{
    applyDnsToNativeConfig(settings.dns, protocolConfig);
    return protocolConfig;
}
