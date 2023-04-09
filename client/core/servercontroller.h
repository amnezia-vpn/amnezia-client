#ifndef SERVERCONTROLLER_H
#define SERVERCONTROLLER_H

#include <QJsonObject>
#include <QObject>
#include "sshconnection.h"
#include "sshremoteprocess.h"
#include "logger.h"
#include "defs.h"

#include "containers/containers_defs.h"

#include "sftpdefs.h"

class Settings;
class VpnConfigurator;

using namespace amnezia;

class ServerController : public QObject
{
    Q_OBJECT
public:
    ServerController(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    typedef QList<QPair<QString, QString>> Vars;

    ErrorCode fromSshConnectionErrorCode(QSsh::SshError error);

    // QSsh exitCode and exitStatus are different things
    ErrorCode fromSshProcessExitStatus(int exitStatus);

    QSsh::SshConnectionParameters sshParams(const ServerCredentials &credentials);

    ErrorCode removeAllContainers(const ServerCredentials &credentials);
    ErrorCode removeContainer(const ServerCredentials &credentials, DockerContainer container);
    ErrorCode setupContainer(const ServerCredentials &credentials, DockerContainer container,
                             QJsonObject &config, bool isUpdate = false);
    ErrorCode updateContainer(const ServerCredentials &credentials, DockerContainer container,
                              const QJsonObject &oldConfig, QJsonObject &newConfig);

    // create initial config - generate passwords, etc
    QJsonObject createContainerInitialConfig(DockerContainer container, int port, TransportProto tp);

    bool isReinstallContainerRequired(DockerContainer container, const QJsonObject &oldConfig, const QJsonObject &newConfig);

    ErrorCode checkOpenVpnServer(DockerContainer container, const ServerCredentials &credentials);

    ErrorCode uploadFileToHost(const ServerCredentials &credentials, const QByteArray &data,
        const QString &remotePath, QSsh::SftpOverwriteMode overwriteMode = QSsh::SftpOverwriteMode::SftpOverwriteExisting);

    ErrorCode uploadTextFileToContainer(DockerContainer container,
        const ServerCredentials &credentials, const QString &file, const QString &path,
        QSsh::SftpOverwriteMode overwriteMode = QSsh::SftpOverwriteMode::SftpOverwriteExisting);

    QByteArray getTextFileFromContainer(DockerContainer container,
        const ServerCredentials &credentials, const QString &path, ErrorCode *errorCode = nullptr);

    ErrorCode setupServerFirewall(const ServerCredentials &credentials);

    QString replaceVars(const QString &script, const Vars &vars);

    ErrorCode runScript(const ServerCredentials &credentials, QString script,
        const std::function<void(const QString &, QSharedPointer<QSsh::SshRemoteProcess>)> &cbReadStdOut = nullptr,
        const std::function<void(const QString &, QSharedPointer<QSsh::SshRemoteProcess>)> &cbReadStdErr = nullptr);

    ErrorCode runContainerScript(const ServerCredentials &credentials, DockerContainer container, QString script,
        const std::function<void(const QString &, QSharedPointer<QSsh::SshRemoteProcess>)> &cbReadStdOut = nullptr,
        const std::function<void(const QString &, QSharedPointer<QSsh::SshRemoteProcess>)> &cbReadStdErr = nullptr);

    Vars genVarsForScript(const ServerCredentials &credentials, DockerContainer container = DockerContainer::None, const QJsonObject &config = QJsonObject());

    QString checkSshConnection(const ServerCredentials &credentials, ErrorCode *errorCode = nullptr);
    QSharedPointer<QSsh::SshConnection> connectToHost(const QSsh::SshConnectionParameters &sshParams);

    void setCancelInstallation(const bool cancel);
    ErrorCode getAlreadyInstalledContainers(const ServerCredentials &credentials, QMap<DockerContainer, QJsonObject> &installedContainers);
private:

    ErrorCode installDockerWorker(const ServerCredentials &credentials, DockerContainer container);
    ErrorCode prepareHostWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config = QJsonObject());
    ErrorCode buildContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config = QJsonObject());
    ErrorCode runContainerWorker(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config);
    ErrorCode configureContainerWorker(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config);
    ErrorCode startupContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config = QJsonObject());
    ErrorCode isServerPortBusy(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config);

    std::shared_ptr<Settings> m_settings;
    std::shared_ptr<VpnConfigurator> m_configurator;

    bool m_cancelInstallation = false;
signals:
    void serverIsBusy(const bool isBusy);
};

#endif // SERVERCONTROLLER_H
