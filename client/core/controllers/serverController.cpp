#include "serverController.h"

#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QPointer>
#include <QTemporaryFile>
#include <QThread>
#include <QTimer>
#include <QtConcurrent>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

#include <chrono>
#include <thread>

#include "containers/containers_defs.h"
#include "core/networkUtilities.h"
#include "core/scripts_registry.h"
#include "core/server_defs.h"
#include "logger.h"
#include "settings.h"
#include "utilities.h"
#include "vpnConfigurationController.h"

namespace
{
    Logger logger("ServerController");
}

ServerController::ServerController(std::shared_ptr<Settings> settings, QObject *parent) : m_settings(settings)
{
}

ServerController::~ServerController()
{
    m_sshClient.disconnectFromHost();
}

ErrorCode ServerController::runScript(const ServerCredentials &credentials, QString script,
                                      const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdOut,
                                      const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdErr)
{

    auto error = m_sshClient.connectToHost(credentials);
    if (error != ErrorCode::NoError) {
        return error;
    }

    script.replace("\r", "");

    qDebug() << "ServerController::Run script";

    QString totalLine;
    const QStringList &lines = script.split("\n", Qt::SkipEmptyParts);
    for (int i = 0; i < lines.count(); i++) {
        QString currentLine = lines.at(i);

        if (totalLine.isEmpty()) {
            totalLine = currentLine;
        } else {
            totalLine = totalLine + "\n" + currentLine;
        }

        QString lineToExec;
        if (currentLine.endsWith("\\")) {
            continue;
        } else {
            lineToExec = totalLine;
            totalLine.clear();
        }

        if (lineToExec.startsWith("#")) {
            continue;
        }

        qDebug().noquote() << lineToExec;

        error = m_sshClient.executeCommand(lineToExec, cbReadStdOut, cbReadStdErr);
        if (error != ErrorCode::NoError) {
            return error;
        }
    }

    qDebug().noquote() << "ServerController::runScript finished\n";
    return ErrorCode::NoError;
}

ErrorCode ServerController::runContainerScript(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials,
                                               QString script,
                                               const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdOut,
                                               const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdErr)
{
    QString fileName = "/opt/amnezia/" + Utils::getRandomString(16) + ".sh";

    ErrorCode e = uploadTextFileToContainer(containerConfig.containerType, serverCredentials, script, fileName);
    if (e)
        return e;

    QString runner = QString("sudo docker exec -i $CONTAINER_NAME %2 %1 ")
                             .arg(fileName, (containerConfig.containerType == DockerContainer::Socks5Proxy ? "sh" : "bash"));
    ContainerConfig tempConfig;
    Vars vars = genVarsForScript(containerConfig, serverCredentials);
    e = runScript(serverCredentials, replaceVars(runner, vars), cbReadStdOut, cbReadStdErr);

    QString remover = QString("sudo docker exec -i $CONTAINER_NAME rm %1 ").arg(fileName);
    Vars vars2 = genVarsForScript(containerConfig, serverCredentials);
    runScript(serverCredentials, replaceVars(remover, vars2), cbReadStdOut, cbReadStdErr);

    return e;
}

ErrorCode ServerController::uploadTextFileToContainer(const DockerContainer &containerType, const ServerCredentials &serverCredentials,
                                                      const QString &file, const QString &path, libssh::ScpOverwriteMode overwriteMode)
{
    const QString containerName = ContainerProps::containerToString(containerType);

    ErrorCode e = ErrorCode::NoError;
    QString tmpFileName = QString("/tmp/%1.tmp").arg(Utils::getRandomString(16));
    e = uploadFileToHost(serverCredentials, file.toUtf8(), tmpFileName);
    if (e)
        return e;

    QString stdOut;
    auto cbReadStd = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    // mkdir
    QString mkdir = QString("sudo docker exec -i %1 mkdir -p  \"$(dirname %2)\"").arg(containerName, path);

    e = runScript(serverCredentials, mkdir);
    if (e)
        return e;

    if (overwriteMode == libssh::ScpOverwriteMode::ScpOverwriteExisting) {
        e = runScript(serverCredentials, QStringLiteral("sudo docker cp %1 %2:/%3").arg(tmpFileName, containerName, path), cbReadStd,
                      cbReadStd);

        if (e)
            return e;
    } else if (overwriteMode == libssh::ScpOverwriteMode::ScpAppendToExisting) {
        e = runScript(serverCredentials, QStringLiteral("sudo docker cp %1 %2:/%3").arg(tmpFileName, containerName, tmpFileName), cbReadStd,
                      cbReadStd);

        if (e)
            return e;

        e = runScript(serverCredentials,
                      QStringLiteral("sudo docker exec -i %1 sh -c \"cat %2 >> %3\"").arg(containerName, tmpFileName, path), cbReadStd,
                      cbReadStd);

        if (e)
            return e;
    } else
        return ErrorCode::NotImplementedError;

    if (stdOut.contains("Error") && stdOut.contains("No such container")) {
        return ErrorCode::ServerContainerMissingError;
    }

    runScript(serverCredentials, QString("sudo shred -u %1").arg(tmpFileName));
    return e;
}

