#ifndef PROTOCOLCONFIG_H
#define PROTOCOLCONFIG_H

#include <QJsonObject>
#include <QSharedPointer>
#include <variant>

class OpenVpnProtocolConfig;
class WireGuardProtocolConfig;
class ShadowsocksProtocolConfig;
class CloakProtocolConfig;
class XrayProtocolConfig;
class AwgProtocolConfig;
class SftpProtocolConfig;
class Socks5ProtocolConfig;
class Ikev2ProtocolConfig;
class TorWebsiteProtocolConfig;

using ProtocolConfigVariant =
        std::variant<QSharedPointer<OpenVpnProtocolConfig>, QSharedPointer<WireGuardProtocolConfig>, QSharedPointer<ShadowsocksProtocolConfig>,
                     QSharedPointer<CloakProtocolConfig>, QSharedPointer<XrayProtocolConfig>, QSharedPointer<AwgProtocolConfig>,
                     QSharedPointer<SftpProtocolConfig>, QSharedPointer<Socks5ProtocolConfig>, QSharedPointer<Ikev2ProtocolConfig>,
                     QSharedPointer<TorWebsiteProtocolConfig> >;

class ProtocolConfig
{
public:
    ProtocolConfig(const QString &protocolName);
    virtual QJsonObject toJson() const;

    static ProtocolConfigVariant getProtocolConfigVariant(const QSharedPointer<ProtocolConfig> &protocolConfig);
    bool isServerSettingsEqual(const QSharedPointer<ProtocolConfig> &protocolConfig);

    QString protocolName;
};

#endif // PROTOCOLCONFIG_H
