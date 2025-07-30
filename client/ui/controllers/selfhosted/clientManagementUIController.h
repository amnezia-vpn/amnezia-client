#ifndef CLIENT_MANAGEMENT_UI_CONTROLLER_H
#define CLIENT_MANAGEMENT_UI_CONTROLLER_H

#include <QObject>
#include <QSharedPointer>

#include "core/defs.h"
#include "core/models/containers/containers_defs.h" 
#include "core/controllers/selfhosted/clientManagementController.h"

class ClientManagementUIController : public QObject
{
    Q_OBJECT

public:
    explicit ClientManagementUIController(
        QSharedPointer<ClientManagementController> clientManagementController,
        QObject *parent = nullptr);

public slots:
    ErrorCode updateModel(const DockerContainer container, const ServerCredentials &credentials);
                          
    ErrorCode renameClient(const int row, const QString &userName, const DockerContainer container, 
                          const ServerCredentials &credentials, bool addTimeStamp = false);
                          
    ErrorCode revokeClient(const int index, const DockerContainer container, 
                          const ServerCredentials &credentials, const int serverIndex);

    // Access to core controller for other UI controllers
    QSharedPointer<ClientManagementController> getClientManagementController() const;

private:
    QSharedPointer<ClientManagementController> m_clientManagementController;
};

#endif // CLIENT_MANAGEMENT_UI_CONTROLLER_H 
