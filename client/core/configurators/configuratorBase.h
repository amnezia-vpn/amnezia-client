#ifndef CONFIGURATORBASE_H
#define CONFIGURATORBASE_H

#include <QObject>
#include <QScopedPointer>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/models/containerConfig.h"
#include "core/models/protocolConfig.h"

class SshSession;

class ConfiguratorBase : public QObject
{
    Q_OBJECT
public:
    explicit ConfiguratorBase(SshSession* sshSession, QObject *parent = nullptr);

    static QScopedPointer<ConfiguratorBase> create(amnezia::Proto protocol,
                                                   SshSession* sshSession);

    virtual amnezia::ProtocolConfig createConfig(const amnezia::ServerCredentials &credentials, amnezia::DockerContainer container,
                                        const amnezia::ContainerConfig &containerConfig,
                                        const amnezia::DnsSettings &dnsSettings,
                                        amnezia::ErrorCode &errorCode) = 0;

    virtual amnezia::ProtocolConfig processConfigWithLocalSettings(const amnezia::ConnectionSettings &settings,
                                                                   amnezia::ProtocolConfig protocolConfig);
    virtual amnezia::ProtocolConfig processConfigWithExportSettings(const amnezia::ExportSettings &settings,
                                                                     amnezia::ProtocolConfig protocolConfig);

protected:
    void applyDnsToNativeConfig(const amnezia::DnsSettings &dns, amnezia::ProtocolConfig &protocolConfig);

    SshSession* m_sshSession;
};

#endif // CONFIGURATORBASE_H
