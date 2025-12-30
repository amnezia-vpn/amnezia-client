#ifndef XRAYINSTALLER_H
#define XRAYINSTALLER_H

#include "installerBase.h"

class XrayInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit XrayInstaller(QObject *parent = nullptr);

    ErrorCode extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                         ServerController* serverController, QJsonObject &config) override;
};

#endif // XRAYINSTALLER_H

