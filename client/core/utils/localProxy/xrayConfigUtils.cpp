#include "xrayConfigUtils.h"

#include "core/utils/constants/configKeys.h"
#include "core/utils/containers/containerUtils.h"

#include <QJsonArray>

using namespace amnezia;

std::optional<QString> xrayConfigUtils::extractLastConfig(const QJsonObject &serverConfig)
{
    const QJsonArray containers = serverConfig.value(configKey::containers).toArray();
    const QString targetContainer = ContainerUtils::containerToString(DockerContainer::Xray);

    for (const QJsonValue &value : containers) {
        const QJsonObject container = value.toObject();
        if (container.value(configKey::container).toString() != targetContainer) {
            continue;
        }

        const QJsonObject proto = container.value(QString(configKey::xray)).toObject();
        const QString serialized = proto.value(configKey::lastConfig).toString();
        if (!serialized.isEmpty()) {
            return serialized;
        }
    }

    return std::nullopt;
}

bool xrayConfigUtils::setInboundPort(QJsonObject &xrayConfig, int port)
{
    if (!xrayConfig.value(QLatin1String("inbounds")).isArray()) {
        return false;
    }

    QJsonArray inbounds = xrayConfig.value(QLatin1String("inbounds")).toArray();
    if (inbounds.isEmpty() || !inbounds.at(0).isObject()) {
        return false;
    }

    QJsonObject firstInbound = inbounds.at(0).toObject();
    firstInbound.insert(QLatin1String("port"), port);
    inbounds[0] = firstInbound;
    xrayConfig.insert(QLatin1String("inbounds"), inbounds);
    return true;
}
