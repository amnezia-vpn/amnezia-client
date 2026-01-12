#include "sshclient.h"

#include <QEventLoop>
#include <QtConcurrent>

#include <fstream>

#ifdef Q_OS_WINDOWS
const uint32_t S_IRWXU = 0644;
#endif

namespace libssh {
    constexpr auto libsshTimeoutError{"Timeout connecting to"};

    std::function<QString()> Client::m_passphraseCallback;

    int Client::callback(const char *prompt, char *buf, size_t len, int echo, int verify, void *userdata)
    {
        auto passphrase = m_passphraseCallback();
        passphrase.toStdString().copy(buf, passphrase.size() + 1);
        return 0;
    }

    ErrorCode Client::connectToHost(const ServerCredentials &credentials)
    {
        if (m_session != nullptr) {
            if (!ssh_is_connected(m_session)) {
                ssh_free(m_session);
                m_session = nullptr;
            }
        }

        if (m_session == nullptr) {
            m_session = ssh_new();

            if (m_session == nullptr) {
                qDebug() << "Failed to create ssh session";
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

            QFutureWatcher<int> watcher;
            QFuture<int> future = QtConcurrent::run([this]() {
                return ssh_connect(m_session);
            });

            QEventLoop wait;
            connect(&watcher, &QFutureWatcher<ErrorCode>::finished, &wait, &QEventLoop::quit);
            watcher.setFuture(future);
            wait.exec();

            int connectionResult = watcher.result();

            if (connectionResult != SSH_OK) {
                return fromLibsshErrorCode();
            }

            std::string authUsername = credentials.userName.toStdString();

            int authResult = SSH_ERROR;
            if (credentials.secretData.contains("BEGIN") && credentials.secretData.contains("PRIVATE KEY")) {
                ssh_key privateKey = nullptr;
                ssh_key publicKey = nullptr;
                authResult = ssh_pki_import_privkey_base64(credentials.secretData.toStdString().c_str(), nullptr, callback, nullptr, &privateKey);
                if (authResult == SSH_OK) {
                    authResult = ssh_pki_export_privkey_to_pubkey(privateKey, &publicKey);
                }

                if (authResult == SSH_OK) {
                    authResult = ssh_userauth_try_publickey(m_session, authUsername.c_str(), publicKey);
                }

                if (authResult == SSH_OK) {
                    authResult = ssh_userauth_publickey(m_session, authUsername.c_str(), privateKey);
                }

                if (publicKey) {
                    ssh_key_free(publicKey);
                }
                if (privateKey) {
                    ssh_key_free(privateKey);
                }
                if (authResult != SSH_OK) {
                    qCritical() << ssh_get_error(m_session);
                    ErrorCode errorCode = fromLibsshErrorCode();
                    if (errorCode == ErrorCode::NoError) {
                        errorCode = ErrorCode::SshPrivateKeyFormatError;
                    }
                    return errorCode;
                }
            } else {
                authResult = ssh_userauth_password(m_session, authUsername.c_str(), credentials.secretData.toStdString().c_str());
                if (authResult != SSH_OK) {
                    return fromLibsshErrorCode();
                }
            }
        }
        return ErrorCode::NoError;
    }

    void Client::disconnectFromHost()
    {
        if (m_session != nullptr) {
            if (ssh_is_connected(m_session)) {
                ssh_disconnect(m_session);
            }
            ssh_free(m_session);
            m_session = nullptr;
        }
    }

    ErrorCode Client::executeCommand(const QString &data,
                                        const std::function<ErrorCode (const QString &, Client &)> &cbReadStdOut,
                                        const std::function<ErrorCode (const QString &, Client &)> &cbReadStdErr)
    {
        m_channel = ssh_channel_new(m_session);

        if (m_channel == nullptr) {
            return closeChannel();
        }

        int result = ssh_channel_open_session(m_channel);

        if (result == SSH_OK && ssh_channel_is_open(m_channel)) {
            qDebug() << "SSH chanel opened";
        } else {
            return closeChannel();
        }

        QFutureWatcher<ErrorCode> watcher;
        connect(&watcher, &QFutureWatcher<ErrorCode>::finished, this, &Client::writeToChannelFinished);

        QFuture<ErrorCode> future = QtConcurrent::run([this, &data, &cbReadStdOut, &cbReadStdErr]() {
            const size_t bufferSize = 2048;

            int bytesRead = 0;
            char buffer[bufferSize];

            int result = ssh_channel_request_exec(m_channel, data.toUtf8());
            if (result == SSH_OK) {
                std::string output;
                auto readOutput = [&](bool isStdErr) {
                    bytesRead = ssh_channel_read(m_channel, buffer, sizeof(buffer), isStdErr);
                    while (bytesRead > 0)
                    {
                        output = std::string(buffer, bytesRead);
                        if (!output.empty()) {
                            if (cbReadStdOut && !isStdErr){
                                auto error = cbReadStdOut(output.c_str(), *this);
                                if (error != ErrorCode::NoError) {
                                    return error;
                                }
                            }
                            if (cbReadStdErr && isStdErr){
                                auto error = cbReadStdErr(output.c_str(), *this);
                                if (error != ErrorCode::NoError) {
                                    return error;
                                }
                            }
                        }
                        bytesRead = ssh_channel_read(m_channel, buffer, sizeof(buffer), isStdErr);
                    }
                    return ErrorCode::NoError;
                };

                auto errorCode = readOutput(false);
                if (errorCode != ErrorCode::NoError) {
                    return errorCode;
                }
                errorCode = readOutput(true);
                if (errorCode != ErrorCode::NoError) {
                    return errorCode;
                }
            } else {
                return closeChannel();
            }
            return closeChannel();
        });
        watcher.setFuture(future);

        QEventLoop wait;
        QObject::connect(this, &Client::writeToChannelFinished, &wait, &QEventLoop::quit);
        wait.exec();

        return watcher.result();
    }

    ErrorCode Client::writeResponse(const QString &data)
    {
        if (m_channel == nullptr) {
            qCritical() << "ssh channel not initialized";
            return fromLibsshErrorCode();
        }

        int bytesWritten = ssh_channel_write(m_channel, data.toUtf8(), (uint32_t)data.size());
        if (bytesWritten == data.size() && ssh_channel_write(m_channel, "\n", 1)) {
            return fromLibsshErrorCode();
        }
        return fromLibsshErrorCode();
    }

    ErrorCode Client::closeChannel()
    {
        if (m_channel != nullptr) {
            if (ssh_channel_is_eof(m_channel)) {
                ssh_channel_send_eof(m_channel);
            }
            if (ssh_channel_is_open(m_channel)) {
                ssh_channel_close(m_channel);
            }
            ssh_channel_free(m_channel);
            m_channel = nullptr;
        }
        return fromLibsshErrorCode();
    }

    ErrorCode Client::scpFileCopy(const ScpOverwriteMode overwriteMode, const QString& localPath, const QString& remotePath, const QString &fileDesc)
    {
        m_scpSession = ssh_scp_new(m_session, SSH_SCP_WRITE, remotePath.toStdString().c_str());

        if (m_scpSession == nullptr) {
            return fromLibsshErrorCode();
        }

        if (ssh_scp_init(m_scpSession) != SSH_OK) {
            auto errorCode = fromLibsshErrorCode();
            closeScpSession();
            return errorCode;
        }

        QFutureWatcher<ErrorCode> watcher;
        connect(&watcher, &QFutureWatcher<ErrorCode>::finished, this, &Client::scpFileCopyFinished);
        QFuture<ErrorCode> future = QtConcurrent::run([this, overwriteMode, &localPath, &remotePath, &fileDesc]() {
            const int accessType = O_WRONLY | O_CREAT | overwriteMode;
            const int localFileSize = QFileInfo(localPath).size();

            int result = ssh_scp_push_file(m_scpSession, remotePath.toStdString().c_str(), localFileSize, accessType);
            if (result != SSH_OK) {
                return fromLibsshErrorCode();
            }

            QFile fin(localPath);

            if (fin.open(QIODevice::ReadOnly)) {
                constexpr size_t bufferSize = 16384;
                int transferred = 0;
                int currentChunkSize = bufferSize;

                while (transferred < localFileSize) {

                    // Last Chunk
                    if ((localFileSize - transferred) < bufferSize) {
                        currentChunkSize = localFileSize % bufferSize;
                    }

                    QByteArray chunk = fin.read(currentChunkSize);
                    if (chunk.size() != currentChunkSize) {
                        return fromFileErrorCode(fin.error());
                    }

                    result = ssh_scp_write(m_scpSession, chunk.data(), chunk.size());
                    if (result != SSH_OK) {
                        return fromLibsshErrorCode();
                    }

                    transferred += currentChunkSize;
                }
            } else {
                return fromFileErrorCode(fin.error());
            }

            return ErrorCode::NoError;
        });
        watcher.setFuture(future);

        QEventLoop wait;
        QObject::connect(this, &Client::scpFileCopyFinished, &wait, &QEventLoop::quit);
        wait.exec();

        closeScpSession();
        return watcher.result();
    }

    void Client::closeScpSession()
    {
        if (m_scpSession != nullptr) {
            ssh_scp_free(m_scpSession);
            m_scpSession = nullptr;
        }
    }

    ErrorCode Client::fromLibsshErrorCode()
    {
        int errorCode = ssh_get_error_code(m_session);
        if (errorCode != SSH_NO_ERROR) {
            QString errorMessage = ssh_get_error(m_session);
            qCritical() << errorMessage;
            if (errorMessage.contains(libsshTimeoutError)) {
                return ErrorCode::SshTimeoutError;
            }
        }

        switch (errorCode) {
        case(SSH_NO_ERROR): return ErrorCode::NoError;
        case(SSH_REQUEST_DENIED): return ErrorCode::SshRequestDeniedError;
        case(SSH_EINTR): return ErrorCode::SshInterruptedError;
        case(SSH_FATAL): return ErrorCode::SshInternalError;
        default: return ErrorCode::SshInternalError;
        }
    }

    ErrorCode Client::fromFileErrorCode(QFileDevice::FileError fileError)
    {
        switch (fileError) {
        case QFileDevice::NoError: return ErrorCode::NoError;
        case QFileDevice::ReadError: return ErrorCode::ReadError;
        case QFileDevice::OpenError: return ErrorCode::OpenError;
        case QFileDevice::PermissionsError: return ErrorCode::PermissionsError;
        case QFileDevice::FatalError: return ErrorCode::FatalError;
        case QFileDevice::AbortError: return ErrorCode::AbortError;
        default: return ErrorCode::UnspecifiedError;
        }
    }

    ErrorCode Client::getDecryptedPrivateKey(const ServerCredentials &credentials, QString &decryptedPrivateKey, const std::function<QString()> &passphraseCallback)
    {
        int authResult = SSH_ERROR;
        ErrorCode errorCode = ErrorCode::NoError;

        ssh_key privateKey = nullptr;
        m_passphraseCallback = passphraseCallback;
        authResult = ssh_pki_import_privkey_base64(credentials.secretData.toStdString().c_str(), nullptr, callback, nullptr, &privateKey);
        if (authResult == SSH_OK) {
            char *b64 = nullptr;

            authResult = ssh_pki_export_privkey_base64(privateKey, nullptr, nullptr, nullptr, &b64);
            decryptedPrivateKey = QString(b64);

            if (authResult != SSH_OK) {
                qDebug() << "failed to export private key";
                errorCode = ErrorCode::InternalError;
            }
            else {
                ssh_string_free_char(b64);
            }
        } else {
            errorCode = ErrorCode::SshPrivateKeyError;
        }

        if (privateKey) {
            ssh_key_free(privateKey);
        }
        return errorCode;
    }
}
