#include "selfHostedAdminServerConfig.h"

#include <QJsonArray>

#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/networkUtilities.h"

namespace amnezia
{

using namespace ContainerEnumNS;

bool SelfHostedAdminServerConfig::hasCredentials() const
{
    return !userName.isEmpty() && !password.isEmpty() && port > 0;
}

bool SelfHostedAdminServerConfig::isReadOnly() const
{
    return !hasCredentials();
}

ServerCredentials SelfHostedAdminServerConfig::credentials() const
{
    ServerCredentials creds;
    creds.hostName = hostName;
    creds.userName = userName;
    creds.secretData = password;
    creds.port = port;
    return creds;
}

bool SelfHostedAdminServerConfig::hasContainers() const
{
    return !containers.isEmpty();
}

ContainerConfig SelfHostedAdminServerConfig::containerConfig(DockerContainer container) const
{
    if (!containers.contains(container)) {
        return ContainerConfig{};
    }
    return containers.value(container);
}

void SelfHostedAdminServerConfig::updateContainerConfig(DockerContainer container, const ContainerConfig &config)
{
    containers[container] = config;
}

void SelfHostedAdminServerConfig::clearCachedClientProfile(DockerContainer container)
{
    if (ContainerUtils::containerService(container) == ServiceType::Other) {
        return;
    }

    ContainerConfig cleared = containerConfig(container);
    cleared.protocolConfig.clearClientConfig();
    containers[container] = cleared;
}

QPair<QString, QString> SelfHostedAdminServerConfig::getDnsPair(bool isAmneziaDnsEnabled, const QString &primaryDns,
                                                                const QString &secondaryDns) const
{
    QString d1 = dns1;
    QString d2 = dns2;
    const bool dnsOnServer = containers.contains(DockerContainer::Dns);

    if (d1.isEmpty() || !NetworkUtilities::checkIPv4Format(d1)) {
        d1 = (isAmneziaDnsEnabled && dnsOnServer) ? protocols::dns::amneziaDnsIp : primaryDns;
    }
    if (d2.isEmpty() || !NetworkUtilities::checkIPv4Format(d2)) {
        d2 = secondaryDns;
    }
    return { d1, d2 };
}

QJsonObject SelfHostedAdminServerConfig::toJson() const
{
    QJsonObject obj;

    if (!description.isEmpty()) {
        obj[configKey::description] = this->description;
    }
    if (!hostName.isEmpty()) {
        obj[configKey::hostName] = hostName;
    }

    QJsonArray containersArray;
    for (auto it = containers.begin(); it != containers.end(); ++it) {
        QJsonObject containerObj = it.value().toJson();
        containersArray.append(containerObj);
    }
    if (!containersArray.isEmpty()) {
        obj[configKey::containers] = containersArray;
    }

    if (defaultContainer != DockerContainer::None) {
        obj[configKey::defaultContainer] = ContainerUtils::containerToString(defaultContainer);
    }

    if (!dns1.isEmpty()) {
        obj[configKey::dns1] = dns1;
    }
    if (!dns2.isEmpty()) {
        obj[configKey::dns2] = dns2;
    }

    if (!userName.isEmpty()) {
        obj[configKey::userName] = userName;
    }
    if (!password.isEmpty()) {
        obj[configKey::password] = password;
    }
    if (port > 0) {
        obj[configKey::port] = port;
    }

    return obj;
}

SelfHostedAdminServerConfig SelfHostedAdminServerConfig::fromJson(const QJsonObject &json)
{
    SelfHostedAdminServerConfig config;

    config.description = json.value(configKey::description).toString();
    config.hostName = json.value(configKey::hostName).toString();

    QJsonArray containersArray = json.value(configKey::containers).toArray();
    for (const QJsonValue &val : containersArray) {
        QJsonObject containerObj = val.toObject();
        ContainerConfig cc = ContainerConfig::fromJson(containerObj);

        QString containerStr = containerObj.value(configKey::container).toString();
        DockerContainer container = ContainerUtils::containerFromString(containerStr);

        config.containers.insert(container, cc);
    }

    QString defaultContainerStr = json.value(configKey::defaultContainer).toString();
    config.defaultContainer = ContainerUtils::containerFromString(defaultContainerStr);

    config.dns1 = json.value(configKey::dns1).toString();
    config.dns2 = json.value(configKey::dns2).toString();

    config.userName = json.value(configKey::userName).toString();
    config.password = json.value(configKey::password).toString();
    if (json.contains(configKey::port)) {
        config.port = json.value(configKey::port).toInt();
    } else {
        config.port = 0;
    }

    if (config.displayName.isEmpty()) {
        config.displayName = config.description.isEmpty() ? config.hostName : config.description;
    }

    return config;
}

} // namespace amnezia
