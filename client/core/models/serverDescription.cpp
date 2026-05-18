#include "serverDescription.h"

#include <QMap>

#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/containers/containerUtils.h"
#include "core/protocols/protocolUtils.h"
#include "core/models/protocols/awgProtocolConfig.h"

using namespace amnezia;

namespace
{

bool computeHasInstalledVpnContainers(const QMap<DockerContainer, ContainerConfig> &containers)
{
    for (auto it = containers.begin(); it != containers.end(); ++it) {
        const DockerContainer container = it.key();
        if (ContainerUtils::containerService(container) == ServiceType::Vpn || container == DockerContainer::SSXray) {
            return true;
        }
    }
    return false;
}

template <typename T>
ServerDescription buildBaseDescription(const T &server)
{
    ServerDescription row;
    row.hostName = server.hostName;
    row.defaultContainer = server.defaultContainer;
    row.primaryDnsIsAmnezia = (server.dns1 == protocols::dns::amneziaDnsIp);
    row.hasInstalledVpnContainers = computeHasInstalledVpnContainers(server.containers);
    return row;
}

QString getBaseDescription(const QMap<DockerContainer, ContainerConfig> &containers,
                         bool isAmneziaDnsEnabled,
                         bool hasWriteAccess,
                         bool primaryDnsIsAmnezia)
{
    QString description;
    if (hasWriteAccess) {
        const bool isDnsInstalled = containers.contains(DockerContainer::Dns);
        if (isAmneziaDnsEnabled && isDnsInstalled) {
            description += QStringLiteral("Amnezia DNS | ");
        }
    } else if (primaryDnsIsAmnezia) {
        description += QStringLiteral("Amnezia DNS | ");
    }
    return description;
}

QString getProtocolName(DockerContainer defaultContainer, const QMap<DockerContainer, ContainerConfig> &containers)
{
    QString containerName = ContainerUtils::containerHumanNames().value(defaultContainer);
    QString protocolVersion;

    if (ContainerUtils::isAwgContainer(defaultContainer)) {
        const auto it = containers.constFind(defaultContainer);
        if (it != containers.cend()) {
            if (const AwgProtocolConfig *awg = it->getAwgProtocolConfig()) {
                protocolVersion = ProtocolUtils::getProtocolVersionString(awg->toJson());
                if (defaultContainer == DockerContainer::Awg && !awg->serverConfig.isThirdPartyConfig) {
                    containerName = QStringLiteral("AmneziaWG Legacy");
                }
            }
        }
    }

    return containerName + protocolVersion + QStringLiteral(" | ");
}

} // namespace

