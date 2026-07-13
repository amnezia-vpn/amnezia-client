#ifndef APIV2SERVERCONFIG_H
#define APIV2SERVERCONFIG_H

#include <QJsonObject>
#include <QMap>
#include <QPair>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/containerConfig.h"
#include "core/models/api/apiConfig.h"
#include "core/models/api/authData.h"
#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"

namespace amnezia
{

using namespace ContainerEnumNS;

struct ApiV2ServerConfig {
    QString description;
    QString displayName;
    QString hostName;
    QMap<DockerContainer, ContainerConfig> containers;
    DockerContainer defaultContainer;
    QString dns1;
    QString dns2;
    
    QString name;
    bool nameOverriddenByUser = false;
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

    QPair<QString, QString> getDnsPair(const QString &primaryDns, const QString &secondaryDns) const;

    QJsonObject toJson() const;
    static ApiV2ServerConfig fromJson(const QJsonObject& json);
};

} // namespace amnezia

#endif // APIV2SERVERCONFIG_H

