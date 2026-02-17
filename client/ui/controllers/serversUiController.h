#ifndef SERVERSUICONTROLLER_H
#define SERVERSUICONTROLLER_H

#include <QObject>

#include <QSet>
#include <QJsonObject>
#include <QStringList>

#include "core/controllers/serversController.h"
#include "core/controllers/settingsController.h"
#include "ui/models/serversModel.h"
#include "ui/models/containersModel.h"
#include "core/models/serverConfig.h"

class ServersUiController : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(int defaultIndex READ getDefaultServerIndex NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(QString defaultServerName READ getDefaultServerName NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(QString defaultServerDefaultContainerName READ getDefaultServerDefaultContainerName NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(QString defaultServerDescriptionCollapsed READ getDefaultServerDescriptionCollapsed NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(QString defaultServerImagePathCollapsed READ getDefaultServerImagePathCollapsed NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(QString defaultServerDescriptionExpanded READ getDefaultServerDescriptionExpanded NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(bool isDefaultServerDefaultContainerHasSplitTunneling READ isDefaultServerDefaultContainerHasSplitTunneling NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(bool isDefaultServerFromApi READ isDefaultServerFromApi NOTIFY defaultServerIndexChanged)
    
    Q_PROPERTY(int processedIndex READ getProcessedServerIndex WRITE setProcessedServerIndex NOTIFY processedServerIndexChanged)
    Q_PROPERTY(int processedContainerIndex READ getProcessedContainerIndex WRITE setProcessedContainerIndex NOTIFY processedContainerIndexChanged)
    Q_PROPERTY(bool processedServerIsPremium READ processedServerIsPremium NOTIFY processedServerIndexChanged)
    
    Q_PROPERTY(bool hasServersFromGatewayApi READ hasServersFromGatewayApi NOTIFY hasServersFromGatewayApiChanged)
    
    Q_PROPERTY(bool isAdVisible READ isAdVisible NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(QString adHeader READ adHeader NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(QString adDescription READ adDescription NOTIFY defaultServerIndexChanged)
    
public:
    explicit ServersUiController(ServersController* serversController,
                                 SettingsController* settingsController,
                                 ServersModel* serversModel,
                                 ContainersModel* containersModel,
                                 ContainersModel* defaultServerContainersModel,
                                 QObject *parent = nullptr);

public slots:
    void removeServer(int index);
    void editServerName(int index, const QString &name);
    void setDefaultServerIndex(int index);
    void setDefaultContainer(int serverIndex, int containerIndex);
    void toggleAmneziaDns(bool enabled);
    void onDefaultServerChanged(int index);
    
    // Getters for properties
    int getDefaultServerIndex() const;
    QString getDefaultServerName() const;
    QString getDefaultServerDefaultContainerName() const;
    QString getDefaultServerDescriptionCollapsed() const;
    QString getDefaultServerImagePathCollapsed() const;
    QString getDefaultServerDescriptionExpanded() const;
    bool isDefaultServerDefaultContainerHasSplitTunneling() const;
    bool isDefaultServerFromApi() const;
    
    int getProcessedServerIndex() const;
    void setProcessedServerIndex(int index);
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
    
    QStringList getAllInstalledServicesName(int serverIndex) const;

signals:
    void errorOccurred(const QString &errorMessage);
    void finished(const QString &message);
    void defaultServerIndexChanged(int index);
    void processedServerIndexChanged(int index);
    void processedContainerIndexChanged(int index);
    void hasServersFromGatewayApiChanged();
    void updateApiCountryModel();
    void updateApiServicesModel();

public:
    void updateModel();
    
private:
    QString getDefaultServerDescription(const ServerConfig& server, int index) const;
    bool isAmneziaDnsContainerInstalled(int serverIndex) const;

    void updateContainersModel();
    void updateDefaultServerContainersModel();
    void updateApiModelsForProcessedServer();
    
    ServersController* m_serversController;
    SettingsController* m_settingsController;
    ServersModel* m_serversModel;
    ContainersModel* m_containersModel;
    ContainersModel* m_defaultServerContainersModel;
    
    int m_processedServerIndex = -1;
    int m_processedContainerIndex = -1;
};

#endif // SERVERSUICONTROLLER_H

