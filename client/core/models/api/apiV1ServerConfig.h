#ifndef APIV1SERVERCONFIG_H
#define APIV1SERVERCONFIG_H

#include <QJsonObject>
#include <QMap>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/containerConfig.h"
#include "core/utils/api/apiEnums.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"

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

