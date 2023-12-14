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
        ServerDescriptionRole,

        HostNameRole,

        CredentialsRole,
        CredentialsLoginRole,

        IsDefaultRole,
        IsCurrentlyProcessedRole,

        HasWriteAccessRole,

        ContainsAmneziaDnsRole,

        DefaultContainerRole
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
    Q_PROPERTY(QString defaultContainerName READ getDefaultContainerName NOTIFY defaultContainerChanged)
    Q_PROPERTY(QString defaultServerDescriptionCollapsed READ getDefaultServerDescriptionCollapsed NOTIFY defaultServerDescriptionChanged)
    Q_PROPERTY(QString defaultServerDescriptionExpanded READ getDefaultServerDescriptionExpanded NOTIFY defaultServerDescriptionChanged)

    Q_PROPERTY(int currentlyProcessedIndex READ getCurrentlyProcessedServerIndex WRITE setCurrentlyProcessedServerIndex
                       NOTIFY currentlyProcessedServerIndexChanged)

public slots:
    void setDefaultServerIndex(const int index);
    const int getDefaultServerIndex();
    const QString getDefaultServerName();
    const QString getDefaultServerHostName();
    const QString getDefaultServerDescriptionCollapsed();
    const QString getDefaultServerDescriptionExpanded();
    bool isDefaultServerCurrentlyProcessed();

    bool isCurrentlyProcessedServerHasWriteAccess();
    bool isDefaultServerHasWriteAccess();
    bool hasServerWithWriteAccess();

    const int getServersCount();

    void setCurrentlyProcessedServerIndex(const int index);
    int getCurrentlyProcessedServerIndex();

    QString getCurrentlyProcessedServerHostName();
    const ServerCredentials getCurrentlyProcessedServerCredentials();
    const ServerCredentials getServerCredentials(const int index);

    void addServer(const QJsonObject &server);
    void editServer(const QJsonObject &server);
    void removeServer();

    bool isDefaultServerConfigContainsAmneziaDns();
    bool isAmneziaDnsContainerInstalled(const int serverIndex);

    QJsonObject getDefaultServerConfig();

    void reloadContainerConfig();
    void updateContainerConfig(const int containerIndex, const QJsonObject config);
    void addContainerConfig(const int containerIndex, const QJsonObject config);

    void clearCachedProfiles();

    ErrorCode removeContainer(const int containerIndex);
    ErrorCode removeAllContainers();

    void setDefaultContainer(const int containerIndex);
    DockerContainer getDefaultContainer();
    const QString getDefaultContainerName();

    QStringList getAllInstalledServicesName(const int serverIndex);

    void toggleAmneziaDns(bool enabled);

protected:
    QHash<int, QByteArray> roleNames() const override;

signals:
    void currentlyProcessedServerIndexChanged(const int index);
    void defaultServerIndexChanged(const int index);
    void defaultServerNameChanged();
    void defaultServerDescriptionChanged();

    void containersUpdated(QJsonArray &containers);
    void defaultContainerChanged(const int containerIndex);

private:
    ServerCredentials serverCredentials(int index) const;
    void updateContainersModel();

    QString getDefaultServerDescription(const QJsonObject &server);

    QJsonArray m_servers;

    std::shared_ptr<Settings> m_settings;

    int m_defaultServerIndex;
    int m_currentlyProcessedServerIndex;

    bool m_isAmneziaDnsEnabled = m_settings->useAmneziaDns();
};

#endif // SERVERSMODEL_H
