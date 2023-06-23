#include "installController.h"

#include <QJsonObject>

#include "core/errorstrings.h"
#include "core/servercontroller.h"
#include "utilities.h"

InstallController::InstallController(const QSharedPointer<ServersModel> &serversModel,
                                     const QSharedPointer<ContainersModel> &containersModel,
                                     const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_containersModel(containersModel), m_settings(settings)
{
}

void InstallController::install(DockerContainer container, int port, TransportProto transportProto)
{
    Proto mainProto = ContainerProps::defaultProtocol(container);

    QJsonObject containerConfig { { config_key::port, QString::number(port) },
                                  { config_key::transport_proto,
                                    ProtocolProps::transportProtoToString(transportProto, mainProto) } };
    QJsonObject config { { config_key::container, ContainerProps::containerToString(container) },
                         { ProtocolProps::protoToString(mainProto), containerConfig } };

    if (m_shouldCreateServer) {
        if (isServerAlreadyExists()) {
            return;
        }
        installServer(container, config);
    } else {
        installContainer(container, config);
    }
}

void InstallController::installServer(DockerContainer container, QJsonObject &config)
{
    ServerController serverController(m_settings);

    QMap<DockerContainer, QJsonObject> installedContainers;
    ErrorCode errorCode =
            serverController.getAlreadyInstalledContainers(m_currentlyInstalledServerCredentials, installedContainers);
    if (!installedContainers.contains(container)) {
        errorCode = serverController.setupContainer(m_currentlyInstalledServerCredentials, container, config);
        installedContainers.insert(container, config);
    }

    bool isInstalledContainerFound = false;
    if (!installedContainers.isEmpty()) {
        isInstalledContainerFound = true;
    }

    if (errorCode == ErrorCode::NoError) {
        QJsonObject server;
        server.insert(config_key::hostName, m_currentlyInstalledServerCredentials.hostName);
        server.insert(config_key::userName, m_currentlyInstalledServerCredentials.userName);
        server.insert(config_key::password, m_currentlyInstalledServerCredentials.secretData);
        server.insert(config_key::port, m_currentlyInstalledServerCredentials.port);
        server.insert(config_key::description, m_settings->nextAvailableServerName());

        QJsonArray containerConfigs;
        for (const QJsonObject &containerConfig : qAsConst(installedContainers)) {
            containerConfigs.append(containerConfig);
        }

        server.insert(config_key::containers, containerConfigs);
        server.insert(config_key::defaultContainer, ContainerProps::containerToString(container));

        m_serversModel->addServer(server);
        m_serversModel->setDefaultServerIndex(m_serversModel->getServersCount() - 1);

        emit installServerFinished(isInstalledContainerFound);
        return;
    }

    emit installationErrorOccurred(errorString(errorCode));
}

void InstallController::installContainer(DockerContainer container, QJsonObject &config)
{
    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    ServerCredentials serverCredentials =
            qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));

    ServerController serverController(m_settings);

    QMap<DockerContainer, QJsonObject> installedContainers;
    ErrorCode errorCode = serverController.getAlreadyInstalledContainers(serverCredentials, installedContainers);

    bool isInstalledContainerFound = false;
    if (!installedContainers.isEmpty()) {
        isInstalledContainerFound = true;
    }

    if (!installedContainers.contains(container)) {
        errorCode = serverController.setupContainer(serverCredentials, container, config);
        installedContainers.insert(container, config);
    }

    if (errorCode == ErrorCode::NoError) {
        for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
            auto modelIndex = m_containersModel->index(iterator.key());
            QJsonObject containerConfig =
                    qvariant_cast<QJsonObject>(m_containersModel->data(modelIndex, ContainersModel::Roles::ConfigRole));
            if (containerConfig.isEmpty()) {
                m_containersModel->setData(m_containersModel->index(iterator.key()), iterator.value(),
                                           ContainersModel::Roles::ConfigRole);
            }
        }

        emit installContainerFinished(isInstalledContainerFound);
        return;
    }

    emit installationErrorOccurred(errorString(errorCode));
}

bool InstallController::isServerAlreadyExists()
{
    for (int i = 0; i < m_serversModel->getServersCount(); i++) {
        auto modelIndex = m_serversModel->index(i);
        const ServerCredentials credentials =
                qvariant_cast<ServerCredentials>(m_serversModel->data(modelIndex, ServersModel::Roles::CredentialsRole));
        if (m_currentlyInstalledServerCredentials.hostName == credentials.hostName
            && m_currentlyInstalledServerCredentials.port == credentials.port) {
            emit serverAlreadyExists(i);
            return true;
        }
    }
    return false;
}

void InstallController::scanServerForInstalledContainers()
{
    int serverIndex = m_serversModel->getCurrentlyProcessedServerIndex();
    ServerCredentials serverCredentials =
            qvariant_cast<ServerCredentials>(m_serversModel->data(serverIndex, ServersModel::Roles::CredentialsRole));

    ServerController serverController(m_settings);

    QMap<DockerContainer, QJsonObject> installedContainers;
    ErrorCode errorCode = serverController.getAlreadyInstalledContainers(serverCredentials, installedContainers);

    if (errorCode == ErrorCode::NoError) {
        bool isInstalledContainerAddedToGui = false;

        for (auto iterator = installedContainers.begin(); iterator != installedContainers.end(); iterator++) {
            auto modelIndex = m_containersModel->index(iterator.key());
            QJsonObject containerConfig =
                    qvariant_cast<QJsonObject>(m_containersModel->data(modelIndex, ContainersModel::Roles::ConfigRole));
            if (containerConfig.isEmpty()) {
                m_containersModel->setData(m_containersModel->index(iterator.key()), iterator.value(),
                                           ContainersModel::Roles::ConfigRole);
                isInstalledContainerAddedToGui = true;
            }
        }

        emit scanServerFinished(isInstalledContainerAddedToGui);
        return;
    }

    emit installationErrorOccurred(errorString(errorCode));
}

QRegularExpression InstallController::ipAddressPortRegExp()
{
    return Utils::ipAddressPortRegExp();
}

void InstallController::setCurrentlyInstalledServerCredentials(const QString &hostName, const QString &userName,
                                                               const QString &secretData)
{
    m_currentlyInstalledServerCredentials.hostName = hostName;
    if (m_currentlyInstalledServerCredentials.hostName.contains(":")) {
        m_currentlyInstalledServerCredentials.port =
                m_currentlyInstalledServerCredentials.hostName.split(":").at(1).toInt();
        m_currentlyInstalledServerCredentials.hostName = m_currentlyInstalledServerCredentials.hostName.split(":").at(0);
    }
    m_currentlyInstalledServerCredentials.userName = userName;
    m_currentlyInstalledServerCredentials.secretData = secretData;
}

void InstallController::setShouldCreateServer(bool shouldCreateServer)
{
    m_shouldCreateServer = shouldCreateServer;
}
