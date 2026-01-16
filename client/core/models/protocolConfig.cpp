#include "protocolConfig.h"

#include "protocols/protocols_defs.h"
#include "containers/containers_defs.h"
#include "protocols/ikev2ProtocolConfig.h"

namespace amnezia
{

using namespace ProtocolEnumNS;

Proto ProtocolConfigUtils::getProtocolType(const ProtocolConfig& config)
{
    return std::visit([](auto&& arg) -> Proto {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            return Proto::Awg;
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            return Proto::WireGuard;
        } else if constexpr (std::is_same_v<T, OpenVpnProtocolConfig>) {
            return Proto::OpenVpn;
        } else if constexpr (std::is_same_v<T, XrayProtocolConfig>) {
            return Proto::Xray;
        } else if constexpr (std::is_same_v<T, SSXrayProtocolConfig>) {
            return Proto::SSXray;
        } else if constexpr (std::is_same_v<T, SftpProtocolConfig>) {
            return Proto::Sftp;
        } else if constexpr (std::is_same_v<T, Socks5ProxyProtocolConfig>) {
            return Proto::Socks5Proxy;
        } else if constexpr (std::is_same_v<T, Ikev2ProtocolConfig>) {
            return Proto::Ikev2;
        }
        return Proto::Any;
    }, config);
}

AwgProtocolConfig& ProtocolConfigUtils::asAwg(ProtocolConfig& config)
{
    return std::get<AwgProtocolConfig>(config);
}

const AwgProtocolConfig& ProtocolConfigUtils::asAwg(const ProtocolConfig& config)
{
    return std::get<AwgProtocolConfig>(config);
}

WireGuardProtocolConfig& ProtocolConfigUtils::asWireGuard(ProtocolConfig& config)
{
    return std::get<WireGuardProtocolConfig>(config);
}

const WireGuardProtocolConfig& ProtocolConfigUtils::asWireGuard(const ProtocolConfig& config)
{
    return std::get<WireGuardProtocolConfig>(config);
}

OpenVpnProtocolConfig& ProtocolConfigUtils::asOpenVpn(ProtocolConfig& config)
{
    return std::get<OpenVpnProtocolConfig>(config);
}

const OpenVpnProtocolConfig& ProtocolConfigUtils::asOpenVpn(const ProtocolConfig& config)
{
    return std::get<OpenVpnProtocolConfig>(config);
}

XrayProtocolConfig& ProtocolConfigUtils::asXray(ProtocolConfig& config)
{
    return std::get<XrayProtocolConfig>(config);
}

const XrayProtocolConfig& ProtocolConfigUtils::asXray(const ProtocolConfig& config)
{
    return std::get<XrayProtocolConfig>(config);
}

SSXrayProtocolConfig& ProtocolConfigUtils::asSSXray(ProtocolConfig& config)
{
    return std::get<SSXrayProtocolConfig>(config);
}

const SSXrayProtocolConfig& ProtocolConfigUtils::asSSXray(const ProtocolConfig& config)
{
    return std::get<SSXrayProtocolConfig>(config);
}

SftpProtocolConfig& ProtocolConfigUtils::asSftp(ProtocolConfig& config)
{
    return std::get<SftpProtocolConfig>(config);
}

const SftpProtocolConfig& ProtocolConfigUtils::asSftp(const ProtocolConfig& config)
{
    return std::get<SftpProtocolConfig>(config);
}

Socks5ProxyProtocolConfig& ProtocolConfigUtils::asSocks5Proxy(ProtocolConfig& config)
{
    return std::get<Socks5ProxyProtocolConfig>(config);
}

const Socks5ProxyProtocolConfig& ProtocolConfigUtils::asSocks5Proxy(const ProtocolConfig& config)
{
    return std::get<Socks5ProxyProtocolConfig>(config);
}

Ikev2ProtocolConfig& ProtocolConfigUtils::asIkev2(ProtocolConfig& config)
{
    return std::get<Ikev2ProtocolConfig>(config);
}

const Ikev2ProtocolConfig& ProtocolConfigUtils::asIkev2(const ProtocolConfig& config)
{
    return std::get<Ikev2ProtocolConfig>(config);
}

