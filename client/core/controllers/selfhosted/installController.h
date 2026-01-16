#ifndef INSTALLCONTROLLER_H
#define INSTALLCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QProcess>

#include "containers/containers_defs.h"
#include "core/utils/defs.h"
#include "core/models/containerConfig.h"

class SshSession;
class ServersRepository;
class AppSettingsRepository;
class InstallerBase;

using namespace amnezia;

class InstallController : public QObject
{
    Q_OBJECT

public:
    explicit InstallController(SshSession* sshSession, 
                              ServersRepository* serversRepository,
                              AppSettingsRepository* appSettingsRepository,
                              QObject *parent = nullptr);
    ~InstallController();

    ErrorCode setupContainer(const ServerCredentials &credentials, DockerContainer container, ContainerConfig &config, bool isUpdate = false);
    ErrorCode updateContainer(int serverIndex, DockerContainer container, const ContainerConfig &oldConfig, ContainerConfig &newConfig);

    ErrorCode rebootServer(int serverIndex);
    ErrorCode removeAllContainers(int serverIndex);
    ErrorCode removeContainer(int serverIndex, DockerContainer container);

    ContainerConfig generateConfig(DockerContainer container, int port, TransportProto transportProto);
    ErrorCode getAlreadyInstalledContainers(const ServerCredentials &credentials, QMap<DockerContainer, ContainerConfig> &installedContainers);
    
    ErrorCode scanServerForInstalledContainers(int serverIndex);
    
    ErrorCode installContainer(const ServerCredentials &credentials, DockerContainer container, int port, TransportProto transportProto, ContainerConfig &config);

    ErrorCode installServer(const ServerCredentials &credentials, DockerContainer container, int port, TransportProto transportProto,
                                         bool &wasContainerInstalled);
    ErrorCode installContainer(int serverIndex, DockerContainer container, int port, TransportProto transportProto,
                                               bool &wasContainerInstalled);
    
    bool isUpdateDockerContainerRequired(DockerContainer container, const ContainerConfig &oldConfig, const ContainerConfig &newConfig);
    
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
    void clientRevocationRequested(int serverIndex, const QJsonObject &containerConfig, DockerContainer container);
    void clientAppendRequested(int serverIndex, const QString &clientId, const QString &clientName, DockerContainer container);

private:
    ErrorCode installDockerWorker(const ServerCredentials &credentials, DockerContainer container);
    ErrorCode prepareHostWorker(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &config = ContainerConfig{});
    ErrorCode buildContainerWorker(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &config);
    ErrorCode runContainerWorker(const ServerCredentials &credentials, DockerContainer container, ContainerConfig &config);
    ErrorCode configureContainerWorker(const ServerCredentials &credentials, DockerContainer container, ContainerConfig &config);
    ErrorCode startupContainerWorker(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &config);

    ErrorCode isServerPortBusy(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &config);
    ErrorCode isUserInSudo(const ServerCredentials &credentials, DockerContainer container);
    ErrorCode isServerDpkgBusy(const ServerCredentials &credentials, DockerContainer container);
    ErrorCode setupServerFirewall(const ServerCredentials &credentials);
    bool isReinstallContainerRequired(DockerContainer container, const ContainerConfig &oldConfig, const ContainerConfig &newConfig);

    ErrorCode prepareContainerConfig(DockerContainer container, const ServerCredentials &credentials, ContainerConfig &containerConfig);

    static void updateContainerConfigAfterInstallation(DockerContainer container, ContainerConfig &containerConfig, const QString &stdOut);

    QScopedPointer<InstallerBase> createInstaller(DockerContainer container);

    SshSession* m_sshSession;
    ServersRepository* m_serversRepository;
    AppSettingsRepository* m_appSettingsRepository;
    bool m_cancelInstallation = false;
    
#ifndef Q_OS_IOS
    QList<QSharedPointer<QProcess>> m_sftpMountProcesses;
#endif
};

#endif // INSTALLCONTROLLER_H

