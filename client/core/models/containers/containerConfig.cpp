#include "containerConfig.h"

ContainerConfig::ContainerConfig()
{
}

bool ContainerConfig::isClientProtocolConfigEmpty()
{
    for (const auto &protocolConfig : protocolConfigs) {
        auto protocolConfigVariant = ProtocolConfig::getProtocolConfigVariant(protocolConfig);
        bool isConfigEmpty = std::visit(
                [](const auto &protocolConfigPtr) -> bool {
                    if constexpr (requires { protocolConfigPtr->clientProtocolConfig.isEmpty; }) {
                        return protocolConfigPtr->clientProtocolConfig.isEmpty;
                    } else {
                        return false;
                    }
                },
                protocolConfigVariant);

        if (!isConfigEmpty) {
            return false;
        }
    }
    return true;
}
