#ifndef CONTAINERS_MODEL_H
#define CONTAINERS_MODEL_H

#include <QAbstractListModel>
#include <QJsonObject>
#include <vector>
#include <utility>

#include "settings.h"
#include "containers/containers_defs.h"

class ContainersModel : public QAbstractListModel
{
    Q_OBJECT
public:
    ContainersModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);
public:
    enum SiteRoles {
        NameRole = Qt::UserRole + 1,
        DescRole,
        DefaultRole,
        ServiceTypeRole,
        IsInstalledRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE void setSelectedServerIndex(int index);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    int m_selectedServerIndex;
    std::shared_ptr<Settings> m_settings;
};

#endif // CONTAINERS_MODEL_H
