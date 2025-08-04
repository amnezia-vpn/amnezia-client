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

#include "core/models/containers/containers_defs.h"
#include "core/networkUtilities.h"
#include "core/scripts_registry.h"
#include "core/server_defs.h"
#include "logger.h"
#include "settings.h"
#include "utilities.h"
#include "configurators/awg_configurator.h"
#include "configurators/cloak_configurator.h"
#include "configurators/ikev2_configurator.h"
#include "configurators/openvpn_configurator.h"
#include "configurators/shadowsocks_configurator.h"
#include "configurators/wireguard_configurator.h"
#include "configurators/xray_configurator.h"
#include "core/models/protocols/sftpProtocolConfig.h"
#include "core/models/protocols/socks5ProtocolConfig.h"

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

ErrorCode ServerController::runContainerScript(const ServerCredentials &credentials, DockerContainer container, QString script,
                                               const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdOut,
                                               const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdErr)
{
    QString fileName = "/opt/amnezia/" + Utils::getRandomString(16) + ".sh";

    ErrorCode e = uploadTextFileToContainer(container, credentials, script, fileName);
    if (e)
        return e;

    QString runner =
            QString("sudo docker exec -i $CONTAINER_NAME %2 %1 ").arg(fileName, (container == DockerContainer::Socks5Proxy ? "sh" : "bash"));
            e = runScript(credentials, replaceVars(runner, generateVarsForContainer(credentials, container)), cbReadStdOut, cbReadStdErr);

    QString remover = QString("sudo docker exec -i $CONTAINER_NAME rm %1 ").arg(fileName);
    runScript(credentials, replaceVars(remover, generateVarsForContainer(credentials, container)), cbReadStdOut, cbReadStdErr);

    return e;
}

ErrorCode ServerController::uploadTextFileToContainer(DockerContainer container, const ServerCredentials &credentials, const QString &file,
                                                      const QString &path, libssh::ScpOverwriteMode overwriteMode)
{
    ErrorCode e = ErrorCode::NoError;
    QString tmpFileName = QString("/tmp/%1.tmp").arg(Utils::getRandomString(16));
    e = uploadFileToHost(credentials, file.toUtf8(), tmpFileName);
    if (e)
        return e;

    QString stdOut;
    auto cbReadStd = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    // mkdir
    QString mkdir = QString("sudo docker exec -i $CONTAINER_NAME mkdir -p  \"$(dirname %1)\"").arg(path);

    e = runScript(credentials, replaceVars(mkdir, generateVarsForContainer(credentials, container)));
    if (e)
        return e;

    if (overwriteMode == libssh::ScpOverwriteMode::ScpOverwriteExisting) {
        e = runScript(credentials,
                      replaceVars(QStringLiteral("sudo docker cp %1 $CONTAINER_NAME:/%2").arg(tmpFileName, path),
                                  generateVarsForContainer(credentials, container)),
                      cbReadStd, cbReadStd);

        if (e)
            return e;
    } else if (overwriteMode == libssh::ScpOverwriteMode::ScpAppendToExisting) {
        e = runScript(credentials,
                      replaceVars(QStringLiteral("sudo docker cp %1 $CONTAINER_NAME:/%2").arg(tmpFileName, tmpFileName),
                                  generateVarsForContainer(credentials, container)),
                      cbReadStd, cbReadStd);

        if (e)
            return e;

        e = runScript(credentials,
                      replaceVars(QStringLiteral("sudo docker exec -i $CONTAINER_NAME sh -c \"cat %1 >> %2\"").arg(tmpFileName, path),
                                  generateVarsForContainer(credentials, container)),
                      cbReadStd, cbReadStd);

        if (e)
            return e;
    } else
        return ErrorCode::NotImplementedError;

    if (stdOut.contains("Error") && stdOut.contains("No such container")) {
        return ErrorCode::ServerContainerMissingError;
    }

    runScript(credentials, replaceVars(QString("sudo shred -u %1").arg(tmpFileName), generateVarsForContainer(credentials, container)));
    return e;
}

