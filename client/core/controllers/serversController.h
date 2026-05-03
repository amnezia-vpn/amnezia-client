#ifndef SERVERSCONTROLLER_H
#define SERVERSCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QVector>

#include <QPair>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/secureServersRepository.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"

class SshSession;
class InstallController;

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
    struct GatewayStacksData
    {
        QSet<QString> userCountryCodes;
        QSet<QString> serviceTypes;

        bool isEmpty() const { return userCountryCodes.isEmpty() && serviceTypes.isEmpty(); }
        bool operator==(const GatewayStacksData &other) const;
        QJsonObject toJson() const;
    };
    
public:
    explicit ServersController(SecureServersRepository* serversRepository, 
                              SecureAppSettingsRepository* appSettingsRepository = nullptr,
                              QObject *parent = nullptr);
    ~ServersController() = default;

    // Server management
    void addServer(const ServerConfig &server);
    void editServer(int index, const ServerConfig &server);
    void removeServer(int index);
    void setDefaultServerIndex(int index);

    // Container management
    void setDefaultContainer(int serverIndex, DockerContainer container);
    void updateContainerConfig(int serverIndex, DockerContainer container, const ContainerConfig &config);

    // Cache management
    void clearCachedProfile(int serverIndex, DockerContainer container);

    // Getters
    QJsonArray getServersArray() const;
    QVector<ServerConfig> getServers() const;
    int getDefaultServerIndex() const;
    int getServersCount() const;
    ServerConfig getServerConfig(int serverIndex) const;
    ServerCredentials getServerCredentials(int serverIndex) const;
    ContainerConfig getContainerConfig(int serverIndex, DockerContainer container) const;
    QPair<QString, QString> getDnsPair(int serverIndex, bool isAmneziaDnsEnabled) const;
    
    GatewayStacksData gatewayStacks() const;

    // Validation
    bool isServerFromApiAlreadyExists(const QString &userCountryCode, const QString &serviceType, const QString &serviceProtocol) const;
    bool hasInstalledContainers(int serverIndex) const;

signals:
    void gatewayStacksExpanded();

public slots:
    void recomputeGatewayStacks();

private:
    SecureServersRepository* m_serversRepository;
    SecureAppSettingsRepository* m_appSettingsRepository;
    GatewayStacksData m_gatewayStacks;
};

#endif // SERVERSCONTROLLER_H

