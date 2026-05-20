#ifndef SERVERDESCRIPTION_H
#define SERVERDESCRIPTION_H

#include <QString>
#include <QJsonArray>

#include "core/utils/containerEnum.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/models/selfhosted/selfHostedAdminServerConfig.h"
#include "core/models/selfhosted/selfHostedUserServerConfig.h"
#include "core/models/selfhosted/nativeServerConfig.h"
#include "core/models/api/legacyApiServerConfig.h"
#include "core/models/api/apiV2ServerConfig.h"

namespace amnezia
{

struct ServerDescription
{
    QString serverId;

    QString serverName;
    QString baseDescription;
    QString hostName;

    int configVersion = 0;

    ServerCredentials selfHostedSshCredentials;
    bool hasWriteAccess = false;

    bool primaryDnsIsAmnezia = false;
    DockerContainer defaultContainer = DockerContainer::None;
    bool hasInstalledVpnContainers = false;

    bool isApiV1 = false;
    bool isApiV2 = false;
    bool isServerFromGatewayApi = false;
    bool isPremium = false;

    bool isCountrySelectionAvailable = false;
    QJsonArray apiAvailableCountries;
    QString apiServerCountryCode;

    bool isAdVisible = false;
    QString adHeader;
    QString adDescription;
    QString adEndpoint;
    bool isRenewalAvailable = false;
    bool isSubscriptionExpired = false;
    bool isSubscriptionExpiringSoon = false;

    QString collapsedServerDescription;
    QString expandedServerDescription;
};

ServerDescription buildServerDescription(const SelfHostedAdminServerConfig &server, bool isAmneziaDnsEnabled);
ServerDescription buildServerDescription(const SelfHostedUserServerConfig &server, bool isAmneziaDnsEnabled);
ServerDescription buildServerDescription(const NativeServerConfig &server, bool isAmneziaDnsEnabled);
ServerDescription buildServerDescription(const LegacyApiServerConfig &server, bool isAmneziaDnsEnabled);
ServerDescription buildServerDescription(const ApiV2ServerConfig &server, bool isAmneziaDnsEnabled);

} // namespace amnezia

#endif
