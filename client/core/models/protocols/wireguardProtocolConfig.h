#ifndef WIREGUARDPROTOCOLCONFIG_H
#define WIREGUARDPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QStringList>

#include "protocolConfig.h"

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

        QString clientId;

        WireGuardData wireGuardData;

        QString hostname;
        int port;

        QString nativeConfig;
    };
}

class WireGuardProtocolConfig : public ProtocolConfig
{
public:
    WireGuardProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);

    QJsonObject toJson() const override;

    wireguard::ServerProtocolConfig serverProtocolConfig;
    wireguard::ClientProtocolConfig clientProtocolConfig;
};

#endif // WIREGUARDPROTOCOLCONFIG_H
