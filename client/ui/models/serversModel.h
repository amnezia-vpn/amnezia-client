#ifndef SERVERSMODEL_H
#define SERVERSMODEL_H

#include <QAbstractListModel>
#include <QVector>

#include "core/utils/selfhosted/sshSession.h"
#include "core/models/serverConfig.h"

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
        ApiServerCountryCodeRole,
        IsAdVisibleRole,
        AdHeaderRole,
        AdDescriptionRole,
        AdEndpointRole,
        IsRenewalAvailableRole,
        IsSubscriptionExpiredRole,
        IsSubscriptionExpiringSoonRole,

        HasAmneziaDns
    };

    ServersModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant data(const int index, int role = Qt::DisplayRole) const;

public slots:
    const int getDefaultServerIndex();
    bool isDefaultServerCurrentlyProcessed();
    bool isDefaultServerFromApi();

    bool isProcessedServerHasWriteAccess();
    bool isDefaultServerHasWriteAccess();
    bool hasServerWithWriteAccess();

    const int getServersCount();

    void setProcessedServerIndex(const int index);

    const ServerCredentials getProcessedServerCredentials();
    QVariant getProcessedServerData(const QString &roleString);

    QVariant getDefaultServerData(const QString roleString);

    bool isServerFromApi(const int serverIndex);

    void updateModel(const QVector<ServerConfig> &servers, int defaultServerIndex, bool isAmneziaDnsEnabled = false);
    
protected:
    QHash<int, QByteArray> roleNames() const override;

signals:
    void processedServerIndexChanged(const int index);
    // emitted when the processed server index or processed server data is changed
    void processedServerChanged();

    void defaultServerIndexChanged(const int index);
    void defaultServerNameChanged();
    void defaultServerDescriptionChanged();

    void defaultServerDefaultContainerChanged(const int containerIndex);

    void updateApiCountryModel();
    void updateApiServicesModel();

private:
    ServerCredentials serverCredentials(int index) const;

    QString getServerDescription(const ServerConfig &server, const int index) const;

    bool serverHasInstalledContainers(const int serverIndex) const;

    QVector<ServerConfig> m_servers;

    int m_defaultServerIndex;
    int m_processedServerIndex;

    bool m_isAmneziaDnsEnabled = false;
};

#endif // SERVERSMODEL_H
