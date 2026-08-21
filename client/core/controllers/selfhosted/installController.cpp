#include "installController.h"

#include "core/models/protocolConfig.h"

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QRegularExpression>
#include <QThread>
#include <QtConcurrent>

#include "core/configurators/configuratorBase.h"
#include "core/configurators/xrayConfigurator.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/installers/awgInstaller.h"
#include "core/installers/installerBase.h"
#include "core/installers/openvpnInstaller.h"
#include "core/installers/sftpInstaller.h"
#include "core/installers/socks5Installer.h"
#include "core/installers/mtProxyInstaller.h"
#include "core/installers/telemtInstaller.h"
#include "core/installers/torInstaller.h"
#include "core/installers/wireguardInstaller.h"
#include "core/installers/xrayInstaller.h"
#include "core/utils/networkUtilities.h"
#include "core/utils/api/apiUtils.h"
#include "core/repositories/secureServersRepository.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/utils/selfhosted/scriptsRegistry.h"
#include "core/utils/selfhosted/sshClient.h"
#include "logger.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/mtProxyProtocolConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "ui/models/protocols/wireguardConfigModel.h"
#include "core/utils/utilities.h"
#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>
#include <QSysInfo>
#ifdef Q_OS_WINDOWS
    #include <windows.h>
#endif

using namespace amnezia;
using namespace ProtocolUtils;

namespace
{
    Logger logger("InstallController");

    QString effectiveXrayPort(const XrayProtocolConfig *cfg)
    {
        if (!cfg || cfg->serverConfig.port.isEmpty()) {
            return QString::fromLatin1(protocols::xray::defaultPort);
        }
        return cfg->serverConfig.port;
    }

    bool dockerDaemonContainerMissing(const QString &out, const QString &containerDockerName)
    {
        if (!out.contains(QLatin1String("Error response from daemon"), Qt::CaseInsensitive)) {
            return false;
        }
        if (out.contains(QLatin1String("No such container"), Qt::CaseInsensitive)
            && out.contains(containerDockerName, Qt::CaseInsensitive)) {
            return true;
        }
        if (out.size() < 700 && out.contains(QLatin1String("is not running"), Qt::CaseInsensitive)) {
            return true;
        }
        return false;
    }

    bool containerKeepsIdentityInDataVolume(DockerContainer container)
    {
        return container == DockerContainer::MtProxy
                || container == DockerContainer::Telemt
                || container == DockerContainer::Xray;
    }

    QString buildRemoveContainerScript(const amnezia::ScriptVars &vars, bool removeDataVolume)
    {
        QString script = SshSession::replaceVars(amnezia::scriptData(SharedScriptType::remove_container), vars);
        if (removeDataVolume) {
            script += QLatin1String(
                    "\nfor attempt in 1 2 3 4 5; do"
                    "\n  sudo docker volume rm -f $CONTAINER_NAME-data && break"
                    "\n  sleep 1"
                    "\ndone"
                    "\necho \"amnezia_volume_left=$(sudo docker volume ls -q -f name=^$CONTAINER_NAME-data$ | head -1)\"");
            script = SshSession::replaceVars(script, vars);
        }
        return script;
    }

    bool dataVolumeSurvivedRemoval(const QString &out)
    {
        static const QRegularExpression reLeft(QStringLiteral("amnezia_volume_left=(\\S+)"));
        return reLeft.match(out).hasMatch();
    }
}

InstallController::InstallController(SecureServersRepository *serversRepository,
                                     SecureAppSettingsRepository* appSettingsRepository,
                                     QObject *parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository),
      m_cancelInstallation(false)
{
}

InstallController::~InstallController()
{
    stopAllSftpMounts();
}

ErrorCode InstallController::setupContainer(const ServerCredentials &credentials, DockerContainer container, ContainerConfig &config,
                                            bool isUpdate)
{
    qDebug().noquote() << "InstallController::setupContainer" << ContainerUtils::containerToString(container);
    SshSession sshSession;
    ErrorCode e = ErrorCode::NoError;

    e = isUserInSudo(credentials, sshSession);
    if (e)
        return e;

    e = isServerDpkgBusy(credentials, sshSession);
    if (e)
        return e;

    e = installDockerWorker(credentials, container, sshSession);
    if (e)
        return e;
    qDebug().noquote() << "InstallController::setupContainer installDockerWorker finished";

    if (!isUpdate) {
        e = isServerPortBusy(credentials, container, config, sshSession);
        if (e)
            return e;
    }

    e = prepareHostWorker(credentials, container, sshSession);
    if (e)
        return e;
    qDebug().noquote() << "InstallController::setupContainer prepareHostWorker finished";

    QMap<QString, QString> xrayStateToMigrate;
    if (isUpdate) {
        e = readXrayStateBeforeVolumeMigration(credentials, container, sshSession, xrayStateToMigrate);
        if (e)
            return e;
        logger.info() << "setupContainer: state read from the old container, files carried=" << xrayStateToMigrate.size();
    }

    const amnezia::ScriptVars removeContainerVars =
            amnezia::genBaseVars(credentials, container, QString(), QString());
    const bool removeDataVolume = !isUpdate && containerKeepsIdentityInDataVolume(container);
    QString removeOut;
    auto collectRemoveOut = [&removeOut](const QString &data, libssh::Client &) {
        removeOut += data + "\n";
        return ErrorCode::NoError;
    };
    sshSession.runScript(credentials, buildRemoveContainerScript(removeContainerVars, removeDataVolume),
                         collectRemoveOut, collectRemoveOut);
    qDebug().noquote() << "InstallController::setupContainer removeContainer finished";
    logger.info() << "setupContainer: old container removed, dataVolumeRemovalRequested="
                  << (removeDataVolume ? "yes" : "no")
                  << "leftoverVolumeReported=" << (dataVolumeSurvivedRemoval(removeOut) ? "yes" : "no");

    if (removeDataVolume && dataVolumeSurvivedRemoval(removeOut)) {
        logger.error() << "Data volume survived removal, refusing to install on top of it, output=" << removeOut;
        return ErrorCode::ServerDataVolumeNotRemoved;
    }

    qDebug().noquote() << "buildContainerWorker start";
    e = buildContainerWorker(credentials, container, config, sshSession);
    if (e)
        return e;
    qDebug().noquote() << "InstallController::setupContainer buildContainerWorker finished";

    e = runContainerWorker(credentials, container, config, sshSession);
    if (e)
        return e;
    qDebug().noquote() << "InstallController::setupContainer runContainerWorker finished";

    e = restoreXrayStateIntoDataVolume(credentials, container, sshSession, xrayStateToMigrate);
    if (e)
        return e;

    e = configureContainerWorker(credentials, container, config, sshSession);
    if (e)
        return e;
    qDebug().noquote() << "InstallController::setupContainer configureContainerWorker finished";

    if (container == DockerContainer::Xray || container == DockerContainer::SSXray) {
        DnsSettings dnsSettings = { m_appSettingsRepository->primaryDns(), m_appSettingsRepository->secondaryDns() };
        XrayConfigurator xrayConfigurator(&sshSession);
        e = xrayConfigurator.writeServerConfigForSetup(credentials, container, config, dnsSettings);
        if (e)
            return e;
        qDebug().noquote() << "InstallController::setupContainer xray writeServerConfigForSetup finished";
    }

    setupServerFirewall(credentials, sshSession);
    qDebug().noquote() << "InstallController::setupContainer setupServerFirewall finished";

    return startupContainerWorker(credentials, container, config, sshSession);
}

