#ifndef SERVERSCONTROLLER_H
#define SERVERSCONTROLLER_H

#include <optional>

#include <QObject>
#include <QVector>
#include <QMap>

#include <QPair>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/secureServersRepository.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/models/containerConfig.h"
#include "core/models/serverDescription.h"

class SshSession;
class InstallController;

using namespace amnezia;

class ServersController : public QObject
{
    Q_OBJECT

public:
    explicit ServersController(SecureServersRepository* serversRepository, 
                              SecureAppSettingsRepository* appSettingsRepository = nullptr,
                              QObject *parent = nullptr);
    ~ServersController() = default;

    // Server management
    bool renameServer(const QString &serverId, const QString &name);
    void removeServer(const QString &serverId);
    void setDefaultServer(const QString &serverId);

    // Container management
    void setDefaultContainer(const QString &serverId, DockerContainer container);

    // Getters
    QVector<ServerDescription> buildServerDescriptions(bool isAmneziaDnsEnabled) const;
    int getDefaultServerIndex() const;
    QString getDefaultServerId() const;
    int getServersCount() const;
    QString getServerId(int serverIndex) const;
    int indexOfServerId(const QString &serverId) const;
    QString notificationDisplayName(const QString &serverId) const;
    std::optional<ApiV2ServerConfig> apiV2Config(const QString &serverId) const;
    std::optional<SelfHostedAdminServerConfig> selfHostedAdminConfig(const QString &serverId) const;
    ServerCredentials getServerCredentials(const QString &serverId) const;
    QMap<DockerContainer, ContainerConfig> getServerContainersMap(const QString &serverId) const;
    DockerContainer getDefaultContainer(const QString &serverId) const;
    ContainerConfig getContainerConfig(const QString &serverId, DockerContainer container) const;

    // Validation
    bool isServerFromApiAlreadyExists(const QString &userCountryCode, const QString &serviceType, const QString &serviceProtocol) const;
    bool hasInstalledContainers(const QString &serverId) const;
    bool isLegacyApiV1Server(const QString &serverId) const;

private:
    void ensureDefaultServerValid();

    SecureServersRepository* m_serversRepository;
    SecureAppSettingsRepository* m_appSettingsRepository;
};

#endif // SERVERSCONTROLLER_H

