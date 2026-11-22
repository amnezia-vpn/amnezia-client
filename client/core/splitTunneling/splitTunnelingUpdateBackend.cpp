#include "splitTunnelingUpdateBackend.h"

#include <QHostAddress>
#include <QJsonArray>

#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"

using namespace amnezia;

SplitTunnelingUpdateBackend::SplitTunnelingUpdateBackend(QObject *parent) : QObject(parent)
{
}

bool SplitTunnelingUpdateBackend::supportsConfiguration(const QJsonObject &configuration) const
{
    const auto protocol = ProtocolUtils::protoFromString(configuration.value(configKey::vpnProto).toString());
    if (protocol != Proto::WireGuard && protocol != Proto::Awg) {
        return false;
    }
    const auto config = configuration.value(ProtocolUtils::key_proto_config_data(protocol)).toObject();
    const QJsonArray defaultAllowedIps { "0.0.0.0/0", "::/0" };
    const int routeMode = configuration.value(configKey::splitTunnelType).toInt(-1);
    if (config.value(configKey::allowedIps).toArray() != defaultAllowedIps
        || config.value(configKey::serverPubKey).toString().isEmpty()
        || configuration.value(configKey::splitTunnelSessionId).toString().isEmpty() || routeMode < 0 || routeMode > 2
        || !configuration.value(configKey::splitTunnelSites).isArray()) {
        return false;
    }
    for (const auto &site : configuration.value(configKey::splitTunnelSites).toArray()) {
        const auto subnet = QHostAddress::parseSubnet(site.toString());
        if (subnet.first.protocol() != QAbstractSocket::IPv4Protocol || subnet.second < 0 || subnet.second > 32) {
            return false;
        }
    }
    return true;
}
