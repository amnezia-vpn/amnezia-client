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
            return ErrorCode::SshTimeoutError;
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
            return ErrorCode::SshAuthenticationError;
        }

        return ErrorCode::NoError;
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
            return ErrorCode::SshAuthenticationError;
        }

        int result = ssh_channel_open_session(m_channel);

        if (result == SSH_OK && ssh_channel_is_open(m_channel)) {
            qDebug() << "SSH chanel opened";
            m_isChannelOpened = true;
        } else {
            qDebug() << ssh_get_error(m_session);
            return ErrorCode::SshAuthenticationError;
        }

        result = ssh_channel_request_pty(m_channel);
        if (result != SSH_OK) {
            qDebug() << ssh_get_error(m_session);
            return ErrorCode::SshInternalError;
        }

        result = ssh_channel_change_pty_size(m_channel, 80, 1024);
        if (result != SSH_OK) {
            qDebug() << ssh_get_error(m_session);
            return ErrorCode::SshInternalError;
        }

        result = ssh_channel_request_shell(m_channel);
        if (result != SSH_OK) {
            qDebug() << ssh_get_error(m_session);
            return ErrorCode::SshInternalError;
        }

        return ErrorCode::NoError;
    }

    ErrorCode Session::writeToChannel(const QString &data,
                                        const std::function<void(const QString &)> &cbReadStdOut,
                                        const std::function<void(const QString &)> &cbReadStdErr)
    {
        if (m_channel == NULL) {
            qDebug() << "ssh channel not initialized";
            return ErrorCode::SshAuthenticationError;
        }

        QFutureWatcher<ErrorCode> watcher;
        connect(&watcher, &QFutureWatcher<ErrorCode>::finished, this, &Session::writeToChannelFinished);

        QFuture<ErrorCode> future = QtConcurrent::run([this, &data, &cbReadStdOut, &cbReadStdErr]() {
            const size_t bufferSize = 2048;

            int bytesRead = 0;
            char buffer[bufferSize];

            int bytesWritten = ssh_channel_write(m_channel, data.toUtf8(), (uint32_t)data.size());
            ssh_channel_write(m_channel, "\n", 1);
            if (bytesWritten == data.size()) {
                auto readOutput = [&](bool isStdErr) {
                    std::string output;
                    if (ssh_channel_is_open(m_channel) && !ssh_channel_is_eof(m_channel)) {
                        bytesRead = ssh_channel_read_timeout(m_channel, buffer, sizeof(buffer), isStdErr, 50);
                        while (bytesRead > 0)
                        {
                            output = std::string(buffer, bytesRead);
                            if (!output.empty()) {
                                qDebug().noquote() << (isStdErr ? "stdErr" : "stdOut") << QString(output.c_str());

                                if (cbReadStdOut && !isStdErr){
                                    cbReadStdOut(output.c_str());
                                }
                                if (cbReadStdErr && isStdErr){
                                    cbReadStdErr(output.c_str());
                                }
                            }
                            bytesRead = ssh_channel_read_timeout(m_channel, buffer, sizeof(buffer), isStdErr, 5000);
                        }
                    }
                    return output;
                };

                readOutput(false);
                readOutput(true);
            } else {
                qDebug() << ssh_get_error(m_session);
                return ErrorCode::SshInternalError;
            }
            m_isNeedSendChannelEof = true;
            return ErrorCode::NoError;
        });
        watcher.setFuture(future);

        QEventLoop wait;

        QObject::connect(this, &Session::writeToChannelFinished, &wait, &QEventLoop::quit);
        wait.exec();

        return watcher.result();
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
            return ErrorCode::SshSftpError;
        }

        int result = sftp_init(m_sftpSession);

        if (result != SSH_OK) {
            qDebug() << ssh_get_error(m_session);
            return ErrorCode::SshSftpError;
        }

        return ErrorCode::NoError;
    }

    ErrorCode Session::sftpFileCopy(const SftpOverwriteMode overwriteMode, const std::string& localPath, const std::string& remotePath, const std::string& fileDesc)
    {
        if (m_sftpSession == NULL) {
            qDebug() << "ssh sftp session not initialized";
            return ErrorCode::SshSftpError;
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
                return ErrorCode::SshSftpError;
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
                        return ErrorCode::SshSftpError;
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
                        return ErrorCode::SshSftpError;
                    }
                }
            } else {
                sftp_close(file);
                qDebug() << ssh_get_error(m_session);
                return ErrorCode::SshSftpError;
            }

            fin.close();

            int result = sftp_close(file);
            if (result != SSH_OK) {
                qDebug() << ssh_get_error(m_session);
                return ErrorCode::SshSftpError;
            }

            return ErrorCode::NoError;
        });
        watcher.setFuture(future);

        QEventLoop wait;

        QObject::connect(this, &Session::sftpFileCopyFinished, &wait, &QEventLoop::quit);
        wait.exec();

        return watcher.result();
    }
}