QByteArray ServerController::getTextFileFromContainer(DockerContainer container, const ServerCredentials &credentials, const QString &path,
                                                      ErrorCode &errorCode)
{

    errorCode = ErrorCode::NoError;

    QString script = QStringLiteral("sudo docker exec -i %1 sh -c \"xxd -p '%2'\"").arg(ContainerProps::containerToString(container), path);

    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data;
        return ErrorCode::NoError;
    };

    errorCode = runScript(credentials, script, cbReadStdOut);
    return QByteArray::fromHex(stdOut.toUtf8());
}

ErrorCode ServerController::uploadFileToHost(const ServerCredentials &credentials, const QByteArray &data, const QString &remotePath,
                                             libssh::ScpOverwriteMode overwriteMode)
{
    auto error = m_sshClient.connectToHost(credentials);
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

ErrorCode ServerController::rebootServer(const ServerCredentials &credentials)
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

    return runScript(credentials, script, cbReadStdOut, cbReadStdErr);
}

ErrorCode ServerController::removeAllContainers(const ServerCredentials &credentials)
{
    return runScript(credentials, amnezia::scriptData(SharedScriptType::remove_all_containers));
}

ErrorCode ServerController::removeContainer(const ServerCredentials &credentials, DockerContainer container)
{
    return runScript(credentials,
                     replaceVars(amnezia::scriptData(SharedScriptType::remove_container), generateVarsForContainer(credentials, container)));
}

ErrorCode ServerController::setupContainer(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config, bool isUpdate)
{
    qDebug().noquote() << "ServerController::setupContainer" << ContainerProps::containerToString(container);
    ErrorCode e = ErrorCode::NoError;

    e = isUserInSudo(credentials, container);
    if (e)
        return e;

    e = isServerDpkgBusy(credentials, container);
    if (e)
        return e;

    e = installDockerWorker(credentials, container);
    if (e)
        return e;
    qDebug().noquote() << "ServerController::setupContainer installDockerWorker finished";

    if (!isUpdate) {
        e = isServerPortBusy(credentials, container, config);
        if (e)
            return e;
    }

    if (!isUpdate) {
        e = isServerPortBusy(credentials, container, config);
        if (e)
            return e;
    }

    e = prepareHostWorker(credentials, container, config);
    if (e)
        return e;
    qDebug().noquote() << "ServerController::setupContainer prepareHostWorker finished";

    removeContainer(credentials, container);
    qDebug().noquote() << "ServerController::setupContainer removeContainer finished";

    qDebug().noquote() << "buildContainerWorker start";
    e = buildContainerWorker(credentials, container, config);
    if (e)
        return e;
    qDebug().noquote() << "ServerController::setupContainer buildContainerWorker finished";

    e = runContainerWorker(credentials, container, config);
    if (e)
        return e;
    qDebug().noquote() << "ServerController::setupContainer runContainerWorker finished";

    e = configureContainerWorker(credentials, container, config);
    if (e)
        return e;
    qDebug().noquote() << "ServerController::setupContainer configureContainerWorker finished";

    setupServerFirewall(credentials);
    qDebug().noquote() << "ServerController::setupContainer setupServerFirewall finished";

    return startupContainerWorker(credentials, container, config);
}

ErrorCode ServerController::updateContainer(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &oldConfig,
                                            QJsonObject &newConfig)
{
    bool reinstallRequired = isReinstallContainerRequired(container, oldConfig, newConfig);
    qDebug() << "ServerController::updateContainer for container" << container << "reinstall required is" << reinstallRequired;

    if (reinstallRequired) {
        return setupContainer(credentials, container, newConfig, true);
    } else {
        ErrorCode e = configureContainerWorker(credentials, container, newConfig);
        if (e)
            return e;

        return startupContainerWorker(credentials, container, newConfig);
    }
}

