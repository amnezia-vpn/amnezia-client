#ifndef ALL_CONTAINERS_MODEL_H
#define ALL_CONTAINERS_MODEL_H

#include <QAbstractListModel>
#include <QJsonObject>
#include <vector>
#include <utility>

#include "containers/containers_defs.h"

class AllContainersModel : public QAbstractListModel
{
    Q_OBJECT
public:
    AllContainersModel(QObject *parent = nullptr);
public:
    enum SiteRoles {
        NameRole = Qt::UserRole + 1,
        DescRole,
        TypeRole,
        InstalledRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void setServerData(const QJsonObject &server);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QJsonObject m_serverData;
};

#endif // ALL_CONTAINERS_MODEL_H
