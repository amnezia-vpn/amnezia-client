#ifndef SERVERCONTROLLER_H
#define SERVERCONTROLLER_H

#include <QObject>
#include "sshconnection.h"

class ServerController : public QObject
{
    Q_OBJECT
public:
    enum ServerType {
        OpenVPN,
        ShadowSocks,
        WireGuard
    };

    static bool removeServer(const QSsh::SshConnectionParameters &sshParams, ServerType sType);
    static bool setupServer(const QSsh::SshConnectionParameters &sshParams, ServerType sType);

    static QSsh::SshConnection *connectToHost(const QSsh::SshConnectionParameters &sshParams);
    static bool runScript(const QSsh::SshConnectionParameters &sshParams, QString script);

    static void uploadTextFileToContainer(const QSsh::SshConnectionParameters &sshParams, QString &file, const QString &path);
    static QString getTextFileFromContainer(const QSsh::SshConnectionParameters &sshParams, const QString &path);

    static bool signCert(const QSsh::SshConnectionParameters &sshParams, QString clientId);

signals:

};

#endif // SERVERCONTROLLER_H