bool ServerController::isReinstallContainerRequired(DockerContainer container, const QJsonObject &oldConfig, const QJsonObject &newConfig)
{
    Proto mainProto = ContainerProps::defaultProtocol(container);

    const QJsonObject &oldProtoConfig = oldConfig.value(ProtocolProps::protoToString(mainProto)).toObject();
    const QJsonObject &newProtoConfig = newConfig.value(ProtocolProps::protoToString(mainProto)).toObject();

    if (container == DockerContainer::OpenVpn) {
        if (oldProtoConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto)
            != newProtoConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto))
            return true;

        if (oldProtoConfig.value(config_key::port).toString(protocols::openvpn::defaultPort)
            != newProtoConfig.value(config_key::port).toString(protocols::openvpn::defaultPort))
            return true;
    }

    if (container == DockerContainer::Cloak) {
        if (oldProtoConfig.value(config_key::port).toString(protocols::cloak::defaultPort)
            != newProtoConfig.value(config_key::port).toString(protocols::cloak::defaultPort))
            return true;
    }

    if (container == DockerContainer::ShadowSocks) {
        if (oldProtoConfig.value(config_key::port).toString(protocols::shadowsocks::defaultPort)
            != newProtoConfig.value(config_key::port).toString(protocols::shadowsocks::defaultPort))
            return true;
    }

    if (container == DockerContainer::Awg) {
        if ((oldProtoConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress)
             != newProtoConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress))
            || (oldProtoConfig.value(config_key::port).toString(protocols::awg::defaultPort)
                != newProtoConfig.value(config_key::port).toString(protocols::awg::defaultPort))
            || (oldProtoConfig.value(config_key::junkPacketCount).toString(protocols::awg::defaultJunkPacketCount)
                != newProtoConfig.value(config_key::junkPacketCount).toString(protocols::awg::defaultJunkPacketCount))
            || (oldProtoConfig.value(config_key::junkPacketMinSize).toString(protocols::awg::defaultJunkPacketMinSize)
                != newProtoConfig.value(config_key::junkPacketMinSize).toString(protocols::awg::defaultJunkPacketMinSize))
            || (oldProtoConfig.value(config_key::junkPacketMaxSize).toString(protocols::awg::defaultJunkPacketMaxSize)
                != newProtoConfig.value(config_key::junkPacketMaxSize).toString(protocols::awg::defaultJunkPacketMaxSize))
            || (oldProtoConfig.value(config_key::initPacketJunkSize).toString(protocols::awg::defaultInitPacketJunkSize)
                != newProtoConfig.value(config_key::initPacketJunkSize).toString(protocols::awg::defaultInitPacketJunkSize))
            || (oldProtoConfig.value(config_key::responsePacketJunkSize).toString(protocols::awg::defaultResponsePacketJunkSize)
                != newProtoConfig.value(config_key::responsePacketJunkSize).toString(protocols::awg::defaultResponsePacketJunkSize))
            || (oldProtoConfig.value(config_key::initPacketMagicHeader).toString(protocols::awg::defaultInitPacketMagicHeader)
                != newProtoConfig.value(config_key::initPacketMagicHeader).toString(protocols::awg::defaultInitPacketMagicHeader))
            || (oldProtoConfig.value(config_key::responsePacketMagicHeader).toString(protocols::awg::defaultResponsePacketMagicHeader)
                != newProtoConfig.value(config_key::responsePacketMagicHeader).toString(protocols::awg::defaultResponsePacketMagicHeader))
            || (oldProtoConfig.value(config_key::underloadPacketMagicHeader).toString(protocols::awg::defaultUnderloadPacketMagicHeader)
                != newProtoConfig.value(config_key::underloadPacketMagicHeader).toString(protocols::awg::defaultUnderloadPacketMagicHeader))
            || (oldProtoConfig.value(config_key::transportPacketMagicHeader).toString(protocols::awg::defaultTransportPacketMagicHeader))
                    != newProtoConfig.value(config_key::transportPacketMagicHeader).toString(protocols::awg::defaultTransportPacketMagicHeader))
            // || (oldProtoConfig.value(config_key::cookieReplyPacketJunkSize).toString(protocols::awg::defaultCookieReplyPacketJunkSize)
            //     != newProtoConfig.value(config_key::cookieReplyPacketJunkSize).toString(protocols::awg::defaultCookieReplyPacketJunkSize))
            // || (oldProtoConfig.value(config_key::transportPacketJunkSize).toString(protocols::awg::defaultTransportPacketJunkSize)
            //     != newProtoConfig.value(config_key::transportPacketJunkSize).toString(protocols::awg::defaultTransportPacketJunkSize))

            return true;
    }

    if (container == DockerContainer::WireGuard) {
        if ((oldProtoConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress)
             != newProtoConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress))
            || (oldProtoConfig.value(config_key::port).toString(protocols::wireguard::defaultPort)
                != newProtoConfig.value(config_key::port).toString(protocols::wireguard::defaultPort)))
            return true;
    }

    if (container == DockerContainer::Socks5Proxy) {
        return true;
    }

    if (container == DockerContainer::Xray) {
        if (oldProtoConfig.value(config_key::port).toString(protocols::xray::defaultPort)
            != newProtoConfig.value(config_key::port).toString(protocols::xray::defaultPort)) {
            return true;
        }
    }

    return false;
}

