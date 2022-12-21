#ifndef SSHSESSION_H
#define SSHSESSION_H

#include <QObject>

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include "defs.h"

using namespace amnezia;

class SshSession : public QObject
{
    Q_OBJECT
public:
    SshSession(QObject *parent = nullptr);
    ~SshSession();

    ErrorCode initChannel(const ServerCredentials &credentials);
    ErrorCode writeToChannel(const QString &data,
                             const std::function<void(const QString &)> &cbReadStdOut,
                             const std::function<void(const QString &)> &cbReadStdErr);
private:
    ErrorCode connectToHost(const ServerCredentials &credentials);

    ssh_session m_session;
    ssh_channel m_channel;

    bool m_isChannelOpened = false;
    bool m_isSessionConnected = false;
    bool m_isNeedSendChannelEof = false;
signals:
    void writeToChannelFinished();
};

#endif // SSHSESSION_H
