#ifndef SERVERSCONTROLLER_H
#define SERVERSCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>

#include <QPair>

#include "containers/containers_defs.h"
#include "core/defs.h"
#include "core/repositories/serversRepository.h"
#include "core/repositories/appSettingsRepository.h"

class ServerController;

using namespace amnezia;

/**
 * @brief Core business logic controller for server operations
 * 
 * This controller contains pure business logic for managing servers.
 */
class ServersController : public QObject
{
    Q_OBJECT
    
public:
    struct GatewayStacks
    {
        QSet<QString> userCountryCodes;
        QSet<QString> serviceTypes;

        bool isEmpty() const { return userCountryCodes.isEmpty() && serviceTypes.isEmpty(); }
        bool operator==(const GatewayStacks &other) const;
        QJsonObject toJson() const;
    };
    
public:
    explicit ServersController(ServersRepository* serversRepository, 
                              AppSettingsRepository* appSettingsRepository = nullptr,
                              QObject *parent = nullptr);
    ~ServersController() = default;

    // Server management
    void addServer(const QJsonObject &server);
    void editServer(int index, const QJsonObject &server);
    void removeServer(int index);
    void setDefaultServerIndex(int index);

    // Container management
    void setDefaultContainer(int serverIndex, DockerContainer container);
    void updateContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config);
    void addContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config);

    // Container operations (requires ServerController)
    ErrorCode removeContainer(ServerController *serverController, int serverIndex, DockerContainer container);
    ErrorCode removeAllContainers(ServerController *serverController, int serverIndex);
    ErrorCode rebootServer(ServerController *serverController, int serverIndex);

    // Cache management
    void clearCachedProfile(int serverIndex, DockerContainer container);
    void reloadContainerConfig(int serverIndex, DockerContainer container);

    // API operations
    void removeApiConfig(int serverIndex);
    bool isApiKeyExpired(int serverIndex) const;

    // Getters
    QJsonArray getServersArray() const;
    int getDefaultServerIndex() const;
    int getServersCount() const;
    QJsonObject getServerConfig(int serverIndex) const;
    ServerCredentials getServerCredentials(int serverIndex) const;
    QJsonObject getContainerConfig(int serverIndex, DockerContainer container) const;
    QPair<QString, QString> getDnsPair(int serverIndex, bool isAmneziaDnsEnabled) const;
    
    GatewayStacks gatewayStacks() const;

    // Validation
    bool isServerFromApiAlreadyExists(const quint16 crc) const;
    bool isServerFromApiAlreadyExists(const QString &userCountryCode, const QString &serviceType, const QString &serviceProtocol) const;

signals:
    void gatewayStacksExpanded();

private:
    void recomputeGatewayStacks();
    
    ServersRepository* m_serversRepository;
    AppSettingsRepository* m_appSettingsRepository;
    GatewayStacks m_gatewayStacks;
};

#endif // SERVERSCONTROLLER_H

