#ifndef SOCKS5PROTOCOLCONFIG_H
#define SOCKS5PROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

#include "protocolConfig.h"

namespace socks5
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

class Socks5ProtocolConfig : public ProtocolConfig
{
public:
    Socks5ProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);

    QJsonObject toJson() const override;

    bool hasEqualServerSettings(const Socks5ProtocolConfig &other) const;
    void clearClientSettings();

    socks5::ServerProtocolConfig serverProtocolConfig;
    socks5::ClientProtocolConfig clientProtocolConfig;
};

#endif // SOCKS5PROTOCOLCONFIG_H 
