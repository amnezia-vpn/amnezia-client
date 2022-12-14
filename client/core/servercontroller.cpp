#include "servercontroller.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QPointer>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QApplication>
#include <QTemporaryFile>
#include <QFileInfo>
#include <filesystem>
#include <iostream>

#include <fstream>
#include <sys/stat.h>

#include <fcntl.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include "sshconnectionmanager.h"

#include <chrono>
#include <thread>

#include "containers/containers_defs.h"
#include "server_defs.h"
#include "settings.h"
#include "scripts_registry.h"
#include "utilities.h"

#include <configurators/vpn_configurator.h>

#define SFTP_TRANSFER_CHUNK_SIZE 16384
#define SSH_BUFFER_SIZE 256

using namespace QSsh;

ServerController::ServerController(std::shared_ptr<Settings> settings, QObject *parent) :
    m_settings(settings)
{
    ssh_init();
}

ServerController::~ServerController()
{
    ssh_finalize();
}

ErrorCode ServerController::connectToHost(const ServerCredentials &credentials, ssh_session &session) {

    if (session == NULL) {
        return ErrorCode::InternalError;
    }

    int port = credentials.port;
    int log_verbosity = SSH_LOG_NOLOG;
    std::string host_ip = credentials.hostName.toStdString();
    std::string host_username = credentials.userName.toStdString() + "@" + host_ip;

    ssh_options_set(session, SSH_OPTIONS_HOST, host_ip.c_str());
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    ssh_options_set(session, SSH_OPTIONS_USER, host_username.c_str());
    ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &log_verbosity);

    int connection_result = ssh_connect(session);

    if (connection_result != SSH_OK) {
        return ErrorCode::SshTimeoutError;
    }

    std::string auth_username = credentials.userName.toStdString();

    int auth_result = SSH_ERROR;
    if (credentials.password.contains("BEGIN") && credentials.password.contains("PRIVATE KEY")) {
        ssh_key priv_key;
        ssh_pki_import_privkey_base64(credentials.password.toStdString().c_str(), nullptr, nullptr, nullptr, &priv_key);
        auth_result = ssh_userauth_publickey(session, auth_username.c_str(), priv_key);
    }
    else {
       auth_result =  ssh_userauth_password(session, auth_username.c_str(), credentials.password.toStdString().c_str());
    }

    if (auth_result != SSH_OK) {
        ssh_disconnect(session);
        return ErrorCode::SshAuthenticationError;
    }

    return ErrorCode::NoError;
}


ErrorCode ServerController::runScript(const ServerCredentials &credentials, QString script,
    const std::function<void(const QString &, QSharedPointer<QSsh::SshRemoteProcess>)> &cbReadStdOut,
    const std::function<void(const QString &, QSharedPointer<QSsh::SshRemoteProcess>)> &cbReadStdErr) {

    ssh_session ssh = ssh_new();

    ErrorCode e = connectToHost(credentials, ssh);
    if (e) {
        ssh_free(ssh);
        return e;
    }
    ssh_channel channel = ssh_channel_new(ssh);

    if (channel == NULL) {
        ssh_disconnect(ssh);
        ssh_free(ssh);
        return ErrorCode::SshAuthenticationError;
    }

    ssh_channel_open_session(channel);

    if (ssh_channel_is_open(channel)) {
        qDebug() << "SSH chanel opened";
    } else {
        ssh_channel_free(channel);
        ssh_disconnect(ssh);
        ssh_free(ssh);
        return ErrorCode::SshAuthenticationError;
    }

    script.replace("\r", "");

    qDebug() << "Run script " << script;

    int exec_result = ssh_channel_request_pty(channel);

    ssh_channel_change_pty_size(channel, 80, 1024);
    ssh_channel_request_shell(channel);

    QString totalLine;
    const QStringList &lines = script.split("\n", Qt::SkipEmptyParts);
    for (int i = 0; i < lines.count(); i++) {
        QString currentLine = lines.at(i);
        QString nextLine;
        if (i + 1 < lines.count()) nextLine = lines.at(i+1);

        if (totalLine.isEmpty()) {
            totalLine = currentLine;
        }
        else {
            totalLine = totalLine + "\n" + currentLine;
        }

        QString lineToExec;
        if (currentLine.endsWith("\\")) {
            continue;
        }
        else {
            lineToExec = totalLine;
            totalLine.clear();
        }

        qDebug()<<"total line " << totalLine;

        // Run collected line
        if (lineToExec.startsWith("#")) {
            continue;
        }

        lineToExec += "\n";

        qDebug().noquote() << "EXEC" << lineToExec;
        Debug::appendSshLog("Run command:" + lineToExec);
        int nwritten, nbytes = 0, tryes = 0;
        char buffer[2048];
        if (ssh_channel_write(channel, lineToExec.toUtf8(), (uint32_t)lineToExec.size()) == lineToExec.size() &&
                        ssh_channel_write(channel, "\n", 1) == 1){
                while (nbytes !=0 || tryes < 100){
                 if (ssh_channel_is_open(channel) && !ssh_channel_is_eof(channel)) {
                  //nbytes = ssh_channel_read_nonblocking(channel, buffer, sizeof(buffer), 0);

                  nbytes = ssh_channel_read_timeout(channel, buffer, sizeof(buffer), 0, 50);

                  if (nbytes > 0) {
                    tryes = 0;
                    std::string strbuf;
                    strbuf.assign(buffer, nbytes);
                    QByteArray qbuff(buffer, nbytes);
                    QString outp(qbuff);

                    if (cbReadStdOut){
                        cbReadStdOut(outp, nullptr);
                    }
                    qDebug().noquote() << QString(strbuf.c_str());

                  } else {
                      tryes++;
                  }
                  std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }

            }
        }
    }

    ssh_channel_send_eof(channel);
    ssh_channel_free(channel);
    ssh_disconnect(ssh);
    ssh_free(ssh);

    return ErrorCode::NoError;
}