namespace amnezia
{

ServerDescription buildServerDescription(const SelfHostedAdminServerConfig &server, bool isAmneziaDnsEnabled)
{
    ServerDescription row = buildBaseDescription(server);
    row.selfHostedSshCredentials.hostName = server.hostName;
    row.selfHostedSshCredentials.userName = server.userName;
    row.selfHostedSshCredentials.secretData = server.password;
    row.selfHostedSshCredentials.port = server.port > 0 ? server.port : 22;

    row.hasWriteAccess = !row.selfHostedSshCredentials.userName.isEmpty()
                         && !row.selfHostedSshCredentials.secretData.isEmpty();

    row.serverName = server.displayName;
    row.baseDescription = getBaseDescription(server.containers, isAmneziaDnsEnabled, row.hasWriteAccess, row.primaryDnsIsAmnezia);

    const QString protocolName = getProtocolName(server.defaultContainer, server.containers);
    row.expandedServerDescription = row.baseDescription + row.hostName;
    row.collapsedServerDescription = row.baseDescription + protocolName + row.hostName;
    return row;
}

ServerDescription buildServerDescription(const SelfHostedUserServerConfig &server, bool isAmneziaDnsEnabled)
{
    ServerDescription row = buildBaseDescription(server);
    row.selfHostedSshCredentials.hostName = server.hostName;
    row.selfHostedSshCredentials.port = 22;
    row.hasWriteAccess = false;

    row.serverName = server.displayName;
    row.baseDescription = getBaseDescription(server.containers, isAmneziaDnsEnabled, row.hasWriteAccess, row.primaryDnsIsAmnezia);

    const QString protocolName = getProtocolName(server.defaultContainer, server.containers);
    row.expandedServerDescription = row.baseDescription + row.hostName;
    row.collapsedServerDescription = row.baseDescription + protocolName + row.hostName;
    return row;
}

ServerDescription buildServerDescription(const NativeServerConfig &server, bool isAmneziaDnsEnabled)
{
    ServerDescription row = buildBaseDescription(server);
    row.hasWriteAccess = false;

    row.serverName = server.displayName;
    row.baseDescription = getBaseDescription(server.containers, isAmneziaDnsEnabled, row.hasWriteAccess, row.primaryDnsIsAmnezia);

    const QString protocolName = getProtocolName(server.defaultContainer, server.containers);
    row.expandedServerDescription = row.baseDescription + row.hostName;
    row.collapsedServerDescription = row.baseDescription + protocolName + row.hostName;
    return row;
}

ServerDescription buildServerDescription(const LegacyApiServerConfig &server, bool /*isAmneziaDnsEnabled*/)
{
    ServerDescription row = buildBaseDescription(server);
    row.configVersion = serverConfigUtils::ConfigSource::Telegram;
    row.isApiV1 = true;
    row.isServerFromGatewayApi = false;
    row.hasWriteAccess = false;

    row.serverName = server.displayName;
    row.baseDescription = server.description;

    const QString fullDescriptionForCollapsed = row.baseDescription;
    row.collapsedServerDescription = fullDescriptionForCollapsed;
    row.expandedServerDescription = fullDescriptionForCollapsed;
    return row;
}

ServerDescription buildServerDescription(const ApiV2ServerConfig &server, bool /*isAmneziaDnsEnabled*/)
{
    ServerDescription row = buildBaseDescription(server);
    row.configVersion = serverConfigUtils::ConfigSource::AmneziaGateway;
    row.isApiV2 = true;
    row.isServerFromGatewayApi = true;
    row.isPremium = server.isPremium() || server.isExternalPremium();
    row.hasWriteAccess = false;

    row.serverName = server.displayName;
    row.baseDescription = server.apiConfig.serverCountryCode.isEmpty() ? server.description : server.apiConfig.serverCountryName;

    row.isCountrySelectionAvailable = !server.apiConfig.availableCountries.isEmpty();
    row.apiAvailableCountries = server.apiConfig.availableCountries;
    row.apiServerCountryCode = server.apiConfig.serverCountryCode;

    row.isAdVisible = server.apiConfig.serviceInfo.isAdVisible;
    row.adHeader = server.apiConfig.serviceInfo.adHeader;
    row.adDescription = server.apiConfig.serviceInfo.adDescription;
    row.adEndpoint = server.apiConfig.serviceInfo.adEndpoint;
    row.isRenewalAvailable = server.apiConfig.serviceInfo.isRenewalAvailable;

    if (!server.apiConfig.isInAppPurchase) {
        if (server.apiConfig.subscriptionExpiredByServer) {
            row.isSubscriptionExpired = true;
        } else if (!server.apiConfig.subscription.endDate.isEmpty()) {
            row.isSubscriptionExpired = apiUtils::isSubscriptionExpired(server.apiConfig.subscription.endDate);
            row.isSubscriptionExpiringSoon = apiUtils::isSubscriptionExpiringSoon(server.apiConfig.subscription.endDate);
        }
    }

    const QString fullDescriptionForCollapsed = row.baseDescription;
    row.collapsedServerDescription = fullDescriptionForCollapsed;
    row.expandedServerDescription = fullDescriptionForCollapsed;
    return row;
}

} // namespace amnezia
