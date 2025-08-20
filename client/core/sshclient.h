#ifndef SSHCLIENT_H
#define SSHCLIENT_H

#include <QObject>
#include <QFile>

#include <fcntl.h>

#ifndef IOS_SIM
#include <libssh/libssh.h>
#endif

#include "defs.h"

using namespace amnezia;

namespace libssh {
    enum ScpOverwriteMode {
        ScpOverwriteExisting = O_TRUNC,
        ScpAppendToExisting = O_APPEND
    };
    class Client : public QObject
    {
        Q_OBJECT
    public:
        Client() = default;
        ~Client() = default;

        ErrorCode connectToHost(const ServerCredentials &credentials);
        void disconnectFromHost();
        ErrorCode executeCommand(const QString &data,
                                 const std::function<ErrorCode (const QString &, Client &)> &cbReadStdOut,
                                 const std::function<ErrorCode (const QString &, Client &)> &cbReadStdErr);
        ErrorCode writeResponse(const QString &data);
        ErrorCode scpFileCopy(const ScpOverwriteMode overwriteMode,
                               const QString &localPath,
                               const QString &remotePath,
                               const QString &fileDesc);
        ErrorCode getDecryptedPrivateKey(const ServerCredentials &credentials, QString &decryptedPrivateKey, const std::function<QString()> &passphraseCallback);
    private:
        ErrorCode closeChannel();
        void closeScpSession();
        ErrorCode fromLibsshErrorCode();
        ErrorCode fromFileErrorCode(QFileDevice::FileError fileError);
        static int callback(const char *prompt, char *buf, size_t len, int echo, int verify, void *userdata);

#ifndef IOS_SIM
        ssh_session m_session = nullptr;
        ssh_channel m_channel = nullptr;
        ssh_scp m_scpSession = nullptr;
#else
        void* m_session = nullptr;
        void* m_channel = nullptr;
        void* m_scpSession = nullptr;
#endif

        static std::function<QString()> m_passphraseCallback;
    signals:
        void writeToChannelFinished();
        void scpFileCopyFinished();
    };
}

#endif // SSHCLIENT_H
