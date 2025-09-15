#include "protocolConfig.h"

#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/cloakProtocolConfig.h"
#include "core/models/protocols/ipsecProtocolConfig.h"
#include "core/models/protocols/openvpnProtocolConfig.h"
#include "core/models/protocols/protocolConfig.h"
#include "core/models/protocols/sftpProtocolConfig.h"
#include "core/models/protocols/shadowsocksProtocolConfig.h"
#include "core/models/protocols/socks5ProxyProtocolConfig.h"
#include "core/models/protocols/torProtocolConfig.h"
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
    case Proto::Ikev2: variant = qSharedPointerCast<IpsecProtocolConfig>(protocolConfig); break;
    case Proto::Sftp: variant = qSharedPointerCast<SftpProtocolConfig>(protocolConfig); break;
    case Proto::Socks5Proxy: variant = qSharedPointerCast<Socks5ProxyProtocolConfig>(protocolConfig); break;
    case Proto::TorWebSite: variant = qSharedPointerCast<TorProtocolConfig>(protocolConfig); break;
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
    case Proto::Ikev2: {
        auto thisConfig = qSharedPointerCast<IpsecProtocolConfig>(QSharedPointer<ProtocolConfig>(this));
        auto otherConfig = qSharedPointerCast<IpsecProtocolConfig>(other);
        return thisConfig->hasEqualServerSettings(*otherConfig.data());
    }
    case Proto::Sftp: {
        auto thisConfig = qSharedPointerCast<SftpProtocolConfig>(QSharedPointer<ProtocolConfig>(this));
        auto otherConfig = qSharedPointerCast<SftpProtocolConfig>(other);
        return thisConfig->hasEqualServerSettings(*otherConfig.data());
    }
    case Proto::Socks5Proxy: {
        auto thisConfig = qSharedPointerCast<Socks5ProxyProtocolConfig>(QSharedPointer<ProtocolConfig>(this));
        auto otherConfig = qSharedPointerCast<Socks5ProxyProtocolConfig>(other);
        return thisConfig->hasEqualServerSettings(*otherConfig.data());
    }
    default: return false;
    }
}

QString ProtocolConfig::getNativeConfig(const QSharedPointer<ProtocolConfig> &protocolConfig)
{
    if (!protocolConfig) {
        return QString();
    }

    auto protocolConfigVariant = ProtocolConfig::getProtocolConfigVariant(protocolConfig);
    return std::visit(
            [](const auto &configPtr) -> QString {
                if constexpr (requires { configPtr->clientProtocolConfig.nativeConfig; }) {
                    return configPtr->clientProtocolConfig.nativeConfig;
                } else {
                    return QString();
                }
            },
            protocolConfigVariant);
}

QJsonObject ProtocolConfig::toJson() const
{
    return QJsonObject();
}

ScriptVars ProtocolConfig::getScriptVars() const
{
    return ScriptVars();
}
