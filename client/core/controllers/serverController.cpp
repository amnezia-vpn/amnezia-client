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
    e = runScript(credentials, replaceVars(runner, genVarsForScript(credentials, container)), cbReadStdOut, cbReadStdErr);

    QString remover = QString("sudo docker exec -i $CONTAINER_NAME rm %1 ").arg(fileName);
    runScript(credentials, replaceVars(remover, genVarsForScript(credentials, container)), cbReadStdOut, cbReadStdErr);

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

    e = runScript(credentials, replaceVars(mkdir, genVarsForScript(credentials, container)));
    if (e)
        return e;

    if (overwriteMode == libssh::ScpOverwriteMode::ScpOverwriteExisting) {
        e = runScript(credentials,
                      replaceVars(QString("sudo docker cp %1 $CONTAINER_NAME:/%2").arg(tmpFileName).arg(path),
                                  genVarsForScript(credentials, container)),
                      cbReadStd, cbReadStd);

        if (e)
            return e;
    } else if (overwriteMode == libssh::ScpOverwriteMode::ScpAppendToExisting) {
        e = runScript(credentials,
                      replaceVars(QString("sudo docker cp %1 $CONTAINER_NAME:/%2").arg(tmpFileName).arg(tmpFileName),
                                  genVarsForScript(credentials, container)),
                      cbReadStd, cbReadStd);

        if (e)
            return e;

        e = runScript(credentials,
                      replaceVars(QString("sudo docker exec -i $CONTAINER_NAME sh -c \"cat %1 >> %2\"").arg(tmpFileName).arg(path),
                                  genVarsForScript(credentials, container)),
                      cbReadStd, cbReadStd);

        if (e)
            return e;
    } else
        return ErrorCode::NotImplementedError;

    if (stdOut.contains("Error") && stdOut.contains("No such container")) {
        return ErrorCode::ServerContainerMissingError;
    }

    runScript(credentials, replaceVars(QString("sudo shred -u %1").arg(tmpFileName), genVarsForScript(credentials, container)));
    return e;
}

QByteArray ServerController::getTextFileFromContainer(DockerContainer container, const ServerCredentials &credentials, const QString &path,
                                                      ErrorCode &errorCode)
{

    errorCode = ErrorCode::NoError;

    QString script = QString("sudo docker exec -i %1 sh -c \"xxd -p \'%2\'\"").arg(ContainerProps::containerToString(container)).arg(path);

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
                     replaceVars(amnezia::scriptData(SharedScriptType::remove_container), genVarsForScript(credentials, container)));
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
            || (oldProtoConfig.value(config_key::transportPacketMagicHeader).toString(protocols::awg::defaultTransportPacketMagicHeader)
                != newProtoConfig.value(config_key::transportPacketMagicHeader).toString(protocols::awg::defaultTransportPacketMagicHeader)))
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
            runScript(credentials, replaceVars(amnezia::scriptData(SharedScriptType::install_docker), genVarsForScript(credentials)),
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
    return runScript(credentials, replaceVars(amnezia::scriptData(SharedScriptType::prepare_host), genVarsForScript(credentials, container)));
}

ErrorCode ServerController::buildContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    QString dockerFilePath = amnezia::server::getDockerfileFolder(container) + "/Dockerfile";
    QString scriptString = QString("sudo rm %1").arg(dockerFilePath);
    ErrorCode errorCode = runScript(credentials, replaceVars(scriptString, genVarsForScript(credentials, container)));
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

    errorCode =
            runScript(credentials,
                      replaceVars(amnezia::scriptData(SharedScriptType::build_container), genVarsForScript(credentials, container, config)),
                      cbReadStdOut);
    if (errorCode)
        return errorCode;

    return errorCode;
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
                                        genVarsForScript(credentials, container, config)),
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
                                                 genVarsForScript(credentials, container, config)),
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

    ErrorCode e = uploadTextFileToContainer(container, credentials, replaceVars(script, genVarsForScript(credentials, container, config)),
                                            "/opt/amnezia/start.sh");
    if (e)
        return e;

    return runScript(credentials,
                     replaceVars("sudo docker exec -d $CONTAINER_NAME sh -c \"chmod a+x /opt/amnezia/start.sh && "
                                 "/opt/amnezia/start.sh\"",
                                 genVarsForScript(credentials, container, config)));
}

