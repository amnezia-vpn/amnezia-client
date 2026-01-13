#ifndef XRAY_CONFIGURATOR_H
#define XRAY_CONFIGURATOR_H

#include <QObject>

#include "configuratorBase.h"
#include "core/utils/defs.h"

class XrayConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    XrayConfigurator(std::shared_ptr<Settings> settings, SshSession* sshSession, QObject *parent = nullptr);

    QString createConfig(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig,
                         ErrorCode &errorCode);

private:
    QString prepareServerConfig(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig,
                                ErrorCode &errorCode);
};

#endif // XRAY_CONFIGURATOR_H
