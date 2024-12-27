#ifndef XRAY_CONFIGURATOR_H
#define XRAY_CONFIGURATOR_H

#include <QObject>

#include "configurator_base.h"
#include "core/defs.h"

class XrayConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    XrayConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent = nullptr);

    QString createConfig(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig,
                         ErrorCode &errorCode);

private:
    QString prepareServerConfig(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig,
                                ErrorCode &errorCode);
};

#endif // XRAY_CONFIGURATOR_H