ErrorCode ServerController::installDockerWorker(const ServerCredentials &credentials, DockerContainer container)
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

    ErrorCode error =
            runScript(credentials, replaceVars(amnezia::scriptData(SharedScriptType::install_docker), generateVarsForContainer(credentials, DockerContainer::None)),
                      cbReadStdOut, cbReadStdErr);

    qDebug().noquote() << "ServerController::installDockerWorker" << stdOut;
    if (stdOut.contains("lock"))
        return ErrorCode::ServerPacketManagerError;
    if (stdOut.contains("command not found"))
        return ErrorCode::ServerDockerFailedError;

    return error;
}

ErrorCode ServerController::prepareHostWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    // create folder on host
    return runScript(credentials, replaceVars(amnezia::scriptData(SharedScriptType::prepare_host), generateVarsForContainer(credentials, container)));
}

ErrorCode ServerController::buildContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    QString dockerFilePath = amnezia::server::getDockerfileFolder(container) + "/Dockerfile";
    QString scriptString = QString("sudo rm %1").arg(dockerFilePath);
    ErrorCode errorCode = runScript(credentials, replaceVars(scriptString, generateVarsForContainer(credentials, container)));
    if (errorCode)
        return errorCode;

    errorCode = uploadFileToHost(credentials, amnezia::scriptData(ProtocolScriptType::dockerfile, container).toUtf8(), dockerFilePath);

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

    ErrorCode error =
            runScript(credentials,
                      replaceVars(amnezia::scriptData(SharedScriptType::build_container), generateVarsForContainer(credentials, container, config)),
                      cbReadStdOut, cbReadStdErr);

    if (stdOut.contains("doesn't work on cgroups v2"))
        return ErrorCode::ServerDockerOnCgroupsV2;
    if (stdOut.contains("cgroup mountpoint does not exist"))
        return ErrorCode::ServerCgroupMountpoint;
    if (stdOut.contains("have reached") && stdOut.contains("pull rate limit"))
        return ErrorCode::DockerPullRateLimit;

    return error;
}

ErrorCode ServerController::runContainerWorker(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    ErrorCode e = runScript(credentials,
                            replaceVars(amnezia::scriptData(ProtocolScriptType::run_container, container),
                                        generateVarsForContainer(credentials, container, config)),
                            cbReadStdOut);

    if (stdOut.contains("address already in use"))
        return ErrorCode::ServerPortAlreadyAllocatedError;
    if (stdOut.contains("is already in use by container"))
        return ErrorCode::ServerPortAlreadyAllocatedError;
    if (stdOut.contains("invalid publish"))
        return ErrorCode::ServerDockerFailedError;

    return e;
}

ErrorCode ServerController::configureContainerWorker(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config)
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

    ErrorCode e = runContainerScript(credentials, container,
                                     replaceVars(amnezia::scriptData(ProtocolScriptType::configure_container, container),
                                                 generateVarsForContainer(credentials, container, config)),
                                     cbReadStdOut, cbReadStdErr);

    VpnConfigurationsController::updateContainerConfigAfterInstallation(container, config, stdOut);

    return e;
}

ErrorCode ServerController::startupContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    QString script = amnezia::scriptData(ProtocolScriptType::container_startup, container);

    if (script.isEmpty()) {
        return ErrorCode::NoError;
    }

    ErrorCode e = uploadTextFileToContainer(container, credentials, replaceVars(script, generateVarsForContainer(credentials, container, config)),
                                            "/opt/amnezia/start.sh");
    if (e)
        return e;

    return runScript(credentials,
                     replaceVars("sudo docker exec -d $CONTAINER_NAME sh -c \"chmod a+x /opt/amnezia/start.sh && "
                                 "/opt/amnezia/start.sh\"",
                                 generateVarsForContainer(credentials, container, config)));
}



