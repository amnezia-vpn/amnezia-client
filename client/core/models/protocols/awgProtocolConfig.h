#ifndef AWGPROTOCOLCONFIG_H
#define AWGPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QStringList>

#include "protocolConfig.h"
#include "wireguardProtocolConfig.h"
#include "protocols/protocols_defs.h"

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

        QString specialJunk1;
        QString specialJunk2;
        QString specialJunk3;
        QString specialJunk4;
        QString specialJunk5;

        QString controlledJunk1;
        QString controlledJunk2;
        QString controlledJunk3;

        QString specialHandshakeTimeout;
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
    AwgProtocolConfig(const QString &protocolName, int port, amnezia::TransportProto transportProto);
    AwgProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);
    AwgProtocolConfig(const AwgProtocolConfig &other);

    QJsonObject toJson() const override;
    ScriptVars getScriptVars() const override;

    bool hasEqualServerSettings(const AwgProtocolConfig &other) const;
    bool hasEqualClientSettings(const AwgProtocolConfig &other) const;
    void clearClientSettings();

    awg::ServerProtocolConfig serverProtocolConfig;
    awg::ClientProtocolConfig clientProtocolConfig;
};

#endif // AWGPROTOCOLCONFIG_H