ErrorCode ServerController::runContainerScript(const ServerCredentials &credentials,
    DockerContainer container, QString script,
    const std::function<void (const QString &, QSharedPointer<QSsh::SshRemoteProcess>)> &cbReadStdOut,
    const std::function<void (const QString &, QSharedPointer<QSsh::SshRemoteProcess>)> &cbReadStdErr)
{
    QString fileName = "/opt/amnezia/" + Utils::getRandomString(16) + ".sh";
    Debug::appendSshLog("Run container script for " + ContainerProps::containerToString(container) + ":\n" + script);

    ErrorCode e = uploadTextFileToContainer(container, credentials, script, fileName);
    if (e) return e;

    QString runner = QString("sudo docker exec -i $CONTAINER_NAME bash %1 ").arg(fileName);
    e = runScript(credentials,
        replaceVars(runner, genVarsForScript(credentials, container)), cbReadStdOut, cbReadStdErr);

    QString remover = QString("sudo docker exec -i $CONTAINER_NAME rm %1 ").arg(fileName);
    runScript(credentials,
        replaceVars(remover, genVarsForScript(credentials, container)), cbReadStdOut, cbReadStdErr);

    return e;
}

ErrorCode ServerController::uploadTextFileToContainer(DockerContainer container,
    const ServerCredentials &credentials, const QString &file, const QString &path,
    QSsh::SftpOverwriteMode overwriteMode)
{
    ErrorCode e = ErrorCode::NoError;
    QString tmpFileName = QString("/tmp/%1.tmp").arg(Utils::getRandomString(16));

    qDebug() << "data" << file;

    e = uploadFileToHost(credentials, file.toUtf8(), tmpFileName);
    if (e) return e;

    QString stdOut;
    auto cbReadStd = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> ) {
        stdOut += data + "\n";
    };

    // mkdir
    QString mkdir = QString("sudo docker exec -i $CONTAINER_NAME mkdir -p  \"$(dirname %1)\"")
            .arg(path);

    e = runScript(credentials,
        replaceVars(mkdir, genVarsForScript(credentials, container)));
    if (e) return e;


    if (overwriteMode == QSsh::SftpOverwriteMode::SftpOverwriteExisting) {
        e = runScript(credentials,
            replaceVars(QString("sudo docker cp %1 $CONTAINER_NAME:/%2").arg(tmpFileName).arg(path),
                genVarsForScript(credentials, container)), cbReadStd, cbReadStd);

        if (e) return e;
    }
    else if (overwriteMode == QSsh::SftpOverwriteMode::SftpAppendToExisting) {
        e = runScript(credentials,
            replaceVars(QString("sudo docker cp %1 $CONTAINER_NAME:/%2").arg(tmpFileName).arg(tmpFileName),
                genVarsForScript(credentials, container)), cbReadStd, cbReadStd);

        if (e) return e;

        e = runScript(credentials,
            replaceVars(QString("sudo docker exec -i $CONTAINER_NAME sh -c \"cat %1 >> %2\"").arg(tmpFileName).arg(path),
                genVarsForScript(credentials, container)), cbReadStd, cbReadStd);

        if (e) return e;
    }
    else return ErrorCode::NotImplementedError;


    if (stdOut.contains("Error: No such container:")) {
        return ErrorCode::ServerContainerMissingError;
    }

    runScript(credentials,
        replaceVars(QString("sudo shred %1").arg(tmpFileName),
              genVarsForScript(credentials, container)));

    runScript(credentials,
            replaceVars(QString("sudo rm %1").arg(tmpFileName),
                genVarsForScript(credentials, container)));

    return e;
}

