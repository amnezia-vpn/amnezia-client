#ifndef APIV2SERVERCONFIG_H
#define APIV2SERVERCONFIG_H

#include <QJsonObject>
#include <QMap>

#include "containers/containers_defs.h"
#include "core/models/containerConfig.h"
#include "core/models/api/apiConfig.h"
#include "core/models/api/authData.h"
#include "core/utils/api/apiDefs.h"

namespace amnezia
{

using namespace ContainerEnumNS;

struct ApiV2ServerConfig {
    QString description;
    QString hostName;
    QMap<DockerContainer, ContainerConfig> containers;
    DockerContainer defaultContainer;
    QString dns1;
    QString dns2;
    
    QString name;
    int crc;
    int configVersion;
    ApiConfig apiConfig;
    AuthData authData;
    
    QString vpnKey() const;
    QString serviceType() const;
    QString serviceProtocol() const;
    bool isPremium() const;
    bool isFree() const;
    bool isExternalPremium() const;
    bool hasContainers() const;
    ContainerConfig containerConfig(DockerContainer container) const;
    QJsonObject toJson() const;
    static ApiV2ServerConfig fromJson(const QJsonObject& json);
};

} // namespace amnezia

#endif // APIV2SERVERCONFIG_H

