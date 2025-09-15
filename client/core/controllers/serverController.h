#ifndef SERVERCONTROLLER_H
#define SERVERCONTROLLER_H

#include <QJsonObject>
#include <QObject>

#include "containers/containers_defs.h"
#include "core/defs.h"
#include "core/models/containers/containerConfig.h"
#include "core/models/servers/selfHostedServerConfig.h"
#include "core/sshclient.h"

class Settings;
class VpnConfigurator;

using namespace amnezia;

class ServerController : public QObject
{
    Q_OBJECT
public:
    ServerController(std::shared_ptr<Settings> settings, QObject *parent = nullptr);
    ~ServerController();

    typedef QList<QPair<QString, QString>> Vars;

    ErrorCode rebootServer(const ServerCredentials &serverCredentials);
    ErrorCode removeAllContainers(const ServerCredentials &serverCredentials);
    ErrorCode removeContainer(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials);
    ErrorCode setupContainer(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials, bool isUpdate = false);
    ErrorCode updateContainer(const ServerCredentials &serverCredentials, const ContainerConfig &oldConfig, ContainerConfig &newConfig);

    ErrorCode startupContainerWorker(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials);

    ErrorCode uploadTextFileToContainer(const DockerContainer &containerType, const ServerCredentials &serverCredentials,
                                        const QString &file, const QString &path,
                                        libssh::ScpOverwriteMode overwriteMode = libssh::ScpOverwriteMode::ScpOverwriteExisting);
    QByteArray getTextFileFromContainer(const DockerContainer &containerType, const ServerCredentials &credentials, const QString &path,
                                        ErrorCode &errorCode);

    QString replaceVars(const QString &script, const Vars &vars);
    Vars genVarsForScript(const ServerCredentials &serverCredentials, const DockerContainer containerType);
    Vars genVarsForScript(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials);

    ErrorCode runScript(const ServerCredentials &credentials, QString script,
                        const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdOut = nullptr,
                        const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdErr = nullptr);

    ErrorCode runContainerScript(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials, QString script,
                                 const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdOut = nullptr,
                                 const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdErr = nullptr);

    QString checkSshConnection(const ServerCredentials &credentials, ErrorCode &errorCode);

    void cancelInstallation();

    ErrorCode getDecryptedPrivateKey(const ServerCredentials &credentials, QString &decryptedPrivateKey,
                                     const std::function<QString()> &callback);

private:
    ErrorCode installDockerWorker(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials);
    ErrorCode prepareHostWorker(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials);
    ErrorCode buildContainerWorker(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials);
    ErrorCode runContainerWorker(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials);
    ErrorCode configureContainerWorker(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials);

    ErrorCode isServerPortBusy(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials);
    bool isReinstallContainerRequired(DockerContainer container, const ContainerConfig &oldConfig, const ContainerConfig &newConfig);
    ErrorCode isUserInSudo(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials);
    ErrorCode isServerDpkgBusy(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials);

    ErrorCode uploadFileToHost(const ServerCredentials &serverCredentials, const QByteArray &data, const QString &remotePath,
                               libssh::ScpOverwriteMode overwriteMode = libssh::ScpOverwriteMode::ScpOverwriteExisting);

    ErrorCode setupServerFirewall(const ServerCredentials &serverCredentials);

    std::shared_ptr<Settings> m_settings;
    std::shared_ptr<VpnConfigurator> m_configurator;

    bool m_cancelInstallation = false;
    libssh::Client m_sshClient;
signals:
    void serverIsBusy(const bool isBusy);
};

#endif // SERVERCONTROLLER_H