QByteArray ServerController::getTextFileFromContainer(DockerContainer container,
    const ServerCredentials &credentials, const QString &path, ErrorCode *errorCode)
{

    if (errorCode) *errorCode = ErrorCode::NoError;

    QString script = QString("sudo docker exec -i %1 sh -c \"xxd -p \'%2\'\"").
            arg(ContainerProps::containerToString(container)).arg(path);


    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data;
    };

    *errorCode = runScript(credentials, script, cbReadStdOut);

    qDebug().noquote() << "Copy file from container stdout : \n" << stdOut;


    qDebug().noquote() << "Copy file from container END : \n" ;


    int pos=stdOut.lastIndexOf("'\"");

    QString cuted_right(stdOut.right(stdOut.size()-pos-4));

    pos=cuted_right.lastIndexOf("\n");

    QString cuted(cuted_right.left(pos-1));

    qDebug().noquote() << "Copy file cuted : \n" << cuted <<"cut END";

    return QByteArray::fromHex(cuted.toUtf8());
}

ErrorCode ServerController::checkOpenVpnServer(DockerContainer container, const ServerCredentials &credentials)
{
    QString caCert = ServerController::getTextFileFromContainer(container,
        credentials, protocols::openvpn::caCertPath);
    QString taKey = ServerController::getTextFileFromContainer(container,
        credentials, protocols::openvpn::taKeyPath);

    if (!caCert.isEmpty() && !taKey.isEmpty()) {
        return ErrorCode::NoError;
    }
    else {
        return ErrorCode::ServerCheckFailed;
    }
}

ErrorCode ServerController::uploadFileToHost(const ServerCredentials &credentials, const QByteArray &data, const QString &remotePath,
    QSsh::SftpOverwriteMode overwriteMode)
{

    ssh_session ssh = ssh_new();

    ErrorCode e = connectToHost(credentials, ssh);
    if (e) {
        ssh_free(ssh);
        return e;
    }

    ssh_channel channel = ssh_channel_new(ssh);

    if (channel == NULL) {
        ssh_disconnect(ssh);
        ssh_free(ssh);
        return ErrorCode::SshSftpError;
    }

    sftp_session sftp_sess = sftp_new(ssh);

    if (sftp_sess == NULL) {
        ssh_disconnect(ssh);
        ssh_free(ssh);
        return ErrorCode::SshSftpError;
    }

    int init_result = sftp_init(sftp_sess);

    if (init_result != SSH_OK) {

        sftp_free(sftp_sess);
        ssh_disconnect(ssh);
        ssh_free(ssh);

        return ErrorCode::SshSftpError;
    }

    QTemporaryFile localFile;
    localFile.open();
    localFile.write(data);
    localFile.close();

    qDebug() << "remotePath" << remotePath;

    e = copyFileToRemoteHost(ssh, sftp_sess, localFile.fileName().toStdString(), remotePath.toStdString(), "non_desc");

    if (e) {
        sftp_free(sftp_sess);
        ssh_disconnect(ssh);
        ssh_free(ssh);
        return e;
    }
    else {

        sftp_free(sftp_sess);
        ssh_disconnect(ssh);
        ssh_free(ssh);
        return ErrorCode::NoError;
    }
}


