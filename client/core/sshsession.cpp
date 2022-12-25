#include "sshsession.h"

#include <QEventLoop>
#include <QtConcurrent>

#include <fstream>

#ifdef Q_OS_WINDOWS
#define S_IRWXU 0
#endif

namespace libssh {
    Session::Session(QObject *parent) : QObject(parent)
    {

    }

    Session::~Session()
    {
        if (m_isNeedSendChannelEof) {
            ssh_channel_send_eof(m_channel);
        }
        if (m_isChannelOpened) {
            ssh_channel_close(m_channel);
        }
        if (m_isChannelCreated) {
            ssh_channel_free(m_channel);
        }
        if (m_isSftpInitialized) {
            sftp_free(m_sftpSession);
        }
        if (m_isSessionConnected) {
            ssh_disconnect(m_session);
        }
        ssh_free(m_session);
    }

    ErrorCode Session::connectToHost(const ServerCredentials &credentials)
    {
        if (m_session == NULL) {
            return ErrorCode::InternalError;
        }

        int port = credentials.port;
        int logVerbosity = SSH_LOG_NOLOG;
        std::string hostIp = credentials.hostName.toStdString();
        std::string hostUsername = credentials.userName.toStdString() + "@" + hostIp;

        ssh_options_set(m_session, SSH_OPTIONS_HOST, hostIp.c_str());
        ssh_options_set(m_session, SSH_OPTIONS_PORT, &port);
        ssh_options_set(m_session, SSH_OPTIONS_USER, hostUsername.c_str());
        ssh_options_set(m_session, SSH_OPTIONS_LOG_VERBOSITY, &logVerbosity);

        int connectionResult = ssh_connect(m_session);

        if (connectionResult != SSH_OK) {
            qDebug() << ssh_get_error(m_session);
            return fromLibsshErrorCode(ssh_get_error_code(m_session));
        }

        m_isSessionConnected = true;

        std::string authUsername = credentials.userName.toStdString();

        int authResult = SSH_ERROR;
        if (credentials.password.contains("BEGIN") && credentials.password.contains("PRIVATE KEY")) {
            ssh_key privateKey;
            ssh_pki_import_privkey_base64(credentials.password.toStdString().c_str(), nullptr, nullptr, nullptr, &privateKey);
            authResult = ssh_userauth_publickey(m_session, authUsername.c_str(), privateKey);
        }
        else {
            authResult = ssh_userauth_password(m_session, authUsername.c_str(), credentials.password.toStdString().c_str());
        }

        if (authResult != SSH_OK) {
            qDebug() << ssh_get_error(m_session);
            return fromLibsshErrorCode(ssh_get_error_code(m_session));
        }

        return fromLibsshErrorCode(ssh_get_error_code(m_session));
    }

    ErrorCode Session::initChannel(const ServerCredentials &credentials)
    {
        m_session = ssh_new();

        ErrorCode error = connectToHost(credentials);
        if (error) {
            return error;
        }
        m_channel = ssh_channel_new(m_session);

        if (m_channel == NULL) {
            qDebug() << ssh_get_error(m_session);
            return fromLibsshErrorCode(ssh_get_error_code(m_session));
        }

        m_isChannelCreated = true;
        int result = ssh_channel_open_session(m_channel);

        if (result == SSH_OK && ssh_channel_is_open(m_channel)) {
            qDebug() << "SSH chanel opened";
            m_isChannelOpened = true;
        } else {
            qDebug() << ssh_get_error(m_session);
            return fromLibsshErrorCode(ssh_get_error_code(m_session));
        }

        result = ssh_channel_request_pty(m_channel);
        if (result != SSH_OK) {
            qDebug() << ssh_get_error(m_session);
            return fromLibsshErrorCode(ssh_get_error_code(m_session));
        }

        result = ssh_channel_change_pty_size(m_channel, 80, 1024);
        if (result != SSH_OK) {
            qDebug() << ssh_get_error(m_session);
            return fromLibsshErrorCode(ssh_get_error_code(m_session));
        }

        result = ssh_channel_request_shell(m_channel);
        if (result != SSH_OK) {
            qDebug() << ssh_get_error(m_session);
            return fromLibsshErrorCode(ssh_get_error_code(m_session));
        }

        return fromLibsshErrorCode(ssh_get_error_code(m_session));
    }

