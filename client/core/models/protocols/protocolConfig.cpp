#include "protocolConfig.h"

#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/cloakProtocolConfig.h"
#include "core/models/protocols/openvpnProtocolConfig.h"
#include "core/models/protocols/protocolConfig.h"
#include "core/models/protocols/shadowsocksProtocolConfig.h"
#include "core/models/protocols/wireguardProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "protocols/protocols_defs.h"

using namespace amnezia;

ProtocolConfig::ProtocolConfig(const QString &protocolName) : protocolName(protocolName)
{
}

ProtocolConfigVariant ProtocolConfig::getProtocolConfigVariant(const QSharedPointer<ProtocolConfig> &protocolConfig)
{
    ProtocolConfigVariant variant;
    auto proto = ProtocolProps::protoFromString(protocolConfig->protocolName);
    switch (proto) {
    case Proto::OpenVpn: variant = qSharedPointerCast<OpenVpnProtocolConfig>(protocolConfig); break;
    case Proto::WireGuard: variant = qSharedPointerCast<WireGuardProtocolConfig>(protocolConfig); break;
    case Proto::ShadowSocks: variant = qSharedPointerCast<ShadowsocksProtocolConfig>(protocolConfig); break;
    case Proto::Cloak: variant = qSharedPointerCast<CloakProtocolConfig>(protocolConfig); break;
    case Proto::Xray: variant = qSharedPointerCast<XrayProtocolConfig>(protocolConfig); break;
    case Proto::Awg: variant = qSharedPointerCast<AwgProtocolConfig>(protocolConfig); break;
    default: break;
    }
    return variant;
}

bool ProtocolConfig::isServerSettingsEqual(const QSharedPointer<ProtocolConfig> &other)
{
    auto proto = ProtocolProps::protoFromString(protocolName);

    switch (proto) {
    case Proto::OpenVpn: {
        auto thisConfig = qSharedPointerCast<OpenVpnProtocolConfig>(QSharedPointer<ProtocolConfig>(this));
        auto otherConfig = qSharedPointerCast<OpenVpnProtocolConfig>(other);
        return thisConfig->hasEqualServerSettings(*otherConfig.data());
    }
    case Proto::WireGuard: {
        auto thisConfig = qSharedPointerCast<WireGuardProtocolConfig>(QSharedPointer<ProtocolConfig>(this));
        auto otherConfig = qSharedPointerCast<WireGuardProtocolConfig>(other);
        return thisConfig->hasEqualServerSettings(*otherConfig.data());
    }
    case Proto::ShadowSocks: {
        auto thisConfig = qSharedPointerCast<ShadowsocksProtocolConfig>(QSharedPointer<ProtocolConfig>(this));
        auto otherConfig = qSharedPointerCast<ShadowsocksProtocolConfig>(other);
        return thisConfig->hasEqualServerSettings(*otherConfig.data());
    }
    case Proto::Cloak: {
        auto thisConfig = qSharedPointerCast<CloakProtocolConfig>(QSharedPointer<ProtocolConfig>(this));
        auto otherConfig = qSharedPointerCast<CloakProtocolConfig>(other);
        return thisConfig->hasEqualServerSettings(*otherConfig.data());
    }
    case Proto::Xray: {
        auto thisConfig = qSharedPointerCast<XrayProtocolConfig>(QSharedPointer<ProtocolConfig>(this));
        auto otherConfig = qSharedPointerCast<XrayProtocolConfig>(other);
        return thisConfig->hasEqualServerSettings(*otherConfig.data());
    }
    case Proto::Awg: {
        auto thisConfig = qSharedPointerCast<AwgProtocolConfig>(QSharedPointer<ProtocolConfig>(this));
        auto otherConfig = qSharedPointerCast<AwgProtocolConfig>(other);
        return thisConfig->hasEqualServerSettings(*otherConfig.data());
    }
    default: return false;
    }
}

QJsonObject ProtocolConfig::toJson() const
{
    return QJsonObject();
}
