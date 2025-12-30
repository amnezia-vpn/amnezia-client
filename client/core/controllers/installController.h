#ifndef INSTALLCONTROLLER_H
#define INSTALLCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QProcess>

#include "containers/containers_defs.h"
#include "core/defs.h"

class ServerController;
class ServersRepository;
class InstallerBase;
class Settings;

using namespace amnezia;

class InstallController : public QObject
{
    Q_OBJECT

public:
    explicit InstallController(ServerController* serverController, 
                              ServersRepository* serversRepository,
                              const std::shared_ptr<Settings> &settings,
                              QObject *parent = nullptr);
    ~InstallController();

    ErrorCode setupContainer(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config, bool isUpdate = false);
    ErrorCode updateContainer(int serverIndex, DockerContainer container, const QJsonObject &oldConfig, QJsonObject &newConfig);

    ErrorCode rebootServer(int serverIndex);
    ErrorCode removeAllContainers(int serverIndex);
    ErrorCode removeContainer(int serverIndex, DockerContainer container);

    QJsonObject generateConfig(DockerContainer container, int port, TransportProto transportProto);
    ErrorCode getAlreadyInstalledContainers(const ServerCredentials &credentials, QMap<DockerContainer, QJsonObject> &installedContainers);
    
    ErrorCode scanServerForInstalledContainers(int serverIndex);
    
    ErrorCode installContainer(const ServerCredentials &credentials, DockerContainer container, int port, TransportProto transportProto, QJsonObject &config);

    ErrorCode installServer(const ServerCredentials &credentials, DockerContainer container, int port, TransportProto transportProto,
                                         bool &wasContainerInstalled);
    ErrorCode installContainer(const ServerCredentials &credentials, DockerContainer container, int port, TransportProto transportProto,
                                               int serverIndex, bool &wasContainerInstalled);
    
    bool isUpdateDockerContainerRequired(DockerContainer container, const QJsonObject &oldConfig, const QJsonObject &newConfig);
    
    ErrorCode checkSshConnection(const ServerCredentials &credentials, QString &output, std::function<QString()> passphraseCallback = nullptr);
    
    bool isServerAlreadyExists(const ServerCredentials &credentials, int &existingServerIndex);
    
    ErrorCode mountSftpDrive(const ServerCredentials &credentials, const QString &port, const QString &password, const QString &username);
    void stopAllSftpMounts();

    void cancelInstallation();

    void clearCachedProfile(int serverIndex, DockerContainer container);

    ErrorCode validateAndPrepareConfig(int serverIndex);

signals:
    void serverIsBusy(const bool isBusy);
    void cancelInstallationRequested();
    void clientRevocationRequested(const QJsonObject &containerConfig, DockerContainer container, 
                                   const ServerCredentials &credentials, int serverIndex);
    void clientAppendRequested(DockerContainer container, const ServerCredentials &credentials,
                               const QJsonObject &containerConfig, const QString &clientName);

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

    ErrorCode prepareContainerConfig(DockerContainer container, const ServerCredentials &credentials, QJsonObject &containerConfig, int serverIndex = -1);

    QScopedPointer<InstallerBase> createInstaller(DockerContainer container);

    ServerController* m_serverController;
    ServersRepository* m_serversRepository;
    std::shared_ptr<Settings> m_settings;
    bool m_cancelInstallation = false;
    
#ifndef Q_OS_IOS
    QList<QSharedPointer<QProcess>> m_sftpMountProcesses;
#endif
};

#endif // INSTALLCONTROLLER_H