ErrorCode InstallController::updateServerConfig(const QString &serverId, DockerContainer container, const ContainerConfig &oldConfig,
                                                ContainerConfig &newConfig)
{
    if (!isUpdateDockerContainerRequired(container, oldConfig, newConfig)) {
        logger.info() << "updateServerConfig for" << ContainerUtils::containerToString(container)
                      << ": work level=none, reason=no server-side settings changed (a client-only edit still applies locally)";
        auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
        if (!adminConfig.has_value()) {
            return ErrorCode::InternalError;
        }
        if (container == DockerContainer::Xray || container == DockerContainer::SSXray) {
            if (const auto *xray = newConfig.getXrayProtocolConfig()) {
                ServerCredentials credentials = adminConfig->credentials();
                if (credentials.isValid()) {
                    SshSession sshSession;
                    XrayConfigurator xrayConfigurator(&sshSession);
                    xrayConfigurator.uploadClientTemplate(credentials, container, xray->clientTemplate);
                }
            }
        }
        if (container == DockerContainer::MtProxy) {
            ServerCredentials credentials = adminConfig->credentials();
            SshSession sshSession;
            MtProxyInstaller::uploadClientSettingsSnapshot(sshSession, credentials, container, newConfig);
        } else if (container == DockerContainer::Telemt) {
            ServerCredentials credentials = adminConfig->credentials();
            SshSession sshSession;
            TelemtInstaller::uploadClientSettingsSnapshot(sshSession, credentials, container, newConfig);
        }
        adminConfig->updateContainerConfig(container, newConfig);
        m_serversRepository->editServer(serverId, adminConfig->toJson(), serverConfigUtils::ConfigType::SelfHostedAdmin);
        return ErrorCode::NoError;
    }

    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }
    SshSession sshSession;

    bool reinstallRequired = isReinstallContainerRequired(container, oldConfig, newConfig);
    qDebug() << "InstallController::updateServerConfig for container" << container << "reinstall required is" << reinstallRequired;

    ErrorCode errorCode = ErrorCode::NoError;
    if (reinstallRequired) {
        logger.info() << "updateServerConfig for" << ContainerUtils::containerToString(container)
                      << ": work level=full container recreate, reason=settings baked into the container changed"
                         " (for Xray the port)";
        if (container == DockerContainer::Xray) {
            errorCode = isServerPortBusy(credentials, container, newConfig, sshSession);
            if (errorCode != ErrorCode::NoError) {
                if (errorCode == ErrorCode::ServerPortAlreadyAllocatedError) {
                    logger.error() << "Xray reinstall refused, port busy, error=201";
                }
                return errorCode;
            }
        }

        errorCode = setupContainer(credentials, container, newConfig, true);

        // Reinstall pulls the latest container image, so the server runs the latest protocol version
        if (errorCode == ErrorCode::NoError && container == DockerContainer::Awg2) {
            if (auto* awgConfig = newConfig.getAwgProtocolConfig()) {
                awgConfig->serverConfig.protocolVersion = protocols::awg::awgV3;
            }
        }
    } else if (container == DockerContainer::Xray) {
        logger.info() << "updateServerConfig for amnezia-xray : work level=in-place server.json,"
                         " reason=server settings changed, port unchanged";
        DnsSettings dnsSettings = { m_appSettingsRepository->primaryDns(), m_appSettingsRepository->secondaryDns() };
        XrayConfigurator xrayConfigurator(&sshSession);
        errorCode = xrayConfigurator.writeServerConfigForSetup(credentials, container, newConfig, dnsSettings);
        if (errorCode == ErrorCode::NoError) {
            errorCode = sshSession.runScript(
                    credentials,
                    sshSession.replaceVars(QStringLiteral("sudo docker restart $CONTAINER_NAME"),
                                           amnezia::genBaseVars(credentials, container, QString(), QString())));
        }
    } else if (container != DockerContainer::SSXray) {
        logger.info() << "updateServerConfig for" << ContainerUtils::containerToString(container)
                      << ": work level=reconfigure and restart the existing container,"
                         " reason=other settings changed";
        errorCode = configureContainerWorker(credentials, container, newConfig, sshSession);
        if (errorCode == ErrorCode::NoError) {
            errorCode = startupContainerWorker(credentials, container, newConfig, sshSession);
        }

        if (errorCode == ErrorCode::NoError
            && (container == DockerContainer::MtProxy || container == DockerContainer::Telemt)) {
            const QString containerName = ContainerUtils::containerToString(container);
            errorCode = sshSession.runScript(credentials, "sudo docker restart " + containerName);
        }
    }

    if (errorCode == ErrorCode::NoError) {
        if (container == DockerContainer::MtProxy) {
            MtProxyInstaller::uploadClientSettingsSnapshot(sshSession, credentials, container, newConfig);
        } else if (container == DockerContainer::Telemt) {
            TelemtInstaller::uploadClientSettingsSnapshot(sshSession, credentials, container, newConfig);
        }
        if (reinstallRequired) {
            // OpenVPN/WireGuard reinstall mints new client keys, so the old profile is revoked.
            // Xray identity lives in the data volume; the uuid is unchanged. Revoking it here
            // drops the admin from clients[] (seen after a port change with a shared account).
            if (container == DockerContainer::Xray) {
                logger.info() << "Xray container recreate: not revoking the volume uuid";
            } else {
                clearCachedProfile(serverId, container);
            }
        }
        adminConfig->updateContainerConfig(container, newConfig);
        m_serversRepository->editServer(serverId, adminConfig->toJson(), serverConfigUtils::ConfigType::SelfHostedAdmin);
    }

    return errorCode;
}

ErrorCode InstallController::updateClientConfig(const QString &serverId, DockerContainer container, ContainerConfig &newConfig)
{
    switch (m_serversRepository->serverKind(serverId)) {
    case serverConfigUtils::ConfigType::SelfHostedAdmin: {
        auto config = m_serversRepository->selfHostedAdminConfig(serverId);
        if (!config.has_value()) {
            return ErrorCode::InternalError;
        }
        config->updateContainerConfig(container, newConfig);
        m_serversRepository->editServer(serverId, config->toJson(), serverConfigUtils::ConfigType::SelfHostedAdmin);
        return ErrorCode::NoError;
    }
    case serverConfigUtils::ConfigType::SelfHostedUser: {
        auto config = m_serversRepository->selfHostedUserConfig(serverId);
        if (!config.has_value()) {
            return ErrorCode::InternalError;
        }
        config->updateContainerConfig(container, newConfig);
        m_serversRepository->editServer(serverId, config->toJson(), serverConfigUtils::ConfigType::SelfHostedUser);
        return ErrorCode::NoError;
    }
    case serverConfigUtils::ConfigType::Native: {
        auto config = m_serversRepository->nativeConfig(serverId);
        if (!config.has_value()) {
            return ErrorCode::InternalError;
        }
        config->updateContainerConfig(container, newConfig);
        m_serversRepository->editServer(serverId, config->toJson(), serverConfigUtils::ConfigType::Native);
        return ErrorCode::NoError;
    }
    default:
        return ErrorCode::InternalError;
    }
}

void InstallController::clearCachedProfile(const QString &serverId, DockerContainer container)
{
    if (ContainerUtils::containerService(container) == ServiceType::Other) {
        return;
    }

    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return;
    }

    const ContainerConfig containerConfigModel = adminConfig->containerConfig(container);

    adminConfig->clearCachedClientProfile(container);
    m_serversRepository->editServer(serverId, adminConfig->toJson(), serverConfigUtils::ConfigType::SelfHostedAdmin);

    emit clientRevocationRequested(serverId, containerConfigModel, container);
}

ErrorCode InstallController::validateAndPrepareConfig(const QString &serverId)
{
    const auto kind = m_serversRepository->serverKind(serverId);

    DockerContainer container = DockerContainer::None;
    ContainerConfig containerConfig;

    switch (kind) {
    case serverConfigUtils::ConfigType::SelfHostedAdmin: {
        const auto cfg = m_serversRepository->selfHostedAdminConfig(serverId);
        if (!cfg.has_value()) {
            return ErrorCode::InternalError;
        }
        container = cfg->defaultContainer;
        containerConfig = cfg->containerConfig(container);
        break;
    }
    case serverConfigUtils::ConfigType::SelfHostedUser: {
        const auto cfg = m_serversRepository->selfHostedUserConfig(serverId);
        if (!cfg.has_value()) {
            return ErrorCode::InternalError;
        }
        container = cfg->defaultContainer;
        containerConfig = cfg->containerConfig(container);
        break;
    }
    case serverConfigUtils::ConfigType::Native: {
        const auto cfg = m_serversRepository->nativeConfig(serverId);
        if (!cfg.has_value()) {
            return ErrorCode::InternalError;
        }
        container = cfg->defaultContainer;
        containerConfig = cfg->containerConfig(container);
        break;
    }
    default:
        return ErrorCode::InternalError;
    }

    if (container == DockerContainer::None) {
        return ErrorCode::NoInstalledContainersError;
    }

    if (containerConfig.protocolConfig.hasClientConfig()) {
        return ErrorCode::NoError;
    }

    if (kind != serverConfigUtils::ConfigType::SelfHostedAdmin) {
        return ErrorCode::InternalError;
    }

    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }

    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }

    SshSession sshSession;
    const QString clientName = QString("Admin [%1]").arg(QSysInfo::prettyProductName());
    const ErrorCode errorCode = processContainerForAdmin(container, containerConfig, credentials, sshSession, serverId, clientName);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    adminConfig->updateContainerConfig(container, containerConfig);
    m_serversRepository->editServer(serverId, adminConfig->toJson(), serverConfigUtils::ConfigType::SelfHostedAdmin);

    return ErrorCode::NoError;
}

