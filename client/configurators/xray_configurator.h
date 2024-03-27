#ifndef XRAY_CONFIGURATOR_H
#define XRAY_CONFIGURATOR_H

#include <QObject>

#include "configurator_base.h"
#include "core/defs.h"

class XrayConfigurator : ConfiguratorBase
{
    Q_OBJECT
public:
    XrayConfigurator(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    QString genXrayConfig(const ServerCredentials &credentials, DockerContainer container,
        const QJsonObject &containerConfig, QString &clientId, ErrorCode *errorCode = nullptr);
};

#endif // XRAY_CONFIGURATOR_H
