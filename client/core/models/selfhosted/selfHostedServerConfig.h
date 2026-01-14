#ifndef SELFHOSTEDSERVERCONFIG_H
#define SELFHOSTEDSERVERCONFIG_H

#include <QJsonObject>
#include <QMap>
#include <optional>

#include "containers/containers_defs.h"
#include "core/models/containerConfig.h"
#include "core/utils/defs.h"

namespace amnezia
{

using namespace ContainerEnumNS;

struct SelfHostedServerConfig {
    QString description;
    QString hostName;
    QMap<DockerContainer, ContainerConfig> containers;
    DockerContainer defaultContainer;
    QString dns1;
    QString dns2;
    
    std::optional<QString> userName;
    std::optional<QString> password;
    std::optional<int> port;
    
    bool hasCredentials() const;
    bool isReadOnly() const;
    ServerCredentials credentials() const;
    bool hasContainers() const;
    ContainerConfig containerConfig(DockerContainer container) const;
    QJsonObject toJson() const;
    static SelfHostedServerConfig fromJson(const QJsonObject& json);
};

} // namespace amnezia

#endif // SELFHOSTEDSERVERCONFIG_H

