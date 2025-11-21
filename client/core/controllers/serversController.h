#ifndef SERVERSCONTROLLER_H
#define SERVERSCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>

#include "containers/containers_defs.h"
#include "core/defs.h"
#include "settings.h"

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
    explicit ServersController(std::shared_ptr<Settings> settings, QObject *parent = nullptr);
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

    // DNS operations
    QPair<QString, QString> getDnsPair(int serverIndex, bool isAmneziaDnsEnabled) const;
    bool isAmneziaDnsEnabled() const;
    void setAmneziaDnsEnabled(bool enabled);

    // Getters
    QJsonArray getServersArray() const;
    int getDefaultServerIndex() const;
    int getServersCount() const;
    QJsonObject getServerConfig(int serverIndex) const;
    ServerCredentials getServerCredentials(int serverIndex) const;
    QJsonObject getContainerConfig(int serverIndex, DockerContainer container) const;
    QString getNextAvailableServerName() const;
    
    GatewayStacks gatewayStacks() const;

    // Validation
    bool isServerFromApiAlreadyExists(const quint16 crc) const;
    bool isServerFromApiAlreadyExists(const QString &userCountryCode, const QString &serviceType, const QString &serviceProtocol) const;

signals:
    void serverAdded(QJsonObject config);
    void serverEdited(int index, QJsonObject config);
    void serverRemoved(int index);
    void defaultServerChanged(int index);
    void gatewayStacksExpanded();

private:
    void recomputeGatewayStacks();
    
    std::shared_ptr<Settings> m_settings;
    GatewayStacks m_gatewayStacks;
};

#endif // SERVERSCONTROLLER_H

