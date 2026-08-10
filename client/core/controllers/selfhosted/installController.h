#ifndef INSTALLCONTROLLER_H
#define INSTALLCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QProcess>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/models/containerConfig.h"
#include "core/repositories/secureServersRepository.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/installers/mtProxyInstaller.h"

class SshSession;
class InstallerBase;

using namespace amnezia;

class InstallController : public QObject
{
    Q_OBJECT

public:
    explicit InstallController(SecureServersRepository* serversRepository,
                              SecureAppSettingsRepository* appSettingsRepository,
                              QObject *parent = nullptr);
    ~InstallController();

    ErrorCode setupContainer(const ServerCredentials &credentials, DockerContainer container, ContainerConfig &config, bool isUpdate = false);

    // Updates server-side container settings (admin self-hosted only): reconfigures the container over SSH.
    ErrorCode updateServerConfig(const QString &serverId, DockerContainer container, const ContainerConfig &oldConfig, ContainerConfig &newConfig);

    // Updates client-local settings only: rewrites the stored container config for any self-hosted/native server. No SSH.
    ErrorCode updateClientConfig(const QString &serverId, DockerContainer container, ContainerConfig &newConfig);

    ErrorCode rebootServer(const QString &serverId);
    ErrorCode removeAllContainers(const QString &serverId);
    ErrorCode removeContainer(const QString &serverId, DockerContainer container);

    ErrorCode setDockerContainerEnabledState(const QString &serverId, DockerContainer container, bool enabled);

    /// statusOut: 0 = not deployed, 1 = running, 2 = stopped, 3 = error
    ErrorCode queryDockerContainerStatus(const QString &serverId, DockerContainer container, int &statusOut);

    ErrorCode queryMtProxyDiagnostics(const QString &serverId, DockerContainer container, int listenPort,
                                      MtProxyContainerDiagnostics &out);

    QString fetchDockerContainerSecret(const QString &serverId, DockerContainer container);

    ContainerConfig generateConfig(DockerContainer container, int port, TransportProto transportProto);
    ErrorCode getAlreadyInstalledContainers(const ServerCredentials &credentials, QMap<DockerContainer, ContainerConfig> &installedContainers, SshSession &sshSession);
    
    ErrorCode scanServerForInstalledContainers(const QString &serverId);
    
    ErrorCode installContainer(const ServerCredentials &credentials, DockerContainer container, int port, TransportProto transportProto, ContainerConfig &config);

    ErrorCode installServer(const ServerCredentials &credentials, DockerContainer container, int port, TransportProto transportProto,
                                         bool &wasContainerInstalled);
    ErrorCode installContainer(const QString &serverId, DockerContainer container, int port, TransportProto transportProto,
                                               bool &wasContainerInstalled);
    
    bool isUpdateDockerContainerRequired(DockerContainer container, const ContainerConfig &oldConfig, const ContainerConfig &newConfig);
    
    ErrorCode checkSshConnection(ServerCredentials &credentials, QString &output,
                                 std::function<QString()> passphraseCallback = nullptr);
    
    bool isServerAlreadyExists(const ServerCredentials &credentials, int &existingServerIndex);
    
    ErrorCode mountSftpDrive(const ServerCredentials &credentials, const QString &port, const QString &password, const QString &username);
    void stopAllSftpMounts();

    void cancelInstallation();

    void clearCachedProfile(const QString &serverId, DockerContainer container);

    ErrorCode validateAndPrepareConfig(const QString &serverId);

    void validateConfig(const QString &serverId);

    void addEmptyServer(const ServerCredentials &credentials);

signals:
    void configValidated(bool isValid);
    void validationErrorOccurred(ErrorCode errorCode);

    void serverIsBusy(const bool isBusy);
    void cancelInstallationRequested();
    void clientRevocationRequested(const QString &serverId, const ContainerConfig &containerConfig, DockerContainer container);
    void clientAppendRequested(const QString &serverId, const QString &clientId, const QString &clientName, DockerContainer container);

private:
    ErrorCode installDockerWorker(const ServerCredentials &credentials, DockerContainer container, SshSession &sshSession);
    ErrorCode prepareHostWorker(const ServerCredentials &credentials, DockerContainer container, SshSession &sshSession);
    ErrorCode buildContainerWorker(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &config, SshSession &sshSession);
    ErrorCode runContainerWorker(const ServerCredentials &credentials, DockerContainer container, ContainerConfig &config, SshSession &sshSession);
    ErrorCode configureContainerWorker(const ServerCredentials &credentials, DockerContainer container, ContainerConfig &config, SshSession &sshSession);
    ErrorCode startupContainerWorker(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &config, SshSession &sshSession);

    ErrorCode isServerPortBusy(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &config, SshSession &sshSession);
    ErrorCode isUserInSudo(const ServerCredentials &credentials, SshSession &sshSession);
    ErrorCode isServerDpkgBusy(const ServerCredentials &credentials, SshSession &sshSession);
    ErrorCode setupServerFirewall(const ServerCredentials &credentials, SshSession &sshSession);
    bool isReinstallContainerRequired(DockerContainer container, const ContainerConfig &oldConfig, const ContainerConfig &newConfig);

    ErrorCode prepareContainerConfig(DockerContainer container, const ServerCredentials &credentials, ContainerConfig &containerConfig, SshSession &sshSession);

    ErrorCode processContainerForAdmin(DockerContainer container, ContainerConfig &containerConfig,
                                       const ServerCredentials &credentials, SshSession &sshSession,
                                       const QString &serverId, const QString &clientName);

    void adminAppendRequested(const QString &serverId, DockerContainer container,
                              const ContainerConfig &containerConfig, const QString &clientName);

    static void updateContainerConfigAfterInstallation(DockerContainer container, ContainerConfig &containerConfig, const QString &stdOut);

    QScopedPointer<InstallerBase> createInstaller(DockerContainer container);

    SecureServersRepository* m_serversRepository;
    SecureAppSettingsRepository* m_appSettingsRepository;
    bool m_cancelInstallation = false;
    
#ifndef Q_OS_IOS
    QList<QSharedPointer<QProcess>> m_sftpMountProcesses;
#endif
};

#endif // INSTALLCONTROLLER_H
