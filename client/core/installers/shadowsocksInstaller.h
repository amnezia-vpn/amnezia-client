#ifndef SHADOWSOCKSINSTALLER_H
#define SHADOWSOCKSINSTALLER_H

#include "installerBase.h"

class ShadowSocksInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit ShadowSocksInstaller(QObject *parent = nullptr);

    ErrorCode extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                         ServerController* serverController, QJsonObject &config) override;
};

#endif // SHADOWSOCKSINSTALLER_H

