#ifndef SERVERSUICONTROLLER_H
#define SERVERSUICONTROLLER_H

#include <QObject>

#include <QSet>
#include <QJsonObject>
#include <QStringList>
#include <QVector>

#include "core/controllers/serversController.h"
#include "core/models/serverDescription.h"
#include "core/controllers/settingsController.h"
#include "ui/models/serversModel.h"
#include "ui/models/containersModel.h"

class ServersUiController : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(QString defaultServerId READ getDefaultServerId NOTIFY defaultServerIdChanged)
    Q_PROPERTY(int defaultServerIndex READ defaultServerIndex NOTIFY defaultServerIndexChanged)

    Q_PROPERTY(QString defaultServerName READ getDefaultServerName NOTIFY defaultServerIdChanged)
    Q_PROPERTY(QString defaultServerDefaultContainerName READ getDefaultServerDefaultContainerName NOTIFY defaultServerIdChanged)
    Q_PROPERTY(QString defaultServerDescriptionCollapsed READ getDefaultServerDescriptionCollapsed NOTIFY defaultServerIdChanged)
    Q_PROPERTY(QString defaultServerImagePathCollapsed READ getDefaultServerImagePathCollapsed NOTIFY defaultServerIdChanged)
    Q_PROPERTY(QString defaultServerDescriptionExpanded READ getDefaultServerDescriptionExpanded NOTIFY defaultServerIdChanged)
    Q_PROPERTY(bool isDefaultServerDefaultContainerHasSplitTunneling READ isDefaultServerDefaultContainerHasSplitTunneling NOTIFY defaultServerIdChanged)
    Q_PROPERTY(bool isDefaultServerFromApi READ isDefaultServerFromApi NOTIFY defaultServerIdChanged)
    
    Q_PROPERTY(QString processedServerId READ getProcessedServerId WRITE setProcessedServerId NOTIFY processedServerIdChanged)
    Q_PROPERTY(int processedServerIndex READ getProcessedServerIndex WRITE setProcessedServerIndex NOTIFY processedServerIndexChanged)
    Q_PROPERTY(int processedContainerIndex READ getProcessedContainerIndex WRITE setProcessedContainerIndex NOTIFY processedContainerIndexChanged)
    Q_PROPERTY(bool processedServerIsPremium READ processedServerIsPremium NOTIFY processedServerIndexChanged)
    
    Q_PROPERTY(bool hasServersFromGatewayApi READ hasServersFromGatewayApi NOTIFY hasServersFromGatewayApiChanged)
    
    Q_PROPERTY(bool isAdVisible READ isAdVisible NOTIFY defaultServerIdChanged)
    Q_PROPERTY(QString adHeader READ adHeader NOTIFY defaultServerIdChanged)
    Q_PROPERTY(QString adDescription READ adDescription NOTIFY defaultServerIdChanged)
    
public:
    explicit ServersUiController(ServersController* serversController,
                                 SettingsController* settingsController,
                                 ServersModel* serversModel,
                                 ContainersModel* containersModel,
                                 ContainersModel* defaultServerContainersModel,
                                 QObject *parent = nullptr);

public slots:
    void removeServer(const QString &serverId);
    void removeServerAtIndex(int index);

    void editServerName(const QString &serverId, const QString &name);

    void setDefaultServer(const QString &serverId);
    void setDefaultServerAtIndex(int index);

    void setDefaultContainer(const QString &serverId, int containerIndex);
    void setDefaultContainerAtIndex(int index, int containerIndex);

    void toggleAmneziaDns(bool enabled);
    void onDefaultServerChanged(const QString &defaultServerId);
    
    // Getters for properties
    QString getDefaultServerId() const;
    QString getDefaultServerName() const;
    QString getDefaultServerDefaultContainerName() const;
    QString getDefaultServerDescriptionCollapsed() const;
    QString getDefaultServerImagePathCollapsed() const;
    QString getDefaultServerDescriptionExpanded() const;
    bool isDefaultServerDefaultContainerHasSplitTunneling() const;
    bool isDefaultServerFromApi() const;
    
    QString getProcessedServerId() const;
    void setProcessedServerId(const QString &serverId);

    int getProcessedServerIndex() const;
    void setProcessedServerIndex(int index);

    int defaultServerIndex() const;

    int getProcessedContainerIndex() const;
    void setProcessedContainerIndex(int index);
    bool processedServerIsPremium() const;
    
    const ServerCredentials getProcessedServerCredentials() const;
    bool isDefaultServerCurrentlyProcessed() const;
    bool isProcessedServerHasWriteAccess() const;
    
    bool hasServersFromGatewayApi() const;
    
    bool isAdVisible() const;
    QString adHeader() const;
    QString adDescription() const;
    
    QString getServerId(int index) const;
    int getServerIndexById(const QString &serverId) const;
    QStringList getAllInstalledServicesName(int serverIndex) const;

signals:
    void errorOccurred(const QString &errorMessage);
    void finished(const QString &message);
    void defaultServerIdChanged(const QString &serverId);
    void defaultServerIndexChanged(int index);
    void processedServerIdChanged(const QString &serverId);
    void processedServerIndexChanged(int index);
    void processedContainerIndexChanged(int index);
    void hasServersFromGatewayApiChanged();
    void updateApiCountryModel();
    void updateApiServicesModel();

public:
    void updateModel();
    
private:
    QString getDefaultServerDescription(const QString &serverId) const;
    int serverIndexForId(const QString &serverId) const;
    bool listHasServersFromGatewayApi() const;

    void updateContainersModel();
    void updateDefaultServerContainersModel();

    ServersController* m_serversController;
    SettingsController* m_settingsController;
    ServersModel* m_serversModel;
    ContainersModel* m_containersModel;
    ContainersModel* m_defaultServerContainersModel;

    QVector<amnezia::ServerDescription> m_orderedServerDescriptions;
    
    int m_processedServerIndex = -1;
    QString m_processedServerId;
    int m_processedContainerIndex = -1;
};

#endif // SERVERSUICONTROLLER_H

