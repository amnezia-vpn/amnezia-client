#ifndef CLIENTMANAGEMENTMODEL_H
#define CLIENTMANAGEMENTMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>

#include "core/servercontroller.h"
#include "settings.h"

class ClientManagementModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        ClientNameRole = Qt::UserRole + 1,
    };

    ClientManagementModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    ErrorCode updateModel(DockerContainer container, ServerCredentials credentials);
    ErrorCode appendClient(const QString &clientId, const QString &clientName, const DockerContainer container,
                           ServerCredentials credentials);
    ErrorCode renameClient(const int row, const QString &userName, const DockerContainer container,
                           ServerCredentials credentials);
    ErrorCode revokeClient(const int index, const DockerContainer container, ServerCredentials credentials);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    bool isClientExists(const QString &clientId);

    ErrorCode revokeOpenVpn(const int row, const DockerContainer container, ServerCredentials credentials);
    ErrorCode revokeWireGuard(const int row, const DockerContainer container, ServerCredentials credentials);

    ErrorCode getOpenVpnClients(ServerController &serverController, DockerContainer container, ServerCredentials credentials, int &count);
    ErrorCode getWireGuardClients(ServerController &serverController, DockerContainer container, ServerCredentials credentials, int &count);

    QJsonArray m_clientsTable;

    std::shared_ptr<Settings> m_settings;
};

#endif // CLIENTMANAGEMENTMODEL_H
