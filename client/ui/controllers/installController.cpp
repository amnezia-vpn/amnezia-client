#include "installController.h"

#include <QJsonObject>

#include "core/servercontroller.h"

InstallController::InstallController(const QSharedPointer<ServersModel> &serversModel,
                                    const QSharedPointer<ContainersModel> &containersModel,
                                    const std::shared_ptr<Settings> &settings,
                                    QObject *parent) : QObject(parent), m_serversModel(serversModel), m_containersModel(containersModel),  m_settings(settings)
{

}

ErrorCode InstallController::install(DockerContainer container, int port, TransportProto transportProto)
{
    Proto mainProto = ContainerProps::defaultProtocol(container);

    QJsonObject containerConfig {
        { config_key::port, QString::number(port) },
        { config_key::transport_proto, ProtocolProps::transportProtoToString(transportProto, mainProto) }
    };
    QJsonObject config {
        { config_key::container, ContainerProps::containerToString(container) },
        { ProtocolProps::protoToString(mainProto), containerConfig }
    };

    if (m_shouldCreateServer) {
        return installServer(container, config);
    } else {
        return installContainer(container, config);
    }
}

ErrorCode InstallController::installServer(DockerContainer container, QJsonObject& config)
{
    //todo check if container already installed
    ServerController serverController(m_settings);
    ErrorCode errorCode = serverController.setupContainer(m_currentlyInstalledServerCredentials, container, config);
    if (errorCode == ErrorCode::NoError) {
        QJsonObject server;
        server.insert(config_key::hostName, m_currentlyInstalledServerCredentials.hostName);
        server.insert(config_key::userName, m_currentlyInstalledServerCredentials.userName);
        server.insert(config_key::password, m_currentlyInstalledServerCredentials.secretData);
        server.insert(config_key::port, m_currentlyInstalledServerCredentials.port);
        server.insert(config_key::description, m_settings->nextAvailableServerName());

        server.insert(config_key::containers, QJsonArray{ config });
        server.insert(config_key::defaultContainer, ContainerProps::containerToString(container));

        m_settings->addServer(server);
        m_settings->setDefaultServer(m_settings->serversCount() - 1);

        //todo change to server finished
        emit installContainerFinished();
    }

    //todo error processing
    return errorCode;
}

ErrorCode InstallController::installContainer(DockerContainer container, QJsonObject& config)
{
    //todo check if container already installed
    ServerCredentials serverCredentials = m_serversModel->getCurrentlyProcessedServerCredentials();

    ServerController serverController(m_settings);
    ErrorCode errorCode = serverController.setupContainer(serverCredentials, container, config);
    if (errorCode == ErrorCode::NoError) {
        m_containersModel->setData(m_containersModel->index(container), config, ContainersModel::Roles::ConfigRole);
        emit installContainerFinished();
    }

    //todo error processing
    return errorCode;
}

void InstallController::setCurrentlyInstalledServerCredentials(const QString &hostName, const QString &userName, const QString &secretData)
{
    m_currentlyInstalledServerCredentials.hostName = hostName;
    if (m_currentlyInstalledServerCredentials.hostName.contains(":")) {
        m_currentlyInstalledServerCredentials.port = m_currentlyInstalledServerCredentials.hostName.split(":").at(1).toInt();
        m_currentlyInstalledServerCredentials.hostName = m_currentlyInstalledServerCredentials.hostName.split(":").at(0);
    }
    m_currentlyInstalledServerCredentials.userName = userName;
    m_currentlyInstalledServerCredentials.secretData = secretData;
}

void InstallController::setShouldCreateServer(bool shouldCreateServer)
{
    m_shouldCreateServer = shouldCreateServer;
}
