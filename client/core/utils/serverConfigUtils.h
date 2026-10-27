#ifndef SERVERCONFIGUTILS_H
#define SERVERCONFIGUTILS_H

#include <QJsonObject>

namespace serverConfigUtils
{

enum ConfigType {
    AmneziaFreeV2 = 0,
    AmneziaFreeV3,
    AmneziaPremiumV1,
    AmneziaPremiumV2,
    SelfHosted,
    ExternalPremium,

    SelfHostedAdmin = 8,
    SelfHostedUser,
    Native,
    Invalid
};

enum ConfigSource {
    Telegram = 1,
    AmneziaGateway
};

constexpr int currentConfigFormatVersion = 1;

int configFormatVersion(const QJsonObject &serverConfigObject);

bool isConfigFormatVersionSupported(const QJsonObject &serverConfigObject);

bool isServerFromApi(const QJsonObject &serverConfigObject);

ConfigSource getConfigSource(const QJsonObject &serverConfigObject);

ConfigType configTypeFromJson(const QJsonObject &serverConfigObject);

bool isLegacyApiSubscription(ConfigType configType);

bool isApiV2Subscription(ConfigType configType);

} // namespace serverConfigUtils

#endif // SERVERCONFIGUTILS_H
