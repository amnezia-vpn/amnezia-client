#ifndef CLIENTMANAGEMENTCONTROLLER_H
#define CLIENTMANAGEMENTCONTROLLER_H

#include <QObject>
#include <QSharedPointer>
#include <QJsonArray>
#include <QJsonObject>
#include <vector>

#include "core/defs.h"
#include "core/models/containers/containers_defs.h"

class ServerController;
class Settings;

using namespace amnezia;

struct WgShowData
{
    QString clientId;
    QString latestHandshake;
    QString dataReceived;
    QString dataSent;
    QString allowedIps;
};

class ClientManagementController : public QObject
{
    Q_OBJECT

public:
    explicit ClientManagementController(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    std::shared_ptr<Settings> getSettings() const { return m_settings; }

    // UI-facing methods (no ServerController parameter - created internally)
    ErrorCode updateClientsData(const DockerContainer container, const ServerCredentials &credentials);
    
    ErrorCode renameClient(const int row, const QString &clientName, const DockerContainer container, 
                          const ServerCredentials &credentials, bool addTimeStamp = false);
    
    ErrorCode revokeClient(const int row, const DockerContainer container, const ServerCredentials &credentials, 
                          const int serverIndex);

    // Internal methods (with clientsTable parameter for core operations)
    ErrorCode updateClientsData(const DockerContainer container, const ServerCredentials &credentials,
                                const QSharedPointer<ServerController> &serverController, QJsonArray &clientsTable);
    
    ErrorCode appendClient(const DockerContainer container, const ServerCredentials &credentials, 
                          const QJsonObject &containerConfig, const QString &clientName, 
                          const QSharedPointer<ServerController> &serverController, QJsonArray &clientsTable);
    
    ErrorCode appendClient(QJsonObject &protocolConfig, const QString &clientName, const DockerContainer container,
                          const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController, 
                          QJsonArray &clientsTable);
    
    ErrorCode appendClient(const QString &clientId, const QString &clientName, const DockerContainer container,
                          const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController, 
                          QJsonArray &clientsTable);
    
    ErrorCode renameClient(const int row, const QString &clientName, const DockerContainer container, 
                          const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController, 
                          QJsonArray &clientsTable, bool addTimeStamp = false);
    
    ErrorCode revokeClient(const int row, const DockerContainer container, const ServerCredentials &credentials, 
                          const int serverIndex, const QSharedPointer<ServerController> &serverController, 
                          QJsonArray &clientsTable);
    
    ErrorCode revokeClient(const QJsonObject &containerConfig, const DockerContainer container, 
                          const ServerCredentials &credentials, const int serverIndex, 
                          const QSharedPointer<ServerController> &serverController, QJsonArray &clientsTable);

    // WireGuard specific operations
    ErrorCode wgShow(const DockerContainer container, const ServerCredentials &credentials,
                     const QSharedPointer<ServerController> &serverController, std::vector<WgShowData> &data);

signals:
    void adminConfigRevoked(const DockerContainer container);
    void clientsDataUpdated(const QJsonArray &clientsTable);
    void clientAdded(const QString &clientId, const QString &clientName);
    void clientRenamed(const int row, const QString &newName);
    void clientRevoked(const int row);
    
    void clientAppendCompleted(ErrorCode errorCode);
    void nativeConfigClientAppendCompleted(ErrorCode errorCode);

private:
    // Protocol-specific client management
    ErrorCode getOpenVpnClients(const DockerContainer container, const ServerCredentials &credentials,
                               const QSharedPointer<ServerController> &serverController, int &count, QJsonArray &clientsTable);
    
    ErrorCode getWireGuardClients(const DockerContainer container, const ServerCredentials &credentials,
                                 const QSharedPointer<ServerController> &serverController, int &count, QJsonArray &clientsTable);
    
    ErrorCode getXrayClients(const DockerContainer container, const ServerCredentials& credentials,
                            const QSharedPointer<ServerController> &serverController, int &count, QJsonArray &clientsTable);

    // Protocol-specific client revocation
    ErrorCode revokeOpenVpn(const int row, const DockerContainer container, const ServerCredentials &credentials, 
                           const int serverIndex, const QSharedPointer<ServerController> &serverController, 
                           QJsonArray &clientsTable);
    
    ErrorCode revokeWireGuard(const int row, const DockerContainer container, const ServerCredentials &credentials,
                             const QSharedPointer<ServerController> &serverController, QJsonArray &clientsTable);
    
    ErrorCode revokeXray(const int row, const DockerContainer container, const ServerCredentials &credentials,
                        const QSharedPointer<ServerController> &serverController, QJsonArray &clientsTable);

    // Helper methods
    bool isClientExists(const QString &clientId, const QJsonArray &clientsTable);
    void migration(const QByteArray &clientsTableString, QJsonArray &clientsTable);
    QString getClientsTableFilePath(const DockerContainer container);

    std::shared_ptr<Settings> m_settings;
};

#endif // CLIENTMANAGEMENTCONTROLLER_H 
