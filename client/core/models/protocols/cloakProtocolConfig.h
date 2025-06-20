#ifndef CLOAKPROTOCOLCONFIG_H
#define CLOAKPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

#include "protocolConfig.h"

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
    };
}

class CloakProtocolConfig : public ProtocolConfig
{
public:
    CloakProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);

    QJsonObject toJson() const override;

    cloak::ServerProtocolConfig serverProtocolConfig;
    cloak::ClientProtocolConfig clientProtocolConfig;
};

#endif // CLOAKPROTOCOLCONFIG_H 
