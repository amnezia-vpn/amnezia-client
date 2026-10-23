#ifndef LEGACYAPISERVERCONFIG_H
#define LEGACYAPISERVERCONFIG_H

#include <QJsonObject>
#include <QMap>

#include "core/utils/containerEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/models/containerConfig.h"

namespace amnezia
{

using namespace ContainerEnumNS;

struct LegacyApiServerConfig {
    QString description;
    QString displayName;
    QString hostName;
    QMap<DockerContainer, ContainerConfig> containers;
    DockerContainer defaultContainer = DockerContainer::None;
    QString dns1;
    QString dns2;

    QString name;
    int crc = 0;

    int configVersion = 0;
    QString apiEndpoint;

    bool hasContainers() const;
    ContainerConfig containerConfig(DockerContainer container) const;
    static LegacyApiServerConfig fromJson(const QJsonObject &json);
};

} // namespace amnezia

#endif // LEGACYAPISERVERCONFIG_H