void InstallController::validateConfig(const QString &serverId)
{
    QFuture<ErrorCode> future = QtConcurrent::run([this, serverId]() {
        return validateAndPrepareConfig(serverId);
    });

    auto *watcher = new QFutureWatcher<ErrorCode>(this);
    connect(watcher, &QFutureWatcher<ErrorCode>::finished, this, [this, watcher]() {
        ErrorCode errorCode = watcher->result();
        watcher->deleteLater();

        if (errorCode == ErrorCode::NoError) {
            emit configValidated(true);
            return;
        }

        emit validationErrorOccurred(errorCode);
        emit configValidated(false);
    });
    watcher->setFuture(future);
}

void InstallController::addEmptyServer(const ServerCredentials &credentials)
{
    SelfHostedAdminServerConfig serverConfig;
    serverConfig.hostName = credentials.hostName;
    serverConfig.userName = credentials.userName;
    serverConfig.password = credentials.secretData;
    serverConfig.port = credentials.port;
    serverConfig.description = m_serversRepository->nextAvailableServerName();
    serverConfig.displayName = serverConfig.description.isEmpty() ? serverConfig.hostName : serverConfig.description;
    serverConfig.defaultContainer = DockerContainer::None;

    m_serversRepository->addServer(QString(), serverConfig.toJson(),
                                    serverConfigUtils::ConfigType::SelfHostedAdmin);
}

ErrorCode InstallController::prepareContainerConfig(DockerContainer container, const ServerCredentials &credentials, ContainerConfig &containerConfig, SshSession &sshSession)
{
    if (!ContainerUtils::isSupportedByCurrentPlatform(container)) {
        return ErrorCode::NoError;
    }

    if (ContainerUtils::containerService(container) != ServiceType::Other) {
        if ((container == DockerContainer::Xray || container == DockerContainer::SSXray)
            && containerConfig.protocolConfig.hasClientConfig()) {
            return ErrorCode::NoError;
        }

        Proto protocol = ContainerUtils::defaultProtocol(container);

        DnsSettings dnsSettings = {
            m_appSettingsRepository->primaryDns(),
            m_appSettingsRepository->secondaryDns()
        };

        auto configurator = ConfiguratorBase::create(protocol, &sshSession);
        ErrorCode errorCode = ErrorCode::NoError;
        ProtocolConfig newProtocolConfig = configurator->createConfig(credentials, container, containerConfig, dnsSettings, errorCode);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }

        containerConfig.protocolConfig = newProtocolConfig;
    }

    return ErrorCode::NoError;
}

void InstallController::adminAppendRequested(const QString &serverId, DockerContainer container,
                                             const ContainerConfig &containerConfig, const QString &clientName)
{
    // Xray admin identity is the volume uuid; it must stay in server.json and must not
    // appear in the Share Users list (revoking it emptied the inbound).
    if (container == DockerContainer::Xray) {
        return;
    }
    if (ContainerUtils::containerService(container) == ServiceType::Other
        || !containerConfig.protocolConfig.hasClientConfig()) {
        return;
    }
    QString clientId = containerConfig.protocolConfig.clientId();
    if (!clientId.isEmpty()) {
        emit clientAppendRequested(serverId, clientId, clientName, container);
    }
}

ErrorCode InstallController::processContainerForAdmin(DockerContainer container, ContainerConfig &containerConfig,
                                                      const ServerCredentials &credentials, SshSession &sshSession,
                                                      const QString &serverId, const QString &clientName)
{
    if (ContainerUtils::isSupportedByCurrentPlatform(container)) {
        ErrorCode errorCode = prepareContainerConfig(container, credentials, containerConfig, sshSession);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }
    }
    adminAppendRequested(serverId, container, containerConfig, clientName);
    return ErrorCode::NoError;
}

ErrorCode InstallController::buildContainerWorker(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &config, SshSession &sshSession)
{
    amnezia::ScriptVars baseVars = amnezia::genBaseVars(credentials, container, QString(), QString());
    
    QString dockerfilePath = "/opt/amnezia/" + ContainerUtils::containerToString(container) + "/Dockerfile";
    QString removeScript = QString("sudo rm %1").arg(dockerfilePath);
    
    ErrorCode errorCode = sshSession.runScript(credentials, sshSession.replaceVars(removeScript, baseVars));
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    errorCode = sshSession.uploadFileToHost(credentials, amnezia::scriptData(ProtocolScriptType::dockerfile, container).toUtf8(), dockerfilePath);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
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

    amnezia::ScriptVars protocolVars = amnezia::genProtocolVarsForContainer(container, config);
    baseVars.append(protocolVars);
    ErrorCode error = sshSession.runScript(
            credentials, sshSession.replaceVars(amnezia::scriptData(SharedScriptType::build_container), baseVars), cbReadStdOut,
            cbReadStdErr);

    if (stdOut.contains("doesn't work on cgroups v2"))
        return ErrorCode::ServerDockerOnCgroupsV2;
    if (stdOut.contains("cgroup mountpoint does not exist"))
        return ErrorCode::ServerCgroupMountpoint;
    if (stdOut.contains("have reached") && stdOut.contains("pull rate limit"))
        return ErrorCode::DockerPullRateLimit;

    if (stdOut.contains("returned a non-zero code")
        || stdOut.contains("failed to solve")
        || stdOut.contains("Unable to find image")
        || stdOut.contains("Couldn't connect to server"))
        return ErrorCode::ServerDockerFailedError;

    return error;
}

ErrorCode InstallController::runContainerWorker(const ServerCredentials &credentials, DockerContainer container, ContainerConfig &config, SshSession &sshSession)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    amnezia::ScriptVars baseVars = amnezia::genBaseVars(credentials, container, QString(), QString());
    amnezia::ScriptVars protocolVars = amnezia::genProtocolVarsForContainer(container, config);
    baseVars.append(protocolVars);
    ErrorCode e = sshSession.runScript(
            credentials, sshSession.replaceVars(amnezia::scriptData(ProtocolScriptType::run_container, container), baseVars),
            cbReadStdOut);

    if (stdOut.contains("address already in use"))
        return ErrorCode::ServerPortAlreadyAllocatedError;
    if (stdOut.contains("is already in use by container"))
        return ErrorCode::ServerPortAlreadyAllocatedError;
    if (stdOut.contains("invalid publish"))
        return ErrorCode::ServerDockerFailedError;
    if (stdOut.contains("Unable to find image") || stdOut.contains("No such image"))
        return ErrorCode::ServerDockerFailedError;

    return e;
}

ErrorCode InstallController::configureContainerWorker(const ServerCredentials &credentials, DockerContainer container, ContainerConfig &config, SshSession &sshSession)
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
    ErrorCode e = sshSession.runContainerScript(
            credentials, container,
            sshSession.replaceVars(amnezia::scriptData(ProtocolScriptType::configure_container, container), baseVars),
            cbReadStdOut, cbReadStdErr);

    if (e != ErrorCode::NoError) {
        return e;
    }

    if (dockerDaemonContainerMissing(stdOut, ContainerUtils::containerToString(container))) {
        qDebug() << "configureContainerWorker: Docker daemon reports container missing/stopped, output:" << stdOut;
        return ErrorCode::ServerContainerMissingError;
    }

    updateContainerConfigAfterInstallation(container, config, stdOut);

    if (container == DockerContainer::MtProxy) {
        MtProxyInstaller::uploadClientSettingsSnapshot(sshSession, credentials, container, config);
    } else if (container == DockerContainer::Telemt) {
        TelemtInstaller::uploadClientSettingsSnapshot(sshSession, credentials, container, config);
    }

    return ErrorCode::NoError;
}

