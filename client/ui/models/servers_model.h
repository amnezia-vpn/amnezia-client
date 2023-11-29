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

    void resetModel();

    Q_PROPERTY(int defaultIndex READ getDefaultServerIndex WRITE setDefaultServerIndex NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(QString defaultServerName READ getDefaultServerName NOTIFY defaultServerNameChanged)
    Q_PROPERTY(QString defaultServerHostName READ getDefaultServerHostName NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(int currentlyProcessedIndex READ getCurrentlyProcessedServerIndex WRITE setCurrentlyProcessedServerIndex
                       NOTIFY currentlyProcessedServerIndexChanged)

public slots:
    void setDefaultServerIndex(const int index);
    const int getDefaultServerIndex();
    const QString getDefaultServerName();
    const QString getDefaultServerHostName();
    bool isDefaultServerCurrentlyProcessed();

    bool isCurrentlyProcessedServerHasWriteAccess();
    bool isDefaultServerHasWriteAccess();
    bool hasServerWithWriteAccess();

    const int getServersCount();

    void setCurrentlyProcessedServerIndex(const int index);
    int getCurrentlyProcessedServerIndex();

    QString getCurrentlyProcessedServerHostName();
    const ServerCredentials getCurrentlyProcessedServerCredentials();

    void addServer(const QJsonObject &server);
    void removeServer();

    bool isDefaultServerConfigContainsAmneziaDns();

    void updateContainersConfig();

protected:
    QHash<int, QByteArray> roleNames() const override;

signals:
    void currentlyProcessedServerIndexChanged(const int index);
    void defaultServerIndexChanged(const int index);
    void defaultServerNameChanged();

private:
    ServerCredentials serverCredentials(int index) const;

    QJsonArray m_servers;

    std::shared_ptr<Settings> m_settings;

    int m_defaultServerIndex;
    int m_currentlyProcessedServerIndex;
};

#endif // SERVERSMODEL_H
