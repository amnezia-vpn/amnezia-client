#ifndef CLIENTMANAGEMENTMODEL_H
#define CLIENTMANAGEMENTMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>

#include "core/controllers/serverController.h"
#include "settings.h"

class ClientManagementModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        ClientNameRole = Qt::UserRole + 1,
        CreationDateRole,
        LatestHandshakeRole,
        DataReceivedRole,
        DataSentRole,
        AllowedIpsRole
    };

    struct WgShowData
    {
        QString clientId;
        QString latestHandshake;
        QString dataReceived;
        QString dataSent;
        QString allowedIps;
    };

    ClientManagementModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    ErrorCode updateModel(const DockerContainer container, const ServerCredentials &credentials,
                          const QSharedPointer<ServerController> &serverController);
    ErrorCode appendClient(const DockerContainer container, const ServerCredentials &credentials, const QJsonObject &containerConfig,
                           const QString &clientName, const QSharedPointer<ServerController> &serverController);
    ErrorCode appendClient(QJsonObject &protocolConfig, const QString &clientName,const DockerContainer container,
                           const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController);
    ErrorCode appendClient(const QString &clientId, const QString &clientName, const DockerContainer container,
                           const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController);
    ErrorCode renameClient(const int row, const QString &userName, const DockerContainer container, const ServerCredentials &credentials,
                           const QSharedPointer<ServerController> &serverController, bool addTimeStamp = false);
    ErrorCode revokeClient(const int index, const DockerContainer container, const ServerCredentials &credentials, const int serverIndex,
                           const QSharedPointer<ServerController> &serverController);
    ErrorCode revokeClient(const QJsonObject &containerConfig, const DockerContainer container, const ServerCredentials &credentials,
                           const int serverIndex, const QSharedPointer<ServerController> &serverController);

protected:
    QHash<int, QByteArray> roleNames() const override;

signals:
    void adminConfigRevoked(const DockerContainer container);

private:
    bool isClientExists(const QString &clientId);

    void migration(const QByteArray &clientsTableString);

    ErrorCode revokeOpenVpn(const int row, const DockerContainer container, const ServerCredentials &credentials, const int serverIndex,
                            const QSharedPointer<ServerController> &serverController);
    ErrorCode revokeWireGuard(const int row, const DockerContainer container, const ServerCredentials &credentials,
                              const QSharedPointer<ServerController> &serverController);
    ErrorCode revokeXray(const int row, const DockerContainer container, const ServerCredentials &credentials,
                         const QSharedPointer<ServerController> &serverController);

    ErrorCode getOpenVpnClients(const DockerContainer container, const ServerCredentials &credentials,
                                const QSharedPointer<ServerController> &serverController, int &count);
    ErrorCode getWireGuardClients(const DockerContainer container, const ServerCredentials &credentials,
                                  const QSharedPointer<ServerController> &serverController, int &count);
    ErrorCode getXrayClients(const DockerContainer container, const ServerCredentials& credentials,
                             const QSharedPointer<ServerController> &serverController, int &count);

    ErrorCode wgShow(const DockerContainer container, const ServerCredentials &credentials,
                     const QSharedPointer<ServerController> &serverController, std::vector<WgShowData> &data);

    QJsonArray m_clientsTable;

    std::shared_ptr<Settings> m_settings;
};

#endif // CLIENTMANAGEMENTMODEL_H
