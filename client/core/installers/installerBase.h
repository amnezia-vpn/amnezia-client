#ifndef INSTALLERBASE_H
#define INSTALLERBASE_H

#include <QObject>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/defs.h"
#include "core/controllers/serverController.h"

class InstallerBase : public QObject
{
    Q_OBJECT
public:
    explicit InstallerBase(QObject *parent = nullptr);

    // Generate configuration for installation
    virtual QJsonObject generateConfig(DockerContainer container, int port, TransportProto transportProto);

    // Extract configuration from installed container
    virtual ErrorCode extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                 ServerController* serverController, QJsonObject &config);

protected:
    QJsonObject createBaseConfig(DockerContainer container, int port, TransportProto transportProto);
};

#endif // INSTALLERBASE_H

