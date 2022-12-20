#include "sshsession.h"

#include <QEventLoop>
#include <QtConcurrent>

SshSession::SshSession(QObject *parent) : QObject(parent)
{

}

SshSession::~SshSession()
{
    if (m_isNeedSendChannelEof) {
        ssh_channel_send_eof(m_channel);
    }
    if (m_isChannelOpened) {
        ssh_channel_free(m_channel);
    }
    if (m_isSessionConnected) {
        ssh_disconnect(m_session);
    }
    ssh_free(m_session);
}

ErrorCode SshSession::connectToHost(const ServerCredentials &credentials)
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

ErrorCode SshSession::initChannel(const ServerCredentials &credentials)
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

ErrorCode SshSession::writeToChannel(const QString &data)
{
    QFutureWatcher<ErrorCode> watcher;
    connect(&watcher, &QFutureWatcher<ErrorCode>::finished, this, &SshSession::writeToChannelFinished);

    QFuture<ErrorCode> future = QtConcurrent::run([this, &data]() {
        return write(data);
    });
    watcher.setFuture(future);

    QEventLoop wait;

    QObject::connect(this, &SshSession::writeToChannelFinished, &wait, &QEventLoop::quit);
    wait.exec();

    return watcher.result();
}

ErrorCode SshSession::write(const QString &data)
{
    const int channelReadTimeoutMs = 10;
    const size_t bufferSize = 2048;

    int bytesToRead = 0;
    int attempts = 0;
    char buffer[bufferSize];

    int bytesWritten = ssh_channel_write(m_channel, data.toUtf8(), (uint32_t)data.size());
    if (bytesWritten == data.size()) {
        while (bytesToRead != 0 || attempts < 100){
            if (ssh_channel_is_open(m_channel) && !ssh_channel_is_eof(m_channel)) {
                bytesToRead = ssh_channel_read_timeout(m_channel, buffer, sizeof(buffer), 0, channelReadTimeoutMs);
                if (bytesToRead > 0) {
                    attempts = 0;
                    std::string strbuf(buffer, bytesToRead);
//                    QByteArray qbuff(buffer, bytesToRead);
//                    QString outp(buffer);

//                    if (cbReadStdOut){
//                        cbReadStdOut(outp, nullptr);
//                    }
                    qDebug().noquote() << QString(strbuf.c_str());
                } else {
                    attempts++;
                }
            }
        }
    } else {
        qDebug() << ssh_get_error(m_session);
        return ErrorCode::SshInternalError;
    }
    return ErrorCode::NoError;
}
