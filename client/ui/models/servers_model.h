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
    enum ServersModelRoles {
        NameRole = Qt::UserRole + 1,
        HostNameRole,
        CredentialsRole,
        IsDefaultRole
    };

    ServersModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void setDefaultServerIndex(int index);
    const int getDefaultServerIndex();
    const int getServersCount();

    void setCurrentlyProcessedServerIndex(int index);
    ServerCredentials getCurrentlyProcessedServerCredentials();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    std::shared_ptr<Settings> m_settings;

    int m_currenlyProcessedServerIndex;
};

#endif // SERVERSMODEL_H