ErrorCode InstallController::startupContainerWorker(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &config, SshSession &sshSession)
{
    QString script = amnezia::scriptData(ProtocolScriptType::container_startup, container);

    if (script.isEmpty()) {
        return ErrorCode::NoError;
    }

    amnezia::ScriptVars baseVars = amnezia::genBaseVars(credentials, container, QString(), QString());
    amnezia::ScriptVars protocolVars = amnezia::genProtocolVarsForContainer(container, config);
    baseVars.append(protocolVars);
    ErrorCode e = sshSession.uploadTextFileToContainer(container, credentials, sshSession.replaceVars(script, baseVars),
                                                                "/opt/amnezia/start.sh");
    if (e)
        return e;

    return sshSession.runScript(
            credentials,
            sshSession.replaceVars("sudo docker exec -d $CONTAINER_NAME sh -c \"chmod a+x /opt/amnezia/start.sh && "
                                            "/opt/amnezia/start.sh\"",
                                            baseVars));
}

ErrorCode InstallController::readXrayStateBeforeVolumeMigration(const ServerCredentials &credentials,
                                                                DockerContainer container, SshSession &sshSession,
                                                                QMap<QString, QString> &outFiles)
{
    outFiles.clear();

    if (container != DockerContainer::Xray) {
        return ErrorCode::NoError;
    }

    namespace px = amnezia::protocols::xray;

    const amnezia::ScriptVars vars = amnezia::genBaseVars(credentials, container, QString(), QString());

    QString stdOut;
    auto collect = [&stdOut](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    const QString probe = QStringLiteral(
            "echo \"amnezia_volume=$(sudo docker volume ls -q -f name=^$CONTAINER_NAME-data$ | head -1)\"\n"
            "echo \"amnezia_container=$(sudo docker ps -a -q -f name=^$CONTAINER_NAME$ | head -1)\"\n"
            "if [ -n \"$(sudo docker ps -a -q -f name=^$CONTAINER_NAME$ | head -1)\" ] && "
            "[ -z \"$(sudo docker ps -q -f name=^$CONTAINER_NAME$ | head -1)\" ]; then\n"
            "  sudo docker start $CONTAINER_NAME > /dev/null 2>&1\n"
            "  sleep 2\n"
            "fi\n"
            "echo \"amnezia_running=$(sudo docker ps -q -f name=^$CONTAINER_NAME$ | head -1)\"\n"
            "echo \"amnezia_mounted=$(sudo docker inspect -f "
            "'{{range .Mounts}}{{if and (eq .Destination \"%1\") "
            "(eq .Name \"$CONTAINER_NAME-data\")}}yes{{end}}{{end}}' "
            "$CONTAINER_NAME 2>/dev/null | head -1)\"")
                                 .arg(QString::fromLatin1(px::dataDir));
    ErrorCode errorCode = sshSession.runScript(credentials, SshSession::replaceVars(probe, vars), collect, collect);
    if (errorCode != ErrorCode::NoError) {
        logger.error() << "Xray key migration: could not probe server state";
        return ErrorCode::XrayKeyMigrationFailed;
    }

    static const QRegularExpression reVolume(QStringLiteral("amnezia_volume=(\\S+)"));
    static const QRegularExpression reContainer(QStringLiteral("amnezia_container=(\\S+)"));
    static const QRegularExpression reRunning(QStringLiteral("amnezia_running=(\\S+)"));
    static const QRegularExpression reMounted(QStringLiteral("amnezia_mounted=(\\S+)"));
    const bool volumeExists = reVolume.match(stdOut).hasMatch();
    const bool containerExists = reContainer.match(stdOut).hasMatch();
    const bool containerRunning = reRunning.match(stdOut).hasMatch();
    const bool dataDirIsVolume = reMounted.match(stdOut).hasMatch();

    logger.info() << "Xray key migration: probe result, dataVolume=" << (volumeExists ? "present" : "absent")
                  << "container=" << (containerExists ? "present" : "absent")
                  << "runningAfterProbe=" << (containerRunning ? "yes" : "no")
                  << "dataDirServedByVolume=" << (dataDirIsVolume ? "yes" : "no");

    if (!containerExists) {
        logger.info() << "Xray key migration: not needed, no existing container to read from";
        return ErrorCode::NoError;
    }
    if (dataDirIsVolume) {
        logger.info() << "Xray key migration: not needed, the data directory is already served by the volume";
        return ErrorCode::NoError;
    }
    if (volumeExists) {
        logger.warning() << "Xray key migration: a data volume exists but this container does not use it,"
                         << "carrying the live keys so the stale volume does not replace them";
    }
    if (!containerRunning) {
        logger.error() << "Xray key migration: container will not start, cannot read the keys out of it";
        return ErrorCode::XrayKeyMigrationFailed;
    }

    const QStringList requiredPaths = {
        QString::fromLatin1(px::PrivateKeyPath),
        QString::fromLatin1(px::PublicKeyPath),
        QString::fromLatin1(px::uuidPath),
        QString::fromLatin1(px::shortidPath),
    };

    for (const QString &path : requiredPaths) {
        QString content;
        bool read = false;
        for (int attempt = 0; attempt < 3 && !read; ++attempt) {
            ErrorCode fileError = ErrorCode::NoError;
            content = QString::fromUtf8(sshSession.getTextFileFromContainer(container, credentials, path, fileError));
            if (fileError == ErrorCode::NoError && !content.trimmed().isEmpty()) {
                read = true;
                break;
            }
            if (attempt < 2) {
                QThread::msleep(500);
            }
        }
        if (!read) {
            logger.error() << "Xray key migration: failed to read" << path << ", aborting before container removal";
            return ErrorCode::XrayKeyMigrationFailed;
        }
        outFiles.insert(path, content);
    }

    QString serverConfig;
    bool configRead = false;
    for (int attempt = 0; attempt < 3 && !configRead; ++attempt) {
        ErrorCode configError = ErrorCode::NoError;
        serverConfig = QString::fromUtf8(sshSession.getTextFileFromContainer(
                container, credentials, QString::fromLatin1(px::serverConfigPath), configError));
        if (configError == ErrorCode::NoError && !serverConfig.trimmed().isEmpty()) {
            configRead = true;
            break;
        }
        if (attempt < 2) {
            QThread::msleep(500);
        }
    }
    if (!configRead) {
        logger.error() << "Xray key migration: server config unreadable, aborting before container removal";
        return ErrorCode::XrayKeyMigrationFailed;
    }
    outFiles.insert(QString::fromLatin1(px::serverConfigPath), serverConfig);

    ErrorCode templateError = ErrorCode::NoError;
    const QString clientTemplate = QString::fromUtf8(sshSession.getTextFileFromContainer(
            container, credentials, QString::fromLatin1(px::clientTemplatePath), templateError));
    if (templateError == ErrorCode::NoError && !clientTemplate.trimmed().isEmpty()) {
        outFiles.insert(QString::fromLatin1(px::clientTemplatePath), clientTemplate);
        logger.info() << "Xray key migration: the client template is coming along too";
    } else {
        logger.info() << "Xray key migration: no client template to carry, this server never had one";
    }

    logger.info() << "Xray key migration: carrying" << outFiles.size() << "files into the data volume";
    return ErrorCode::NoError;
}

ErrorCode InstallController::restoreXrayStateIntoDataVolume(const ServerCredentials &credentials,
                                                            DockerContainer container, SshSession &sshSession,
                                                            const QMap<QString, QString> &files)
{
    if (files.isEmpty()) {
        return ErrorCode::NoError;
    }

    for (auto it = files.constBegin(); it != files.constEnd(); ++it) {
        const ErrorCode errorCode = sshSession.uploadTextFileToContainer(
                container, credentials, it.value(), it.key(), libssh::ScpOverwriteMode::ScpOverwriteExisting);
        if (errorCode != ErrorCode::NoError) {
            logger.error() << "Xray key migration: failed to write back" << it.key();
            return ErrorCode::XrayKeyMigrationFailed;
        }
    }

    logger.info() << "Xray key migration: restored" << files.size() << "files";
    return ErrorCode::NoError;
}

ErrorCode InstallController::isServerPortBusy(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &config, SshSession &sshSession)
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

    const Proto protocol = ContainerUtils::defaultProtocol(container);
    QStringList fixedPorts = ContainerUtils::fixedPortsForContainer(container);

    QString port = config.protocolConfig.port();
    if (port.isEmpty()) {
        port = QString::number(ProtocolUtils::defaultPort(protocol));
    }
    QString transportProto = config.protocolConfig.transportProto();
    if (transportProto.isEmpty()) {
        transportProto = ProtocolUtils::transportProtoToString(ProtocolUtils::defaultTransportProto(protocol), protocol);
    }

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

        ErrorCode errorCode = sshSession.runScript(
                credentials,
                sshSession.replaceVars(tcpProtoScript, amnezia::genBaseVars(credentials, container, QString(), QString())),
                cbReadStdOut, cbReadStdErr);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }

        errorCode = sshSession.runScript(
                credentials,
                sshSession.replaceVars(udpProtoScript, amnezia::genBaseVars(credentials, container, QString(), QString())),
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

    ErrorCode errorCode = sshSession.runScript(
            credentials, sshSession.replaceVars(script, amnezia::genBaseVars(credentials, container, QString(), QString())),
            cbReadStdOut, cbReadStdErr);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    if (!stdOut.isEmpty()) {
        return ErrorCode::ServerPortAlreadyAllocatedError;
    }
    return ErrorCode::NoError;
}

