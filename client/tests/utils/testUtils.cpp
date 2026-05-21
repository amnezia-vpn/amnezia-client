#include "utils/testUtils.h"

#include <QSettings>

#include "core/repositories/secureServersRepository.h"
#include "core/utils/serverConfigUtils.h"

namespace amnezia::test
{

QString getValueFromIni(const QString &key, const QString &iniFileName)
{
    QSettings settings(iniFileName, QSettings::IniFormat);
    return settings.value(key).toString();
}

bool isIniValueConfigured(const QString &value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    if (trimmed.endsWith(QStringLiteral("_config_string"))) {
        return false;
    }
    return true;
}

std::optional<QString> serverDescription(SecureServersRepository *repo, const QString &serverId)
{
    switch (repo->serverKind(serverId)) {
    case serverConfigUtils::ConfigType::SelfHostedAdmin: {
        const auto cfg = repo->selfHostedAdminConfig(serverId);
        return cfg ? std::optional<QString>(cfg->description) : std::nullopt;
    }
    case serverConfigUtils::ConfigType::SelfHostedUser: {
        const auto cfg = repo->selfHostedUserConfig(serverId);
        return cfg ? std::optional<QString>(cfg->description) : std::nullopt;
    }
    case serverConfigUtils::ConfigType::Native: {
        const auto cfg = repo->nativeConfig(serverId);
        return cfg ? std::optional<QString>(cfg->description) : std::nullopt;
    }
    default:
        return std::nullopt;
    }
}

std::optional<QString> serverDescriptionAt(SecureServersRepository *repo, int index)
{
    return serverDescription(repo, repo->serverIdAt(index));
}

} // namespace amnezia::test
