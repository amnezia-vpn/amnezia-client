#ifndef TESTSERVERREPOSITORYHELPERS_H
#define TESTSERVERREPOSITORYHELPERS_H

#include <QString>

#include "core/repositories/secureServersRepository.h"

namespace amnezia::test
{

inline QString serverDescription(SecureServersRepository *repo, const QString &serverId)
{
    switch (repo->serverKind(serverId)) {
    case SecureServersRepository::ServerConfigKind::SelfHostedAdmin: {
        const auto cfg = repo->selfHostedAdminConfig(serverId);
        return cfg.has_value() ? cfg->description : QString();
    }
    case SecureServersRepository::ServerConfigKind::SelfHostedUser: {
        const auto cfg = repo->selfHostedUserConfig(serverId);
        return cfg.has_value() ? cfg->description : QString();
    }
    case SecureServersRepository::ServerConfigKind::Native: {
        const auto cfg = repo->nativeConfig(serverId);
        return cfg.has_value() ? cfg->description : QString();
    }
    case SecureServersRepository::ServerConfigKind::ApiV2: {
        const auto cfg = repo->apiV2Config(serverId);
        return cfg.has_value() ? cfg->description : QString();
    }
    case SecureServersRepository::ServerConfigKind::LegacyApiV1: {
        const auto cfg = repo->legacyApiConfig(serverId);
        return cfg.has_value() ? cfg->description : QString();
    }
    case SecureServersRepository::ServerConfigKind::Invalid:
        return {};
    }
    return {};
}

inline void setServerDescription(SecureServersRepository *repo, const QString &serverId, const QString &description)
{
    switch (repo->serverKind(serverId)) {
    case SecureServersRepository::ServerConfigKind::SelfHostedAdmin: {
        auto cfg = repo->selfHostedAdminConfig(serverId);
        if (!cfg.has_value()) return;
        cfg->description = description;
        cfg->displayName = description;
        repo->editServer(serverId, cfg->toJson(), SecureServersRepository::ServerConfigKind::SelfHostedAdmin);
        return;
    }
    case SecureServersRepository::ServerConfigKind::SelfHostedUser: {
        auto cfg = repo->selfHostedUserConfig(serverId);
        if (!cfg.has_value()) return;
        cfg->description = description;
        cfg->displayName = description;
        repo->editServer(serverId, cfg->toJson(), SecureServersRepository::ServerConfigKind::SelfHostedUser);
        return;
    }
    case SecureServersRepository::ServerConfigKind::Native: {
        auto cfg = repo->nativeConfig(serverId);
        if (!cfg.has_value()) return;
        cfg->description = description;
        cfg->displayName = description;
        repo->editServer(serverId, cfg->toJson(), SecureServersRepository::ServerConfigKind::Native);
        return;
    }
    case SecureServersRepository::ServerConfigKind::ApiV2: {
        auto cfg = repo->apiV2Config(serverId);
        if (!cfg.has_value()) return;
        cfg->description = description;
        cfg->displayName = description;
        repo->editServer(serverId, cfg->toJson(), SecureServersRepository::ServerConfigKind::ApiV2);
        return;
    }
    case SecureServersRepository::ServerConfigKind::LegacyApiV1: {
        auto cfg = repo->legacyApiConfig(serverId);
        if (!cfg.has_value()) return;
        cfg->description = description;
        cfg->displayName = description;
        repo->editServer(serverId, cfg->toJson(), SecureServersRepository::ServerConfigKind::LegacyApiV1);
        return;
    }
    case SecureServersRepository::ServerConfigKind::Invalid:
        return;
    }
}

} // namespace amnezia::test

#endif
