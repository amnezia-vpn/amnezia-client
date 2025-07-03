#ifndef XRAYPROTOCOLCONFIG_H
#define XRAYPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

#include "protocolConfig.h"

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
    };
}

class XrayProtocolConfig : public ProtocolConfig
{
public:
    XrayProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);

    QJsonObject toJson() const override;

    bool hasEqualServerSettings(const XrayProtocolConfig &other) const;
    void clearClientSettings();

    xray::ServerProtocolConfig serverProtocolConfig;
    xray::ClientProtocolConfig clientProtocolConfig;
};

#endif // XRAYPROTOCOLCONFIG_H 
