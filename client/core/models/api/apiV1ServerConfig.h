#ifndef APIV1SERVERCONFIG_H
#define APIV1SERVERCONFIG_H

#include <QJsonObject>
#include <QMap>

#include "containers/containers_defs.h"
#include "core/models/containerConfig.h"
#include "core/utils/api/apiDefs.h"

namespace amnezia
{

using namespace ContainerEnumNS;

struct ApiV1ServerConfig {
    QString description;
    QString hostName;
    QMap<DockerContainer, ContainerConfig> containers;
    DockerContainer defaultContainer;
    QString dns1;
    QString dns2;
    
    QString name;
    QString protocol;
    QString apiEndpoint;
    QString apiKey;
    int crc;
    int configVersion;
    
    bool isPremium() const;
    bool isFree() const;
    QString vpnKey() const;
    bool hasContainers() const;
    ContainerConfig containerConfig(DockerContainer container) const;
    QJsonObject toJson() const;
    static ApiV1ServerConfig fromJson(const QJsonObject& json);
};

} // namespace amnezia

#endif // APIV1SERVERCONFIG_H

