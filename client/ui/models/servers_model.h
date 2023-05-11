#ifndef SERVERSMODEL_H
#define SERVERSMODEL_H

#include <QAbstractListModel>

#include "settings.h"

struct ServerModelContent {
    QString desc;
    QString address;
    bool isDefault;
};

class ServersModel : public QAbstractListModel
{
    Q_OBJECT
public:
    ServersModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);
public:
    enum SiteRoles {
        DescRole = Qt::UserRole + 1,
        AddressRole,
        IsDefaultRole
    };

    void refresh();
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<ServerModelContent> m_data;
    std::shared_ptr<Settings> m_settings;
};

#endif // SERVERSMODEL_H
