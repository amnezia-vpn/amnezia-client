#ifndef USERSCONTROLLER_H
#define USERSCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/secureServersRepository.h"
#include "core/models/containerConfig.h"
#include "core/models/protocolConfig.h"

class UsersController : public QObject
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

    explicit UsersController(SecureServersRepository* serversRepository, QObject *parent = nullptr);

signals:
    void clientsUpdated(const QJsonArray &clients);
    void clientAdded(const QJsonObject &client);
    void clientRenamed(int row, const QString &newName);
    void clientRevoked(int row);
    void revokeFinished(ErrorCode errorCode);
    void adminConfigRevoked(const QString &serverId, DockerContainer container);

public slots:
    ErrorCode updateClients(const QString &serverId, const DockerContainer container);
    ErrorCode appendClient(const QString &serverId, const QString &clientId, const QString &clientName, const DockerContainer container);
    ErrorCode renameClient(const QString &serverId, const int row, const QString &userName, const DockerContainer container, bool addTimeStamp = false);
    ErrorCode revokeClient(const QString &serverId, const int index, const DockerContainer container);
    ErrorCode revokeClient(const QString &serverId, const ContainerConfig &containerConfig, const DockerContainer container);

private:
    bool isClientExists(const QString &clientId, const QJsonArray &clientsTable);
    int clientIndexById(const QString &clientId, const QJsonArray &clientsTable);
    void migration(const QByteArray &clientsTableString, QJsonArray &clientsTable);

    ErrorCode revokeOpenVpn(const int row, const DockerContainer container, const ServerCredentials &credentials,
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

    SecureServersRepository* m_serversRepository;
    QJsonArray m_clientsTable;
};

#endif // USERSCONTROLLER_H