ServerController::Vars ServerController::generateVarsForContainer(const ServerCredentials &credentials, DockerContainer container,
                                                                  const QJsonObject &config)
{
    // For VPN containers, use configurator pattern
    if (ContainerProps::containerService(container) != ServiceType::Other) {
        for (Proto protocol : ContainerProps::protocolsForContainer(container)) {
            QScopedPointer<ConfiguratorBase> configurator;
            
            // Create the appropriate configurator for this protocol
            switch (protocol) {
            case Proto::OpenVpn:
                configurator.reset(new OpenVpnConfigurator(m_settings, QSharedPointer<ServerController>(this, [](ServerController*){})));
                break;
            case Proto::ShadowSocks:
                configurator.reset(new ShadowSocksConfigurator(m_settings, QSharedPointer<ServerController>(this, [](ServerController*){})));
                break;
            case Proto::Cloak:
                configurator.reset(new CloakConfigurator(m_settings, QSharedPointer<ServerController>(this, [](ServerController*){})));
                break;
            case Proto::WireGuard:
                configurator.reset(new WireguardConfigurator(m_settings, QSharedPointer<ServerController>(this, [](ServerController*){}), false));
                break;
            case Proto::Awg:
                configurator.reset(new AwgConfigurator(m_settings, QSharedPointer<ServerController>(this, [](ServerController*){})));
                break;
            case Proto::Ikev2:
                configurator.reset(new Ikev2Configurator(m_settings, QSharedPointer<ServerController>(this, [](ServerController*){})));
                break;
            case Proto::Xray:
            case Proto::SSXray:
                configurator.reset(new XrayConfigurator(m_settings, QSharedPointer<ServerController>(this, [](ServerController*){})));
                break;
            default:
                continue;
            }
            
            if (configurator) {
                return configurator->generateProtocolVars(credentials, container, config);
            }
        }
    }

    // Handle non-VPN services (SFTP, Socks5) with direct variable generation
    Vars vars;
    
    // Common variables that apply to all containers
    vars.append({{"$REMOTE_HOST", credentials.hostName}});
    vars.append({{"$CONTAINER_NAME", ContainerProps::containerToString(container)}});
    vars.append({{"$DOCKERFILE_FOLDER", "/opt/amnezia/" + ContainerProps::containerToString(container)}});
    vars.append({{"$PRIMARY_SERVER_DNS", m_settings->primaryDns()}});
    vars.append({{"$SECONDARY_SERVER_DNS", m_settings->secondaryDns()}});

    QString serverIp = (container != DockerContainer::Awg && container != DockerContainer::WireGuard && container != DockerContainer::Xray)
            ? NetworkUtilities::getIPAddress(credentials.hostName)
            : credentials.hostName;
    if (!serverIp.isEmpty()) {
        vars.append({{"$SERVER_IP_ADDRESS", serverIp}});
    }

    // Handle container-specific variables for non-VPN services
    if (container == DockerContainer::Sftp) {
        SftpProtocolConfig protocolConfig(config.value(ProtocolProps::protoToString(Proto::Sftp)).toObject(),
                                         ProtocolProps::protoToString(Proto::Sftp));
        
        QString port = protocolConfig.serverProtocolConfig.port;
        if (port.isEmpty()) {
            port = QString::number(ProtocolProps::defaultPort(Proto::Sftp));
        }
        
        vars.append({{"$SFTP_PORT", port}});
        vars.append({{"$SFTP_USER", protocolConfig.serverProtocolConfig.userName}});
        vars.append({{"$SFTP_PASSWORD", protocolConfig.serverProtocolConfig.password}});
    } else if (container == DockerContainer::Socks5Proxy) {
        Socks5ProtocolConfig protocolConfig(config.value(ProtocolProps::protoToString(Proto::Socks5Proxy)).toObject(),
                                           ProtocolProps::protoToString(Proto::Socks5Proxy));
        
        QString port = protocolConfig.serverProtocolConfig.port;
        if (port.isEmpty()) {
            port = protocols::socks5Proxy::defaultPort;
        }
        
        vars.append({{"$SOCKS5_PROXY_PORT", port}});
        
        const QString &username = protocolConfig.serverProtocolConfig.userName;
        const QString &password = protocolConfig.serverProtocolConfig.password;
        QString socks5user = (!username.isEmpty() && !password.isEmpty()) ? QString("users %1:CL:%2").arg(username, password) : "";
        vars.append({{"$SOCKS5_USER", socks5user}});
        vars.append({{"$SOCKS5_AUTH_TYPE", socks5user.isEmpty() ? "none" : "strong"}});
    }

    return vars;
}

