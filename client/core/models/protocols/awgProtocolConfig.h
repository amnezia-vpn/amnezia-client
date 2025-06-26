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

    const int messageInitiationSize = 148;
    const int messageResponseSize = 92;
}

class AwgProtocolConfig : public ProtocolConfig
{
public:
    AwgProtocolConfig(const QString &protocolName);
    AwgProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);
    AwgProtocolConfig(const AwgProtocolConfig &other);

    QJsonObject toJson() const override;

    bool hasEqualServerSettings(const AwgProtocolConfig &other) const;
    bool hasEqualClientSettings(const AwgProtocolConfig &other) const;
    void clearClientSettings();

    awg::ServerProtocolConfig serverProtocolConfig;
    awg::ClientProtocolConfig clientProtocolConfig;
};

#endif // AWGPROTOCOLCONFIG_H
