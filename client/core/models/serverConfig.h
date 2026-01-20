#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include <variant>
#include <QJsonObject>
#include <QMap>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/selfhosted/selfHostedServerConfig.h"
#include "core/models/selfhosted/nativeServerConfig.h"
#include "core/models/api/apiV1ServerConfig.h"
#include "core/models/api/apiV2ServerConfig.h"
#include "core/models/containerConfig.h"

namespace amnezia
{

using namespace ContainerEnumNS;

using ServerConfig = std::variant<
    SelfHostedServerConfig,
    NativeServerConfig,
    ApiV1ServerConfig,
    ApiV2ServerConfig
>;

namespace ServerConfigUtils {
    bool isSelfHosted(const ServerConfig& config);
    bool isNative(const ServerConfig& config);
    bool isApiV1Config(const ServerConfig& config);
    bool isApiV2Config(const ServerConfig& config);
    bool isApiConfig(const ServerConfig& config);
    
    SelfHostedServerConfig& asSelfHosted(ServerConfig& config);
    const SelfHostedServerConfig& asSelfHosted(const ServerConfig& config);
    
    NativeServerConfig& asNative(ServerConfig& config);
    const NativeServerConfig& asNative(const ServerConfig& config);
    
    ApiV1ServerConfig& asApiV1(ServerConfig& config);
    const ApiV1ServerConfig& asApiV1(const ServerConfig& config);
    
    ApiV2ServerConfig& asApiV2(ServerConfig& config);
    const ApiV2ServerConfig& asApiV2(const ServerConfig& config);
    
    QString description(const ServerConfig& config);
    QString hostName(const ServerConfig& config);
    QMap<DockerContainer, ContainerConfig> containers(const ServerConfig& config);
    DockerContainer defaultContainer(const ServerConfig& config);
    QString dns1(const ServerConfig& config);
    QString dns2(const ServerConfig& config);
    bool hasContainers(const ServerConfig& config);
    ContainerConfig containerConfig(const ServerConfig& config, DockerContainer container);
    int crc(const ServerConfig& config);
    int configVersion(const ServerConfig& config);
    
    QJsonObject toJson(const ServerConfig& config);
    ServerConfig fromJson(const QJsonObject& json);
    
    template<typename Visitor>
    auto visit(ServerConfig& config, Visitor&& visitor)
    {
        return std::visit(std::forward<Visitor>(visitor), config);
    }
    
    template<typename Visitor>
    auto visit(const ServerConfig& config, Visitor&& visitor)
    {
        return std::visit(std::forward<Visitor>(visitor), config);
    }
    
    // Utility function to get DNS pair for a server
    QPair<QString, QString> getDnsPair(const ServerConfig &serverConfig, 
                                       bool isAmneziaDnsEnabled,
                                       const QString &primaryDns,
                                       const QString &secondaryDns);
}

} // namespace amnezia

#endif // SERVERCONFIG_H

