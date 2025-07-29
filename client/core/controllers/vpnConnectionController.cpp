#include "vpnConnectionController.h"

#include <QtConcurrent>

#include "core/controllers/vpnConfigurationController.h"
#include "core/controllers/selfhosted/serverController.h"
#include "settings.h"
#include "logger.h"
#include "utilities.h"

namespace
{
    Logger logger("VpnConnectionController");
}

VpnConnectionController::VpnConnectionController(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
}

QFuture<QJsonObject> VpnConnectionController::prepareVpnConfiguration(const QSharedPointer<ServerConfig> &serverConfig,
                                                                     DockerContainer container,
                                                                     const QJsonObject &containerConfig)
{
    return QtConcurrent::run([this, serverConfig, container, containerConfig]() -> QJsonObject {
        logger.info() << "Preparing VPN configuration for container" << ContainerProps::containerToString(container);
        
        QSharedPointer<ServerController> serverController(new ServerController(m_settings));
        VpnConfigurationsController vpnConfigurationController(m_settings, serverController);
        
        // TODO: Extract DNS pair from serverConfig
        QPair<QString, QString> dns = qMakePair(QString(), QString());
        
        auto vpnConfiguration = vpnConfigurationController.createVpnConfiguration(dns, 
                                                                                 serverConfig->toJson(), 
                                                                                 containerConfig, 
                                                                                 container);
        
        emit configurationPrepared(vpnConfiguration);
        return vpnConfiguration;
    });
}

QFuture<bool> VpnConnectionController::validateConnection(const QSharedPointer<ServerConfig> &serverConfig,
                                                        DockerContainer container)
{
    return QtConcurrent::run([this, serverConfig, container]() -> bool {
        logger.info() << "Validating connection for container" << ContainerProps::containerToString(container);
        
        if (!isServerSupported(serverConfig, container)) {
            return false;
        }
        
        // TODO: Implement connection validation logic
        // Check if container is supported by current platform
        // Check if server is reachable
        // Check if container is properly configured
        
        return true;
    });
}

QFuture<ErrorCode> VpnConnectionController::establishConnection(const QSharedPointer<ServerConfig> &serverConfig,
                                                              DockerContainer container,
                                                              const QJsonObject &vpnConfiguration)
{
    return QtConcurrent::run([this, serverConfig, container, vpnConfiguration]() -> ErrorCode {
        logger.info() << "Establishing connection for container" << ContainerProps::containerToString(container);
        
        // TODO: Implement connection establishment logic
        // This will delegate to VpnConnection or similar
        
        emit connectionEstablished();
        return ErrorCode::NoError;
    });
}

QFuture<ErrorCode> VpnConnectionController::terminateConnection()
{
    return QtConcurrent::run([this]() -> ErrorCode {
        logger.info() << "Terminating VPN connection";
        
        // TODO: Implement connection termination logic
        // This will delegate to VpnConnection or similar
        
        emit connectionTerminated();
        return ErrorCode::NoError;
    });
}

bool VpnConnectionController::isServerSupported(const QSharedPointer<ServerConfig> &serverConfig, DockerContainer container) const
{
    // TODO: Implement server support validation
    // Check if container is supported by current platform
    Q_UNUSED(serverConfig)
    
    return ContainerProps::isSupportedByCurrentPlatform(container);
}

QJsonObject VpnConnectionController::createVpnConfiguration(const QPair<QString, QString> &dns,
                                                           const QJsonObject &serverConfig,
                                                           const QJsonObject &containerConfig,
                                                           DockerContainer container)
{
    // TODO: Implement VPN configuration creation logic
    // This will be moved from existing code
    Q_UNUSED(dns)
    Q_UNUSED(serverConfig)
    Q_UNUSED(containerConfig)
    Q_UNUSED(container)
    
    return QJsonObject();
} 