ErrorCode ServerController::copyFileToRemoteHost(ssh_session& ssh, sftp_session& sftp, std::string local_path, std::string remote_path, std::string file_desc) {
    int access_type = O_WRONLY | O_CREAT | O_TRUNC;
    sftp_file file;
    char buffer[SFTP_TRANSFER_CHUNK_SIZE];
    int length {sizeof (buffer)};

    file = sftp_open(sftp, remote_path.c_str(), access_type, 0);//S_IRWXU);

    if (file == NULL) {
        return ErrorCode::SshSftpError;
    }

    int local_file_size   = std::filesystem::file_size(local_path);
    int num_full_chunks   = local_file_size / (SFTP_TRANSFER_CHUNK_SIZE);

    std::ifstream fin(local_path, std::ios::binary | std::ios::in);

    if (fin.is_open()) {

        for (int current_chunk_id = 0; current_chunk_id < num_full_chunks; current_chunk_id++) {
            // Getting chunk from local file
            fin.read(buffer, length);

            int nwritten = sftp_write(file, buffer, length);

            std::string chunk(buffer, length);

            qDebug() << "write -> " << QString(chunk.c_str());

            if (nwritten != length) {
                fin.close();
                sftp_close(file);
                return ErrorCode::SshSftpError;
            }
        }

        int last_chapter_size = local_file_size % (SFTP_TRANSFER_CHUNK_SIZE);

        if (last_chapter_size != 0) {
            char* last_chapter_buffer = new char[last_chapter_size];
            fin.read(last_chapter_buffer, last_chapter_size);

            QByteArray arr_tmp(last_chapter_buffer, last_chapter_size);

            QString std_test = QString::fromUtf8(arr_tmp);

            qDebug() << "test file " << std_test;

            int nwritten = sftp_write(file, last_chapter_buffer, last_chapter_size);

            if (nwritten != last_chapter_size) {

                fin.close();
                sftp_close(file);
                delete[] last_chapter_buffer;
                return ErrorCode::SshSftpError;
            }
            delete[] last_chapter_buffer;
        }

    } else {
        sftp_close(file);
        return ErrorCode::SshSftpError;
    }

    fin.close();

    int close_result = sftp_close(file);
    if (close_result != SSH_OK) {
        return ErrorCode::SshSftpError;
    }

    return ErrorCode::NoError;
}

ErrorCode ServerController::fromSshConnectionErrorCode(SshError error)
{
    switch (error) {
    case(SshNoError): return ErrorCode::NoError;
    case(QSsh::SshSocketError): return ErrorCode::SshSocketError;
    case(QSsh::SshTimeoutError): return ErrorCode::SshTimeoutError;
    case(QSsh::SshProtocolError): return ErrorCode::SshProtocolError;
    case(QSsh::SshHostKeyError): return ErrorCode::SshHostKeyError;
    case(QSsh::SshKeyFileError): return ErrorCode::SshKeyFileError;
    case(QSsh::SshAuthenticationError): return ErrorCode::SshAuthenticationError;
    case(QSsh::SshClosedByServerError): return ErrorCode::SshClosedByServerError;
    case(QSsh::SshInternalError): return ErrorCode::SshInternalError;
    default: return ErrorCode::SshInternalError;
    }
}

ErrorCode ServerController::fromSshProcessExitStatus(int exitStatus)
{
    qDebug() << exitStatus;
    switch (SshRemoteProcess::ExitStatus(exitStatus)) {
    case(SshRemoteProcess::ExitStatus::NormalExit): return ErrorCode::NoError;
    case(SshRemoteProcess::ExitStatus::FailedToStart): return ErrorCode::FailedToStartRemoteProcessError;
    case(SshRemoteProcess::ExitStatus::CrashExit): return ErrorCode::RemoteProcessCrashError;
    default: return ErrorCode::SshInternalError;
    }
}

SshConnectionParameters ServerController::sshParams(const ServerCredentials &credentials)
{
    QSsh::SshConnectionParameters sshParams;
    if (credentials.password.contains("BEGIN") && credentials.password.contains("PRIVATE KEY")) {
        sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePublicKey;
        sshParams.privateKeyFile = credentials.password;
    }
    else {
        sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePassword;
        sshParams.setPassword(credentials.password);
    }
    sshParams.setHost(credentials.hostName);
    sshParams.setUserName(credentials.userName);
    sshParams.timeout = 10;
    sshParams.setPort(credentials.port);
    sshParams.hostKeyCheckingMode = QSsh::SshHostKeyCheckingMode::SshHostKeyCheckingNone;
    sshParams.options = SshIgnoreDefaultProxy;

    return sshParams;
}

