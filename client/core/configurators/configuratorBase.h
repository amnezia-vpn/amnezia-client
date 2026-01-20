#ifndef CONFIGURATORBASE_H
#define CONFIGURATORBASE_H

#include <QObject>
#include <QScopedPointer>

#include "containers/containers_defs.h"
#include "core/utils/defs.h"
#include "core/models/containerConfig.h"
#include "core/models/protocolConfig.h"

class SshSession;

class ConfiguratorBase : public QObject
{
    Q_OBJECT
public:
    explicit ConfiguratorBase(SshSession* sshSession, QObject *parent = nullptr);

    static QScopedPointer<ConfiguratorBase> create(Proto protocol,
                                                   SshSession* sshSession);

    virtual ProtocolConfig createConfig(const ServerCredentials &credentials, DockerContainer container,
                                        const ContainerConfig &containerConfig,
                                        const DnsSettings &dnsSettings,
                                        ErrorCode &errorCode) = 0;

    virtual QString processConfigWithLocalSettings(const QPair<QString, QString> &dns, 
                                                   const bool isApiConfig,
                                                   const SplitTunnelingSettings &splitTunneling,
                                                   QString &protocolConfigString);
    virtual QString processConfigWithExportSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                    QString &protocolConfigString);

protected:
    void processConfigWithDnsSettings(const QPair<QString, QString> &dns, QString &protocolConfigString);

    SshSession* m_sshSession;
};

#endif // CONFIGURATORBASE_H
