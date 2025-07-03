#ifndef SHADOWSOCKSPROTOCOLCONFIG_H
#define SHADOWSOCKSPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

#include "protocolConfig.h"

namespace shadowsocks
{
    struct ServerProtocolConfig
    {
        QString port;
        QString cipher;
    };

    struct ClientProtocolConfig
    {
        bool isEmpty = true;
    };
}

class ShadowsocksProtocolConfig : public ProtocolConfig
{
public:
    ShadowsocksProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);

    QJsonObject toJson() const override;

    bool hasEqualServerSettings(const ShadowsocksProtocolConfig &other) const;
    void clearClientSettings();

    shadowsocks::ServerProtocolConfig serverProtocolConfig;
    shadowsocks::ClientProtocolConfig clientProtocolConfig;
};

#endif // SHADOWSOCKSPROTOCOLCONFIG_H 