QString ProtocolConfigUtils::port(const ProtocolConfig& config)
{
    return std::visit([](auto&& arg) -> QString {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            return arg.serverConfig.port;
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            return arg.serverConfig.port;
        } else if constexpr (std::is_same_v<T, OpenVpnProtocolConfig>) {
            return arg.serverConfig.port;
        } else if constexpr (std::is_same_v<T, XrayProtocolConfig>) {
            return arg.serverConfig.port;
        } else if constexpr (std::is_same_v<T, SSXrayProtocolConfig>) {
            return arg.serverConfig.port;
        } else if constexpr (std::is_same_v<T, SftpProtocolConfig>) {
            return arg.port;
        } else if constexpr (std::is_same_v<T, Socks5ProxyProtocolConfig>) {
            return arg.port;
        } else if constexpr (std::is_same_v<T, Ikev2ProtocolConfig>) {
            return QString();
        }
        return QString();
    }, config);
}

QString ProtocolConfigUtils::transportProto(const ProtocolConfig& config)
{
    return std::visit([](auto&& arg) -> QString {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            return arg.serverConfig.transportProto;
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            return arg.serverConfig.transportProto;
        } else if constexpr (std::is_same_v<T, OpenVpnProtocolConfig>) {
            return arg.serverConfig.transportProto;
        } else if constexpr (std::is_same_v<T, XrayProtocolConfig>) {
            return arg.serverConfig.transportProto;
        } else if constexpr (std::is_same_v<T, SSXrayProtocolConfig>) {
            return arg.serverConfig.transportProto;
        } else if constexpr (std::is_same_v<T, Ikev2ProtocolConfig>) {
            return QString();
        }
        return QString();
    }, config);
}

QString ProtocolConfigUtils::portWithDefault(const ProtocolConfig& config, Proto protocol)
{
    QString portValue = port(config);
    if (portValue.isEmpty()) {
        return QString::number(ProtocolProps::defaultPort(protocol));
    }
    return portValue;
}

QString ProtocolConfigUtils::transportProtoWithDefault(const ProtocolConfig& config, Proto protocol)
{
    QString transportProtoValue = transportProto(config);
    if (transportProtoValue.isEmpty()) {
        return ProtocolProps::transportProtoToString(ProtocolProps::defaultTransportProto(protocol), protocol);
    }
    return transportProtoValue;
}

bool ProtocolConfigUtils::hasClientConfig(const ProtocolConfig& config)
{
    return std::visit([](auto&& arg) -> bool {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig> ||
                      std::is_same_v<T, WireGuardProtocolConfig> ||
                      std::is_same_v<T, OpenVpnProtocolConfig> ||
                      std::is_same_v<T, XrayProtocolConfig> ||
                      std::is_same_v<T, SSXrayProtocolConfig> ||
                      std::is_same_v<T, Ikev2ProtocolConfig>) {
            return arg.hasClientConfig();
        }
        return false;
    }, config);
}

QString ProtocolConfigUtils::clientId(const ProtocolConfig& config)
{
    return std::visit([](auto&& arg) -> QString {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            if (arg.clientConfig.has_value()) {
                return arg.clientConfig->clientId;
            }
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            if (arg.clientConfig.has_value()) {
                return arg.clientConfig->clientId;
            }
        } else if constexpr (std::is_same_v<T, OpenVpnProtocolConfig>) {
            if (arg.clientConfig.has_value()) {
                return arg.clientConfig->clientId;
            }
        } else if constexpr (std::is_same_v<T, XrayProtocolConfig>) {
            if (arg.clientConfig.has_value()) {
                return arg.clientConfig->id;
            }
        } else if constexpr (std::is_same_v<T, Ikev2ProtocolConfig>) {
            if (arg.clientConfig.has_value()) {
                return arg.clientConfig->clientId;
            }
        }
        return QString();
    }, config);
}

