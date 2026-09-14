#ifndef SELFHOSTEDUSERSERVERCONFIG_H
#define SELFHOSTEDUSERSERVERCONFIG_H

#include <QJsonObject>
#include <QMap>
#include <QPair>
#include <optional>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/containerConfig.h"
#include "core/utils/commonStructs.h"

namespace amnezia
{

using namespace ContainerEnumNS;

struct SelfHostedUserServerConfig {
    QString description;
    QString displayName;
    QString hostName;
    QMap<DockerContainer, ContainerConfig> containers;
    DockerContainer defaultContainer;
    QString dns1;
    QString dns2;

    bool hasCredentials() const;
    bool isReadOnly() const;
    std::optional<ServerCredentials> credentials() const;
    bool hasContainers() const;
    ContainerConfig containerConfig(DockerContainer container) const;

    void updateContainerConfig(DockerContainer container, const ContainerConfig &config);

    QPair<QString, QString> getDnsPair(const QString &primaryDns, const QString &secondaryDns) const;

    QJsonObject toJson() const;
    static SelfHostedUserServerConfig fromJson(const QJsonObject &json);
};

} // namespace amnezia

#endif // SELFHOSTEDUSERSERVERCONFIG_H
