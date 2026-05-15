#ifndef SELFHOSTEDADMINSERVERCONFIG_H
#define SELFHOSTEDADMINSERVERCONFIG_H

#include <QJsonObject>
#include <QMap>
#include <QPair>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/containerConfig.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

namespace amnezia
{

using namespace ContainerEnumNS;

struct SelfHostedAdminServerConfig {
    QString description;
    QString displayName;
    QString hostName;
    QMap<DockerContainer, ContainerConfig> containers;
    DockerContainer defaultContainer;
    QString dns1;
    QString dns2;

    QString userName;
    QString password;
    int port = 0;

    bool hasCredentials() const;
    bool isReadOnly() const;
    ServerCredentials credentials() const;
    bool hasContainers() const;
    ContainerConfig containerConfig(DockerContainer container) const;

    void updateContainerConfig(DockerContainer container, const ContainerConfig &config);

    void clearCachedClientProfile(DockerContainer container);

    QPair<QString, QString> getDnsPair(bool isAmneziaDnsEnabled, const QString &primaryDns,
                                       const QString &secondaryDns) const;

    QJsonObject toJson() const;
    static SelfHostedAdminServerConfig fromJson(const QJsonObject &json);
};

} // namespace amnezia

#endif // SELFHOSTEDADMINSERVERCONFIG_H
