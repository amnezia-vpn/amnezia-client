#ifndef VPNCONFIGIRATIONSCONTROLLER_H
#define VPNCONFIGIRATIONSCONTROLLER_H

#include <QObject>

#include "configurators/configurator_base.h"
#include "core/models/containers/containers_defs.h"
#include "core/models/containers/containerConfig.h"
#include "core/defs.h"
#include "core/models/protocols/protocolConfig.h"
#include "core/models/servers/serverConfig.h"
#include "settings.h"

class VpnConfigurationsController : public QObject
{
    Q_OBJECT
public:
    explicit VpnConfigurationsController(const std::shared_ptr<Settings> &settings, QSharedPointer<ServerController> serverController,
                                         QObject *parent = nullptr);

public slots:
    ErrorCode createProtocolConfigForContainer(const ServerCredentials &credentials, const DockerContainer container,
                                               ContainerConfig &containerConfig);
    ErrorCode createProtocolConfigString(const bool isApiConfig, const QPair<QString, QString> &dns, const ServerCredentials &credentials,
                                         const DockerContainer container, const ContainerConfig &containerConfig, const Proto protocol,
                                         QString &protocolConfigString);
    QJsonObject createVpnConfiguration(const QPair<QString, QString> &dns, const QSharedPointer<ServerConfig> &serverConfig,
                                       const ContainerConfig &containerConfig, const DockerContainer container);

    static void updateContainerConfigAfterInstallation(const DockerContainer container, ContainerConfig &containerConfig, const QString &stdOut);
signals:

public:
    QSharedPointer<ProtocolConfig> createProtocolConfig(const Proto protocol);

private:
    QScopedPointer<ConfiguratorBase> createConfigurator(const Proto protocol);

    std::shared_ptr<Settings> m_settings;
    QSharedPointer<ServerController> m_serverController;
};

#endif // VPNCONFIGIRATIONSCONTROLLER_H
