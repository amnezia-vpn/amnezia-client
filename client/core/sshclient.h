#ifndef SSHCLIENT_H
#define SSHCLIENT_H

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
    class Client : public QObject
    {
        Q_OBJECT
    public:
        Client(QObject *parent = nullptr);
        ~Client();

        ErrorCode connectToHost(const ServerCredentials &credentials);
        void disconnectFromHost();
        ErrorCode executeCommand(const QString &data,
                                 const std::function<ErrorCode (const QString &, Client &)> &cbReadStdOut,
                                 const std::function<ErrorCode (const QString &, Client &)> &cbReadStdErr);
        ErrorCode writeResponse(const QString &data);
        ErrorCode sftpFileCopy(const SftpOverwriteMode overwriteMode,
                               const QString &localPath,
                               const QString &remotePath,
                               const QString& fileDesc);
        ErrorCode getDecryptedPrivateKey(const ServerCredentials &credentials, QString &decryptedPrivateKey, const std::function<QString()> &passphraseCallback);
    private:
        ErrorCode closeChannel();
        ErrorCode closeSftpSession();
        ErrorCode fromLibsshErrorCode();
        ErrorCode fromLibsshSftpErrorCode(int errorCode);
        static int callback(const char *prompt, char *buf, size_t len, int echo, int verify, void *userdata);

        ssh_session m_session = nullptr;
        ssh_channel m_channel = nullptr;
        sftp_session m_sftpSession = nullptr;

        static std::function<QString()> m_passphraseCallback;
    signals:
        void writeToChannelFinished();
        void sftpFileCopyFinished();
    };
}

#endif // SSHCLIENT_H
