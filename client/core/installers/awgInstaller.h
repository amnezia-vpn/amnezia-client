#ifndef AWGINSTALLER_H
#define AWGINSTALLER_H

#include "installerBase.h"

class AwgInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit AwgInstaller(QObject *parent = nullptr);

    QJsonObject generateConfig(DockerContainer container, int port, TransportProto transportProto) override;
    ErrorCode extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                         ServerController* serverController, QJsonObject &config) override;

private:
    void generateAwgParameters(QJsonObject &containerConfig);
};

#endif // AWGINSTALLER_H

