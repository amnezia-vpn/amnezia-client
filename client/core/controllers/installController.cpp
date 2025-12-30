#include "installController.h"

#include <QDebug>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QRegularExpression>
#include <QThread>
#include <QtConcurrent>

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"
#include "core/controllers/vpnConfigurationController.h"
#include "core/installers/awgInstaller.h"
#include "core/installers/cloakInstaller.h"
#include "core/installers/installerBase.h"
#include "core/installers/openvpnInstaller.h"
#include "core/installers/sftpInstaller.h"
#include "core/installers/shadowsocksInstaller.h"
#include "core/installers/socks5Installer.h"
#include "core/installers/torInstaller.h"
#include "core/installers/wireguardInstaller.h"
#include "core/installers/xrayInstaller.h"
#include "core/networkUtilities.h"
#include "core/api/apiUtils.h"
#include "core/repositories/serversRepository.h"
#include "core/scripts_registry.h"
#include "core/sshclient.h"
#include "logger.h"
#include "protocols/protocols_defs.h"
#include "settings.h"
#include "ui/models/protocols/awgConfigModel.h"
#include "ui/models/protocols/wireguardConfigModel.h"
#include "utilities.h"
#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>
#ifdef Q_OS_WINDOWS
    #include <windows.h>
#endif

using namespace amnezia;

namespace
{
    Logger logger("InstallController");
}

InstallController::InstallController(ServerController *serverController,
                                     ServersRepository *serversRepository,
                                     const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent),
      m_serverController(serverController),
      m_serversRepository(serversRepository),
      m_settings(settings),
      m_cancelInstallation(false)
{
}

InstallController::~InstallController()
{
    stopAllSftpMounts();
}

ErrorCode InstallController::setupContainer(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config,
                                            bool isUpdate)
{
    qDebug().noquote() << "InstallController::setupContainer" << ContainerProps::containerToString(container);
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
    qDebug().noquote() << "InstallController::setupContainer installDockerWorker finished";

    if (!isUpdate) {
        e = isServerPortBusy(credentials, container, config);
        if (e)
            return e;
    }

    e = prepareHostWorker(credentials, container, config);
    if (e)
        return e;
    qDebug().noquote() << "InstallController::setupContainer prepareHostWorker finished";

    m_serverController->runScript(credentials,
                                  m_serverController->replaceVars(amnezia::scriptData(SharedScriptType::remove_container),
                                                                  amnezia::genBaseVars(credentials, container, QString(), QString())));
    qDebug().noquote() << "InstallController::setupContainer removeContainer finished";

    qDebug().noquote() << "buildContainerWorker start";
    e = buildContainerWorker(credentials, container, config);
    if (e)
        return e;
    qDebug().noquote() << "InstallController::setupContainer buildContainerWorker finished";

    e = runContainerWorker(credentials, container, config);
    if (e)
        return e;
    qDebug().noquote() << "InstallController::setupContainer runContainerWorker finished";

    e = configureContainerWorker(credentials, container, config);
    if (e)
        return e;
    qDebug().noquote() << "InstallController::setupContainer configureContainerWorker finished";

    setupServerFirewall(credentials);
    qDebug().noquote() << "InstallController::setupContainer setupServerFirewall finished";

    return startupContainerWorker(credentials, container, config);
}

ErrorCode InstallController::updateContainer(int serverIndex, DockerContainer container, const QJsonObject &oldConfig,
                                             QJsonObject &newConfig)
{
    if (!isUpdateDockerContainerRequired(container, oldConfig, newConfig)) {
        m_serversRepository->setContainerConfig(serverIndex, container, newConfig);
        return ErrorCode::NoError;
    }

    ServerCredentials credentials = m_serversRepository->serverCredentials(serverIndex);

    bool reinstallRequired = isReinstallContainerRequired(container, oldConfig, newConfig);
    qDebug() << "InstallController::updateContainer for container" << container << "reinstall required is" << reinstallRequired;

    ErrorCode errorCode = ErrorCode::NoError;
    if (reinstallRequired) {
        errorCode = setupContainer(credentials, container, newConfig, true);
    } else {
        errorCode = configureContainerWorker(credentials, container, newConfig);
        if (errorCode == ErrorCode::NoError) {
            errorCode = startupContainerWorker(credentials, container, newConfig);
        }
    }

    if (errorCode == ErrorCode::NoError) {
        clearCachedProfile(serverIndex, container);
        m_serversRepository->setContainerConfig(serverIndex, container, newConfig);
    }

    return errorCode;
}

