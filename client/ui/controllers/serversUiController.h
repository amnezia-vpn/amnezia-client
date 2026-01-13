#ifndef SERVERSUICONTROLLER_H
#define SERVERSUICONTROLLER_H

#include <QObject>

#include <QSet>
#include <QJsonObject>
#include <QStringList>

#include "core/controllers/serversController.h"
#include "core/repositories/qServersRepository.h"
#include "core/repositories/qAppSettingsRepository.h"
#include "ui/models/servers_model.h"
#include "ui/models/containers_model.h"

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
    Q_PROPERTY(bool processedServerIsPremium READ processedServerIsPremium NOTIFY processedServerIndexChanged)
    
    Q_PROPERTY(bool hasServersFromGatewayApi READ hasServersFromGatewayApi NOTIFY hasServersFromGatewayApiChanged)
    
    Q_PROPERTY(bool isAdVisible READ isAdVisible NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(QString adHeader READ adHeader NOTIFY defaultServerIndexChanged)
    Q_PROPERTY(QString adDescription READ adDescription NOTIFY defaultServerIndexChanged)
    
public:
    explicit ServersUiController(ServersController* serversController,
                                 QServersRepository* serversRepository,
                                 QAppSettingsRepository* appSettingsRepository,
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
    void onAddServer(QJsonObject config);
    void onServerEdited(int index, QJsonObject config);
    void onServerRemoved(int index);
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
    bool processedServerIsPremium() const;
    
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
    void hasServersFromGatewayApiChanged();

public:
    void updateModel();
    
private:
    QString getDefaultServerDescription(const QJsonObject &server, int index) const;
    bool isAmneziaDnsContainerInstalled(int serverIndex) const;

    void updateContainersModel();
    void updateDefaultServerContainersModel();
    
    ServersController* m_serversController;
    QServersRepository* m_serversRepository;
    QAppSettingsRepository* m_appSettingsRepository;
    ServersModel* m_serversModel;
    ContainersModel* m_containersModel;
    ContainersModel* m_defaultServerContainersModel;
    
    int m_processedServerIndex = 0;
};

#endif // SERVERSUICONTROLLER_H