QString ServerController::checkSshConnection(const ServerCredentials &credentials, ErrorCode &errorCode)
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

    errorCode = runScript(credentials, amnezia::scriptData(SharedScriptType::check_connection), cbReadStdOut, cbReadStdErr);

    return stdOut;
}

void ServerController::cancelInstallation()
{
    m_cancelInstallation = true;
}

ErrorCode ServerController::setupServerFirewall(const ServerCredentials &credentials)
{
    return runScript(credentials, replaceVars(amnezia::scriptData(SharedScriptType::setup_host_firewall), generateVarsForContainer(credentials, DockerContainer::None)));
}

QString ServerController::replaceVars(const QString &script, const Vars &vars)
{
    QString s = script;
    for (const QPair<QString, QString> &var : vars) {
        s.replace(var.first, var.second);
    }
    return s;
}

ErrorCode ServerController::isServerPortBusy(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    if (container == DockerContainer::Dns) {
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

    const Proto protocol = ContainerProps::defaultProtocol(container);
    const QString containerString = ProtocolProps::protoToString(protocol);
    const QJsonObject containerConfig = config.value(containerString).toObject();

    QStringList fixedPorts = ContainerProps::fixedPortsForContainer(container);

    QString defaultPort("%1");
    QString port = containerConfig.value(config_key::port).toString(defaultPort.arg(ProtocolProps::defaultPort(protocol)));
    QString defaultTransportProto = ProtocolProps::transportProtoToString(ProtocolProps::defaultTransportProto(protocol), protocol);
    QString transportProto = containerConfig.value(config_key::transport_proto).toString(defaultTransportProto);

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

        ErrorCode errorCode =
                runScript(credentials, replaceVars(tcpProtoScript, generateVarsForContainer(credentials, container)), cbReadStdOut, cbReadStdErr);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }

        errorCode = runScript(credentials, replaceVars(udpProtoScript, generateVarsForContainer(credentials, container)), cbReadStdOut, cbReadStdErr);
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

    ErrorCode errorCode = runScript(credentials, replaceVars(script, generateVarsForContainer(credentials, container)), cbReadStdOut, cbReadStdErr);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    if (!stdOut.isEmpty()) {
        return ErrorCode::ServerPortAlreadyAllocatedError;
    }
    return ErrorCode::NoError;
}

ErrorCode ServerController::isUserInSudo(const ServerCredentials &credentials, DockerContainer container)
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
    ErrorCode error = runScript(credentials, replaceVars(scriptData, generateVarsForContainer(credentials, DockerContainer::None)), cbReadStdOut, cbReadStdErr);

    if (credentials.userName != "root" && stdOut.contains("sudo:") && !stdOut.contains("uname:") && stdOut.contains("not found"))
        return ErrorCode::ServerSudoPackageIsNotPreinstalled;
    if (credentials.userName != "root" && !stdOut.contains("sudo") && !stdOut.contains("wheel"))
        return ErrorCode::ServerUserNotInSudo;
    if (stdOut.contains("can't cd to") || stdOut.contains("Permission denied") || stdOut.contains("No such file or directory"))
        return ErrorCode::ServerUserDirectoryNotAccessible;
    if (stdOut.contains("sudoers") || stdOut.contains("is not allowed to run sudo on"))
        return ErrorCode::ServerUserNotAllowedInSudoers;
    if (stdOut.contains("password is required"))
        return ErrorCode::ServerUserPasswordRequired;

    return error;
}

ErrorCode ServerController::isServerDpkgBusy(const ServerCredentials &credentials, DockerContainer container)
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

    QFuture<ErrorCode> future = QtConcurrent::run([this, &stdOut, &cbReadStdOut, &cbReadStdErr, &credentials]() {
        // max 100 attempts
        for (int i = 0; i < 30; ++i) {
            if (m_cancelInstallation) {
                return ErrorCode::ServerCancelInstallation;
            }
            stdOut.clear();
            runScript(credentials, replaceVars(amnezia::scriptData(SharedScriptType::check_server_is_busy), generateVarsForContainer(credentials, DockerContainer::None)),
                      cbReadStdOut, cbReadStdErr);

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