void InstallController::clearCachedProfile(int serverIndex, DockerContainer container)
{
    if (ContainerProps::containerService(container) == ServiceType::Other) {
        return;
    }

    QJsonObject containerConfig = m_serversRepository->containerConfig(serverIndex, container);
    ServerCredentials credentials = m_serversRepository->serverCredentials(serverIndex);

    m_serversRepository->clearLastConnectionConfig(serverIndex, container);

    emit clientRevocationRequested(containerConfig, container, credentials, serverIndex);
}

ErrorCode InstallController::validateAndPrepareConfig(int serverIndex)
{
    QJsonObject serverConfigObject = m_serversRepository->server(serverIndex);

    if (apiUtils::isServerFromApi(serverConfigObject)) {
        return ErrorCode::NoError;
    }

    auto defaultContainerString = serverConfigObject.value(config_key::defaultContainer).toString();
    DockerContainer container = ContainerProps::containerFromString(defaultContainerString);

    if (container == DockerContainer::None) {
        return ErrorCode::NoInstalledContainersError;
    }

    QJsonObject containerConfig = m_serversRepository->containerConfig(serverIndex, container);
    ServerCredentials credentials = m_serversRepository->serverCredentials(serverIndex);

    auto isProtocolConfigExists = [](const QJsonObject &containerConfig, const DockerContainer container) {
        for (Proto protocol : ContainerProps::protocolsForContainer(container)) {
            QString protocolConfig =
                    containerConfig.value(ProtocolProps::protoToString(protocol)).toObject().value(config_key::last_config).toString();

            if (protocolConfig.isEmpty()) {
                return false;
            }
        }
        return true;
    };

    if (!isProtocolConfigExists(containerConfig, container)) {
        ErrorCode errorCode = prepareContainerConfig(container, credentials, containerConfig, serverIndex);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }
    }

    return ErrorCode::NoError;
}

ErrorCode InstallController::prepareContainerConfig(DockerContainer container, const ServerCredentials &credentials, QJsonObject &containerConfig, int serverIndex)
{
    if (!ContainerProps::isSupportedByCurrentPlatform(container)) {
        return ErrorCode::NoError;
    }

    QSharedPointer<ServerController> serverController(m_serverController, [](ServerController *) { }); // non-owning pointer
    VpnConfigurationsController vpnConfigurationsController(m_settings, serverController);
    
    ErrorCode errorCode = vpnConfigurationsController.createProtocolConfigForContainer(credentials, container, containerConfig);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    if (serverIndex >= 0) {
        m_serversRepository->setContainerConfig(serverIndex, container, containerConfig);
    }

    QString clientName = QString("Admin [%1]").arg(QSysInfo::prettyProductName());
    emit clientAppendRequested(container, credentials, containerConfig, clientName);

    return ErrorCode::NoError;
}

ErrorCode InstallController::buildContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
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

    amnezia::ScriptVars baseVars = amnezia::genBaseVars(credentials, container, QString(), QString());
    amnezia::ScriptVars protocolVars = amnezia::genProtocolVarsForContainer(container, config);
    baseVars.append(protocolVars);
    ErrorCode error = m_serverController->runScript(
            credentials, m_serverController->replaceVars(amnezia::scriptData(SharedScriptType::build_container), baseVars), cbReadStdOut,
            cbReadStdErr);

    if (stdOut.contains("doesn't work on cgroups v2"))
        return ErrorCode::ServerDockerOnCgroupsV2;
    if (stdOut.contains("cgroup mountpoint does not exist"))
        return ErrorCode::ServerCgroupMountpoint;
    if (stdOut.contains("have reached") && stdOut.contains("pull rate limit"))
        return ErrorCode::DockerPullRateLimit;

    return error;
}

