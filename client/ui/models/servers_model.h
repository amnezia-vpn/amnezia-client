#ifndef SERVERSMODEL_H
#define SERVERSMODEL_H

#include <QAbstractListModel>

#include "core/controllers/selfhosted/serverController.h"
#include "core/controllers/selfhosted/selfhostedConfigController.h"
#include "core/controllers/settingsController.h"
#include "core/models/servers/serverConfig.h"
#include "settings.h"

class ServersModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        ServerDescriptionRole,
        CollapsedServerDescriptionRole,
        ExpandedServerDescriptionRole,
        HostNameRole,

        CredentialsRole,
        CredentialsLoginRole,

        IsDefaultRole,
        IsCurrentlyProcessedRole,

        HasWriteAccessRole,

        ContainsAmneziaDnsRole,

        DefaultContainerRole,

        HasInstalledContainers,

        IsServerFromTelegramApiRole,
        IsServerFromGatewayApiRole,
        ApiConfigRole,
        IsCountrySelectionAvailableRole,
        ApiAvailableCountriesRole,
        ApiServerCountryCodeRole
    };

    ServersModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool setData(const int index, const QVariant &value, int role = Qt::EditRole);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant data(const int index, int role = Qt::DisplayRole) const;

    void resetModel();

    Q_PROPERTY(int defaultIndex READ getDefaultServerIndex WRITE setDefaultServerIndex NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(QString defaultServerName READ getDefaultServerName NOTIFY defaultServerNameChanged)
    Q_PROPERTY(QString defaultServerDefaultContainerName READ getDefaultServerDefaultContainerName NOTIFY defaultServerDefaultContainerChanged)
    Q_PROPERTY(QString defaultServerDescriptionCollapsed READ getDefaultServerDescriptionCollapsed NOTIFY defaultServerDefaultContainerChanged)
    Q_PROPERTY(QString defaultServerImagePathCollapsed READ getDefaultServerImagePathCollapsed NOTIFY defaultServerDefaultContainerChanged)
    Q_PROPERTY(QString defaultServerDescriptionExpanded READ getDefaultServerDescriptionExpanded NOTIFY defaultServerDefaultContainerChanged)
    Q_PROPERTY(bool isDefaultServerDefaultContainerHasSplitTunneling READ isDefaultServerDefaultContainerHasSplitTunneling NOTIFY
                       defaultServerDefaultContainerChanged)
    Q_PROPERTY(bool isDefaultServerFromApi READ isDefaultServerFromApi NOTIFY defaultServerIndexChanged)

    Q_PROPERTY(int processedIndex READ getProcessedServerIndex WRITE setProcessedServerIndex NOTIFY processedServerIndexChanged)
    Q_PROPERTY(bool processedServerIsPremium READ processedServerIsPremium NOTIFY processedServerChanged)

    bool processedServerIsPremium() const;

public slots:
    void setDefaultServerIndex(const int index);
    const int getDefaultServerIndex();
    const QString getDefaultServerName();
    const QString getDefaultServerDescriptionCollapsed();
    const QString getDefaultServerImagePathCollapsed();
    const QString getDefaultServerDescriptionExpanded();
    const QString getDefaultServerDefaultContainerName();
    bool isDefaultServerCurrentlyProcessed();
    bool isDefaultServerFromApi();

    bool isProcessedServerHasWriteAccess();
    bool isDefaultServerHasWriteAccess();
    bool hasServerWithWriteAccess();

    const int getServersCount();

    void setProcessedServerIndex(const int index);
    int getProcessedServerIndex();

    const ServerCredentials getProcessedServerCredentials();
    const ServerCredentials getServerCredentials(const int index);

    void addServer(const QSharedPointer<ServerConfig> &serverConfig);
    void editServer(const QSharedPointer<ServerConfig> &serverConfig, const int serverIndex);
    void removeProcessedServer();
    void removeServer(const int serverIndex);

    QSharedPointer<const ServerConfig> getServerConfig(const int serverIndex);

    void clearCachedProfile(const DockerContainer container);

    ErrorCode removeContainer(const QSharedPointer<ServerController> &serverController, const int containerIndex);
    ErrorCode removeAllContainers(const QSharedPointer<ServerController> &serverController);
    ErrorCode rebootServer(const QSharedPointer<ServerController> &serverController);

    void setDefaultContainer(const int serverIndex, const int containerIndex);

    QStringList getAllInstalledServicesName(const int serverIndex);

    void toggleAmneziaDns(bool enabled);
    QPair<QString, QString> getDnsPair(const int serverIndex);

    bool isServerFromApiAlreadyExists(const quint16 crc);

    QVariant getDefaultServerData(const QString roleString);

    QVariant getProcessedServerData(const QString roleString);
    bool setProcessedServerData(const QString &roleString, const QVariant &value);

    bool isDefaultServerDefaultContainerHasSplitTunneling();

    bool isServerFromApi(const int serverIndex);

protected:
    QHash<int, QByteArray> roleNames() const override;

signals:
    void processedServerIndexChanged(const int index);
    // emitted when the processed server index or processed server data is changed
    void processedServerChanged();

    void defaultServerIndexChanged(const int index);
    void defaultServerNameChanged();
    void defaultServerDescriptionChanged();

    void containersUpdated(const QMap<QString, ContainerConfig> &containerConfigs);
    void defaultServerContainersUpdated(const QMap<QString, ContainerConfig> &containerConfigs);
    void defaultServerDefaultContainerChanged(const int containerIndex);

    void updateApiCountryModel();
    void updateApiServicesModel();

private:
    ServerCredentials serverCredentials(int index) const;

    void updateContainersModel();
    void updateDefaultServerContainersModel();

    QString getServerDescription(const int index) const;

    bool isAmneziaDnsContainerInstalled(const int serverIndex) const;

    bool serverHasInstalledContainers(const int serverIndex) const;

    QVector<QSharedPointer<ServerConfig>> m_servers1;

    std::shared_ptr<Settings> m_settings;
    std::shared_ptr<SelfhostedConfigController> m_serverConfigController;
    std::shared_ptr<SettingsController> m_settingsController;

    int m_defaultServerIndex;
    int m_processedServerIndex;

    bool m_isAmneziaDnsEnabled;
};

#endif // SERVERSMODEL_H
