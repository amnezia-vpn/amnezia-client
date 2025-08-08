#ifndef TORWEBSITEPROTOCOLCONFIG_H
#define TORWEBSITEPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

#include "protocolConfig.h"

namespace torWebsite
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

class TorWebsiteProtocolConfig : public ProtocolConfig
{
public:
    TorWebsiteProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);

    QJsonObject toJson() const override;

    bool hasEqualServerSettings(const TorWebsiteProtocolConfig &other) const;
    void clearClientSettings();

    torWebsite::ServerProtocolConfig serverProtocolConfig;
    torWebsite::ClientProtocolConfig clientProtocolConfig;
};

#endif // TORWEBSITEPROTOCOLCONFIG_H
