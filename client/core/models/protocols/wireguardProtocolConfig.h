#ifndef WIREGUARDPROTOCOLCONFIG_H
#define WIREGUARDPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QStringList>

#include "protocolConfig.h"
#include "protocols/protocols_defs.h"

namespace wireguard
{
    struct WireGuardData
    {
        QStringList allowedIps;

        QString clientIp;
        QString clientPrivateKey;
        QString clientPublicKey;
        QString mtu;
        QString persistentKeepAlive;
        QString pskKey;
        QString serverPubKey;
    };

    struct ServerProtocolConfig
    {
        QString port;
        QString transportProto;

        QString subnetAddress;
    };

    struct ClientProtocolConfig
    {
        bool isEmpty = true;

        WireGuardData wireGuardData;

        QString hostname;
        int port;

        QString clientId;
        QString nativeConfig;
    };
}

class WireGuardProtocolConfig : public ProtocolConfig
{
public:
    WireGuardProtocolConfig(const QString &protocolName, int port, amnezia::TransportProto transportProto);
    WireGuardProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);
    WireGuardProtocolConfig(const WireGuardProtocolConfig &other);

    QJsonObject toJson() const override;
    ScriptVars getScriptVars() const override;

    bool hasEqualServerSettings(const WireGuardProtocolConfig &other) const;
    void clearClientSettings();

    wireguard::ServerProtocolConfig serverProtocolConfig;
    wireguard::ClientProtocolConfig clientProtocolConfig;
};

#endif // WIREGUARDPROTOCOLCONFIG_H
