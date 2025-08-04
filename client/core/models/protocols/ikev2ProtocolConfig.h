#ifndef IKEV2PROTOCOLCONFIG_H
#define IKEV2PROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>
#include <QByteArray>

#include "protocolConfig.h"

namespace ikev2
{
    struct ServerProtocolConfig
    {
        bool isEmpty = true;
    };

    struct ClientProtocolConfig
    {
        bool isEmpty = true;
        
        QString hostName;
        QString userName;
        QString cert;
        QString password;
        
        QString nativeConfig;
    };
}

class Ikev2ProtocolConfig : public ProtocolConfig
{
public:
    Ikev2ProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);
    explicit Ikev2ProtocolConfig(const QString &protocolName);

    QJsonObject toJson() const override;

    bool hasEqualServerSettings(const Ikev2ProtocolConfig &other) const;
    void clearClientSettings();

    ikev2::ServerProtocolConfig serverProtocolConfig;
    ikev2::ClientProtocolConfig clientProtocolConfig;
};

#endif // IKEV2PROTOCOLCONFIG_H
