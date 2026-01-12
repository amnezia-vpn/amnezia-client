#ifndef CLIENTMANAGEMENTCONTROLLER_H
#define CLIENTMANAGEMENTCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>

#include "containers/containers_defs.h"
#include "core/utils/sshSession.h"
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

    explicit ClientManagementController(ServersRepository* serversRepository, QObject *parent = nullptr);

signals:
    void clientsUpdated(const QJsonArray &clients);
    void clientAdded(const QJsonObject &client);
    void clientRenamed(int row, const QString &newName);
    void clientRevoked(int row);
    void adminConfigRevoked(DockerContainer container);

public slots:
    ErrorCode updateClients(const DockerContainer container, const ServerCredentials &credentials,
                            SshSession* sshSession);
    ErrorCode appendClient(const DockerContainer container, const ServerCredentials &credentials, const QJsonObject &containerConfig,
                          const QString &clientName, SshSession* sshSession);
    ErrorCode appendClient(QJsonObject &protocolConfig, const QString &clientName, const DockerContainer container,
                           const ServerCredentials &credentials, SshSession* sshSession);
    ErrorCode appendClient(const QString &clientId, const QString &clientName, const DockerContainer container,
                           const ServerCredentials &credentials, SshSession* sshSession);
    ErrorCode renameClient(const int row, const QString &userName, const DockerContainer container,
                           const ServerCredentials &credentials, SshSession* sshSession, bool addTimeStamp = false);
    ErrorCode revokeClient(const int index, const DockerContainer container, const ServerCredentials &credentials,
                          const int serverIndex, SshSession* sshSession);
    ErrorCode revokeClient(const QJsonObject &containerConfig, const DockerContainer container, const ServerCredentials &credentials,
                          const int serverIndex, SshSession* sshSession);

private:
    bool isClientExists(const QString &clientId, const QJsonArray &clientsTable);
    int clientIndexById(const QString &clientId, const QJsonArray &clientsTable);
    void migration(const QByteArray &clientsTableString, QJsonArray &clientsTable);

    ErrorCode revokeOpenVpn(const int row, const DockerContainer container, const ServerCredentials &credentials, const int serverIndex,
                            SshSession* sshSession, QJsonArray &clientsTable);
    ErrorCode revokeWireGuard(const int row, const DockerContainer container, const ServerCredentials &credentials,
                              SshSession* sshSession, QJsonArray &clientsTable);
    ErrorCode revokeXray(const int row, const DockerContainer container, const ServerCredentials &credentials,
                         SshSession* sshSession, QJsonArray &clientsTable);

    ErrorCode getOpenVpnClients(const DockerContainer container, const ServerCredentials &credentials,
                                SshSession* sshSession, int &count, QJsonArray &clientsTable);
    ErrorCode getWireGuardClients(const DockerContainer container, const ServerCredentials &credentials,
                                  SshSession* sshSession, int &count, QJsonArray &clientsTable);
    ErrorCode getXrayClients(const DockerContainer container, const ServerCredentials& credentials,
                             SshSession* sshSession, int &count, QJsonArray &clientsTable);

    ErrorCode wgShow(const DockerContainer container, const ServerCredentials &credentials,
                     SshSession* sshSession, std::vector<WgShowData> &data);

    ServersRepository* m_serversRepository;
    QJsonArray m_clientsTable;
};

#endif // CLIENTMANAGEMENTCONTROLLER_H

