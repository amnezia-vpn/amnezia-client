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

    static QSsh::SshConnectionParameters sshParams(const ServerCredentials &credentials);

    static ErrorCode removeServer(const ServerCredentials &credentials, Protocol proto);
    static ErrorCode setupServer(const ServerCredentials &credentials, Protocol proto);

    static ErrorCode checkOpenVpnServer(const ServerCredentials &credentials);

    static ErrorCode uploadTextFileToContainer(const ServerCredentials &credentials, QString &file, const QString &path);
    static QString getTextFileFromContainer(const ServerCredentials &credentials, const QString &path, ErrorCode *errorCode = nullptr);

    static ErrorCode signCert(const ServerCredentials &credentials, QString clientId);

private:
    static QSsh::SshConnection *connectToHost(const QSsh::SshConnectionParameters &sshParams);
    static ErrorCode runScript(const QSsh::SshConnectionParameters &sshParams, QString script);

    static ErrorCode setupOpenVpnServer(const ServerCredentials &credentials);
    static ErrorCode setupShadowSocksServer(const ServerCredentials &credentials);

};

#endif // SERVERCONTROLLER_H