bool InstallController::isReinstallContainerRequired(DockerContainer container, const ContainerConfig &oldConfig, const ContainerConfig &newConfig)
{
    if (container == DockerContainer::OpenVpn) {
        const auto* oldOvpnConfig = oldConfig.getOpenVpnProtocolConfig();
        const auto* newOvpnConfig = newConfig.getOpenVpnProtocolConfig();
        
        if (oldOvpnConfig && newOvpnConfig) {
            if (!oldOvpnConfig->serverConfig.hasEqualServerSettings(newOvpnConfig->serverConfig)) {
                return true;
            }
        }
    }

    if (ContainerUtils::isAwgContainer(container)) {
        const auto* oldAwgConfig = oldConfig.getAwgProtocolConfig();
        const auto* newAwgConfig = newConfig.getAwgProtocolConfig();
        
        if (oldAwgConfig && newAwgConfig) {
            if (!oldAwgConfig->serverConfig.hasEqualServerSettings(newAwgConfig->serverConfig)) {
                return true;
            }
        }
    }

    if (container == DockerContainer::WireGuard) {
        const auto* oldWgConfig = oldConfig.getWireGuardProtocolConfig();
        const auto* newWgConfig = newConfig.getWireGuardProtocolConfig();
        
        if (oldWgConfig && newWgConfig) {
            if (!oldWgConfig->serverConfig.hasEqualServerSettings(newWgConfig->serverConfig)) {
                return true;
            }
        }
    }

    if (container == DockerContainer::Xray) {
        const auto *oldXrayConfig = oldConfig.getXrayProtocolConfig();
        const auto *newXrayConfig = newConfig.getXrayProtocolConfig();

        if (oldXrayConfig && newXrayConfig) {
            const QString oldPort = effectiveXrayPort(oldXrayConfig);
            const QString newPort = effectiveXrayPort(newXrayConfig);
            if (oldPort != newPort) {
                logger.info() << "Xray reinstall required, port changed" << oldPort << "->" << newPort;
                return true;
            }
            logger.info() << "Xray reinstall not required, port unchanged (" << newPort << ")";
        }
    }

    if (container == DockerContainer::SSXray) {
        const auto *oldXrayConfig = oldConfig.getXrayProtocolConfig();
        const auto *newXrayConfig = newConfig.getXrayProtocolConfig();

        if (oldXrayConfig && newXrayConfig) {
            if (!oldXrayConfig->serverConfig.hasEqualServerSettings(newXrayConfig->serverConfig)) {
                logger.info() << "SSXray reinstall required, server settings changed";
                return true;
            }
            logger.info() << "SSXray reinstall not required, server settings unchanged";
        }
    }

    if (container == DockerContainer::MtProxy) {
        const auto *oldMt = oldConfig.getMtProxyProtocolConfig();
        const auto *newMt = newConfig.getMtProxyProtocolConfig();
        if (oldMt && newMt) {
            const QString oldPort =
                    oldMt->port.isEmpty() ? QString(protocols::mtProxy::defaultPort) : oldMt->port;
            const QString newPort =
                    newMt->port.isEmpty() ? QString(protocols::mtProxy::defaultPort) : newMt->port;
            if (oldPort != newPort) {
                return true;
            }
        }
    }

    if (container == DockerContainer::Telemt) {
        const auto *oldT = oldConfig.getTelemtProtocolConfig();
        const auto *newT = newConfig.getTelemtProtocolConfig();
        if (oldT && newT) {
            const QString oldPort =
                    oldT->port.isEmpty() ? QString(protocols::telemt::defaultPort) : oldT->port;
            const QString newPort =
                    newT->port.isEmpty() ? QString(protocols::telemt::defaultPort) : newT->port;
            if (oldPort != newPort) {
                return true;
            }
        }
    }

    if (container == DockerContainer::Socks5Proxy) {
        return true;
    }

    return false;
}

void InstallController::cancelInstallation()
{
    m_cancelInstallation = true;
}

ErrorCode InstallController::installDockerWorker(const ServerCredentials &credentials, DockerContainer container, SshSession &sshSession)
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

    ErrorCode error = sshSession.runScript(
            credentials,
            sshSession.replaceVars(amnezia::scriptData(SharedScriptType::install_docker),
                                            amnezia::genBaseVars(credentials, DockerContainer::None, QString(), QString())),
            cbReadStdOut, cbReadStdErr);

    qDebug().noquote() << "InstallController::installDockerWorker" << stdOut;

    if (container == DockerContainer::MtProxy || container == DockerContainer::Telemt) {
        QString conntrackOut;
        auto cbConntrack = [&](const QString &data, libssh::Client &) {
            conntrackOut += data + "\n";
            return ErrorCode::NoError;
        };
        sshSession.runScript(
                credentials,
                sshSession.replaceVars(amnezia::scriptData(SharedScriptType::install_conntrack),
                                       amnezia::genBaseVars(credentials, DockerContainer::None, QString(), QString())),
                cbConntrack, cbConntrack);
        qDebug().noquote() << "InstallController::installDockerWorker install_conntrack:" << conntrackOut;
    }

    if (container == DockerContainer::Awg2) {
        QRegularExpression kernelVersionRegex(R"(Linux\s+(\d+)\.(\d+)[^\d]*)");
        QRegularExpressionMatch match = kernelVersionRegex.match(stdOut);
        if (match.hasMatch()) {
            int majorVersion = match.captured(1).toInt();
            int minorVersion = match.captured(2).toInt();

            if (majorVersion < 4 || (majorVersion == 4 && minorVersion < 14)) {
                return ErrorCode::ServerLinuxKernelTooOld;
            }
        }
    }

    if (stdOut.contains("lock"))
        return ErrorCode::ServerPacketManagerError;
    if (stdOut.contains("Container runtime is not supported"))
        return ErrorCode::ServerContainerRuntimeNotSupported;
    
    QRegularExpression notFoundRegex(
        R"(^.*(?:sudo:|docker:).*not found.*$)",
        QRegularExpression::MultilineOption);

    if (notFoundRegex.match(stdOut).hasMatch()) {
        return ErrorCode::ServerDockerFailedError;
    }
    
    if (stdOut.contains("Container runtime service not running"))
        return ErrorCode::ContainerRuntimeServiceNotRunning;

    return error;
}

ErrorCode InstallController::prepareHostWorker(const ServerCredentials &credentials, DockerContainer container, SshSession &sshSession)
{
    // create folder on host
    return sshSession.runScript(credentials,
                                         sshSession.replaceVars(amnezia::scriptData(SharedScriptType::prepare_host),
                                                                         amnezia::genBaseVars(credentials, container, QString(), QString())));
}

