#ifndef SERVERSCONTROLLER_H
#define SERVERSCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QVector>

#include <QPair>

#include "containers/containers_defs.h"
#include "core/utils/defs.h"
#include "core/repositories/qServersRepository.h"
#include "core/repositories/qAppSettingsRepository.h"
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
    struct GatewayStacks
    {
        QSet<QString> userCountryCodes;
        QSet<QString> serviceTypes;

        bool isEmpty() const { return userCountryCodes.isEmpty() && serviceTypes.isEmpty(); }
        bool operator==(const GatewayStacks &other) const;
        QJsonObject toJson() const;
    };
    
public:
    explicit ServersController(QServersRepository* serversRepository, 
                              AppSettingsRepository* appSettingsRepository = nullptr,
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
    void addContainerConfig(int serverIndex, DockerContainer container, const ContainerConfig &config);

    // Cache management
    void clearCachedProfile(int serverIndex, DockerContainer container);
    void reloadContainerConfig(int serverIndex, DockerContainer container);

    // Getters
    QJsonArray getServersArray() const;
    int getDefaultServerIndex() const;
    int getServersCount() const;
    ServerConfig getServerConfig(int serverIndex) const;
    ServerCredentials getServerCredentials(int serverIndex) const;
    QJsonObject getContainerConfig(int serverIndex, DockerContainer container) const;
    QPair<QString, QString> getDnsPair(int serverIndex, bool isAmneziaDnsEnabled) const;
    
    GatewayStacks gatewayStacks() const;

    // Validation
    bool isServerFromApiAlreadyExists(const quint16 crc) const;
    bool isServerFromApiAlreadyExists(const QString &userCountryCode, const QString &serviceType, const QString &serviceProtocol) const;
    bool hasInstalledContainers(int serverIndex) const;

signals:
    void gatewayStacksExpanded();

public slots:
    void recomputeGatewayStacks();

private:
    QServersRepository* m_serversRepository;
    AppSettingsRepository* m_appSettingsRepository;
    GatewayStacks m_gatewayStacks;
};

#endif // SERVERSCONTROLLER_H