QByteArray ServerController::getTextFileFromContainer(const DockerContainer &containerType, const ServerCredentials &credentials,
                                                      const QString &path, ErrorCode &errorCode)
{
    errorCode = ErrorCode::NoError;

    QString script =
            QStringLiteral("sudo docker exec -i %1 sh -c \"xxd -p '%2'\"").arg(ContainerProps::containerToString(containerType), path);

    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data;
        return ErrorCode::NoError;
    };

    errorCode = runScript(credentials, script, cbReadStdOut);
    return QByteArray::fromHex(stdOut.toUtf8());
}

ErrorCode ServerController::uploadFileToHost(const ServerCredentials &serverCredentials, const QByteArray &data, const QString &remotePath,
                                             libssh::ScpOverwriteMode overwriteMode)
{
    auto error = m_sshClient.connectToHost(serverCredentials);
    if (error != ErrorCode::NoError) {
        return error;
    }

    QTemporaryFile localFile;
    localFile.open();
    localFile.write(data);
    localFile.close();

    error = m_sshClient.scpFileCopy(overwriteMode, localFile.fileName(), remotePath, "non_desc");

    if (error != ErrorCode::NoError) {
        return error;
    }
    return ErrorCode::NoError;
}

ErrorCode ServerController::rebootServer(const ServerCredentials &serverCredentials)
{
    QString script = QString("sudo reboot");

    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data;
        return ErrorCode::NoError;
    };

    auto cbReadStdErr = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    return runScript(serverCredentials, script, cbReadStdOut, cbReadStdErr);
}

ErrorCode ServerController::removeAllContainers(const ServerCredentials &serverCredentials)
{
    return runScript(serverCredentials, amnezia::scriptData(SharedScriptType::remove_all_containers));
}

ErrorCode ServerController::removeContainer(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials)
{
    Vars vars = genVarsForScript(containerConfig, serverCredentials);
    return runScript(serverCredentials, replaceVars(amnezia::scriptData(SharedScriptType::remove_container), vars));
}

ErrorCode ServerController::setupContainer(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials, bool isUpdate)
{
    qDebug().noquote() << "ServerController::setupContainer" << containerConfig.containerName;
    ErrorCode e = ErrorCode::NoError;

    e = isUserInSudo(containerConfig, serverCredentials);
    if (e)
        return e;

    e = isServerDpkgBusy(containerConfig, serverCredentials);
    if (e)
        return e;

    e = installDockerWorker(containerConfig, serverCredentials);
    if (e)
        return e;
    qDebug().noquote() << "ServerController::setupContainer installDockerWorker finished";

    if (!isUpdate) {
        e = isServerPortBusy(containerConfig, serverCredentials);
        if (e)
            return e;
    }

    if (!isUpdate) {
        e = isServerPortBusy(containerConfig, serverCredentials);
        if (e)
            return e;
    }

    e = prepareHostWorker(containerConfig, serverCredentials);
    if (e)
        return e;
    qDebug().noquote() << "ServerController::setupContainer prepareHostWorker finished";

    removeContainer(containerConfig, serverCredentials);
    qDebug().noquote() << "ServerController::setupContainer removeContainer finished";

    qDebug().noquote() << "buildContainerWorker start";
    e = buildContainerWorker(containerConfig, serverCredentials);
    if (e)
        return e;
    qDebug().noquote() << "ServerController::setupContainer buildContainerWorker finished";

    e = runContainerWorker(containerConfig, serverCredentials);
    if (e)
        return e;
    qDebug().noquote() << "ServerController::setupContainer runContainerWorker finished";

    e = configureContainerWorker(containerConfig, serverCredentials);
    if (e)
        return e;
    qDebug().noquote() << "ServerController::setupContainer configureContainerWorker finished";

    setupServerFirewall(serverCredentials);
    qDebug().noquote() << "ServerController::setupContainer setupServerFirewall finished";

    return startupContainerWorker(containerConfig, serverCredentials);
}

