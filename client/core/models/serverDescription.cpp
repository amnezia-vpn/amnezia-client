#include "serverDescription.h"

#include <QMap>

#include "core/utils/api/apiEnums.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/containers/containerUtils.h"

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

QString selfHostedSnippet(const QMap<DockerContainer, ContainerConfig> &containers,
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

    row.nameForNameRole = server.displayName;
    row.descriptionSnippet = selfHostedSnippet(server.containers, isAmneziaDnsEnabled, row.hasWriteAccess, row.primaryDnsIsAmnezia);

    const QString fullDescriptionForCollapsed = row.descriptionSnippet + row.hostName;
    row.collapsedServerDescription = fullDescriptionForCollapsed;
    row.expandedServerDescription = fullDescriptionForCollapsed;
    return row;
}

ServerDescription buildServerDescription(const SelfHostedUserServerConfig &server, bool isAmneziaDnsEnabled)
{
    ServerDescription row = buildBaseDescription(server);
    row.selfHostedSshCredentials.hostName = server.hostName;
    row.selfHostedSshCredentials.port = 22;
    row.hasWriteAccess = false;

    row.nameForNameRole = server.displayName;
    row.descriptionSnippet = selfHostedSnippet(server.containers, isAmneziaDnsEnabled, row.hasWriteAccess, row.primaryDnsIsAmnezia);

    const QString fullDescriptionForCollapsed = row.descriptionSnippet + row.hostName;
    row.collapsedServerDescription = fullDescriptionForCollapsed;
    row.expandedServerDescription = fullDescriptionForCollapsed;
    return row;
}

ServerDescription buildServerDescription(const NativeServerConfig &server, bool isAmneziaDnsEnabled)
{
    ServerDescription row = buildBaseDescription(server);
    row.hasWriteAccess = false;

    row.nameForNameRole = server.displayName;
    row.descriptionSnippet = selfHostedSnippet(server.containers, isAmneziaDnsEnabled, row.hasWriteAccess, row.primaryDnsIsAmnezia);

    const QString fullDescriptionForCollapsed = row.descriptionSnippet + row.hostName;
    row.collapsedServerDescription = fullDescriptionForCollapsed;
    row.expandedServerDescription = fullDescriptionForCollapsed;
    return row;
}

ServerDescription buildServerDescription(const LegacyApiServerConfig &server, bool /*isAmneziaDnsEnabled*/)
{
    ServerDescription row = buildBaseDescription(server);
    row.configVersion = apiDefs::ConfigSource::Telegram;
    row.isApiV1 = true;
    row.isServerFromGatewayApi = false;
    row.hasWriteAccess = false;

    row.nameForNameRole = server.displayName;
    row.descriptionSnippet = server.description;

    const QString fullDescriptionForCollapsed = row.descriptionSnippet;
    row.collapsedServerDescription = fullDescriptionForCollapsed;
    row.expandedServerDescription = fullDescriptionForCollapsed;
    return row;
}

ServerDescription buildServerDescription(const ApiV2ServerConfig &server, bool /*isAmneziaDnsEnabled*/)
{
    ServerDescription row = buildBaseDescription(server);
    row.configVersion = apiDefs::ConfigSource::AmneziaGateway;
    row.isApiV2 = true;
    row.isServerFromGatewayApi = true;
    row.isPremium = server.isPremium() || server.isExternalPremium();
    row.hasWriteAccess = false;

    row.nameForNameRole = server.displayName;
    row.descriptionSnippet = server.apiConfig.serverCountryCode.isEmpty() ? server.description : server.apiConfig.serverCountryName;

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

    const QString fullDescriptionForCollapsed = row.descriptionSnippet;
    row.collapsedServerDescription = fullDescriptionForCollapsed;
    row.expandedServerDescription = fullDescriptionForCollapsed;
    return row;
}

} // namespace amnezia
