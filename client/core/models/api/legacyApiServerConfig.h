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

// Legacy unsupported API v1/v2-on-old-format: minimal model. Display: name, description, hostName, crc.
// dns1/dns2 are never loaded for this type (always empty) but align fields with other server config shapes.
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

    /// Not for UI — persisted so reload + `getConfigType` still treats this row as legacy.
    int configVersion = 0;
    /// Same: required with configVersion for Premium V1 vs Free V2 discrimination after save/load (see apiUtils.cpp).
    QString apiEndpoint;

    bool hasContainers() const;
    ContainerConfig containerConfig(DockerContainer container) const;
    QJsonObject toJson() const;
    static LegacyApiServerConfig fromJson(const QJsonObject &json);
};

} // namespace amnezia

#endif // LEGACYAPISERVERCONFIG_H
