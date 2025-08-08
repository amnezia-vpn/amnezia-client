#ifndef CLIENTMANAGEMENTCONTROLLER_H
#define CLIENTMANAGEMENTCONTROLLER_H

#include <QObject>
#include <QSharedPointer>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <vector>

#include "core/defs.h"
#include "core/models/containers/containers_defs.h"
#include "core/models/containers/containerConfig.h"
#include "core/models/protocols/protocolConfig.h"
#include "core/models/clientInfo.h"

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

    // Core methods using ClientInfo model
    ErrorCode updateClientsData(const DockerContainer container, const ServerCredentials &credentials,
                                const QSharedPointer<ServerController> &serverController, QList<ClientInfo> &clientsList);
    
    ErrorCode appendClient(const DockerContainer container, const ServerCredentials &credentials, 
                          const ContainerConfig &containerConfig, const QString &clientName, 
                          const QSharedPointer<ServerController> &serverController, QList<ClientInfo> &clientsList);
    
    ErrorCode appendClient(QSharedPointer<ProtocolConfig> &protocolConfig, const QString &clientName, const DockerContainer container,
                          const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController, 
                          QList<ClientInfo> &clientsList);
    
    ErrorCode appendClient(const QString &clientId, const QString &clientName, const DockerContainer container,
                          const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController, 
                          QList<ClientInfo> &clientsList);
    
    ErrorCode renameClient(const int row, const QString &clientName, const DockerContainer container, 
                          const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController, 
                          QList<ClientInfo> &clientsList, bool addTimeStamp = false);
    
    ErrorCode revokeClient(const int row, const DockerContainer container, const ServerCredentials &credentials, 
                          const int serverIndex, const QSharedPointer<ServerController> &serverController, 
                          QList<ClientInfo> &clientsList);
    
    ErrorCode revokeClient(const ContainerConfig &containerConfig, const DockerContainer container, 
                          const ServerCredentials &credentials, const int serverIndex, 
                          const QSharedPointer<ServerController> &serverController, QList<ClientInfo> &clientsList);

    // WireGuard specific operations
    ErrorCode wgShow(const DockerContainer container, const ServerCredentials &credentials,
                     const QSharedPointer<ServerController> &serverController, std::vector<WgShowData> &data);

signals:
    void adminConfigRevoked(const DockerContainer container);
    void clientsDataUpdated(const QList<ClientInfo> &clientsList);
    void clientAdded(const QString &clientId, const QString &clientName);
    void clientRenamed(const int row, const QString &newName);
    void clientRevoked(const int row);
    
    void clientAppendCompleted(ErrorCode errorCode);
    void nativeConfigClientAppendCompleted(ErrorCode errorCode);

private:
    // Protocol-specific client management
    ErrorCode getOpenVpnClients(const DockerContainer container, const ServerCredentials &credentials,
                               const QSharedPointer<ServerController> &serverController, int &count, QList<ClientInfo> &clientsList);
    
    ErrorCode getWireGuardClients(const DockerContainer container, const ServerCredentials &credentials,
                                 const QSharedPointer<ServerController> &serverController, int &count, QList<ClientInfo> &clientsList);
    
    ErrorCode getXrayClients(const DockerContainer container, const ServerCredentials& credentials,
                            const QSharedPointer<ServerController> &serverController, int &count, QList<ClientInfo> &clientsList);

    // Protocol-specific client revocation
    ErrorCode revokeOpenVpn(const int row, const DockerContainer container, const ServerCredentials &credentials, 
                           const int serverIndex, const QSharedPointer<ServerController> &serverController, 
                           QList<ClientInfo> &clientsList);
    
    ErrorCode revokeWireGuard(const int row, const DockerContainer container, const ServerCredentials &credentials,
                             const QSharedPointer<ServerController> &serverController, QList<ClientInfo> &clientsList);
    
    ErrorCode revokeXray(const int row, const DockerContainer container, const ServerCredentials &credentials,
                        const QSharedPointer<ServerController> &serverController, QList<ClientInfo> &clientsList);

    // Helper methods
    bool isClientExists(const QString &clientId, const QList<ClientInfo> &clientsList);
    void migration(const QByteArray &clientsTableString, QList<ClientInfo> &clientsList);
    QString getClientsTableFilePath(const DockerContainer container);
    
    // JSON serialization for persistence only
    static QList<ClientInfo> clientsFromJsonArray(const QJsonArray &jsonArray);
    static QJsonArray clientsToJsonArray(const QList<ClientInfo> &clientsList);

    std::shared_ptr<Settings> m_settings;
};

#endif // CLIENTMANAGEMENTCONTROLLER_H 
