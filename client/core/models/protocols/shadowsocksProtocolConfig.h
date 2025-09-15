#ifndef SHADOWSOCKSPROTOCOLCONFIG_H
#define SHADOWSOCKSPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

#include "protocolConfig.h"
#include "protocols/protocols_defs.h"

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

        QString nativeConfig;
    };
}

class ShadowsocksProtocolConfig : public ProtocolConfig
{
public:
    ShadowsocksProtocolConfig(const QString &protocolName, int port);
    ShadowsocksProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);
    ShadowsocksProtocolConfig(const ShadowsocksProtocolConfig &other);

    QJsonObject toJson() const override;
    ScriptVars getScriptVars() const override;

    bool hasEqualServerSettings(const ShadowsocksProtocolConfig &other) const;
    void clearClientSettings();

    shadowsocks::ServerProtocolConfig serverProtocolConfig;
    shadowsocks::ClientProtocolConfig clientProtocolConfig;
};

#endif // SHADOWSOCKSPROTOCOLCONFIG_H 
