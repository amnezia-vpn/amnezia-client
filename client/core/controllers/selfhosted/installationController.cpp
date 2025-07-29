#include "installationController.h"

#include <QtConcurrent>

#include "core/controllers/vpnConfigurationController.h"
#include "core/networkUtilities.h"
#include "settings.h"
#include "logger.h"

namespace
{
    Logger logger("InstallationController");
}

InstallationController::InstallationController(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
}

QFuture<ErrorCode> InstallationController::installContainer(const QSharedPointer<ServerConfig> &serverConfig,
                                                           DockerContainer container,
                                                           const QJsonObject &containerConfig)
{
    return QtConcurrent::run([this, serverConfig, container, containerConfig]() -> ErrorCode {
        // TODO: Implement container installation logic
        // This will be moved from UI InstallController
        logger.info() << "Installing container" << ContainerProps::containerToString(container);
        return ErrorCode::NoError;
    });
}

QFuture<ErrorCode> InstallationController::scanServerForInstalledContainers(const QSharedPointer<ServerConfig> &serverConfig)
{
    return QtConcurrent::run([this, serverConfig]() -> ErrorCode {
        // TODO: Implement server scanning logic
        // This will be moved from UI InstallController
        logger.info() << "Scanning server for installed containers";
        return ErrorCode::NoError;
    });
}

QFuture<ErrorCode> InstallationController::removeContainer(const QSharedPointer<ServerConfig> &serverConfig,
                                                          DockerContainer container)
{
    return QtConcurrent::run([this, serverConfig, container]() -> ErrorCode {
        // TODO: Implement container removal logic
        logger.info() << "Removing container" << ContainerProps::containerToString(container);
        return ErrorCode::NoError;
    });
}

QFuture<ErrorCode> InstallationController::updateContainer(const QSharedPointer<ServerConfig> &serverConfig,
                                                          DockerContainer container,
                                                          const QJsonObject &containerConfig)
{
    return QtConcurrent::run([this, serverConfig, container, containerConfig]() -> ErrorCode {
        // TODO: Implement container update logic
        logger.info() << "Updating container" << ContainerProps::containerToString(container);
        return ErrorCode::NoError;
    });
}

QFuture<ErrorCode> InstallationController::rebootServer(const QSharedPointer<ServerConfig> &serverConfig)
{
    return QtConcurrent::run([this, serverConfig]() -> ErrorCode {
        // TODO: Implement server reboot logic
        logger.info() << "Rebooting server";
        return ErrorCode::NoError;
    });
}

QFuture<ErrorCode> InstallationController::removeAllContainers(const QSharedPointer<ServerConfig> &serverConfig)
{
    return QtConcurrent::run([this, serverConfig]() -> ErrorCode {
        // TODO: Implement all containers removal logic
        logger.info() << "Removing all containers";
        return ErrorCode::NoError;
    });
}

QFuture<bool> InstallationController::checkSshConnection(const QSharedPointer<ServerConfig> &serverConfig)
{
    return QtConcurrent::run([this, serverConfig]() -> bool {
        // TODO: Implement SSH connection check logic
        logger.info() << "Checking SSH connection";
        return true;
    });
}

QFuture<ErrorCode> InstallationController::mountSftpDrive(const QSharedPointer<ServerConfig> &serverConfig,
                                                         const QString &port,
                                                         const QString &password,
                                                         const QString &username)
{
    return QtConcurrent::run([this, serverConfig, port, password, username]() -> ErrorCode {
        // TODO: Implement SFTP mounting logic
        logger.info() << "Mounting SFTP drive";
        return ErrorCode::NoError;
    });
}

ErrorCode InstallationController::getAlreadyInstalledContainers(const ServerCredentials &serverCredentials,
                                                               const QSharedPointer<ServerController> &serverController,
                                                               QMap<DockerContainer, QJsonObject> &installedContainers)
{
    // TODO: Implement logic to get already installed containers
    // This will be moved from UI InstallController
    return ErrorCode::NoError;
} 