ErrorCode InstallController::isUserInSudo(const ServerCredentials &credentials, SshSession &sshSession)
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
    ErrorCode error = sshSession.runScript(
            credentials,
            sshSession.replaceVars(scriptData, amnezia::genBaseVars(credentials, DockerContainer::None, QString(), QString())),
            cbReadStdOut, cbReadStdErr);

    if (credentials.userName != "root" && stdOut.contains("sudo:") && !stdOut.contains("uname:") && stdOut.contains("not found"))
        return ErrorCode::ServerSudoPackageIsNotPreinstalled;
    if (credentials.userName != "root" && !stdOut.contains("sudo") && !stdOut.contains("wheel"))
        return ErrorCode::ServerUserNotInSudo;
    if (stdOut.contains("can't cd to") || stdOut.contains("Permission denied") || stdOut.contains("No such file or directory"))
        return ErrorCode::ServerUserDirectoryNotAccessible;
    if (stdOut.contains(QRegularExpression(R"(\bsudoers\b)")) || stdOut.contains("is not allowed to") || stdOut.contains("can't do that"))
        return ErrorCode::ServerUserNotAllowedInSudoers;
    if (stdOut.contains("password is required") || stdOut.contains("authentication is required"))
        return ErrorCode::ServerUserPasswordRequired;

    return error;
}

ErrorCode InstallController::isServerDpkgBusy(const ServerCredentials &credentials, SshSession &sshSession)
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

    QFuture<ErrorCode> future = QtConcurrent::run([this, &stdOut, &cbReadStdOut, &cbReadStdErr, &credentials, &sshSession]() {
        // max 100 attempts
        for (int i = 0; i < 30; ++i) {
            if (m_cancelInstallation) {
                return ErrorCode::ServerCancelInstallation;
            }
            stdOut.clear();
            sshSession.runScript(
                    credentials,
                    sshSession.replaceVars(amnezia::scriptData(SharedScriptType::check_server_is_busy),
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

ErrorCode InstallController::setupServerFirewall(const ServerCredentials &credentials, SshSession &sshSession)
{
    return sshSession.runScript(
            credentials,
            sshSession.replaceVars(amnezia::scriptData(SharedScriptType::setup_host_firewall),
                                            amnezia::genBaseVars(credentials, DockerContainer::None, QString(), QString())));
}

ErrorCode InstallController::rebootServer(const QString &serverId)
{
    const auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }
    SshSession sshSession;

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

    return sshSession.runScript(credentials, script, cbReadStdOut, cbReadStdErr);
}

ErrorCode InstallController::removeAllContainers(const QString &serverId)
{
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }
    SshSession sshSession;
    ErrorCode errorCode = sshSession.runScript(credentials, amnezia::scriptData(SharedScriptType::remove_all_containers));

    if (errorCode == ErrorCode::NoError) {
        adminConfig->containers.clear();
        adminConfig->defaultContainer = DockerContainer::None;
        m_serversRepository->editServer(serverId, adminConfig->toJson(), serverConfigUtils::ConfigType::SelfHostedAdmin);
    }

    return errorCode;
}

ErrorCode InstallController::removeContainer(const QString &serverId, DockerContainer container)
{
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }
    SshSession sshSession;
    const amnezia::ScriptVars removeContainerVars =
            amnezia::genBaseVars(credentials, container, QString(), QString());
    const bool removeDataVolume = containerKeepsIdentityInDataVolume(container);
    QString removeOut;
    auto collectRemoveOut = [&removeOut](const QString &data, libssh::Client &) {
        removeOut += data + "\n";
        return ErrorCode::NoError;
    };
    ErrorCode errorCode = sshSession.runScript(
            credentials, buildRemoveContainerScript(removeContainerVars, removeDataVolume), collectRemoveOut,
            collectRemoveOut);
    logger.info() << "removeContainer" << ContainerUtils::containerToString(container)
                  << ": script finished, dataVolumeRemovalRequested=" << (removeDataVolume ? "yes" : "no")
                  << "leftoverVolumeReported=" << (dataVolumeSurvivedRemoval(removeOut) ? "yes" : "no");

    if (errorCode == ErrorCode::NoError && removeDataVolume && dataVolumeSurvivedRemoval(removeOut)) {
        logger.error() << "Data volume survived protocol removal, output=" << removeOut;
        errorCode = ErrorCode::ServerDataVolumeNotRemoved;
    }

    if (errorCode == ErrorCode::NoError) {
        QMap<DockerContainer, ContainerConfig> containers = adminConfig->containers;
        containers.remove(container);

        DockerContainer defaultContainer = adminConfig->defaultContainer;
        if (defaultContainer == container) {
            if (containers.isEmpty()) {
                defaultContainer = DockerContainer::None;
            } else {
                defaultContainer = containers.begin().key();
            }
        }

        adminConfig->containers = containers;
        adminConfig->defaultContainer = defaultContainer;
        m_serversRepository->editServer(serverId, adminConfig->toJson(), serverConfigUtils::ConfigType::SelfHostedAdmin);
    }

    return errorCode;
}

QScopedPointer<InstallerBase> InstallController::createInstaller(DockerContainer container)
{
    switch (container) {
    case DockerContainer::Awg: return QScopedPointer<InstallerBase>(new AwgInstaller(this));
    case DockerContainer::Awg2: return QScopedPointer<InstallerBase>(new AwgInstaller(this));
    case DockerContainer::WireGuard: return QScopedPointer<InstallerBase>(new WireguardInstaller(this));
    case DockerContainer::OpenVpn: return QScopedPointer<InstallerBase>(new OpenVpnInstaller(this));
    case DockerContainer::Xray:
    case DockerContainer::SSXray: return QScopedPointer<InstallerBase>(new XrayInstaller(this));
    case DockerContainer::TorWebSite: return QScopedPointer<InstallerBase>(new TorInstaller(this));
    case DockerContainer::Sftp: return QScopedPointer<InstallerBase>(new SftpInstaller(this));
    case DockerContainer::Socks5Proxy: return QScopedPointer<InstallerBase>(new Socks5Installer(this));
    case DockerContainer::MtProxy: return QScopedPointer<InstallerBase>(new MtProxyInstaller(this));
    case DockerContainer::Telemt: return QScopedPointer<InstallerBase>(new TelemtInstaller(this));
    default: return QScopedPointer<InstallerBase>(new InstallerBase(this));
    }
}

ContainerConfig InstallController::generateConfig(DockerContainer container, int port, TransportProto transportProto)
{
    auto installer = createInstaller(container);
    return installer->generateConfig(container, port, transportProto);
}

ErrorCode InstallController::installContainer(const ServerCredentials &credentials, DockerContainer container, int port,
                                              TransportProto transportProto, ContainerConfig &config)
{
    config = generateConfig(container, port, transportProto);
    return setupContainer(credentials, container, config, false);
}


bool InstallController::isUpdateDockerContainerRequired(DockerContainer container, const ContainerConfig &oldConfig, const ContainerConfig &newConfig)
{
    if (ContainerUtils::isAwgContainer(container)) {
        const auto* oldAwgConfig = oldConfig.getAwgProtocolConfig();
        const auto* newAwgConfig = newConfig.getAwgProtocolConfig();
        
        if (oldAwgConfig && newAwgConfig) {
            if (oldAwgConfig->serverConfig.hasEqualServerSettings(newAwgConfig->serverConfig)) {
                return false;
            }
        }
    } else if (container == DockerContainer::WireGuard) {
        const auto* oldWgConfig = oldConfig.getWireGuardProtocolConfig();
        const auto* newWgConfig = newConfig.getWireGuardProtocolConfig();
        
        if (oldWgConfig && newWgConfig) {
            if (oldWgConfig->serverConfig.hasEqualServerSettings(newWgConfig->serverConfig)) {
                return false;
            }
        }
    } else if (container == DockerContainer::Xray) {
        const auto *oldXray = oldConfig.getXrayProtocolConfig();
        const auto *newXray = newConfig.getXrayProtocolConfig();
        if (oldXray && newXray && oldXray->serverConfig.hasEqualServerSettings(newXray->serverConfig)) {
            logger.info() << "Xray server update not required, server settings compare equal";
            return false;
        }
        if (oldXray && newXray) {
            logger.info() << "Xray server update required, differing settings:"
                          << oldXray->serverConfig.serverViewDifferences(newXray->serverConfig).join(
                                     QLatin1String(", "));
        } else {
            logger.info() << "Xray server update required, no config to compare against";
        }
        return true;
    } else if (container == DockerContainer::MtProxy) {
        const auto *oldMt = oldConfig.getMtProxyProtocolConfig();
        const auto *newMt = newConfig.getMtProxyProtocolConfig();
        if (!oldMt || !newMt) {
            return true;
        }
        return !oldMt->equalsDockerDeploymentSettings(*newMt);
    } else if (container == DockerContainer::Telemt) {
        const auto *oldT = oldConfig.getTelemtProtocolConfig();
        const auto *newT = newConfig.getTelemtProtocolConfig();
        if (!oldT || !newT) {
            return true;
        }
        return !oldT->equalsDockerDeploymentSettings(*newT);
    }

    logger.info() << "Server update required for" << ContainerUtils::containerToString(container)
                  << ", this container has no settings comparison, so every save goes to the server";
    return true;
}

ErrorCode InstallController::scanServerForInstalledContainers(const QString &serverId)
{
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }
    SshSession sshSession;

    QMap<DockerContainer, ContainerConfig> installedContainers;
    ErrorCode errorCode = getAlreadyInstalledContainers(credentials, installedContainers, sshSession);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QMap<DockerContainer, ContainerConfig> containers = adminConfig->containers;
    bool hasNewContainers = false;

    QString clientName = QString("Admin [%1]").arg(QSysInfo::prettyProductName());
    for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
        if (!containers.contains(iterator.key())) {
            ContainerConfig containerConfig = iterator.value();
            errorCode = processContainerForAdmin(iterator.key(), containerConfig, credentials, sshSession,
                                                 serverId, clientName);
            if (errorCode != ErrorCode::NoError) {
                return errorCode;
            }
            containers.insert(iterator.key(), containerConfig);
            hasNewContainers = true;

            DockerContainer defaultContainer = adminConfig->defaultContainer;
            if (defaultContainer == DockerContainer::None
                && ContainerUtils::containerService(iterator.key()) != ServiceType::Other
                && ContainerUtils::isSupportedByCurrentPlatform(iterator.key())) {
                adminConfig->defaultContainer = iterator.key();
            }
        }
    }

    if (hasNewContainers) {
        adminConfig->containers = containers;
        m_serversRepository->editServer(serverId, adminConfig->toJson(), serverConfigUtils::ConfigType::SelfHostedAdmin);
    }

    return ErrorCode::NoError;
}

