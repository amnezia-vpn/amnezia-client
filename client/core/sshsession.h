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
    ErrorCode initSftp(const ServerCredentials &credentials);
    ErrorCode writeToChannel(const QString &data,
                             const std::function<void(const QString &)> &cbReadStdOut,
                             const std::function<void(const QString &)> &cbReadStdErr);
    ErrorCode sftpFileCopy(const std::string& localPath, const std::string& remotePath, const std::string& fileDesc);
private:
    ErrorCode connectToHost(const ServerCredentials &credentials);

    ssh_session m_session;
    ssh_channel m_channel;
    sftp_session m_sftpSession;

    bool m_isChannelOpened = false;
    bool m_isSessionConnected = false;
    bool m_isNeedSendChannelEof = false;
    bool m_isSftpInitialized = false;
signals:
    void writeToChannelFinished();
    void sftpFileCopyFinished();
};

#endif // SSHSESSION_H
