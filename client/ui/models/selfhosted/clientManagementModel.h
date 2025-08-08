#ifndef CLIENTMANAGEMENTMODEL_H
#define CLIENTMANAGEMENTMODEL_H

#include <QAbstractListModel>
#include <QList>

#include "core/controllers/selfhosted/clientManagementController.h"
#include "core/models/clientInfo.h"

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

    explicit ClientManagementModel(QSharedPointer<ClientManagementController> clientManagementController,
                                   QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    QHash<int, QByteArray> roleNames() const override;

signals:
    void adminConfigRevoked(const DockerContainer container);

private slots:
    void onClientsDataUpdated(const QList<ClientInfo> &clientsList);
    void onClientRenamed(const int row, const QString &newName);

private:
    QList<ClientInfo> m_clientsList;
    QSharedPointer<ClientManagementController> m_clientManagementController;
};

#endif // CLIENTMANAGEMENTMODEL_H
