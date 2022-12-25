#ifndef SSHSESSION_H
#define SSHSESSION_H

#include <QObject>

#include <fcntl.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include "defs.h"

using namespace amnezia;

namespace libssh {
    enum SftpOverwriteMode {
        /*! Overwrite any existing files */
        SftpOverwriteExisting = O_TRUNC,
        /*! Append new content if the file already exists */
        SftpAppendToExisting = O_APPEND
    };

    class Session : public QObject
    {
        Q_OBJECT
    public:
        Session(QObject *parent = nullptr);
        ~Session();

        ErrorCode initChannel(const ServerCredentials &credentials);
        ErrorCode initSftp(const ServerCredentials &credentials);
        ErrorCode writeToChannel(const QString &data,
                                 const std::function<void(const QString &, Session &)> &cbReadStdOut,
                                 const std::function<void(const QString &, Session &)> &cbReadStdErr);
        ErrorCode writeToChannel(const QString &data);
        ErrorCode sftpFileCopy(const SftpOverwriteMode overwriteMode, const std::string& localPath, const std::string& remotePath, const std::string& fileDesc);
    private:
        ErrorCode connectToHost(const ServerCredentials &credentials);
        ErrorCode fromLibsshErrorCode(int errorCode);
        ErrorCode fromLibsshSftpErrorCode(int errorCode);

        ssh_session m_session;
        ssh_channel m_channel;
        sftp_session m_sftpSession;

        bool m_isChannelCreated = false;
        bool m_isChannelOpened = false;
        bool m_isSessionConnected = false;
        bool m_isNeedSendChannelEof = false;
        bool m_isSftpInitialized = false;
    signals:
        void writeToChannelFinished();
        void sftpFileCopyFinished();
    };
}

#endif // SSHSESSION_H
