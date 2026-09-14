#ifndef CLIENTMANAGEMENTMODEL_H
#define CLIENTMANAGEMENTMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>

class ClientManagementModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        ClientIdRole = Qt::UserRole + 1,
        ClientNameRole,
        CreationDateRole,
        LatestHandshakeRole,
        DataReceivedRole,
        DataSentRole,
        AllowedIpsRole
    };

    explicit ClientManagementModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const QJsonArray &clients);
    void updateClientName(int row, const QString &newName);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QJsonArray m_clientsTable;
};

#endif // CLIENTMANAGEMENTMODEL_H
