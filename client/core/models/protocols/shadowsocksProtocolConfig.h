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
        
        QString server;
        QString serverPort;
        QString localPort;
        QString password;
        int timeout = 60;
        QString method;
        
        QString nativeConfig;
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
