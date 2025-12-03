#ifndef INSTALLCONTROLLER_H
#define INSTALLCONTROLLER_H

#include <QObject>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/defs.h"

class ServerController;
class VpnConfigurationsController;
class ServersRepository;

using namespace amnezia;

class InstallController : public QObject
{
    Q_OBJECT

public:
    explicit InstallController(ServerController* serverController, QObject *parent = nullptr);

    ErrorCode setupContainer(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config, bool isUpdate = false);
    ErrorCode updateContainer(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &oldConfig, QJsonObject &newConfig);

    ErrorCode rebootServer(ServersRepository* serversRepository, int serverIndex);
    ErrorCode removeAllContainers(ServersRepository* serversRepository, int serverIndex);
    ErrorCode removeContainer(ServersRepository* serversRepository, int serverIndex, DockerContainer container);

    void cancelInstallation();

signals:
    void serverIsBusy(const bool isBusy);
    void cancelInstallationRequested();

private:
    ErrorCode installDockerWorker(const ServerCredentials &credentials, DockerContainer container);
    ErrorCode prepareHostWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config = QJsonObject());
    ErrorCode buildContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config);
    ErrorCode runContainerWorker(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config);
    ErrorCode configureContainerWorker(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config);
    ErrorCode startupContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config);

    ErrorCode isServerPortBusy(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config);
    ErrorCode isUserInSudo(const ServerCredentials &credentials, DockerContainer container);
    ErrorCode isServerDpkgBusy(const ServerCredentials &credentials, DockerContainer container);
    ErrorCode setupServerFirewall(const ServerCredentials &credentials);
    bool isReinstallContainerRequired(DockerContainer container, const QJsonObject &oldConfig, const QJsonObject &newConfig);

    ServerController* m_serverController;
    bool m_cancelInstallation = false;
    // VpnConfigurationsController* m_vpnConfigurationsController; // TODO: uncomment when ready
};

#endif // INSTALLCONTROLLER_H