ErrorCode InstallController::runContainerWorker(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    amnezia::ScriptVars baseVars = amnezia::genBaseVars(credentials, container, QString(), QString());
    amnezia::ScriptVars protocolVars = amnezia::genProtocolVarsForContainer(container, config);
    baseVars.append(protocolVars);
    ErrorCode e = m_serverController->runScript(
            credentials, m_serverController->replaceVars(amnezia::scriptData(ProtocolScriptType::run_container, container), baseVars),
            cbReadStdOut);

    if (stdOut.contains("address already in use"))
        return ErrorCode::ServerPortAlreadyAllocatedError;
    if (stdOut.contains("is already in use by container"))
        return ErrorCode::ServerPortAlreadyAllocatedError;
    if (stdOut.contains("invalid publish"))
        return ErrorCode::ServerDockerFailedError;

    return e;
}

ErrorCode InstallController::configureContainerWorker(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config)
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

    amnezia::ScriptVars baseVars = amnezia::genBaseVars(credentials, container, QString(), QString());
    amnezia::ScriptVars protocolVars = amnezia::genProtocolVarsForContainer(container, config);
    baseVars.append(protocolVars);
    ErrorCode e = m_serverController->runContainerScript(
            credentials, container,
            m_serverController->replaceVars(amnezia::scriptData(ProtocolScriptType::configure_container, container), baseVars),
            cbReadStdOut, cbReadStdErr);

    VpnConfigurationsController::updateContainerConfigAfterInstallation(container, config, stdOut);

    return e;
}

ErrorCode InstallController::startupContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    QString script = amnezia::scriptData(ProtocolScriptType::container_startup, container);

    if (script.isEmpty()) {
        return ErrorCode::NoError;
    }

    amnezia::ScriptVars baseVars = amnezia::genBaseVars(credentials, container, QString(), QString());
    amnezia::ScriptVars protocolVars = amnezia::genProtocolVarsForContainer(container, config);
    baseVars.append(protocolVars);
    ErrorCode e = m_serverController->uploadTextFileToContainer(container, credentials, m_serverController->replaceVars(script, baseVars),
                                                                "/opt/amnezia/start.sh");
    if (e)
        return e;

    return m_serverController->runScript(
            credentials,
            m_serverController->replaceVars("sudo docker exec -d $CONTAINER_NAME sh -c \"chmod a+x /opt/amnezia/start.sh && "
                                            "/opt/amnezia/start.sh\"",
                                            baseVars));
}

ErrorCode InstallController::isServerPortBusy(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
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

        ErrorCode errorCode = m_serverController->runScript(
                credentials,
                m_serverController->replaceVars(tcpProtoScript, amnezia::genBaseVars(credentials, container, QString(), QString())),
                cbReadStdOut, cbReadStdErr);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }

        errorCode = m_serverController->runScript(
                credentials,
                m_serverController->replaceVars(udpProtoScript, amnezia::genBaseVars(credentials, container, QString(), QString())),
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

    ErrorCode errorCode = m_serverController->runScript(
            credentials, m_serverController->replaceVars(script, amnezia::genBaseVars(credentials, container, QString(), QString())),
            cbReadStdOut, cbReadStdErr);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    if (!stdOut.isEmpty()) {
        return ErrorCode::ServerPortAlreadyAllocatedError;
    }
    return ErrorCode::NoError;
}

bool InstallController::isReinstallContainerRequired(DockerContainer container, const QJsonObject &oldConfig, const QJsonObject &newConfig)
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
                != newProtoConfig.value(config_key::transportPacketMagicHeader).toString(protocols::awg::defaultTransportPacketMagicHeader))
            || (oldProtoConfig.value(config_key::cookieReplyPacketJunkSize).toString(protocols::awg::defaultCookieReplyPacketJunkSize)
                != newProtoConfig.value(config_key::cookieReplyPacketJunkSize).toString(protocols::awg::defaultCookieReplyPacketJunkSize))
            || (oldProtoConfig.value(config_key::transportPacketJunkSize).toString(protocols::awg::defaultTransportPacketJunkSize)
                != newProtoConfig.value(config_key::transportPacketJunkSize).toString(protocols::awg::defaultTransportPacketJunkSize)))
            return true;
    }

    if (container == DockerContainer::WireGuard) {
        if ((oldProtoConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress)
             != newProtoConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress))
            || (oldProtoConfig.value(config_key::port).toString(protocols::wireguard::defaultPort)
                != newProtoConfig.value(config_key::port).toString(protocols::wireguard::defaultPort)))
            return true;
    }

    if (container == DockerContainer::Xray) {
        if (oldProtoConfig.value(config_key::port).toString(protocols::xray::defaultPort)
            != newProtoConfig.value(config_key::port).toString(protocols::xray::defaultPort))
            return true;
    }

    return false;
}

