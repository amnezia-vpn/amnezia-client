#ifndef SERVERCONTROLLER_H
#define SERVERCONTROLLER_H

#include <QObject>
#include "sshconnection.h"
#include "sshremoteprocess.h"
#include "defs.h"

using namespace amnezia;

class ServerController : public QObject
{
    Q_OBJECT
public:
    typedef QList<QPair<QString, QString>> Vars;

    static ErrorCode fromSshConnectionErrorCode(QSsh::SshError error);

    // QSsh exitCode and exitStatus are different things
    static ErrorCode fromSshProcessExitStatus(int exitStatus);

    static QSsh::SshConnectionParameters sshParams(const ServerCredentials &credentials);

    static ErrorCode removeServer(const ServerCredentials &credentials, Protocol proto);
    static ErrorCode setupServer(const ServerCredentials &credentials, Protocol proto);

    static ErrorCode checkOpenVpnServer(DockerContainer container, const ServerCredentials &credentials);

    static ErrorCode uploadFileToHost(const ServerCredentials &credentials, const QByteArray &data, const QString &remotePath);

    static ErrorCode uploadTextFileToContainer(DockerContainer container,
        const ServerCredentials &credentials, const QString &file, const QString &path);

    static QString getTextFileFromContainer(DockerContainer container,
        const ServerCredentials &credentials, const QString &path, ErrorCode *errorCode = nullptr);

    static ErrorCode setupServerFirewall(const ServerCredentials &credentials);

    static QString replaceVars(const QString &script, const Vars &vars);

    static ErrorCode runScript(const QSsh::SshConnectionParameters &sshParams, QString script,
        const std::function<void(const QString &, QSharedPointer<QSsh::SshRemoteProcess>)> &cbReadStdOut = nullptr,
        const std::function<void(const QString &, QSharedPointer<QSsh::SshRemoteProcess>)> &cbReadStdErr = nullptr);

    static Vars genVarsForScript(const ServerCredentials &credentials, DockerContainer container = DockerContainer::None);

private:
    static QSsh::SshConnection *connectToHost(const QSsh::SshConnectionParameters &sshParams);

    static ErrorCode installDocker(const ServerCredentials &credentials);

    static ErrorCode setupOpenVpnServer(const ServerCredentials &credentials);
    static ErrorCode setupOpenVpnOverCloakServer(const ServerCredentials &credentials);
    static ErrorCode setupShadowSocksServer(const ServerCredentials &credentials);
};

#endif // SERVERCONTROLLER_H