ErrorCode ServerController::removeAllContainers(const ServerCredentials &credentials)
{
    return runScript(credentials,
        amnezia::scriptData(SharedScriptType::remove_all_containers));
}

ErrorCode ServerController::removeContainer(const ServerCredentials &credentials, DockerContainer container)
{
    return runScript(credentials,
        replaceVars(amnezia::scriptData(SharedScriptType::remove_container),
            genVarsForScript(credentials, container)));
}

ErrorCode ServerController::setupContainer(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config)
{
    qDebug().noquote() << "ServerController::setupContainer" << ContainerProps::containerToString(container);
    //qDebug().noquote() << QJsonDocument(config).toJson();
    ErrorCode e = ErrorCode::NoError;

    e = installDockerWorker(credentials, container);
    if (e) return e;
    qDebug().noquote() << "ServerController::setupContainer installDockerWorker finished";

    e = prepareHostWorker(credentials, container, config);
    if (e) return e;
    qDebug().noquote() << "ServerController::setupContainer prepareHostWorker finished";

    removeContainer(credentials, container);
    qDebug().noquote() << "ServerController::setupContainer removeContainer finished";

    qDebug().noquote() << "buildContainerWorker start";
    e = buildContainerWorker(credentials, container, config);
    if (e) return e;
    qDebug().noquote() << "ServerController::setupContainer buildContainerWorker finished";

    e = runContainerWorker(credentials, container, config);
    if (e) return e;
    qDebug().noquote() << "ServerController::setupContainer runContainerWorker finished";

    e = configureContainerWorker(credentials, container, config);
    if (e) return e;
    qDebug().noquote() << "ServerController::setupContainer configureContainerWorker finished";

    setupServerFirewall(credentials);
    qDebug().noquote() << "ServerController::setupContainer setupServerFirewall finished";

    return startupContainerWorker(credentials, container, config);
}

ErrorCode ServerController::updateContainer(const ServerCredentials &credentials, DockerContainer container,
    const QJsonObject &oldConfig, QJsonObject &newConfig)
{
    bool reinstallRequred = isReinstallContainerRequred(container, oldConfig, newConfig);
    qDebug() << "ServerController::updateContainer for container" << container << "reinstall required is" << reinstallRequred;

    if (reinstallRequred) {
        return setupContainer(credentials, container, newConfig);
    }
    else {
        ErrorCode e = configureContainerWorker(credentials, container, newConfig);
        if (e) return e;

        return startupContainerWorker(credentials, container, newConfig);
    }
}

QJsonObject ServerController::createContainerInitialConfig(DockerContainer container, int port, TransportProto tp)
{
    Proto mainProto = ContainerProps::defaultProtocol(container);

    QJsonObject config {
        { config_key::container, ContainerProps::containerToString(container) }
    };

    QJsonObject protoConfig;
    protoConfig.insert(config_key::port, QString::number(port));
    protoConfig.insert(config_key::transport_proto, ProtocolProps::transportProtoToString(tp, mainProto));


    if (container == DockerContainer::Sftp) {
        protoConfig.insert(config_key::userName, protocols::sftp::defaultUserName);
        protoConfig.insert(config_key::password, Utils::getRandomString(10));
    }

    config.insert(ProtocolProps::protoToString(mainProto), protoConfig);

    return config;
}

bool ServerController::isReinstallContainerRequred(DockerContainer container, const QJsonObject &oldConfig, const QJsonObject &newConfig)
{
    Proto mainProto = ContainerProps::defaultProtocol(container);

    const QJsonObject &oldProtoConfig = oldConfig.value(ProtocolProps::protoToString(mainProto)).toObject();
    const QJsonObject &newProtoConfig = newConfig.value(ProtocolProps::protoToString(mainProto)).toObject();

    if (container == DockerContainer::OpenVpn) {
        if (oldProtoConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto) !=
            newProtoConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto))
                return true;

        if (oldProtoConfig.value(config_key::port).toString(protocols::openvpn::defaultPort) !=
            newProtoConfig.value(config_key::port).toString(protocols::openvpn::defaultPort))
                return true;
    }

    if (container == DockerContainer::Cloak) {
        if (oldProtoConfig.value(config_key::port).toString(protocols::cloak::defaultPort) !=
            newProtoConfig.value(config_key::port).toString(protocols::cloak::defaultPort))
                return true;
    }

    if (container == DockerContainer::ShadowSocks) {
         if (oldProtoConfig.value(config_key::port).toString(protocols::shadowsocks::defaultPort) !=
            newProtoConfig.value(config_key::port).toString(protocols::shadowsocks::defaultPort))
                return true;
    }

    return false;
}