ErrorCode InstallController::installServer(const ServerCredentials &credentials, DockerContainer container, int port,
                                           TransportProto transportProto, bool &wasContainerInstalled)
{
    SshSession sshSession;
    QMap<DockerContainer, ContainerConfig> installedContainers;
    ErrorCode errorCode = getAlreadyInstalledContainers(credentials, installedContainers, sshSession);
    if (errorCode) {
        return errorCode;
    }

    wasContainerInstalled = false;
    if (!installedContainers.contains(container)) {
        ContainerConfig config;
        errorCode = installContainer(credentials, container, port, transportProto, config);
        if (errorCode) {
            return errorCode;
        }

        installedContainers.insert(container, config);
        wasContainerInstalled = true;
    }

    QMap<DockerContainer, ContainerConfig> preparedContainers;
    for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
        DockerContainer container = iterator.key();
        ContainerConfig containerConfig = iterator.value();

        if (ContainerUtils::isSupportedByCurrentPlatform(container)) {
            errorCode = prepareContainerConfig(container, credentials, containerConfig, sshSession);
            if (errorCode != ErrorCode::NoError) {
                return errorCode;
            }
        }
        preparedContainers.insert(container, containerConfig);
    }

    SelfHostedAdminServerConfig serverConfig;
    serverConfig.hostName = credentials.hostName;
    serverConfig.userName = credentials.userName;
    serverConfig.password = credentials.secretData;
    serverConfig.port = credentials.port;
    serverConfig.description = m_serversRepository->nextAvailableServerName();

    for (auto iterator = preparedContainers.begin(); iterator != preparedContainers.end(); iterator++) {
        serverConfig.containers.insert(iterator.key(), iterator.value());
    }

    serverConfig.defaultContainer = container;

    serverConfig.displayName = serverConfig.description.isEmpty() ? serverConfig.hostName : serverConfig.description;

    const QString newServerId = m_serversRepository->addServer(QString(), serverConfig.toJson(),
                                                               serverConfigUtils::ConfigType::SelfHostedAdmin);
    QString clientName = QString("Admin [%1]").arg(QSysInfo::prettyProductName());
    for (auto iterator = preparedContainers.begin(); iterator != preparedContainers.end(); iterator++) {
        adminAppendRequested(newServerId, iterator.key(), iterator.value(), clientName);
    }

    return ErrorCode::NoError;
}

ErrorCode InstallController::installContainer(const QString &serverId, DockerContainer container, int port,
                                              TransportProto transportProto, bool &wasContainerInstalled)
{
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }
    SshSession sshSession;
    
    QMap<DockerContainer, ContainerConfig> installedContainers;
    ErrorCode errorCode = getAlreadyInstalledContainers(credentials, installedContainers, sshSession);
    if (errorCode) {
        return errorCode;
    }

    wasContainerInstalled = false;
    if (!installedContainers.contains(container)) {
        ContainerConfig config;
        errorCode = installContainer(credentials, container, port, transportProto, config);
        if (errorCode) {
            return errorCode;
        }

        installedContainers.insert(container, config);
        wasContainerInstalled = true;
    }

    QString clientName = QString("Admin [%1]").arg(QSysInfo::prettyProductName());
    for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
        ContainerConfig existingConfigModel = adminConfig->containerConfig(iterator.key());
        if (existingConfigModel.container == DockerContainer::None) {
            ContainerConfig containerConfig = iterator.value();
            errorCode = processContainerForAdmin(iterator.key(), containerConfig, credentials, sshSession,
                                                 serverId, clientName);
            if (errorCode != ErrorCode::NoError) {
                return errorCode;
            }
            adminConfig->updateContainerConfig(iterator.key(), containerConfig);
            m_serversRepository->editServer(serverId, adminConfig->toJson(),
                                            serverConfigUtils::ConfigType::SelfHostedAdmin);
        }
    }

    return ErrorCode::NoError;
}

ErrorCode InstallController::checkSshConnection(ServerCredentials &credentials, QString &output,
                                                std::function<QString()> passphraseCallback)
{
    SshSession sshSession;
    ErrorCode errorCode = ErrorCode::NoError;

    if (credentials.secretData.contains("BEGIN") && credentials.secretData.contains("PRIVATE KEY")) {
        if (!passphraseCallback) {
            return ErrorCode::SshPrivateKeyError;
        }

        QString decryptedPrivateKey;
        errorCode = sshSession.getDecryptedPrivateKey(credentials, decryptedPrivateKey, passphraseCallback);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }
        credentials.secretData = decryptedPrivateKey;
    }

    output = sshSession.checkSshConnection(credentials, errorCode);
    return errorCode;
}

