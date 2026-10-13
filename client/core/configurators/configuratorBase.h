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
    // Native configs are cleaned up only for the protocols that list the DNS servers
    // line by line; JSON/plist/base64 content must not be text-processed (#3251 review).
    enum class DnsEntryCleanup {
        None,
        OpenVpn,    // drop "dhcp-option DNS" lines that lost their address
        Wireguard   // drop empty servers from the "DNS = a, b" line
    };

    void applyDnsToNativeConfig(const amnezia::DnsSettings &dns, amnezia::ProtocolConfig &protocolConfig,
                                DnsEntryCleanup cleanup = DnsEntryCleanup::None);

    SshSession* m_sshSession;
};

#endif // CONFIGURATORBASE_H
