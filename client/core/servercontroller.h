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

    static ErrorCode fromSshConnectionErrorCode(QSsh::SshError error);

    // QSsh exitCode and exitStatus are different things
    static ErrorCode fromSshProcessExitStatus(int exitStatus);

    static QString caCertPath() { return "/opt/amneziavpn_data/pki/ca.crt"; }
    static QString clientCertPath() { return "/opt/amneziavpn_data/pki/issued/"; }
    static QString taKeyPath() { return "/opt/amneziavpn_data/ta.key"; }

    static QString getContainerName(amnezia::DockerContainer container);

    static QSsh::SshConnectionParameters sshParams(const ServerCredentials &credentials);

    static ErrorCode removeServer(const ServerCredentials &credentials, Protocol proto);
    static ErrorCode setupServer(const ServerCredentials &credentials, Protocol proto);

    static ErrorCode checkOpenVpnServer(DockerContainer container, const ServerCredentials &credentials);

    static ErrorCode uploadTextFileToContainer(DockerContainer container,
        const ServerCredentials &credentials, QString &file, const QString &path);

    static QString getTextFileFromContainer(DockerContainer container,
        const ServerCredentials &credentials, const QString &path, ErrorCode *errorCode = nullptr);

    static ErrorCode signCert(DockerContainer container,
        const ServerCredentials &credentials, QString clientId);

    static int ssRemotePort() { return 6789; } // TODO move to ShadowSocksDefs.h
    static int ssContainerPort() { return 8585; } // TODO move to ShadowSocksDefs.h
    static QString ssEncryption() { return "chacha20-ietf-poly1305"; } // TODO move to ShadowSocksDefs.h

    static ErrorCode setupServerFirewall(const ServerCredentials &credentials);
private:
    static QSsh::SshConnection *connectToHost(const QSsh::SshConnectionParameters &sshParams);
    static ErrorCode runScript(DockerContainer container,
        const QSsh::SshConnectionParameters &sshParams, QString script,
        const std::function<void(const QString &)> &cbReadStdOut = nullptr,
        const std::function<void(const QString &)> &cbReadStdErr = nullptr);

    static ErrorCode setupOpenVpnServer(const ServerCredentials &credentials);
    static ErrorCode setupShadowSocksServer(const ServerCredentials &credentials);

};

#endif // SERVERCONTROLLER_H