ErrorCode ServerController::installDockerWorker(const ServerCredentials &credentials, DockerContainer container)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";

       // if (data.contains("Automatically restart Docker daemon?")) {
       //     proc->write("yes\n");
       // }
    };
    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> ) {
        stdOut += data + "\n";
    };

    ErrorCode e = runScript(credentials,
        replaceVars(amnezia::scriptData(SharedScriptType::install_docker),
            genVarsForScript(credentials)),
                cbReadStdOut, cbReadStdErr);

    if (stdOut.contains("command not found")) return ErrorCode::ServerDockerFailedError;

    return e;
}

ErrorCode ServerController::prepareHostWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    // create folder on host
    return runScript(credentials,
        replaceVars(amnezia::scriptData(SharedScriptType::prepare_host),
            genVarsForScript(credentials, container)));
}

ErrorCode ServerController::buildContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    ErrorCode e = uploadFileToHost(credentials, amnezia::scriptData(ProtocolScriptType::dockerfile, container).toUtf8(),
        amnezia::server::getDockerfileFolder(container) + "/Dockerfile");

    if (e) return e;

    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };
//    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
//        stdOut += data + "\n";
//    };

    e = runScript(credentials,
        replaceVars(amnezia::scriptData(SharedScriptType::build_container),
                    genVarsForScript(credentials, container, config)), cbReadStdOut);
    if (e) return e;

    return e;
}

ErrorCode ServerController::runContainerWorker(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };
   // auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
   //     stdOut += data + "\n";
   // };

    ErrorCode e = runScript(credentials,
        replaceVars(amnezia::scriptData(ProtocolScriptType::run_container, container),
            genVarsForScript(credentials, container, config)), cbReadStdOut);

    qDebug() << "cbReadStdOut: " << stdOut;


    if (stdOut.contains("docker: Error response from daemon")) return ErrorCode::ServerDockerFailedError;

    if (stdOut.contains("address already in use")) return ErrorCode::ServerPortAlreadyAllocatedError;
    if (stdOut.contains("is already in use by container")) return ErrorCode::ServerPortAlreadyAllocatedError;
    if (stdOut.contains("invalid publish")) return ErrorCode::ServerDockerFailedError;

    return e;
}

ErrorCode ServerController::configureContainerWorker(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };
    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };


    ErrorCode e = runContainerScript(credentials, container,
        replaceVars(amnezia::scriptData(ProtocolScriptType::configure_container, container),
            genVarsForScript(credentials, container, config)),
                cbReadStdOut, cbReadStdErr);


    m_configurator->updateContainerConfigAfterInstallation(container, config, stdOut);

    return e;
}

ErrorCode ServerController::startupContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    QString script = amnezia::scriptData(ProtocolScriptType::container_startup, container);

    if (script.isEmpty()) {
        return ErrorCode::NoError;
    }

    ErrorCode e = uploadTextFileToContainer(container, credentials,
        replaceVars(script, genVarsForScript(credentials, container, config)),
            "/opt/amnezia/start.sh");
    if (e) return e;

    return runScript(credentials,
        replaceVars("sudo docker exec -d $CONTAINER_NAME sh -c \"chmod a+x /opt/amnezia/start.sh && /opt/amnezia/start.sh\"",
            genVarsForScript(credentials, container, config)));
}