bool InstallController::isServerAlreadyExists(const ServerCredentials &credentials, int &existingServerIndex)
{
    int serversCount = m_serversRepository->serversCount();
    for (int i = 0; i < serversCount; i++) {
        const QString existingServerId = m_serversRepository->serverIdAt(i);
        const auto adminConfig = m_serversRepository->selfHostedAdminConfig(existingServerId);
        if (!adminConfig.has_value()) {
            continue;
        }
        const ServerCredentials existingCredentials = adminConfig->credentials();
        if (!existingCredentials.isValid()) {
            continue;
        }
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

void InstallController::updateContainerConfigAfterInstallation(DockerContainer container, ContainerConfig &containerConfig, const QString &stdOut)
{
    Proto mainProto = ContainerUtils::defaultProtocol(container);

    if (container == DockerContainer::TorWebSite) {
        if (auto* torProtocolConfig = containerConfig.getTorProtocolConfig()) {
            qDebug() << "amnezia-tor onions" << stdOut;

            QString onion = stdOut;
            onion.replace("\n", "");
            torProtocolConfig->serverConfig.site = onion;
        }
    } else if (container == DockerContainer::MtProxy) {
        if (auto* mtProxyConfig = containerConfig.getMtProxyProtocolConfig()) {
            qDebug() << "amnezia mtproxy" << stdOut;

            static const QRegularExpression reSecret(
                    QStringLiteral(R"(\[\*\]\s+Secret:\s+([0-9a-fA-F]{32}))"),
                    QRegularExpression::CaseInsensitiveOption);
            static const QRegularExpression reTgLink(QStringLiteral(R"(\[\*\]\s+tg://\s+link:\s+(tg://proxy\?[^\s]+))"));
            static const QRegularExpression reTmeLink(
                    QStringLiteral(R"(\[\*\]\s+t\.me\s+link:\s+(https://t\.me/proxy\?[^\s]+))"));

            const QRegularExpressionMatch mSecret = reSecret.match(stdOut);
            const QRegularExpressionMatch mTgLink = reTgLink.match(stdOut);
            const QRegularExpressionMatch mTmeLink = reTmeLink.match(stdOut);

            if (mSecret.hasMatch()) {
                mtProxyConfig->secret = mSecret.captured(1);
            }
            if (mTgLink.hasMatch()) {
                mtProxyConfig->tgLink = mTgLink.captured(1);
            }
            if (mTmeLink.hasMatch()) {
                mtProxyConfig->tmeLink = mTmeLink.captured(1);
            }
        }
    } else if (container == DockerContainer::Telemt) {
        if (auto *telemtConfig = containerConfig.getTelemtProtocolConfig()) {
            qDebug() << "amnezia-telemt configure stdout" << stdOut;

            static const QRegularExpression reSecret(
                    QStringLiteral(R"(\[\*\]\s+Secret:\s+([0-9a-fA-F]{32}))"),
                    QRegularExpression::CaseInsensitiveOption);
            static const QRegularExpression reTgLink(QStringLiteral(R"(\[\*\]\s+tg://\s+link:\s+(tg://proxy\?[^\s]+))"));
            static const QRegularExpression reTmeLink(
                    QStringLiteral(R"(\[\*\]\s+t\.me\s+link:\s+(https://t\.me/proxy\?[^\s]+))"));

            const QRegularExpressionMatch mSecret = reSecret.match(stdOut);
            const QRegularExpressionMatch mTgLink = reTgLink.match(stdOut);
            const QRegularExpressionMatch mTmeLink = reTmeLink.match(stdOut);

            if (mSecret.hasMatch()) {
                telemtConfig->secret = mSecret.captured(1);
            }
            if (mTgLink.hasMatch()) {
                telemtConfig->tgLink = mTgLink.captured(1);
            }
            if (mTmeLink.hasMatch()) {
                telemtConfig->tmeLink = mTmeLink.captured(1);
            }
        }
    }
}

ErrorCode InstallController::getAlreadyInstalledContainers(const ServerCredentials &credentials,
                                                           QMap<DockerContainer, ContainerConfig> &installedContainers, SshSession &sshSession)
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
    ErrorCode errorCode = sshSession.runScript(credentials, script, cbReadStdOut, cbReadStdErr);
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
            QString portStr = containerAndPortMatch.captured(2);
            QString transportProtoStr = containerAndPortMatch.captured(3);
            DockerContainer container = ContainerUtils::containerFromString(name);

            if (container == DockerContainer::None || ContainerUtils::isUnsupportedContainer(container)) {
                continue;
            }

            int port = portStr.toInt();
            TransportProto transportProto = ProtocolUtils::transportProtoFromString(transportProtoStr);

            auto installer = createInstaller(container);
            ContainerConfig config = installer->createBaseConfig(container, port, transportProto);
            ErrorCode extractError = installer->extractConfigFromContainer(container, credentials, &sshSession, config);

            if (extractError != ErrorCode::NoError && extractError != ErrorCode::ServerContainerMissingError) {
                return extractError;
            }

            installedContainers.insert(container, config);
        }

        QRegularExpressionMatch torOrDnsRegMatch = torOrDnsRegExp.match(containerInfo);
        if (torOrDnsRegMatch.hasMatch()) {
            QString name = torOrDnsRegMatch.captured(1);
            QString portStr = torOrDnsRegMatch.captured(2);
            QString transportProtoStr = torOrDnsRegMatch.captured(3);
            DockerContainer container = ContainerUtils::containerFromString(name);

            if (container == DockerContainer::None || ContainerUtils::isUnsupportedContainer(container)) {
                continue;
            }

            int port = portStr.toInt();
            TransportProto transportProto = ProtocolUtils::transportProtoFromString(transportProtoStr);

            auto installer = createInstaller(container);
            ContainerConfig config = installer->createBaseConfig(container, port, transportProto);
            ErrorCode extractError = installer->extractConfigFromContainer(container, credentials, &sshSession, config);

            if (extractError != ErrorCode::NoError && extractError != ErrorCode::ServerContainerMissingError) {
                return extractError;
            }

            installedContainers.insert(container, config);
        }
    }

    return ErrorCode::NoError;
}

ErrorCode InstallController::setDockerContainerEnabledState(const QString &serverId, DockerContainer container, bool enabled)
{
    if (container != DockerContainer::MtProxy && container != DockerContainer::Telemt) {
        return ErrorCode::InternalError;
    }
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }
    const QString containerName = ContainerUtils::containerToString(container);
    SshSession sshSession;
    const QString script = enabled ? QStringLiteral("sudo docker start %1").arg(containerName)
                                   : QStringLiteral("sudo docker stop %1").arg(containerName);
    const ErrorCode runError = sshSession.runScript(credentials, script);
    if (runError != ErrorCode::NoError) {
        return runError;
    }
    ContainerConfig currentConfig = adminConfig->containerConfig(container);
    bool persist = false;
    if (auto *mtConfig = currentConfig.getMtProxyProtocolConfig()) {
        mtConfig->isEnabled = enabled;
        persist = true;
    } else if (auto *telemtConfig = currentConfig.getTelemtProtocolConfig()) {
        telemtConfig->isEnabled = enabled;
        persist = true;
    }
    if (persist) {
        adminConfig->updateContainerConfig(container, currentConfig);
        m_serversRepository->editServer(serverId, adminConfig->toJson(), serverConfigUtils::ConfigType::SelfHostedAdmin);
    }
    return ErrorCode::NoError;
}

ErrorCode InstallController::queryDockerContainerStatus(const QString &serverId, DockerContainer container, int &statusOut)
{
    statusOut = 3;
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }
    const QString containerName = ContainerUtils::containerToString(container);
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data;
        return ErrorCode::NoError;
    };
    SshSession sshSession;
    const QString script = QStringLiteral(
            "sudo docker inspect --format '{{.State.Status}}' %1 2>/dev/null || echo 'not_found'")
            .arg(containerName);
    const ErrorCode errorCode = sshSession.runScript(credentials, script, cbReadStdOut);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }
    const QString status = stdOut.trimmed();
    if (status == QLatin1String("running")) {
        statusOut = 1;
    } else if (status == QLatin1String("not_found") || status.isEmpty()) {
        statusOut = 0;
    } else if (status == QLatin1String("exited") || status == QLatin1String("created")
               || status == QLatin1String("paused")) {
        statusOut = 2;
    } else {
        statusOut = 3;
    }
    return ErrorCode::NoError;
}

ErrorCode InstallController::queryMtProxyDiagnostics(const QString &serverId, DockerContainer container, int listenPort,
                                                     MtProxyContainerDiagnostics &out)
{
    out = {};
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }
    SshSession sshSession;
    return MtProxyInstaller::queryDiagnostics(sshSession, credentials, container, listenPort, out);
}

QString InstallController::fetchDockerContainerSecret(const QString &serverId, DockerContainer container)
{
    if (container != DockerContainer::MtProxy && container != DockerContainer::Telemt) {
        return {};
    }
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return {};
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return {};
    }
    const QString containerName = ContainerUtils::containerToString(container);
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data;
        return ErrorCode::NoError;
    };
    SshSession sshSession;
    const QString path = QStringLiteral("/data/secret");
    const QString cmd = QStringLiteral("sudo docker exec %1 cat %2").arg(containerName, path);
    const ErrorCode errorCode = sshSession.runScript(credentials, cmd, cbReadStdOut);
    if (errorCode != ErrorCode::NoError) {
        return {};
    }
    const QString secret = stdOut.trimmed();
    static const QRegularExpression hex32(QStringLiteral("^[0-9a-fA-F]{32}$"));
    return hex32.match(secret).hasMatch() ? secret : QString();
}
