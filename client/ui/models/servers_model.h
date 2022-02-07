#ifndef SERVERSMODEL_H
#define SERVERSMODEL_H

#include <QAbstractListModel>
#include <vector>
#include <utility>

struct ServerModelContent {
    QString desc;
    QString address;
    bool isDefault;
};

class ServersModel : public QAbstractListModel
{
    Q_OBJECT
public:
    ServersModel(QObject *parent = nullptr);
public:
    enum SiteRoles {
        DescRole = Qt::UserRole + 1,
        AddressRole,
        IsDefaultRole
    };

    void clearData();
    void setContent(const std::vector<ServerModelContent>& data);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    std::vector<ServerModelContent> content;
};

#endif // SERVERSMODEL_H