ErrorCode ServerController::updateContainer(const ServerCredentials &serverCredentials, const ContainerConfig &oldConfig,
                                            ContainerConfig &newConfig)
{
    bool reinstallRequired = isReinstallContainerRequired(oldConfig.containerType, oldConfig, newConfig);
    qDebug() << "ServerController::updateContainer for container" << oldConfig.containerName << "reinstall required is" << reinstallRequired;

    if (reinstallRequired) {
        return setupContainer(newConfig, serverCredentials, true);
    } else {
        ErrorCode e = configureContainerWorker(newConfig, serverCredentials);
        if (e)
            return e;

        return startupContainerWorker(newConfig, serverCredentials);
    }
}

bool ServerController::isReinstallContainerRequired(const DockerContainer container, const ContainerConfig &oldConfig,
                                                    const ContainerConfig &newConfig)
{
    Proto mainProto = ContainerProps::defaultProtocol(container);

    auto oldProtocolConfig = oldConfig.protocolConfigs.value(ProtocolProps::protoToString(mainProto));
    auto newProtocolConfig = newConfig.protocolConfigs.value(ProtocolProps::protoToString(mainProto));

    return !oldProtocolConfig->isServerSettingsEqual(newProtocolConfig);
}

ErrorCode ServerController::installDockerWorker(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &client) {
        stdOut += data + "\n";

        if (data.contains("Automatically restart Docker daemon?")) {
            return client.writeResponse("yes");
        }
        return ErrorCode::NoError;
    };
    auto cbReadStdErr = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    Vars vars = genVarsForScript(containerConfig, serverCredentials);
    ErrorCode error = runScript(serverCredentials, replaceVars(amnezia::scriptData(SharedScriptType::install_docker), vars), cbReadStdOut,
                                cbReadStdErr);

    qDebug().noquote() << "ServerController::installDockerWorker" << stdOut;
    if (stdOut.contains("lock"))
        return ErrorCode::ServerPacketManagerError;
    if (stdOut.contains("command not found"))
        return ErrorCode::ServerDockerFailedError;

    return error;
}

ErrorCode ServerController::prepareHostWorker(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials)
{
    // create folder on host
    Vars vars = genVarsForScript(containerConfig, serverCredentials);
    return runScript(serverCredentials, replaceVars(amnezia::scriptData(SharedScriptType::prepare_host), vars));
}

ErrorCode ServerController::buildContainerWorker(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials)
{
    QString dockerFilePath = amnezia::server::getDockerfileFolder(containerConfig.containerType) + "/Dockerfile";
    QString scriptString = QString("sudo rm %1").arg(dockerFilePath);
    Vars vars = genVarsForScript(containerConfig, serverCredentials);
    ErrorCode errorCode = runScript(serverCredentials, replaceVars(scriptString, vars));
    if (errorCode)
        return errorCode;

    errorCode = uploadFileToHost(
            serverCredentials, amnezia::scriptData(ProtocolScriptType::dockerfile, containerConfig.containerType).toUtf8(), dockerFilePath);

    if (errorCode)
        return errorCode;

    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };
    auto cbReadStdErr = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    ErrorCode error = runScript(
            serverCredentials,
            replaceVars(amnezia::scriptData(SharedScriptType::build_container), genVarsForScript(containerConfig, serverCredentials)),
            cbReadStdOut, cbReadStdErr);

    if (stdOut.contains("doesn't work on cgroups v2"))
        return ErrorCode::ServerDockerOnCgroupsV2;
    if (stdOut.contains("cgroup mountpoint does not exist"))
        return ErrorCode::ServerCgroupMountpoint;
    if (stdOut.contains("have reached") && stdOut.contains("pull rate limit"))
        return ErrorCode::DockerPullRateLimit;

    return error;
}

ErrorCode ServerController::runContainerWorker(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    ErrorCode e = runScript(serverCredentials,
                            replaceVars(amnezia::scriptData(ProtocolScriptType::run_container, containerConfig.containerType),
                                        genVarsForScript(containerConfig, serverCredentials)),
                            cbReadStdOut);

    if (stdOut.contains("address already in use"))
        return ErrorCode::ServerPortAlreadyAllocatedError;
    if (stdOut.contains("is already in use by container"))
        return ErrorCode::ServerPortAlreadyAllocatedError;
    if (stdOut.contains("invalid publish"))
        return ErrorCode::ServerDockerFailedError;

    return e;
}

ErrorCode ServerController::configureContainerWorker(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };
    auto cbReadStdErr = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    ErrorCode e = runContainerScript(containerConfig, serverCredentials,
                                     replaceVars(amnezia::scriptData(ProtocolScriptType::configure_container, containerConfig.containerType),
                                                 genVarsForScript(containerConfig, serverCredentials)),
                                     cbReadStdOut, cbReadStdErr);
    return e;
}

