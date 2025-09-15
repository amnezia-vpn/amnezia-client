#ifndef SOCKS5PROXYPROTOCOLCONFIG_H
#define SOCKS5PROXYPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

#include "protocolConfig.h"
#include "protocols/protocols_defs.h"

namespace socks5Proxy
{
    struct ServerProtocolConfig
    {
        QString port;
        QString userName;
        QString password;
    };

    struct ClientProtocolConfig
    {
        bool isEmpty = true;
    };
}

class Socks5ProxyProtocolConfig : public ProtocolConfig
{
public:
    Socks5ProxyProtocolConfig(const QString &protocolName, int port);
    Socks5ProxyProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);
    Socks5ProxyProtocolConfig(const Socks5ProxyProtocolConfig &other);

    QJsonObject toJson() const override;
    ScriptVars getScriptVars() const override;

    bool hasEqualServerSettings(const Socks5ProxyProtocolConfig &other) const;
    void clearClientSettings();

    socks5Proxy::ServerProtocolConfig serverProtocolConfig;
    socks5Proxy::ClientProtocolConfig clientProtocolConfig;
};

#endif // SOCKS5PROXYPROTOCOLCONFIG_H
