#ifndef XRAY_CONFIGURATOR_H
#define XRAY_CONFIGURATOR_H

#include <QObject>

#include "configurator_base.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/defs.h"

class XrayConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    XrayConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent = nullptr);

    QSharedPointer<ProtocolConfig> createConfig(const ServerCredentials &credentials, DockerContainer container,
                                                const QSharedPointer<ProtocolConfig> &protocolConfig, ErrorCode &errorCode) override;

    Vars generateProtocolVars(const ServerCredentials &credentials, DockerContainer container,
                             const QSharedPointer<ProtocolConfig> &protocolConfig) const override;

private:
    QString prepareServerConfig(const ServerCredentials &credentials, DockerContainer container,
                                const QSharedPointer<ProtocolConfig> &protocolConfig, ErrorCode &errorCode);
};

#endif // XRAY_CONFIGURATOR_H