ServerController::Vars ServerController::genVarsForScript(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    const QJsonObject &openvpnConfig = config.value(ProtocolProps::protoToString(Proto::OpenVpn)).toObject();
    const QJsonObject &cloakConfig = config.value(ProtocolProps::protoToString(Proto::Cloak)).toObject();
    const QJsonObject &ssConfig = config.value(ProtocolProps::protoToString(Proto::ShadowSocks)).toObject();
    const QJsonObject &wireguarConfig = config.value(ProtocolProps::protoToString(Proto::WireGuard)).toObject();
    const QJsonObject &sftpConfig = config.value(ProtocolProps::protoToString(Proto::Sftp)).toObject();
    //

    Vars vars;

    vars.append({{"$REMOTE_HOST", credentials.hostName}});

    // OpenVPN vars
    vars.append({{"$OPENVPN_SUBNET_IP", openvpnConfig.value(config_key::subnet_address).toString(protocols::openvpn::defaultSubnetAddress) }});
    vars.append({{"$OPENVPN_SUBNET_CIDR", openvpnConfig.value(config_key::subnet_cidr).toString(protocols::openvpn::defaultSubnetCidr) }});
    vars.append({{"$OPENVPN_SUBNET_MASK", openvpnConfig.value(config_key::subnet_mask).toString(protocols::openvpn::defaultSubnetMask) }});

    vars.append({{"$OPENVPN_PORT", openvpnConfig.value(config_key::port).toString(protocols::openvpn::defaultPort) }});
    vars.append({{"$OPENVPN_TRANSPORT_PROTO", openvpnConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto) }});

    bool isNcpDisabled = openvpnConfig.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable);
    vars.append({{"$OPENVPN_NCP_DISABLE",  isNcpDisabled ? protocols::openvpn::ncpDisableString : "" }});

    vars.append({{"$OPENVPN_CIPHER", openvpnConfig.value(config_key::cipher).toString(protocols::openvpn::defaultCipher) }});
    vars.append({{"$OPENVPN_HASH", openvpnConfig.value(config_key::hash).toString(protocols::openvpn::defaultHash) }});

    bool isTlsAuth = openvpnConfig.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth);
    vars.append({{"$OPENVPN_TLS_AUTH", isTlsAuth ? protocols::openvpn::tlsAuthString : "" }});
    if (!isTlsAuth) {
        // erase $OPENVPN_TA_KEY, so it will not set in OpenVpnConfigurator::genOpenVpnConfig
        vars.append({{"$OPENVPN_TA_KEY", "" }});
    }

    vars.append({{"$OPENVPN_ADDITIONAL_CLIENT_CONFIG", openvpnConfig.value(config_key::additional_client_config).
                  toString(protocols::openvpn::defaultAdditionalClientConfig) }});
    vars.append({{"$OPENVPN_ADDITIONAL_SERVER_CONFIG", openvpnConfig.value(config_key::additional_server_config).
                  toString(protocols::openvpn::defaultAdditionalServerConfig) }});

    // ShadowSocks vars
    vars.append({{"$SHADOWSOCKS_SERVER_PORT", ssConfig.value(config_key::port).toString(protocols::shadowsocks::defaultPort) }});
    vars.append({{"$SHADOWSOCKS_LOCAL_PORT", ssConfig.value(config_key::local_port).toString(protocols::shadowsocks::defaultLocalProxyPort) }});
    vars.append({{"$SHADOWSOCKS_CIPHER", ssConfig.value(config_key::cipher).toString(protocols::shadowsocks::defaultCipher) }});

    vars.append({{"$CONTAINER_NAME", ContainerProps::containerToString(container)}});
    vars.append({{"$DOCKERFILE_FOLDER", "/opt/amnezia/" + ContainerProps::containerToString(container)}});

    // Cloak vars
    vars.append({{"$CLOAK_SERVER_PORT", cloakConfig.value(config_key::port).toString(protocols::cloak::defaultPort) }});
    vars.append({{"$FAKE_WEB_SITE_ADDRESS", cloakConfig.value(config_key::site).toString(protocols::cloak::defaultRedirSite) }});

    // Wireguard vars
    vars.append({{"$WIREGUARD_SUBNET_IP", wireguarConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress) }});
    vars.append({{"$WIREGUARD_SUBNET_CIDR", wireguarConfig.value(config_key::subnet_cidr).toString(protocols::wireguard::defaultSubnetCidr) }});
    vars.append({{"$WIREGUARD_SUBNET_MASK", wireguarConfig.value(config_key::subnet_mask).toString(protocols::wireguard::defaultSubnetMask) }});

    vars.append({{"$WIREGUARD_SERVER_PORT", wireguarConfig.value(config_key::port).toString(protocols::wireguard::defaultPort) }});

    // IPsec vars
    vars.append({{"$IPSEC_VPN_L2TP_NET", "192.168.42.0/24"}});
    vars.append({{"$IPSEC_VPN_L2TP_POOL", "192.168.42.10-192.168.42.250"}});
    vars.append({{"$IPSEC_VPN_L2TP_LOCAL", "192.168.42.1"}});

    vars.append({{"$IPSEC_VPN_XAUTH_NET", "192.168.43.0/24"}});
    vars.append({{"$IPSEC_VPN_XAUTH_POOL", "192.168.43.10-192.168.43.250"}});

    vars.append({{"$IPSEC_VPN_SHA2_TRUNCBUG", "yes"}});

    vars.append({{"$IPSEC_VPN_VPN_ANDROID_MTU_FIX", "yes"}});
    vars.append({{"$IPSEC_VPN_DISABLE_IKEV2", "no"}});
    vars.append({{"$IPSEC_VPN_DISABLE_L2TP", "no"}});
    vars.append({{"$IPSEC_VPN_DISABLE_XAUTH", "no"}});

    vars.append({{"$IPSEC_VPN_C2C_TRAFFIC", "no"}});

    vars.append({{"$PRIMARY_SERVER_DNS", m_settings->primaryDns()}});
    vars.append({{"$SECONDARY_SERVER_DNS", m_settings->secondaryDns()}});


    // Sftp vars
    vars.append({{"$SFTP_PORT", sftpConfig.value(config_key::port).toString(QString::number(ProtocolProps::defaultPort(Proto::Sftp))) }});
    vars.append({{"$SFTP_USER", sftpConfig.value(config_key::userName).toString() }});
    vars.append({{"$SFTP_PASSWORD", sftpConfig.value(config_key::password).toString() }});


    QString serverIp = Utils::getIPAddress(credentials.hostName);
    if (!serverIp.isEmpty()) {
        vars.append({{"$SERVER_IP_ADDRESS", serverIp}});
    }
    else {
        qWarning() << "ServerController::genVarsForScript unable to resolve address for credentials.hostName";
    }

    return vars;
}

