#ifndef XRAYPROTOCOLCONFIG_H
#define XRAYPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

#include "protocolConfig.h"
#include "protocols/protocols_defs.h"

namespace xray
{
    struct ServerProtocolConfig
    {
        QString site;
        QString port;
        QString transportProto;
    };

    struct ClientProtocolConfig
    {
        bool isEmpty = true;

        QString clientId;
        QString nativeConfig;
    };
}

class XrayProtocolConfig : public ProtocolConfig
{
public:
    XrayProtocolConfig(const QString &protocolName, int port, amnezia::TransportProto transportProto);
    XrayProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);
    XrayProtocolConfig(const XrayProtocolConfig &other);

    QJsonObject toJson() const override;
    ScriptVars getScriptVars() const override;

    bool hasEqualServerSettings(const XrayProtocolConfig &other) const;
    void clearClientSettings();

    xray::ServerProtocolConfig serverProtocolConfig;
    xray::ClientProtocolConfig clientProtocolConfig;
};

#endif // XRAYPROTOCOLCONFIG_H 
