#ifndef APIENUMS_H
#define APIENUMS_H

namespace apiDefs
{
    enum ConfigType {
        AmneziaFreeV2 = 0,
        AmneziaFreeV3,
        AmneziaPremiumV1,
        AmneziaPremiumV2,
        AmneziaTrialV2,
        SelfHosted,
        ExternalPremium,
        ExternalTrial
    };

    enum ConfigSource {
        Telegram = 1,
        AmneziaGateway
    };
}

#endif // APIENUMS_H