    ErrorCode Session::writeToChannel(const QString &data,
                                        const std::function<void(const QString &, Session &)> &cbReadStdOut,
                                        const std::function<void(const QString &, Session &)> &cbReadStdErr)
    {
        if (m_channel == NULL) {
            qDebug() << "ssh channel not initialized";
            return fromLibsshErrorCode(ssh_get_error_code(m_session));
        }

        QFutureWatcher<ErrorCode> watcher;
        connect(&watcher, &QFutureWatcher<ErrorCode>::finished, this, &Session::writeToChannelFinished);

        QFuture<ErrorCode> future = QtConcurrent::run([this, &data, &cbReadStdOut, &cbReadStdErr]() {
            const size_t bufferSize = 2048;

            int bytesRead = 0;
            char buffer[bufferSize];

            int bytesWritten = ssh_channel_write(m_channel, data.toUtf8(), (uint32_t)data.size());
            if (bytesWritten == data.size() && ssh_channel_write(m_channel, "\n", 1)) {
                auto readOutput = [&](bool isStdErr) {
                    std::string output;
                    if (ssh_channel_is_open(m_channel) && !ssh_channel_is_eof(m_channel)) {
                        bytesRead = ssh_channel_read_timeout(m_channel, buffer, sizeof(buffer), isStdErr, 200);
                        while (bytesRead > 0)
                        {
                            output = std::string(buffer, bytesRead);
                            if (!output.empty()) {
                                qDebug().noquote() << (isStdErr ? "stdErr" : "stdOut") << QString(output.c_str());

                                if (cbReadStdOut && !isStdErr){
                                    cbReadStdOut(output.c_str(), *this);
                                }
                                if (cbReadStdErr && isStdErr){
                                    cbReadStdErr(output.c_str(), *this);
                                }
                            }
                            bytesRead = ssh_channel_read_timeout(m_channel, buffer, sizeof(buffer), isStdErr, 2000);
                        }
                    }
                    return output;
                };

                readOutput(false);
                readOutput(true);
            } else {
                qDebug() << ssh_get_error(m_session);
                return fromLibsshErrorCode(ssh_get_error_code(m_session));
            }
            m_isNeedSendChannelEof = true;
            return fromLibsshErrorCode(ssh_get_error_code(m_session));
        });
        watcher.setFuture(future);

        QEventLoop wait;
        QObject::connect(this, &Session::writeToChannelFinished, &wait, &QEventLoop::quit);
        wait.exec();

        return watcher.result();
    }

    ErrorCode Session::writeToChannel(const QString &data)
    {
        if (m_channel == NULL) {
            qDebug() << "ssh channel not initialized";
            return fromLibsshErrorCode(ssh_get_error_code(m_session));
        }

        int bytesWritten = ssh_channel_write(m_channel, data.toUtf8(), (uint32_t)data.size());
        if (bytesWritten == data.size() && ssh_channel_write(m_channel, "\n", 1)) {
            return fromLibsshErrorCode(ssh_get_error_code(m_session));
        }
        qDebug() << ssh_get_error(m_session);
        return fromLibsshErrorCode(ssh_get_error_code(m_session));
    }

    ErrorCode Session::initSftp(const ServerCredentials &credentials)
    {
        m_session = ssh_new();

        ErrorCode error = connectToHost(credentials);
        if (error) {
            return error;
        }

        m_sftpSession = sftp_new(m_session);

        if (m_sftpSession == NULL) {
            qDebug() << ssh_get_error(m_session);
            return fromLibsshErrorCode(ssh_get_error_code(m_session));
        }

        int result = sftp_init(m_sftpSession);

        if (result != SSH_OK) {
            qDebug() << ssh_get_error(m_session);
            return fromLibsshSftpErrorCode(sftp_get_error(m_sftpSession));
        }

        return ErrorCode::NoError;
    }