ErrorCode ServerController::startupContainerWorker(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials)
{
    QString script = amnezia::scriptData(ProtocolScriptType::container_startup, containerConfig.containerType);

    if (script.isEmpty()) {
        return ErrorCode::NoError;
    }

    ErrorCode e =
            uploadTextFileToContainer(containerConfig.containerType, serverCredentials,
                                      replaceVars(script, genVarsForScript(containerConfig, serverCredentials)), "/opt/amnezia/start.sh");
    if (e)
        return e;

    return runScript(serverCredentials,
                     replaceVars("sudo docker exec -d $CONTAINER_NAME sh -c \"chmod a+x /opt/amnezia/start.sh && "
                                 "/opt/amnezia/start.sh\"",
                                 genVarsForScript(containerConfig, serverCredentials)));
}

ServerController::Vars ServerController::genVarsForScript(const ServerCredentials &serverCredentials, const DockerContainer containerType)
{
    Vars vars;

    vars.append({ "$REMOTE_HOST", serverCredentials.hostName });
    vars.append({ "$CONTAINER_NAME", ContainerProps::containerToString(containerType) });
    vars.append({ "$DOCKERFILE_FOLDER", "/opt/amnezia/" + ContainerProps::containerToString(containerType) });

    vars.append({ "$PRIMARY_SERVER_DNS", m_settings->primaryDns() });
    vars.append({ "$SECONDARY_SERVER_DNS", m_settings->secondaryDns() });

    QString serverIp =
            (containerType != DockerContainer::Awg && containerType != DockerContainer::WireGuard && containerType != DockerContainer::Xray)
            ? NetworkUtilities::getIPAddress(serverCredentials.hostName)
            : serverCredentials.hostName;

    if (!serverIp.isEmpty()) {
        vars.append({ "$SERVER_IP_ADDRESS", serverIp });
    }

    return vars;
}

ServerController::Vars ServerController::genVarsForScript(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials)
{
    Vars vars;

    vars.append(genVarsForScript(serverCredentials, containerConfig.containerType));

    for (const auto &porotoclConfig : containerConfig.protocolConfigs) {
        vars.append(porotoclConfig->getScriptVars());
    }

    return vars;
}

QString ServerController::checkSshConnection(const ServerCredentials &serverCredentials, ErrorCode &errorCode)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };
    auto cbReadStdErr = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    errorCode = runScript(serverCredentials, amnezia::scriptData(SharedScriptType::check_connection), cbReadStdOut, cbReadStdErr);

    return stdOut;
}

void ServerController::cancelInstallation()
{
    m_cancelInstallation = true;
}

ErrorCode ServerController::setupServerFirewall(const ServerCredentials &serverCredentials)
{
    Vars vars = genVarsForScript(serverCredentials, DockerContainer::None);
    return runScript(serverCredentials, replaceVars(amnezia::scriptData(SharedScriptType::setup_host_firewall), vars));
}

QString ServerController::replaceVars(const QString &script, const Vars &vars)
{
    QString s = script;
    for (const QPair<QString, QString> &var : vars) {
        s.replace(var.first, var.second);
    }
    return s;
}

ErrorCode ServerController::isServerPortBusy(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials)
{
    if (containerConfig.containerType == DockerContainer::Dns) {
        return ErrorCode::NoError;
    }

    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };
    auto cbReadStdErr = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    Proto mainProto = ContainerProps::defaultProtocol(containerConfig.containerType);
    auto protocolConfig = containerConfig.protocolConfigs.value(ProtocolProps::protoToString(mainProto));
    auto protocolConfigVariant = ProtocolConfig::getProtocolConfigVariant(protocolConfig);

    QString port;
    QString transportProto;
    std::visit(
            [&port, &transportProto](const auto &configPtr) {
                if constexpr (requires {
                                  configPtr->serverProtocolConfig;
                                  configPtr->serverProtocolConfig.port;
                                  configPtr->serverProtocolConfig.transportProto;
                              }) {
                    port = configPtr->serverProtocolConfig.port;
                    transportProto = configPtr->serverProtocolConfig.transportProto;
                }
            },
            protocolConfigVariant);

    QStringList fixedPorts = ContainerProps::fixedPortsForContainer(containerConfig.containerType);

    // TODO reimplement with netstat
    QString script = QString("which lsof > /dev/null 2>&1 || true && sudo lsof -i -P -n 2>/dev/null | grep -E ':%1 ").arg(port);
    for (auto &port : fixedPorts) {
        script = script.append("|:%1").arg(port);
    }

    if (transportProto == "tcpandudp") {
        QString tcpProtoScript = script;
        QString udpProtoScript = script;
        tcpProtoScript.append("' | grep -i tcp");
        udpProtoScript.append("' | grep -i udp");
        tcpProtoScript.append(" | grep LISTEN");

        ErrorCode errorCode = runScript(serverCredentials, replaceVars(tcpProtoScript, genVarsForScript(containerConfig, serverCredentials)),
                                        cbReadStdOut, cbReadStdErr);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }

        errorCode = runScript(serverCredentials, replaceVars(udpProtoScript, genVarsForScript(containerConfig, serverCredentials)),
                              cbReadStdOut, cbReadStdErr);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }

        if (!stdOut.isEmpty()) {
            return ErrorCode::ServerPortAlreadyAllocatedError;
        }
        return ErrorCode::NoError;
    }

    script = script.append("' | grep -i %1").arg(transportProto);

    if (transportProto == "tcp") {
        script = script.append(" | grep LISTEN");
    }

    ErrorCode errorCode = runScript(serverCredentials, replaceVars(script, genVarsForScript(containerConfig, serverCredentials)),
                                    cbReadStdOut, cbReadStdErr);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    if (!stdOut.isEmpty()) {
        return ErrorCode::ServerPortAlreadyAllocatedError;
    }
    return ErrorCode::NoError;
}