QJsonObject ProtocolConfigUtils::getClientConfigJson(const ProtocolConfig& config)
{
    return std::visit([](auto&& arg) -> QJsonObject {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            if (arg.hasClientConfig()) {
                return arg.clientConfig->toJson();
            }
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            if (arg.hasClientConfig()) {
                return arg.clientConfig->toJson();
            }
        } else if constexpr (std::is_same_v<T, OpenVpnProtocolConfig>) {
            if (arg.hasClientConfig()) {
                return arg.clientConfig->toJson();
            }
        } else if constexpr (std::is_same_v<T, XrayProtocolConfig>) {
            if (arg.hasClientConfig()) {
                return arg.clientConfig->toJson();
            }
        } else if constexpr (std::is_same_v<T, SSXrayProtocolConfig>) {
            if (arg.hasClientConfig()) {
                return arg.clientConfig->toJson();
            }
        } else if constexpr (std::is_same_v<T, Ikev2ProtocolConfig>) {
            if (arg.hasClientConfig()) {
                return arg.clientConfig->toJson();
            }
        }
        return QJsonObject();
    }, config);
}

void ProtocolConfigUtils::setClientConfigJson(ProtocolConfig& config, const QJsonObject& clientJson)
{
    std::visit([&clientJson](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            arg.setClientConfig(AwgClientConfig::fromJson(clientJson));
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            arg.setClientConfig(WireGuardClientConfig::fromJson(clientJson));
        } else if constexpr (std::is_same_v<T, OpenVpnProtocolConfig>) {
            arg.setClientConfig(OpenVpnClientConfig::fromJson(clientJson));
        } else if constexpr (std::is_same_v<T, XrayProtocolConfig>) {
            arg.setClientConfig(XrayClientConfig::fromJson(clientJson));
        } else if constexpr (std::is_same_v<T, SSXrayProtocolConfig>) {
            arg.setClientConfig(SSXrayClientConfig::fromJson(clientJson));
        } else if constexpr (std::is_same_v<T, Ikev2ProtocolConfig>) {
            arg.setClientConfig(Ikev2ClientConfig::fromJson(clientJson));
        }
    }, config);
}

void ProtocolConfigUtils::clearClientConfig(ProtocolConfig& config)
{
    std::visit([](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig> ||
                      std::is_same_v<T, WireGuardProtocolConfig> ||
                      std::is_same_v<T, OpenVpnProtocolConfig> ||
                      std::is_same_v<T, XrayProtocolConfig> ||
                      std::is_same_v<T, SSXrayProtocolConfig> ||
                      std::is_same_v<T, Ikev2ProtocolConfig>) {
            arg.clearClientConfig();
        }
    }, config);
}

QJsonObject ProtocolConfigUtils::toJson(const ProtocolConfig& config, Proto protocolType)
{
    return std::visit([protocolType](auto&& arg) -> QJsonObject {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            return arg.toJson();
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            return arg.toJson();
        } else if constexpr (std::is_same_v<T, OpenVpnProtocolConfig>) {
            return arg.toJson();
        } else if constexpr (std::is_same_v<T, XrayProtocolConfig>) {
            return arg.toJson();
        } else if constexpr (std::is_same_v<T, SSXrayProtocolConfig>) {
            return arg.toJson();
        } else if constexpr (std::is_same_v<T, SftpProtocolConfig>) {
            return arg.toJson();
        } else if constexpr (std::is_same_v<T, Socks5ProxyProtocolConfig>) {
            return arg.toJson();
        } else if constexpr (std::is_same_v<T, Ikev2ProtocolConfig>) {
            return arg.toJson();
        }
        return QJsonObject();
    }, config);
}

ProtocolConfig ProtocolConfigUtils::fromJson(const QJsonObject& json, Proto protocolType)
{
    switch (protocolType) {
    case Proto::Awg:
        return AwgProtocolConfig::fromJson(json);
    case Proto::WireGuard:
        return WireGuardProtocolConfig::fromJson(json);
    case Proto::OpenVpn:
        return OpenVpnProtocolConfig::fromJson(json);
    case Proto::Xray:
        return XrayProtocolConfig::fromJson(json);
    case Proto::SSXray:
        return SSXrayProtocolConfig::fromJson(json);
    case Proto::Sftp:
        return SftpProtocolConfig::fromJson(json);
    case Proto::Socks5Proxy:
        return Socks5ProxyProtocolConfig::fromJson(json);
    case Proto::Ikev2:
        return Ikev2ProtocolConfig::fromJson(json);
    default:
        return AwgProtocolConfig{};
    }
}

} // namespace amnezia
