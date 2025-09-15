#ifndef CLOAKPROTOCOLCONFIG_H
#define CLOAKPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

#include "protocolConfig.h"
#include "protocols/protocols_defs.h"

namespace cloak
{
    struct ServerProtocolConfig
    {
        QString port;
        QString cipher;
        QString site;
    };

    struct ClientProtocolConfig
    {
        bool isEmpty = true;

        QString nativeConfig;
    };
}

class CloakProtocolConfig : public ProtocolConfig
{
public:
    CloakProtocolConfig(const QString &protocolName, int port);
    CloakProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);
    CloakProtocolConfig(const CloakProtocolConfig &other);

    QJsonObject toJson() const override;
    ScriptVars getScriptVars() const override;

    bool hasEqualServerSettings(const CloakProtocolConfig &other) const;
    void clearClientSettings();

    cloak::ServerProtocolConfig serverProtocolConfig;
    cloak::ClientProtocolConfig clientProtocolConfig;
};

#endif // CLOAKPROTOCOLCONFIG_H 