    ErrorCode Session::sftpFileCopy(const SftpOverwriteMode overwriteMode, const std::string& localPath, const std::string& remotePath, const std::string& fileDesc)
    {
        if (m_sftpSession == NULL) {
            qDebug() << "ssh sftp session not initialized";
            return ErrorCode::SshInternalError;
        }

        QFutureWatcher<ErrorCode> watcher;
        connect(&watcher, &QFutureWatcher<ErrorCode>::finished, this, &Session::sftpFileCopyFinished);

        QFuture<ErrorCode> future = QtConcurrent::run([this, overwriteMode, &localPath, &remotePath, &fileDesc]() {
            int accessType = O_WRONLY | O_CREAT | overwriteMode;
            sftp_file file;
            const size_t bufferSize = 16384;
            char buffer[bufferSize];

            file = sftp_open(m_sftpSession, remotePath.c_str(), accessType, S_IRWXU);

            if (file == NULL) {
                qDebug() << ssh_get_error(m_session);
                return fromLibsshSftpErrorCode(sftp_get_error(m_sftpSession));
            }

            int localFileSize = std::filesystem::file_size(localPath);
            int chunksCount = localFileSize / (bufferSize);

            std::ifstream fin(localPath, std::ios::binary | std::ios::in);

            if (fin.is_open()) {
                for (int currentChunkId = 0; currentChunkId < chunksCount; currentChunkId++) {
                    fin.read(buffer, bufferSize);

                    int bytesWritten = sftp_write(file, buffer, bufferSize);

                    std::string chunk(buffer, bufferSize);
                    qDebug() << "sftp write: " << QString(chunk.c_str());

                    if (bytesWritten != bufferSize) {
                        fin.close();
                        sftp_close(file);
                        qDebug() << ssh_get_error(m_session);
                        return fromLibsshSftpErrorCode(sftp_get_error(m_sftpSession));
                    }
                }

                int lastChunkSize = localFileSize % (bufferSize);

                if (lastChunkSize != 0) {
                    fin.read(buffer, lastChunkSize);

                    std::string chunk(buffer, lastChunkSize);
                    qDebug() << "sftp write: " << QString(chunk.c_str());

                    int bytesWritten = sftp_write(file, buffer, lastChunkSize);

                    if (bytesWritten != lastChunkSize) {
                        fin.close();
                        sftp_close(file);
                        qDebug() << ssh_get_error(m_session);
                        return fromLibsshSftpErrorCode(sftp_get_error(m_sftpSession));
                    }
                }
            } else {
                sftp_close(file);
                qDebug() << ssh_get_error(m_session);
                return fromLibsshSftpErrorCode(sftp_get_error(m_sftpSession));
            }

            fin.close();

            int result = sftp_close(file);
            if (result != SSH_OK) {
                qDebug() << ssh_get_error(m_session);
                return fromLibsshSftpErrorCode(sftp_get_error(m_sftpSession));
            }

            return ErrorCode::NoError;
        });
        watcher.setFuture(future);

        QEventLoop wait;
        QObject::connect(this, &Session::sftpFileCopyFinished, &wait, &QEventLoop::quit);
        wait.exec();

        return watcher.result();
    }

    ErrorCode Session::fromLibsshErrorCode(int errorCode)
    {
        switch (errorCode) {
        case(SSH_NO_ERROR): return ErrorCode::NoError;
        case(SSH_REQUEST_DENIED): return ErrorCode::SshRequsetDeniedError;
        case(SSH_EINTR): return ErrorCode::SshInterruptedError;
        case(SSH_FATAL): return ErrorCode::SshInternalError;
        default: return ErrorCode::SshInternalError;
        }
    }
    ErrorCode Session::fromLibsshSftpErrorCode(int errorCode)
    {
        switch (errorCode) {
        case(SSH_FX_OK): return ErrorCode::NoError;
        case(SSH_FX_EOF): return ErrorCode::SshSftpEofError;
        case(SSH_FX_NO_SUCH_FILE): return ErrorCode::SshSftpNoSuchFileError;
        case(SSH_FX_PERMISSION_DENIED): return ErrorCode::SshSftpPermissionDeniedError;
        case(SSH_FX_FAILURE): return ErrorCode::SshSftpFailureError;
        case(SSH_FX_BAD_MESSAGE): return ErrorCode::SshSftpBadMessageError;
        case(SSH_FX_NO_CONNECTION): return ErrorCode::SshSftpNoConnectionError;
        case(SSH_FX_CONNECTION_LOST): return ErrorCode::SshSftpConnectionLostError;
        case(SSH_FX_OP_UNSUPPORTED): return ErrorCode::SshSftpOpUnsupportedError;
        case(SSH_FX_INVALID_HANDLE): return ErrorCode::SshSftpInvalidHandleError;
        case(SSH_FX_NO_SUCH_PATH): return ErrorCode::SshSftpNoSuchPathError;
        case(SSH_FX_FILE_ALREADY_EXISTS): return ErrorCode::SshSftpFileAlreadyExistsError;
        case(SSH_FX_WRITE_PROTECT): return ErrorCode::SshSftpWriteProtectError;
        case(SSH_FX_NO_MEDIA): return ErrorCode::SshSftpNoMediaError;
        default: return ErrorCode::SshSftpFailureError;
        }
    }
}