ErrorCode ServerController::isUserInSudo(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };
    auto cbReadStdErr = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    const QString scriptData = amnezia::scriptData(SharedScriptType::check_user_in_sudo);
    Vars vars = genVarsForScript(containerConfig, serverCredentials);
    ErrorCode error = runScript(serverCredentials, replaceVars(scriptData, vars), cbReadStdOut, cbReadStdErr);

    if (serverCredentials.userName != "root" && stdOut.contains("sudo:") && !stdOut.contains("uname:") && stdOut.contains("not found"))
        return ErrorCode::ServerSudoPackageIsNotPreinstalled;
    if (serverCredentials.userName != "root" && !stdOut.contains("sudo") && !stdOut.contains("wheel"))
        return ErrorCode::ServerUserNotInSudo;
    if (stdOut.contains("can't cd to") || stdOut.contains("Permission denied") || stdOut.contains("No such file or directory"))
        return ErrorCode::ServerUserDirectoryNotAccessible;
    if (stdOut.contains("sudoers") || stdOut.contains("is not allowed to run sudo on"))
        return ErrorCode::ServerUserNotAllowedInSudoers;
    if (stdOut.contains("password is required"))
        return ErrorCode::ServerUserPasswordRequired;

    return error;
}

ErrorCode ServerController::isServerDpkgBusy(const ContainerConfig &containerConfig, const ServerCredentials &serverCredentials)
{
    m_cancelInstallation = false;
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };
    auto cbReadStdErr = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    QFutureWatcher<ErrorCode> watcher;

    QFuture<ErrorCode> future = QtConcurrent::run([this, &stdOut, &cbReadStdOut, &cbReadStdErr, &serverCredentials, &containerConfig]() {
        // max 100 attempts
        for (int i = 0; i < 30; ++i) {
            if (m_cancelInstallation) {
                return ErrorCode::ServerCancelInstallation;
            }
            stdOut.clear();
            Vars vars = genVarsForScript(containerConfig, serverCredentials);
            runScript(serverCredentials, replaceVars(amnezia::scriptData(SharedScriptType::check_server_is_busy), vars), cbReadStdOut,
                      cbReadStdErr);

            if (stdOut.contains("Packet manager not found"))
                return ErrorCode::ServerPacketManagerError;
            if (stdOut.contains("fuser not installed") || stdOut.contains("cat not installed"))
                return ErrorCode::NoError;

            if (stdOut.isEmpty()) {
                return ErrorCode::NoError;
            } else {
#ifdef MZ_DEBUG
                qDebug().noquote() << stdOut;
#endif
                emit serverIsBusy(true);
                QThread::msleep(10000);
            }
        }
        return ErrorCode::ServerPacketManagerError;
    });

    QEventLoop wait;
    QObject::connect(&watcher, &QFutureWatcher<ErrorCode>::finished, &wait, &QEventLoop::quit);
    watcher.setFuture(future);
    wait.exec();

    emit serverIsBusy(false);

    return future.result();
}

ErrorCode ServerController::getDecryptedPrivateKey(const ServerCredentials &credentials, QString &decryptedPrivateKey,
                                                   const std::function<QString()> &callback)
{
    auto error = m_sshClient.getDecryptedPrivateKey(credentials, decryptedPrivateKey, callback);
    return error;
}
