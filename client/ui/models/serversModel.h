#ifndef SERVERSMODEL_H
#define SERVERSMODEL_H

#include <QAbstractListModel>
#include <QVector>

#include "core/utils/selfhosted/sshSession.h"
#include "core/models/serverDescription.h"

class ServersModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        ServerDescriptionRole,
        HostNameRole,
        ServerIdRole,

        CredentialsLoginRole,

        IsDefaultRole,

        HasWriteAccessRole,

        DefaultContainerRole,

        HasInstalledContainers,

        IsServerFromGatewayApiRole,
        IsSubscriptionExpiredRole,
        IsSubscriptionExpiringSoonRole,
    };

    ServersModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant data(const int index, int role = Qt::DisplayRole) const;

public slots:
    void updateModel(const QVector<amnezia::ServerDescription> &descriptions,
                     const QString &defaultServerId);
    void setDefaultServerId(const QString &serverId);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    ServerCredentials serverCredentials(int index) const;

    QVector<amnezia::ServerDescription> m_descriptions;

    QString m_defaultServerId;
};

#endif // SERVERSMODEL_H
