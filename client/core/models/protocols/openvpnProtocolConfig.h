#ifndef OPENVPNPROTOCOLCONFIG_H
#define OPENVPNPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

#include "protocolConfig.h"

namespace openvpn
{
    struct ServerProtocolConfig
    {
        QString subnetAddress;
        QString transportProto;
        QString port;
        bool ncpDisable;
        QString hash;
        QString cipher;
        bool tlsAuth;
        bool blockOutsideDns;
        QString additionalClientConfig;
        QString additionalServerConfig;
    };

    struct ClientProtocolConfig
    {
        bool isEmpty = true;

        QString clientId;

        QString nativeConfig;
    };
}

class OpenVpnProtocolConfig : public ProtocolConfig
{
public:
    OpenVpnProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);

    QJsonObject toJson() const override;

    bool hasEqualServerSettings(const OpenVpnProtocolConfig &other) const;
    void clearClientSettings();

    openvpn::ServerProtocolConfig serverProtocolConfig;
    openvpn::ClientProtocolConfig clientProtocolConfig;
};

#endif // OPENVPNPROTOCOLCONFIG_H 
