#ifndef VPNCONFIGIRATIONSCONTROLLER_H
#define VPNCONFIGIRATIONSCONTROLLER_H

#include <QObject>

#include "configurators/configurator_base.h"
#include "containers/containers_defs.h"
#include "core/defs.h"
#include "settings.h"

class VpnConfigurationsController : public QObject
{
    Q_OBJECT
public:
    explicit VpnConfigurationsController(const std::shared_ptr<Settings> &settings, QSharedPointer<ServerController> serverController, QObject *parent = nullptr);

public slots:
    ErrorCode createProtocolConfigForContainer(const ServerCredentials &credentials, const DockerContainer container,
                                               QJsonObject &containerConfig);
    ErrorCode createProtocolConfigString(const bool isApiConfig, const QPair<QString, QString> &dns, const ServerCredentials &credentials,
                                         const DockerContainer container, const QJsonObject &containerConfig, const Proto protocol,
                                         QString &protocolConfigString);
    QJsonObject createVpnConfiguration(const QPair<QString, QString> &dns, const QJsonObject &serverConfig,
                                       const QJsonObject &containerConfig, const DockerContainer container, ErrorCode &errorCode);

    static void updateContainerConfigAfterInstallation(const DockerContainer container, QJsonObject &containerConfig, const QString &stdOut);
signals:

private:
    QScopedPointer<ConfiguratorBase> createConfigurator(const Proto protocol);

    std::shared_ptr<Settings> m_settings;
    QSharedPointer<ServerController> m_serverController;
};

#endif // VPNCONFIGIRATIONSCONTROLLER_H
