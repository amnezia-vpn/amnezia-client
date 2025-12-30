#ifndef CLOAKINSTALLER_H
#define CLOAKINSTALLER_H

#include "installerBase.h"

class CloakInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit CloakInstaller(QObject *parent = nullptr);

    ErrorCode extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                         ServerController* serverController, QJsonObject &config) override;
};

#endif // CLOAKINSTALLER_H

