#ifndef NATIVESERVERCONFIG_H
#define NATIVESERVERCONFIG_H

#include <QJsonObject>
#include <QMap>

#include "containers/containers_defs.h"
#include "core/models/containerConfig.h"

namespace amnezia
{

using namespace ContainerEnumNS;

struct NativeServerConfig {
    QString description;
    QString hostName;
    QMap<DockerContainer, ContainerConfig> containers;
    DockerContainer defaultContainer;
    QString dns1;
    QString dns2;
    
    bool hasContainers() const;
    ContainerConfig containerConfig(DockerContainer container) const;
    QJsonObject toJson() const;
    static NativeServerConfig fromJson(const QJsonObject& json);
};

} // namespace amnezia

#endif // NATIVESERVERCONFIG_H

