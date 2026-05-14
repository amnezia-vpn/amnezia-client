#ifndef TESTSERVERREPOSITORYHELPERS_H
#define TESTSERVERREPOSITORYHELPERS_H

#include <QString>
#include <QJsonObject>

#include "core/repositories/secureServersRepository.h"
#include "core/utils/serverConfigUtils.h"

namespace amnezia::test
{

inline QString serverDescription(SecureServersRepository *repo, const QString &serverId)
{
    switch (repo->serverKind(serverId)) {
    case serverConfigUtils::ConfigType::SelfHostedAdmin: {
        const auto cfg = repo->selfHostedAdminConfig(serverId);
        return cfg.has_value() ? cfg->description : QString();
    }
    case serverConfigUtils::ConfigType::SelfHostedUser: {
        const auto cfg = repo->selfHostedUserConfig(serverId);
        return cfg.has_value() ? cfg->description : QString();
    }
    case serverConfigUtils::ConfigType::Native: {
        const auto cfg = repo->nativeConfig(serverId);
        return cfg.has_value() ? cfg->description : QString();
    }
    case serverConfigUtils::ConfigType::AmneziaPremiumV2:
    case serverConfigUtils::ConfigType::AmneziaFreeV3:
    case serverConfigUtils::ConfigType::ExternalPremium: {
        const auto cfg = repo->apiV2Config(serverId);
        return cfg.has_value() ? cfg->description : QString();
    }
    case serverConfigUtils::ConfigType::AmneziaPremiumV1:
    case serverConfigUtils::ConfigType::AmneziaFreeV2: {
        const auto cfg = repo->legacyApiConfig(serverId);
        return cfg.has_value() ? cfg->description : QString();
    }
    case serverConfigUtils::ConfigType::Invalid:
    default:
        return {};
    }
}

inline void setServerDescription(SecureServersRepository *repo, const QString &serverId, const QString &description)
{
    const serverConfigUtils::ConfigType kind = repo->serverKind(serverId);
    switch (kind) {
    case serverConfigUtils::ConfigType::SelfHostedAdmin: {
        auto cfg = repo->selfHostedAdminConfig(serverId);
        if (!cfg.has_value()) return;
        cfg->description = description;
        cfg->displayName = description;
        repo->editServer(serverId, cfg->toJson(), kind);
        return;
    }
    case serverConfigUtils::ConfigType::SelfHostedUser: {
        auto cfg = repo->selfHostedUserConfig(serverId);
        if (!cfg.has_value()) return;
        cfg->description = description;
        cfg->displayName = description;
        repo->editServer(serverId, cfg->toJson(), kind);
        return;
    }
    case serverConfigUtils::ConfigType::Native: {
        auto cfg = repo->nativeConfig(serverId);
        if (!cfg.has_value()) return;
        cfg->description = description;
        cfg->displayName = description;
        repo->editServer(serverId, cfg->toJson(), kind);
        return;
    }
    case serverConfigUtils::ConfigType::AmneziaPremiumV2:
    case serverConfigUtils::ConfigType::AmneziaFreeV3:
    case serverConfigUtils::ConfigType::ExternalPremium: {
        auto cfg = repo->apiV2Config(serverId);
        if (!cfg.has_value()) return;
        cfg->description = description;
        cfg->displayName = description;
        repo->editServer(serverId, cfg->toJson(), kind);
        return;
    }
    case serverConfigUtils::ConfigType::AmneziaPremiumV1:
    case serverConfigUtils::ConfigType::AmneziaFreeV2:
    case serverConfigUtils::ConfigType::Invalid:
    default:
        return;
    }
}

} // namespace amnezia::test

#endif
