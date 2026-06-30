#ifndef NATIVESERVERCONFIG_H
#define NATIVESERVERCONFIG_H

#include <QJsonObject>
#include <QMap>
#include <QPair>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/containerConfig.h"

namespace amnezia
{

using namespace ContainerEnumNS;

struct NativeServerConfig {
    QString description;
    QString displayName;
    QString hostName;
    QMap<DockerContainer, ContainerConfig> containers;
    DockerContainer defaultContainer;
    QString dns1;
    QString dns2;
    
    bool hasContainers() const;
    ContainerConfig containerConfig(DockerContainer container) const;

    void updateContainerConfig(DockerContainer container, const ContainerConfig &config);

    QPair<QString, QString> getDnsPair(const QString &primaryDns, const QString &secondaryDns) const;

    QJsonObject toJson() const;
    static NativeServerConfig fromJson(const QJsonObject& json);
};

} // namespace amnezia

#endif // NATIVESERVERCONFIG_H

