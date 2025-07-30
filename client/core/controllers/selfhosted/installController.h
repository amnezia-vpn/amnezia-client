#ifndef INSTALL_CORE_CONTROLLER_H
#define INSTALL_CORE_CONTROLLER_H

#include <QObject>
#include <QSharedPointer>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>

#include "core/defs.h"
#include "core/models/containers/containers_defs.h"

class ServerController;
class Settings;

using namespace amnezia;

struct InstallResult
{
    ErrorCode errorCode;
    QString message;
    bool isServiceInstall;
    bool isInstalledContainerFound;
};

class InstallController : public QObject
{
    Q_OBJECT

public:
    explicit InstallController(std::shared_ptr<Settings> settings,
                               QObject *parent = nullptr);

    // Main installation operations
    InstallResult installContainer(DockerContainer container, int port, TransportProto transportProto,
                                  const ServerCredentials &serverCredentials, bool shouldCreateServer,
                                  const QString &privateKeyPassphrase = QString());
                                  
    InstallResult scanServerForInstalledContainers(const ServerCredentials &serverCredentials);
    
    InstallResult updateContainer(const DockerContainer container, const ServerCredentials &serverCredentials,
                                 const QJsonObject &containerConfig);

    // Server management operations  
    ErrorCode removeProcessedServer(const ServerCredentials &serverCredentials);
    ErrorCode rebootProcessedServer(const ServerCredentials &serverCredentials);
    ErrorCode removeAllContainers(const ServerCredentials &serverCredentials);
    ErrorCode removeProcessedContainer(const DockerContainer container, const ServerCredentials &serverCredentials);
    
    ErrorCode removeApiConfig(const QJsonObject &serverConfig);
    ErrorCode clearCachedProfile(const ServerCredentials &serverCredentials);

    // Utility methods  
    ErrorCode mountSftpDrive(const ServerCredentials &serverCredentials, const QString &port, 
                            const QString &password, const QString &username);
                            
    QString getNextAvailableServerName() const;
    ErrorCode addEmptyServer(const ServerCredentials &serverCredentials);
    bool isConfigValid(const ServerCredentials &serverCredentials) const;

signals:
    void clientAppendRequested(const DockerContainer container, const ServerCredentials &credentials,
                              const QJsonObject &containerConfig, const QString &clientName,
                              const QSharedPointer<ServerController> &serverController);
    
    void containerInstalled(const InstallResult &result);
    void serverScanned(const InstallResult &result);
    void containerUpdated(const InstallResult &result);
    void serverRebooted(const QString &message);
    void serverRemoved(const QString &message);
    void allContainersRemoved(const QString &message);
    void containerRemoved(const QString &message);
    void installationError(ErrorCode errorCode);
    void wrongInstallationUser(const QString &message);
    void serverIsBusy(bool isBusy);
    void cachedProfileCleared(const QString &message);
    void apiConfigRemoved(const QString &message);
    void noInstalledContainers();
    
private:
    // Installation helpers
    ErrorCode installServer(const DockerContainer container, 
                           const QMap<DockerContainer, QJsonObject> &installedContainers,
                           const ServerCredentials &serverCredentials,
                           const QSharedPointer<ServerController> &serverController,
                           QString &finishMessage, QJsonObject &serverConfig);
                           
    ErrorCode installContainer(const DockerContainer container,
                              const QMap<DockerContainer, QJsonObject> &installedContainers,
                              const ServerCredentials &serverCredentials,
                              const QSharedPointer<ServerController> &serverController,
                              QString &finishMessage);

    ErrorCode getAlreadyInstalledContainers(const ServerCredentials &credentials,
                                            const QSharedPointer<ServerController> &serverController,
                                            QMap<DockerContainer, QJsonObject> &installedContainers);

    QJsonObject generateContainerConfig(const DockerContainer container, int port, 
                                       const TransportProto transportProto);
    
    bool isServerAlreadyExists(const ServerCredentials &serverCredentials) const;

    std::shared_ptr<Settings> m_settings;
};

#endif // INSTALL_CORE_CONTROLLER_H 