ServerController::Vars ServerController::genVarsForScript(const ServerCredentials &credentials, DockerContainer container,
                                                          const QJsonObject &config)
{
    const QJsonObject &openvpnConfig = config.value(ProtocolProps::protoToString(Proto::OpenVpn)).toObject();
    const QJsonObject &cloakConfig = config.value(ProtocolProps::protoToString(Proto::Cloak)).toObject();
    const QJsonObject &ssConfig = config.value(ProtocolProps::protoToString(Proto::ShadowSocks)).toObject();
    const QJsonObject &wireguarConfig = config.value(ProtocolProps::protoToString(Proto::WireGuard)).toObject();
    const QJsonObject &amneziaWireguarConfig = config.value(ProtocolProps::protoToString(Proto::Awg)).toObject();
    const QJsonObject &xrayConfig = config.value(ProtocolProps::protoToString(Proto::Xray)).toObject();
    const QJsonObject &sftpConfig = config.value(ProtocolProps::protoToString(Proto::Sftp)).toObject();
    const QJsonObject &socks5ProxyConfig = config.value(ProtocolProps::protoToString(Proto::Socks5Proxy)).toObject();

    Vars vars;

    vars.append({ { "$REMOTE_HOST", credentials.hostName } });

    // OpenVPN vars
    vars.append({ { "$OPENVPN_SUBNET_IP",
                    openvpnConfig.value(config_key::subnet_address).toString(protocols::openvpn::defaultSubnetAddress) } });
    vars.append({ { "$OPENVPN_SUBNET_CIDR", openvpnConfig.value(config_key::subnet_cidr).toString(protocols::openvpn::defaultSubnetCidr) } });
    vars.append({ { "$OPENVPN_SUBNET_MASK", openvpnConfig.value(config_key::subnet_mask).toString(protocols::openvpn::defaultSubnetMask) } });

    vars.append({ { "$OPENVPN_PORT", openvpnConfig.value(config_key::port).toString(protocols::openvpn::defaultPort) } });
    vars.append({ { "$OPENVPN_TRANSPORT_PROTO",
                    openvpnConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto) } });

    bool isNcpDisabled = openvpnConfig.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable);
    vars.append({ { "$OPENVPN_NCP_DISABLE", isNcpDisabled ? protocols::openvpn::ncpDisableString : "" } });

    vars.append({ { "$OPENVPN_CIPHER", openvpnConfig.value(config_key::cipher).toString(protocols::openvpn::defaultCipher) } });
    vars.append({ { "$OPENVPN_HASH", openvpnConfig.value(config_key::hash).toString(protocols::openvpn::defaultHash) } });

    bool isTlsAuth = openvpnConfig.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth);
    vars.append({ { "$OPENVPN_TLS_AUTH", isTlsAuth ? protocols::openvpn::tlsAuthString : "" } });
    if (!isTlsAuth) {
        // erase $OPENVPN_TA_KEY, so it will not set in OpenVpnConfigurator::genOpenVpnConfig
        vars.append({ { "$OPENVPN_TA_KEY", "" } });
    }

    vars.append({ { "$OPENVPN_ADDITIONAL_CLIENT_CONFIG",
                    openvpnConfig.value(config_key::additional_client_config).toString(protocols::openvpn::defaultAdditionalClientConfig) } });
    vars.append({ { "$OPENVPN_ADDITIONAL_SERVER_CONFIG",
                    openvpnConfig.value(config_key::additional_server_config).toString(protocols::openvpn::defaultAdditionalServerConfig) } });

    // ShadowSocks vars
    vars.append({ { "$SHADOWSOCKS_SERVER_PORT", ssConfig.value(config_key::port).toString(protocols::shadowsocks::defaultPort) } });
    vars.append({ { "$SHADOWSOCKS_LOCAL_PORT",
                    ssConfig.value(config_key::local_port).toString(protocols::shadowsocks::defaultLocalProxyPort) } });
    vars.append({ { "$SHADOWSOCKS_CIPHER", ssConfig.value(config_key::cipher).toString(protocols::shadowsocks::defaultCipher) } });

    vars.append({ { "$CONTAINER_NAME", ContainerProps::containerToString(container) } });
    vars.append({ { "$DOCKERFILE_FOLDER", "/opt/amnezia/" + ContainerProps::containerToString(container) } });

    // Cloak vars
    vars.append({ { "$CLOAK_SERVER_PORT", cloakConfig.value(config_key::port).toString(protocols::cloak::defaultPort) } });
    vars.append({ { "$FAKE_WEB_SITE_ADDRESS", cloakConfig.value(config_key::site).toString(protocols::cloak::defaultRedirSite) } });

    // Xray vars
    vars.append({ { "$XRAY_SITE_NAME", xrayConfig.value(config_key::site).toString(protocols::xray::defaultSite) } });
    vars.append({ { "$XRAY_SERVER_PORT", xrayConfig.value(config_key::port).toString(protocols::xray::defaultPort) } });

    // Wireguard vars
    vars.append({ { "$WIREGUARD_SUBNET_IP",
                    wireguarConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress) } });
    vars.append({ { "$WIREGUARD_SUBNET_CIDR",
                    wireguarConfig.value(config_key::subnet_cidr).toString(protocols::wireguard::defaultSubnetCidr) } });
    vars.append({ { "$WIREGUARD_SUBNET_MASK",
                    wireguarConfig.value(config_key::subnet_mask).toString(protocols::wireguard::defaultSubnetMask) } });

    vars.append({ { "$WIREGUARD_SERVER_PORT", wireguarConfig.value(config_key::port).toString(protocols::wireguard::defaultPort) } });

    // IPsec vars
    vars.append({ { "$IPSEC_VPN_L2TP_NET", "192.168.42.0/24" } });
    vars.append({ { "$IPSEC_VPN_L2TP_POOL", "192.168.42.10-192.168.42.250" } });
    vars.append({ { "$IPSEC_VPN_L2TP_LOCAL", "192.168.42.1" } });

    vars.append({ { "$IPSEC_VPN_XAUTH_NET", "192.168.43.0/24" } });
    vars.append({ { "$IPSEC_VPN_XAUTH_POOL", "192.168.43.10-192.168.43.250" } });

    vars.append({ { "$IPSEC_VPN_SHA2_TRUNCBUG", "yes" } });

    vars.append({ { "$IPSEC_VPN_VPN_ANDROID_MTU_FIX", "yes" } });
    vars.append({ { "$IPSEC_VPN_DISABLE_IKEV2", "no" } });
    vars.append({ { "$IPSEC_VPN_DISABLE_L2TP", "no" } });
    vars.append({ { "$IPSEC_VPN_DISABLE_XAUTH", "no" } });

    vars.append({ { "$IPSEC_VPN_C2C_TRAFFIC", "no" } });

    vars.append({ { "$PRIMARY_SERVER_DNS", m_settings->primaryDns() } });
    vars.append({ { "$SECONDARY_SERVER_DNS", m_settings->secondaryDns() } });

    // Sftp vars
    vars.append({ { "$SFTP_PORT", sftpConfig.value(config_key::port).toString(QString::number(ProtocolProps::defaultPort(Proto::Sftp))) } });
    vars.append({ { "$SFTP_USER", sftpConfig.value(config_key::userName).toString() } });
    vars.append({ { "$SFTP_PASSWORD", sftpConfig.value(config_key::password).toString() } });

    // Amnezia wireguard vars
    vars.append({ { "$AWG_SUBNET_IP",
                    amneziaWireguarConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress) } });
    vars.append({ { "$AWG_SERVER_PORT", amneziaWireguarConfig.value(config_key::port).toString(protocols::awg::defaultPort) } });

    vars.append({ { "$JUNK_PACKET_COUNT", amneziaWireguarConfig.value(config_key::junkPacketCount).toString() } });
    vars.append({ { "$JUNK_PACKET_MIN_SIZE", amneziaWireguarConfig.value(config_key::junkPacketMinSize).toString() } });
    vars.append({ { "$JUNK_PACKET_MAX_SIZE", amneziaWireguarConfig.value(config_key::junkPacketMaxSize).toString() } });
    vars.append({ { "$INIT_PACKET_JUNK_SIZE", amneziaWireguarConfig.value(config_key::initPacketJunkSize).toString() } });
    vars.append({ { "$RESPONSE_PACKET_JUNK_SIZE", amneziaWireguarConfig.value(config_key::responsePacketJunkSize).toString() } });
    vars.append({ { "$INIT_PACKET_MAGIC_HEADER", amneziaWireguarConfig.value(config_key::initPacketMagicHeader).toString() } });
    vars.append({ { "$RESPONSE_PACKET_MAGIC_HEADER", amneziaWireguarConfig.value(config_key::responsePacketMagicHeader).toString() } });
    vars.append({ { "$UNDERLOAD_PACKET_MAGIC_HEADER", amneziaWireguarConfig.value(config_key::underloadPacketMagicHeader).toString() } });
    vars.append({ { "$TRANSPORT_PACKET_MAGIC_HEADER", amneziaWireguarConfig.value(config_key::transportPacketMagicHeader).toString() } });

    // Socks5 proxy vars
    vars.append({ { "$SOCKS5_PROXY_PORT", socks5ProxyConfig.value(config_key::port).toString(protocols::socks5Proxy::defaultPort) } });
    auto username = socks5ProxyConfig.value(config_key::userName).toString();
    auto password = socks5ProxyConfig.value(config_key::password).toString();
    QString socks5user = (!username.isEmpty() && !password.isEmpty()) ? QString("users %1:CL:%2").arg(username, password) : "";
    vars.append({ { "$SOCKS5_USER", socks5user } });
    vars.append({ { "$SOCKS5_AUTH_TYPE", socks5user.isEmpty() ? "none" : "strong" } });

    QString serverIp = (container != DockerContainer::Awg && container != DockerContainer::WireGuard && container != DockerContainer::Xray)
            ? NetworkUtilities::getIPAddress(credentials.hostName)
            : credentials.hostName;
    if (!serverIp.isEmpty()) {
        vars.append({ { "$SERVER_IP_ADDRESS", serverIp } });
    } else {
        qWarning() << "ServerController::genVarsForScript unable to resolve address for credentials.hostName";
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
    return runScript(credentials, replaceVars(amnezia::scriptData(SharedScriptType::setup_host_firewall), genVarsForScript(credentials)));
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
    QString script = QString("which lsof &>/dev/null || true && sudo lsof -i -P -n 2>/dev/null | grep -E ':%1 ").arg(port);
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
                runScript(credentials, replaceVars(tcpProtoScript, genVarsForScript(credentials, container)), cbReadStdOut, cbReadStdErr);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }

        errorCode = runScript(credentials, replaceVars(udpProtoScript, genVarsForScript(credentials, container)), cbReadStdOut, cbReadStdErr);
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

    ErrorCode errorCode = runScript(credentials, replaceVars(script, genVarsForScript(credentials, container)), cbReadStdOut, cbReadStdErr);
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
    if (credentials.userName == "root") {
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

    const QString scriptData = amnezia::scriptData(SharedScriptType::check_user_in_sudo);
    ErrorCode error = runScript(credentials, replaceVars(scriptData, genVarsForScript(credentials)), cbReadStdOut, cbReadStdErr);

    if (!stdOut.contains("sudo"))
        return ErrorCode::ServerUserNotInSudo;

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
            runScript(credentials, replaceVars(amnezia::scriptData(SharedScriptType::check_server_is_busy), genVarsForScript(credentials)),
                      cbReadStdOut, cbReadStdErr);

            if (stdOut.contains("Packet manager not found"))
                return ErrorCode::ServerPacketManagerError;
            if (stdOut.contains("fuser not installed"))
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
