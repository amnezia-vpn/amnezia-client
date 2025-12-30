#ifndef TORINSTALLER_H
#define TORINSTALLER_H

#include "installerBase.h"

class TorInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit TorInstaller(QObject *parent = nullptr);

    ErrorCode extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                         ServerController* serverController, QJsonObject &config) override;
};

#endif // TORINSTALLER_H

