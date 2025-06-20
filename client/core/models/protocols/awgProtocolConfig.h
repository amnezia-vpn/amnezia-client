#ifndef AWGPROTOCOLCONFIG_H
#define AWGPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QStringList>

#include "protocolConfig.h"
#include "wireguardProtocolConfig.h"

namespace awg
{
    struct AwgData
    {
        QString junkPacketCount;
        QString junkPacketMinSize;
        QString junkPacketMaxSize;

        QString initPacketJunkSize;
        QString responsePacketJunkSize;

        QString initPacketMagicHeader;
        QString responsePacketMagicHeader;
        QString underloadPacketMagicHeader;
        QString transportPacketMagicHeader;
    };

    struct ServerProtocolConfig
    {
        QString port;
        QString transportProto;

        QString subnetAddress;

        AwgData awgData;
    };

    struct ClientProtocolConfig
    {
        bool isEmpty = true;

        QString clientId;

        wireguard::WireGuardData wireGuardData;

        AwgData awgData;

        QString hostname;
        int port;

        QString nativeConfig;
    };
}

class AwgProtocolConfig : public ProtocolConfig
{
public:
    AwgProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);

    QJsonObject toJson() const override;

    awg::ServerProtocolConfig serverProtocolConfig;
    awg::ClientProtocolConfig clientProtocolConfig;
};

#endif // AWGPROTOCOLCONFIG_H