void InstallController::cancelInstallation()
{
    m_cancelInstallation = true;
}

ErrorCode InstallController::installDockerWorker(const ServerCredentials &credentials, DockerContainer container)
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

    ErrorCode error = m_serverController->runScript(
            credentials,
            m_serverController->replaceVars(amnezia::scriptData(SharedScriptType::install_docker),
                                            amnezia::genBaseVars(credentials, DockerContainer::None, QString(), QString())),
            cbReadStdOut, cbReadStdErr);

    qDebug().noquote() << "InstallController::installDockerWorker" << stdOut;
    if (stdOut.contains("lock"))
        return ErrorCode::ServerPacketManagerError;
    if (stdOut.contains("command not found"))
        return ErrorCode::ServerDockerFailedError;

    return error;
}

ErrorCode InstallController::prepareHostWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    Q_UNUSED(config);
    // create folder on host
    return m_serverController->runScript(credentials,
                                         m_serverController->replaceVars(amnezia::scriptData(SharedScriptType::prepare_host),
                                                                         amnezia::genBaseVars(credentials, container, QString(), QString())));
}

ErrorCode InstallController::isUserInSudo(const ServerCredentials &credentials, DockerContainer container)
{
    Q_UNUSED(container);
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
    ErrorCode error = m_serverController->runScript(
            credentials,
            m_serverController->replaceVars(scriptData, amnezia::genBaseVars(credentials, DockerContainer::None, QString(), QString())),
            cbReadStdOut, cbReadStdErr);

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

ErrorCode InstallController::isServerDpkgBusy(const ServerCredentials &credentials, DockerContainer container)
{
    Q_UNUSED(container);
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
            m_serverController->runScript(
                    credentials,
                    m_serverController->replaceVars(amnezia::scriptData(SharedScriptType::check_server_is_busy),
                                                    amnezia::genBaseVars(credentials, DockerContainer::None, QString(), QString())),
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

ErrorCode InstallController::setupServerFirewall(const ServerCredentials &credentials)
{
    return m_serverController->runScript(
            credentials,
            m_serverController->replaceVars(amnezia::scriptData(SharedScriptType::setup_host_firewall),
                                            amnezia::genBaseVars(credentials, DockerContainer::None, QString(), QString())));
}

ErrorCode InstallController::rebootServer(int serverIndex)
{
    auto credentials = m_serversRepository->serverCredentials(serverIndex);

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

    return m_serverController->runScript(credentials, script, cbReadStdOut, cbReadStdErr);
}

ErrorCode InstallController::removeAllContainers(int serverIndex)
{
    auto credentials = m_serversRepository->serverCredentials(serverIndex);
    ErrorCode errorCode = m_serverController->runScript(credentials, amnezia::scriptData(SharedScriptType::remove_all_containers));

    if (errorCode == ErrorCode::NoError) {
        QJsonObject server = m_serversRepository->server(serverIndex);
        server.insert(config_key::containers, QJsonArray());
        server.insert(config_key::defaultContainer, ContainerProps::containerToString(DockerContainer::None));

        m_serversRepository->editServer(serverIndex, server);
    }

    return errorCode;
}

ErrorCode InstallController::removeContainer(int serverIndex, DockerContainer container)
{
    auto credentials = m_serversRepository->serverCredentials(serverIndex);
    ErrorCode errorCode = m_serverController->runScript(
            credentials,
            m_serverController->replaceVars(amnezia::scriptData(SharedScriptType::remove_container),
                                            amnezia::genBaseVars(credentials, container, QString(), QString())));

    if (errorCode == ErrorCode::NoError) {
        QJsonObject server = m_serversRepository->server(serverIndex);
        QJsonArray containers = server.value(config_key::containers).toArray();

        for (auto it = containers.begin(); it != containers.end(); it++) {
            if (it->toObject().value(config_key::container).toString() == ContainerProps::containerToString(container)) {
                containers.erase(it);
                break;
            }
        }

        server.insert(config_key::containers, containers);

        auto defaultContainer = ContainerProps::containerFromString(server.value(config_key::defaultContainer).toString());
        if (defaultContainer == container) {
            if (containers.empty()) {
                defaultContainer = DockerContainer::None;
            } else {
                defaultContainer =
                        ContainerProps::containerFromString(containers.begin()->toObject().value(config_key::container).toString());
            }
            server.insert(config_key::defaultContainer, ContainerProps::containerToString(defaultContainer));
        }

        m_serversRepository->editServer(serverIndex, server);
    }

    return errorCode;
}

QScopedPointer<InstallerBase> InstallController::createInstaller(DockerContainer container)
{
    switch (container) {
    case DockerContainer::Awg: return QScopedPointer<InstallerBase>(new AwgInstaller(this));
    case DockerContainer::WireGuard: return QScopedPointer<InstallerBase>(new WireguardInstaller(this));
    case DockerContainer::OpenVpn: return QScopedPointer<InstallerBase>(new OpenVpnInstaller(this));
    case DockerContainer::Xray:
    case DockerContainer::SSXray: return QScopedPointer<InstallerBase>(new XrayInstaller(this));
    case DockerContainer::Cloak: return QScopedPointer<InstallerBase>(new CloakInstaller(this));
    case DockerContainer::ShadowSocks: return QScopedPointer<InstallerBase>(new ShadowSocksInstaller(this));
    case DockerContainer::TorWebSite: return QScopedPointer<InstallerBase>(new TorInstaller(this));
    case DockerContainer::Sftp: return QScopedPointer<InstallerBase>(new SftpInstaller(this));
    case DockerContainer::Socks5Proxy: return QScopedPointer<InstallerBase>(new Socks5Installer(this));
    default: return QScopedPointer<InstallerBase>(new InstallerBase(this));
    }
}

QJsonObject InstallController::generateConfig(DockerContainer container, int port, TransportProto transportProto)
{
    auto installer = createInstaller(container);
    return installer->generateConfig(container, port, transportProto);
}

ErrorCode InstallController::installContainer(const ServerCredentials &credentials, DockerContainer container, int port,
                                              TransportProto transportProto, QJsonObject &config)
{
    config = generateConfig(container, port, transportProto);
    return setupContainer(credentials, container, config, false);
}


bool InstallController::isUpdateDockerContainerRequired(DockerContainer container, const QJsonObject &oldConfig, const QJsonObject &newConfig)
{
    Proto mainProto = ContainerProps::defaultProtocol(container);

    const QJsonObject &oldProtoConfig = oldConfig.value(ProtocolProps::protoToString(mainProto)).toObject();
    const QJsonObject &newProtoConfig = newConfig.value(ProtocolProps::protoToString(mainProto)).toObject();

    if (container == DockerContainer::Awg) {
        const AwgConfig oldConfig(oldProtoConfig);
        const AwgConfig newConfig(newProtoConfig);

        if (oldConfig.hasEqualServerSettings(newConfig)) {
            return false;
        }
    } else if (container == DockerContainer::WireGuard) {
        const WgConfig oldConfig(oldProtoConfig);
        const WgConfig newConfig(newProtoConfig);

        if (oldConfig.hasEqualServerSettings(newConfig)) {
            return false;
        }
    }

    return true;
}

ErrorCode InstallController::scanServerForInstalledContainers(int serverIndex)
{
    ServerCredentials credentials = m_serversRepository->serverCredentials(serverIndex);

    QMap<DockerContainer, QJsonObject> installedContainers;
    ErrorCode errorCode = getAlreadyInstalledContainers(credentials, installedContainers);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonObject server = m_serversRepository->server(serverIndex);
    QJsonArray containers = server.value(config_key::containers).toArray();
    bool hasNewContainers = false;

    for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
        QJsonObject existingConfig = m_serversRepository->containerConfig(serverIndex, iterator.key());
        if (existingConfig.isEmpty()) {
            QJsonObject containerConfig = iterator.value();

            if (ContainerProps::isSupportedByCurrentPlatform(iterator.key())) {
                errorCode = prepareContainerConfig(iterator.key(), credentials, containerConfig, serverIndex);
                if (errorCode != ErrorCode::NoError) {
                    return errorCode;
                }
            } else {
                m_serversRepository->setContainerConfig(serverIndex, iterator.key(), containerConfig);
            }

            containers.push_back(containerConfig);
            hasNewContainers = true;

            auto defaultContainer = server.value(config_key::defaultContainer).toString();
            if (ContainerProps::containerFromString(defaultContainer) == DockerContainer::None
                && ContainerProps::containerService(iterator.key()) != ServiceType::Other
                && ContainerProps::isSupportedByCurrentPlatform(iterator.key())) {
                server.insert(config_key::defaultContainer, ContainerProps::containerToString(iterator.key()));
            }
        }
    }

    if (hasNewContainers) {
        server.insert(config_key::containers, containers);
        m_serversRepository->editServer(serverIndex, server);
    }

    return ErrorCode::NoError;
}

ErrorCode InstallController::installServer(const ServerCredentials &credentials, DockerContainer container, int port,
                                           TransportProto transportProto, bool &wasContainerInstalled)
{
    QMap<DockerContainer, QJsonObject> installedContainers;
    ErrorCode errorCode = getAlreadyInstalledContainers(credentials, installedContainers);
    if (errorCode) {
        return errorCode;
    }

    wasContainerInstalled = false;
    if (!installedContainers.contains(container)) {
        QJsonObject config;
        errorCode = installContainer(credentials, container, port, transportProto, config);
        if (errorCode) {
            return errorCode;
        }

        installedContainers.insert(container, config);
        wasContainerInstalled = true;
    }

    QMap<DockerContainer, QJsonObject> preparedContainers;
    
    for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
        QJsonObject containerConfig = iterator.value();

        if (ContainerProps::isSupportedByCurrentPlatform(iterator.key())) {
            errorCode = prepareContainerConfig(iterator.key(), credentials, containerConfig);
            if (errorCode != ErrorCode::NoError) {
                return errorCode;
            }
        }

        preparedContainers.insert(iterator.key(), containerConfig);
    }

    QJsonObject server;
    server.insert(config_key::hostName, credentials.hostName);
    server.insert(config_key::userName, credentials.userName);
    server.insert(config_key::password, credentials.secretData);
    server.insert(config_key::port, credentials.port);
    server.insert(config_key::description, m_settings->nextAvailableServerName());

    QJsonArray containerConfigs;
    for (auto iterator = preparedContainers.begin(); iterator != preparedContainers.end(); iterator++) {
        containerConfigs.append(iterator.value());
    }

    server.insert(config_key::containers, containerConfigs);
    server.insert(config_key::defaultContainer, ContainerProps::containerToString(container));

    m_serversRepository->addServer(server);

    return ErrorCode::NoError;
}

ErrorCode InstallController::installContainer(const ServerCredentials &credentials, DockerContainer container, int port,
                                              TransportProto transportProto, int serverIndex, bool &wasContainerInstalled)
{
    QMap<DockerContainer, QJsonObject> installedContainers;
    ErrorCode errorCode = getAlreadyInstalledContainers(credentials, installedContainers);
    if (errorCode) {
        return errorCode;
    }

    wasContainerInstalled = false;
    if (!installedContainers.contains(container)) {
        QJsonObject config;
        errorCode = installContainer(credentials, container, port, transportProto, config);
        if (errorCode) {
            return errorCode;
        }

        installedContainers.insert(container, config);
        wasContainerInstalled = true;
    }

    for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
        QJsonObject existingConfig = m_serversRepository->containerConfig(serverIndex, iterator.key());
        if (existingConfig.isEmpty()) {
            QJsonObject containerConfig = iterator.value();
            
            if (ContainerProps::isSupportedByCurrentPlatform(iterator.key())) {
                errorCode = prepareContainerConfig(iterator.key(), credentials, containerConfig, serverIndex);
                if (errorCode != ErrorCode::NoError) {
                    return errorCode;
                }
            } else {
                m_serversRepository->setContainerConfig(serverIndex, iterator.key(), containerConfig);
            }
        }
    }

    return ErrorCode::NoError;
}

