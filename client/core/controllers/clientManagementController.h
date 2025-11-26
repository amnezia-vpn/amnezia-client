#ifndef CLIENTMANAGEMENTCONTROLLER_H
#define CLIENTMANAGEMENTCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"
#include "core/defs.h"
#include "core/repositories/serversRepository.h"

class ClientManagementController : public QObject
{
    Q_OBJECT

public:
    struct WgShowData
    {
        QString clientId;
        QString latestHandshake;
        QString dataReceived;
        QString dataSent;
        QString allowedIps;
    };

    explicit ClientManagementController(std::shared_ptr<ServersRepository> serversRepository, QObject *parent = nullptr);

signals:
    void clientsUpdated(const QJsonArray &clients);
    void clientAdded(const QJsonObject &client);
    void clientRenamed(int row, const QString &newName);
    void clientRevoked(int row);
    void adminConfigRevoked(DockerContainer container);

public slots:
    ErrorCode updateClients(const DockerContainer container, const ServerCredentials &credentials,
                            const QSharedPointer<ServerController> &serverController);
    ErrorCode appendClient(const DockerContainer container, const ServerCredentials &credentials, const QJsonObject &containerConfig,
                          const QString &clientName, const QSharedPointer<ServerController> &serverController);
    ErrorCode appendClient(QJsonObject &protocolConfig, const QString &clientName, const DockerContainer container,
                           const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController);
    ErrorCode appendClient(const QString &clientId, const QString &clientName, const DockerContainer container,
                           const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController);
    ErrorCode renameClient(const int row, const QString &userName, const DockerContainer container,
                           const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController, bool addTimeStamp = false);
    ErrorCode revokeClient(const int index, const DockerContainer container, const ServerCredentials &credentials,
                          const int serverIndex, const QSharedPointer<ServerController> &serverController);
    ErrorCode revokeClient(const QJsonObject &containerConfig, const DockerContainer container, const ServerCredentials &credentials,
                          const int serverIndex, const QSharedPointer<ServerController> &serverController);

private:
    bool isClientExists(const QString &clientId, const QJsonArray &clientsTable);
    int clientIndexById(const QString &clientId, const QJsonArray &clientsTable);
    void migration(const QByteArray &clientsTableString, QJsonArray &clientsTable);

    ErrorCode revokeOpenVpn(const int row, const DockerContainer container, const ServerCredentials &credentials, const int serverIndex,
                            const QSharedPointer<ServerController> &serverController, QJsonArray &clientsTable);
    ErrorCode revokeWireGuard(const int row, const DockerContainer container, const ServerCredentials &credentials,
                              const QSharedPointer<ServerController> &serverController, QJsonArray &clientsTable);
    ErrorCode revokeXray(const int row, const DockerContainer container, const ServerCredentials &credentials,
                         const QSharedPointer<ServerController> &serverController, QJsonArray &clientsTable);

    ErrorCode getOpenVpnClients(const DockerContainer container, const ServerCredentials &credentials,
                                const QSharedPointer<ServerController> &serverController, int &count, QJsonArray &clientsTable);
    ErrorCode getWireGuardClients(const DockerContainer container, const ServerCredentials &credentials,
                                  const QSharedPointer<ServerController> &serverController, int &count, QJsonArray &clientsTable);
    ErrorCode getXrayClients(const DockerContainer container, const ServerCredentials& credentials,
                             const QSharedPointer<ServerController> &serverController, int &count, QJsonArray &clientsTable);

    ErrorCode wgShow(const DockerContainer container, const ServerCredentials &credentials,
                     const QSharedPointer<ServerController> &serverController, std::vector<WgShowData> &data);

    std::shared_ptr<ServersRepository> m_serversRepository;
    QJsonArray m_clientsTable;
};

#endif // CLIENTMANAGEMENTCONTROLLER_H

