#ifndef CLIENTMANAGEMENTMODEL_H
#define CLIENTMANAGEMENTMODEL_H

#include <QAbstractListModel>

#include "settings.h"

class ClientManagementModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ClientRoles {
        NameRole = Qt::UserRole + 1,
        CertIdRole,
        CertDataRole
    };

    struct ClientInfo
    {
        QString name;
        QString certId;
        QString certData;
    };

    ClientManagementModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    void clearData();
    void setContent(const QVector<ClientInfo> &data);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void setData(const QModelIndex &index, QVariant data, int role = Qt::DisplayRole);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    std::shared_ptr<Settings> m_settings; //TODO remove this?
    QVector<ClientInfo> m_content;
};

#endif // CLIENTMANAGEMENTMODEL_H
