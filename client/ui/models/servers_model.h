#ifndef SERVERSMODEL_H
#define SERVERSMODEL_H

#include <QAbstractListModel>

#include "settings.h"

class ServersModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        HostNameRole,
        CredentialsRole,
        CredentialsLoginRole,
        IsDefaultRole,
        IsCurrentlyProcessedRole,
        HasWriteAccessRole,
        ContainsAmneziaDnsRole
    };

    ServersModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant data(const int index, int role = Qt::DisplayRole) const;

    Q_PROPERTY(int defaultIndex READ getDefaultServerIndex WRITE setDefaultServerIndex NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(int currentlyProcessedIndex READ getCurrentlyProcessedServerIndex WRITE setCurrentlyProcessedServerIndex
                       NOTIFY currentlyProcessedServerIndexChanged)

public slots:
    void setDefaultServerIndex(const int index);
    const int getDefaultServerIndex();
    bool isDefaultServerCurrentlyProcessed();

    bool isCurrentlyProcessedServerHasWriteAccess();
    bool isDefaultServerHasWriteAccess();

    const int getServersCount();

    void setCurrentlyProcessedServerIndex(const int index);
    int getCurrentlyProcessedServerIndex();

    void addServer(const QJsonObject &server);
    void removeServer();

    bool isDefaultServerConfigContainsAmneziaDns();

protected:
    QHash<int, QByteArray> roleNames() const override;

signals:
    void currentlyProcessedServerIndexChanged(const int index);
    void defaultServerIndexChanged();

private:
    QJsonArray m_servers;

    std::shared_ptr<Settings> m_settings;

    int m_defaultServerIndex;
    int m_currenlyProcessedServerIndex;
};

#endif // SERVERSMODEL_H
