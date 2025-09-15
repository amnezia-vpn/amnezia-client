#ifndef PROTOCOLCONFIG_H
#define PROTOCOLCONFIG_H

#include <type_traits>
#include <variant>

#include <QJsonObject>
#include <QSharedPointer>

#include "protocols/protocols_defs.h"

class OpenVpnProtocolConfig;
class WireGuardProtocolConfig;
class ShadowsocksProtocolConfig;
class CloakProtocolConfig;
class XrayProtocolConfig;
class AwgProtocolConfig;
class IpsecProtocolConfig;
class SftpProtocolConfig;
class Socks5ProxyProtocolConfig;
class TorProtocolConfig;

using ProtocolConfigVariant =
        std::variant<QSharedPointer<OpenVpnProtocolConfig>, QSharedPointer<WireGuardProtocolConfig>,
                     QSharedPointer<ShadowsocksProtocolConfig>, QSharedPointer<CloakProtocolConfig>, QSharedPointer<XrayProtocolConfig>,
                     QSharedPointer<AwgProtocolConfig>, QSharedPointer<IpsecProtocolConfig>, QSharedPointer<SftpProtocolConfig>,
                     QSharedPointer<Socks5ProxyProtocolConfig>, QSharedPointer<TorProtocolConfig>>;

using ScriptVars = QList<QPair<QString, QString>>;

class ProtocolConfig
{
public:
    ProtocolConfig(const QString &protocolName);
    virtual QJsonObject toJson() const;
    virtual ScriptVars getScriptVars() const;

    static ProtocolConfigVariant getProtocolConfigVariant(const QSharedPointer<ProtocolConfig> &protocolConfig);
    bool isServerSettingsEqual(const QSharedPointer<ProtocolConfig> &protocolConfig);
    static QString getNativeConfig(const QSharedPointer<ProtocolConfig> &protocolConfig);

    template<typename T> static constexpr bool isVpnProtocol()
    {
        using ProtocolType = std::decay_t<T>;
        return std::is_same_v<ProtocolType, OpenVpnProtocolConfig> || std::is_same_v<ProtocolType, WireGuardProtocolConfig>
                || std::is_same_v<ProtocolType, AwgProtocolConfig> || std::is_same_v<ProtocolType, XrayProtocolConfig>
                || std::is_same_v<ProtocolType, IpsecProtocolConfig> || std::is_same_v<ProtocolType, ShadowsocksProtocolConfig>
                || std::is_same_v<ProtocolType, CloakProtocolConfig>;
    }

    QString protocolName;
    amnezia::Proto protocolType;
};

#endif // PROTOCOLCONFIG_H
