#ifndef VPNCONFIGIRATIONSCONTROLLER_H
#define VPNCONFIGIRATIONSCONTROLLER_H

#include <QObject>

#include "configurators/configurator_base.h"
#include "containers/containers_defs.h"
#include "core/defs.h"
#include "core/models/containers/containerConfig.h"
#include "core/models/servers/selfHostedServerConfig.h"
#include "core/models/servers/serverConfig.h"
#include "settings.h"

class VpnConfigurationsController : public QObject
{
    Q_OBJECT
public:
    explicit VpnConfigurationsController(const std::shared_ptr<Settings> &settings, QSharedPointer<ServerController> serverController,
                                         QObject *parent = nullptr);

public slots:
    ErrorCode createClientProtocolConfigs(const ServerCredentials &serverCredentials, ContainerConfig &containerConfig);
    ErrorCode createClientProtocolConfig(const ServerCredentials &serverCredentials, const ContainerConfig &containerConfig,
                                         const Proto protocol, QSharedPointer<ProtocolConfig> &protocolConfig);
    void processNativeConfigForExport(const QPair<QString, QString> &dns, QSharedPointer<ProtocolConfig> &protocolConfig);
    QJsonObject createVpnConfiguration(const QPair<QString, QString> &dns, const ContainerConfig &containerConfig,
                                       const DockerContainer containerType, const int configVersion, const QString &hostName);

    static void updateContainerConfigAfterInstallation(const DockerContainer container, ContainerConfig &containerConfig,
                                                       const QString &stdOut);
signals:

private:
    QScopedPointer<ConfiguratorBase> createConfigurator(const Proto protocol);

    std::shared_ptr<Settings> m_settings;
    QSharedPointer<ServerController> m_serverController;
};

#endif // VPNCONFIGIRATIONSCONTROLLER_H