ErrorCode InstallController::checkSshConnection(const ServerCredentials &credentials, QString &output,
                                                std::function<QString()> passphraseCallback)
{
    ErrorCode errorCode = ErrorCode::NoError;

    ServerCredentials processedCredentials = credentials;

    if (processedCredentials.secretData.contains("BEGIN") && processedCredentials.secretData.contains("PRIVATE KEY")) {
        if (!passphraseCallback) {
            return ErrorCode::SshPrivateKeyError;
        }

        QString decryptedPrivateKey;
        errorCode = m_serverController->getDecryptedPrivateKey(processedCredentials, decryptedPrivateKey, passphraseCallback);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }
        processedCredentials.secretData = decryptedPrivateKey;
    }

    output = m_serverController->checkSshConnection(processedCredentials, errorCode);
    return errorCode;
}

bool InstallController::isServerAlreadyExists(const ServerCredentials &credentials, int &existingServerIndex)
{
    int serversCount = m_serversRepository->serversCount();
    for (int i = 0; i < serversCount; i++) {
        const ServerCredentials existingCredentials = m_serversRepository->serverCredentials(i);
        if (credentials.hostName == existingCredentials.hostName && credentials.port == existingCredentials.port) {
            existingServerIndex = i;
            return true;
        }
    }
    existingServerIndex = -1;
    return false;
}

