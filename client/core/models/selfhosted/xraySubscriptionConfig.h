#ifndef XRAYSUBSCRIPTIONCONFIG_H
#define XRAYSUBSCRIPTIONCONFIG_H

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <optional>

#include "core/models/containerConfig.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"

namespace amnezia
{

    using namespace ContainerEnumNS;

    struct XRaySubscriptionConfig
    {
        QString description;
        QString displayName;
        QString hostName;
        QMap<DockerContainer, ContainerConfig> containers;
        DockerContainer defaultContainer;
        QString dns1;
        QString dns2;

        QString subLink;
        QJsonArray configString;
        QJsonArray configName;
        int currentConfig;

        bool hasContainers() const;
        ContainerConfig containerConfig(DockerContainer container) const;
        QJsonObject toJson() const;
        static XRaySubscriptionConfig fromJson(const QJsonObject &json);
    };

} // namespace amnezia

#endif // XRAYSUBSCRIPTIONCONFIG_H