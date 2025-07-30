#include "clientManagementUIController.h"

ClientManagementUIController::ClientManagementUIController(
    QSharedPointer<ClientManagementController> clientManagementController,
    QObject *parent)
    : QObject(parent),
      m_clientManagementController(clientManagementController)
{
}

ErrorCode ClientManagementUIController::updateModel(const DockerContainer container, const ServerCredentials &credentials)
{
    return m_clientManagementController->updateClientsData(container, credentials);
}

ErrorCode ClientManagementUIController::renameClient(const int row, const QString &userName, const DockerContainer container,
                                                    const ServerCredentials &credentials, bool addTimeStamp)
{
    return m_clientManagementController->renameClient(row, userName, container, credentials, addTimeStamp);
}

ErrorCode ClientManagementUIController::revokeClient(const int index, const DockerContainer container,
                                                    const ServerCredentials &credentials, const int serverIndex)
{
    return m_clientManagementController->revokeClient(index, container, credentials, serverIndex);
}

QSharedPointer<ClientManagementController> ClientManagementUIController::getClientManagementController() const
{
    return m_clientManagementController;
} 