ErrorCode InstallController::mountSftpDrive(const ServerCredentials &credentials, const QString &port, const QString &password,
                                            const QString &username)
{
    QString mountPath;
    QString cmd;
    QString hostname = credentials.hostName;

#ifdef Q_OS_WINDOWS
    mountPath = Utils::getNextDriverLetter() + ":";
    cmd = "C:\\Program Files\\SSHFS-Win\\bin\\sshfs.exe";
#elif defined AMNEZIA_DESKTOP
    mountPath = QString("%1/sftp:%2:%3").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), hostname, port);
    QDir dir(mountPath);
    if (!dir.exists()) {
        dir.mkpath(mountPath);
    }

    cmd = "/usr/local/bin/sshfs";

    QSharedPointer<QProcess> process(new QProcess(this));
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process.get(), &QProcess::readyRead, this, [process, mountPath]() {
        QString s = process->readAll();
        if (s.contains("The service sshfs has been started")) {
            QDesktopServices::openUrl(QUrl("file:///" + mountPath));
        }
        qDebug() << s;
    });

    process->setProgram(cmd);

    QString args = QString("%1@%2:/ %3 "
                           "-o port=%4 "
                           "-f "
                           "-o reconnect "
                           "-o rellinks "
                           "-o fstypename=SSHFS "
                           "-o ssh_command=/usr/bin/ssh.exe "
                           "-o UserKnownHostsFile=/dev/null "
                           "-o StrictHostKeyChecking=no "
                           "-o password_stdin")
                           .arg(username, hostname, mountPath, port);

    process->setArguments(args.split(" ", Qt::SkipEmptyParts));
    process->start();
    process->waitForStarted(50);
    if (process->state() != QProcess::Running) {
        qDebug() << "mountSftpDrive process not started";
        qDebug() << args;
        return ErrorCode::ServerContainerMissingError;
    } else {
        process->write((password + "\n").toUtf8());
    }

    m_sftpMountProcesses.append(process);
