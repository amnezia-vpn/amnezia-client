#ifndef TORPROTOCOLCONFIG_H
#define TORPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

#include "protocolConfig.h"
#include "protocols/protocols_defs.h"

namespace tor
{
    struct ServerProtocolConfig
    {
        QString site;
    };

    struct ClientProtocolConfig
    {
        bool isEmpty = true;
    };
}

class TorProtocolConfig : public ProtocolConfig
{
public:
    TorProtocolConfig(const QString &protocolName);
    TorProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);
    TorProtocolConfig(const TorProtocolConfig &other);

    QJsonObject toJson() const override;
    ScriptVars getScriptVars() const override;

    bool hasEqualServerSettings(const TorProtocolConfig &other) const;
    void clearClientSettings();

    tor::ServerProtocolConfig serverProtocolConfig;
    tor::ClientProtocolConfig clientProtocolConfig;
};

#endif // TORPROTOCOLCONFIG_H