QString ServerController::checkSshConnection(const ServerCredentials &credentials, ErrorCode *errorCode)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };
    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> ) {
        stdOut += data + "\n";
    };

    ErrorCode e = runScript(credentials,
        amnezia::scriptData(SharedScriptType::check_connection), cbReadStdOut, cbReadStdErr);

    if (errorCode) *errorCode = e;

    return stdOut;
}

SshConnection *ServerController::connectToHost(const SshConnectionParameters &sshParams)
{
    SshConnection *client = acquireConnection(sshParams);
    if (!client) return nullptr;

    QEventLoop waitssh;
    QObject::connect(client, &SshConnection::connected, &waitssh, [&]() {
        qDebug() << "Server connected by ssh";
        waitssh.quit();
    });

    QObject::connect(client, &SshConnection::disconnected, &waitssh, [&]() {
        qDebug() << "Server disconnected by ssh";
        waitssh.quit();
    });

    QObject::connect(client, &SshConnection::error, &waitssh, [&](QSsh::SshError error) {
        qCritical() << "Ssh error:" << error << client->errorString();
        waitssh.quit();
    });


    //    QObject::connect(client, &SshConnection::dataAvailable, [&](const QString &message) {
    //        qCritical() << "Ssh message:" << message;
    //    });

    //qDebug() << "Connection state" << client->state();

    if (client->state() == SshConnection::State::Unconnected) {
        client->connectToHost();
        waitssh.exec();
    }


    //    QObject::connect(&client, &SshClient::sshDataReceived, [&](){
    //        qDebug().noquote() << "Data received";
    //    });


    //    if(client.sshState() != SshClient::SshState::Ready) {
    //        qCritical() << "Can't connect to server";
    //        return false;
    //    }
    //    else {
    //        qDebug() << "SSh connection established";
    //    }


    //    QObject::connect(proc, &SshProcess::finished, &wait, &QEventLoop::quit);
    //    QObject::connect(proc, &SshProcess::failed, &wait, &QEventLoop::quit);

    return client;
}

void ServerController::disconnectFromHost(const ServerCredentials &credentials)
{
    SshConnection *client = acquireConnection(sshParams(credentials));
    if (client) client->disconnectFromHost();
}

ErrorCode ServerController::setupServerFirewall(const ServerCredentials &credentials)
{
    return runScript(credentials,
        replaceVars(amnezia::scriptData(SharedScriptType::setup_host_firewall),
            genVarsForScript(credentials)));
}

QString ServerController::replaceVars(const QString &script, const Vars &vars)
{
    QString s = script;
    for (const QPair<QString, QString> &var : vars) {
        //qDebug() << "Replacing" << var.first << var.second;
        s.replace(var.first, var.second);
    }
    //qDebug().noquote() << script;
    return s;
}