#else
    Q_UNUSED(mountPath);
    Q_UNUSED(cmd);
    Q_UNUSED(password);
    return ErrorCode::NoError;
#endif

    return ErrorCode::NoError;
}

void InstallController::stopAllSftpMounts()
{
#ifdef Q_OS_WINDOWS
    for (QSharedPointer<QProcess> process : m_sftpMountProcesses) {
        Utils::signalCtrl(process->processId(), CTRL_C_EVENT);
        process->kill();
        process->waitForFinished();
    }
    m_sftpMountProcesses.clear();
#endif
}

ErrorCode InstallController::getAlreadyInstalledContainers(const ServerCredentials &credentials,
                                                           QMap<DockerContainer, QJsonObject> &installedContainers)
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

    QString script = QString("sudo docker ps --format '{{.Names}} {{.Ports}}'");
    ErrorCode errorCode = m_serverController->runScript(credentials, script, cbReadStdOut, cbReadStdErr);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    const static QRegularExpression containerAndPortRegExp("(amnezia[-a-z0-9]*).*?:([0-9]*)->[0-9]*/(udp|tcp).*");
    const static QRegularExpression torOrDnsRegExp("(amnezia-(?:torwebsite|dns)).*?([0-9]*)/(udp|tcp).*");

    QStringList containerInfos = stdOut.split("\n");
    for (const QString &containerInfo : containerInfos) {
        if (containerInfo.isEmpty()) {
            continue;
        }

        QRegularExpressionMatch containerAndPortMatch = containerAndPortRegExp.match(containerInfo);
        if (containerAndPortMatch.hasMatch()) {
            QString name = containerAndPortMatch.captured(1);
            QString port = containerAndPortMatch.captured(2);
            QString transportProto = containerAndPortMatch.captured(3);
            DockerContainer container = ContainerProps::containerFromString(name);

            QJsonObject config;
            Proto mainProto = ContainerProps::defaultProtocol(container);
            const auto &protocols = ContainerProps::protocolsForContainer(container);

            for (const auto &protocol : protocols) {
                QJsonObject containerConfig;

                bool shouldProcessProtocol = false;
                if (container == DockerContainer::ShadowSocks || container == DockerContainer::Cloak) {
                    shouldProcessProtocol = true;
                } else {
                    shouldProcessProtocol = (protocol == mainProto);
                }

                if (shouldProcessProtocol) {
                    containerConfig.insert(config_key::port, port);
                    containerConfig.insert(config_key::transport_proto, transportProto);

                    auto installer = createInstaller(container);
                    ErrorCode extractError = installer->extractConfigFromContainer(container, credentials, m_serverController, config);

                    if (extractError != ErrorCode::NoError && extractError != ErrorCode::ServerContainerMissingError) {
                        return extractError;
                    }

                    config.insert(config_key::container, ContainerProps::containerToString(container));
                }
                if (shouldProcessProtocol) {
                    config.insert(ProtocolProps::protoToString(protocol), containerConfig);
                }
            }
            installedContainers.insert(container, config);
        }

        QRegularExpressionMatch torOrDnsRegMatch = torOrDnsRegExp.match(containerInfo);
        if (torOrDnsRegMatch.hasMatch()) {
            QString name = torOrDnsRegMatch.captured(1);
            QString port = torOrDnsRegMatch.captured(2);
            QString transportProto = torOrDnsRegMatch.captured(3);
            DockerContainer container = ContainerProps::containerFromString(name);

            QJsonObject config;
            Proto mainProto = ContainerProps::defaultProtocol(container);
            for (auto protocol : ContainerProps::protocolsForContainer(container)) {
                QJsonObject containerConfig;
                if (protocol == mainProto) {
                    containerConfig.insert(config_key::port, port);
                    containerConfig.insert(config_key::transport_proto, transportProto);

                    auto installer = createInstaller(container);
                    ErrorCode extractError = installer->extractConfigFromContainer(container, credentials, m_serverController, config);

                    if (extractError != ErrorCode::NoError && extractError != ErrorCode::ServerContainerMissingError) {
                        return extractError;
                    }

                    config.insert(config_key::container, ContainerProps::containerToString(container));
                }
                config.insert(ProtocolProps::protoToString(protocol), containerConfig);
            }
            installedContainers.insert(container, config);
        }
    }

    return ErrorCode::NoError;
}
