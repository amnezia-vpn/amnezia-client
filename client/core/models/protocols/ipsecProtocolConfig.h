#ifndef IPSECPROTOCOLCONFIG_H
#define IPSECPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

#include "protocolConfig.h"

namespace ipsec
{
    struct ServerProtocolConfig
    {
        QString l2tpNet;
        QString l2tpPool;
        QString l2tpLocal;
        QString xauthNet;
        QString xauthPool;
        QString sha2TruncBug;
        QString androidMtuFix;
        QString disableIkev2;
        QString disableL2tp;
        QString disableXauth;
        QString c2cTraffic;
    };

    struct ClientProtocolConfig
    {
        bool isEmpty = true;

        QString nativeConfig;
    };
}

class IpsecProtocolConfig : public ProtocolConfig
{
public:
    IpsecProtocolConfig(const QString &protocolName);
    IpsecProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);
    IpsecProtocolConfig(const IpsecProtocolConfig &other);

    QJsonObject toJson() const override;
    ScriptVars getScriptVars() const override;

    bool hasEqualServerSettings(const IpsecProtocolConfig &other) const;
    void clearClientSettings();

    ipsec::ServerProtocolConfig serverProtocolConfig;
    ipsec::ClientProtocolConfig clientProtocolConfig;
};

#endif // IPSECPROTOCOLCONFIG_H
