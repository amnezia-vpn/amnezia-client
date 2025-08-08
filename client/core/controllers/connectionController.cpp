#include "connectionController.h"

#include <QtConcurrent>

#include "core/controllers/vpnConfigurationController.h"
#include "core/controllers/selfhosted/serverController.h"
#include "core/models/containers/containers_defs.h"
#include "core/models/containers/containerConfig.h"
#include "settings.h"
#include "logger.h"
#include "utilities.h"

namespace
{
    Logger logger("ConnectionController");
}

ConnectionController::ConnectionController(const QSharedPointer<VpnConnection> &vpnConnection,
                                           std::shared_ptr<Settings> settings,
                                           QObject *parent)
    : QObject(parent), m_vpnConnection(vpnConnection), m_settings(settings)
{
}

QFuture<QJsonObject> ConnectionController::prepareVpnConfiguration(const QSharedPointer<ServerConfig> &serverConfig,
                                                                  DockerContainer container,
                                                                  const QPair<QString, QString> &dns) const
{
    return QtConcurrent::run([this, serverConfig, container, dns]() -> QJsonObject {
        logger.info() << "Preparing VPN configuration for container" << ContainerProps::containerToString(container);
        
        QSharedPointer<ServerController> serverController(new ServerController(m_settings));
        VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

        QString containerName = ContainerProps::containerToString(container);
        const ContainerConfig &containerConfig = serverConfig->containerConfigs.value(containerName);

        auto vpnConfiguration = vpnConfigurationController.createVpnConfiguration(dns,
                                                                                 serverConfig,
                                                                                 containerConfig,
                                                                                 container);
        
        emit configurationPrepared(vpnConfiguration);
        return vpnConfiguration;
    });
}

QFuture<ErrorCode> ConnectionController::openConnection(const int serverIndex,
                                                        const QSharedPointer<ServerConfig> &serverConfig,
                                                        const DockerContainer container,
                                                        const ServerCredentials &credentials,
                                                        const QPair<QString, QString> &dns)
{
    return QtConcurrent::run([this, serverIndex, serverConfig, container, credentials, dns]() -> ErrorCode {
        if (!isServerSupported(container)) {
            emit connectionError(ErrorCode::NotSupportedOnThisPlatform);
            return ErrorCode::NotSupportedOnThisPlatform;
        }

        QSharedPointer<ServerController> serverController(new ServerController(m_settings));
        VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

        const QString containerName = ContainerProps::containerToString(container);
        const ContainerConfig &containerConfig = serverConfig->containerConfigs.value(containerName);
        auto vpnConfiguration = vpnConfigurationController.createVpnConfiguration(dns, serverConfig, containerConfig, container);
        emit connectionProgress(QString("Connecting to %1...").arg(ContainerProps::containerToString(container)));

        QMetaObject::invokeMethod(m_vpnConnection.get(), "connectToVpn", Qt::QueuedConnection,
                                  Q_ARG(int, serverIndex),
                                  Q_ARG(ServerCredentials, credentials),
                                  Q_ARG(DockerContainer, container),
                                  Q_ARG(QJsonObject, vpnConfiguration));
        return ErrorCode::NoError;
    });
}

QFuture<ErrorCode> ConnectionController::closeConnection()
{
    return QtConcurrent::run([this]() -> ErrorCode {
        QMetaObject::invokeMethod(m_vpnConnection.get(), "disconnectFromVpn", Qt::QueuedConnection);
        return ErrorCode::NoError;
    });
}

bool ConnectionController::isServerSupported(DockerContainer container) const
{
    return ContainerProps::isSupportedByCurrentPlatform(container);
}
