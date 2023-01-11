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
        OpenVpnCertIdRole,
        OpenVpnCertDataRole,
        WireGuardPublicKey,
    };

    ClientManagementModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    void clearData();
    void setContent(const QVector<QVariant> &data);
    QJsonObject getContent(Proto protocol);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void setData(const QModelIndex &index, QVariant data, int role = Qt::DisplayRole);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    std::shared_ptr<Settings> m_settings; //TODO remove this?
    QVector<QVariant> m_content;
};

#endif // CLIENTMANAGEMENTMODEL_H
