#ifndef OPENVPNPROTOCOLCONFIG_H
#define OPENVPNPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

#include "protocolConfig.h"
#include "protocols/protocols_defs.h"

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
    OpenVpnProtocolConfig(const QString &protocolName, int port, amnezia::TransportProto transportProto);
    OpenVpnProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);
    OpenVpnProtocolConfig(const OpenVpnProtocolConfig &other);

    QJsonObject toJson() const override;
    ScriptVars getScriptVars() const override;

    bool hasEqualServerSettings(const OpenVpnProtocolConfig &other) const;
    void clearClientSettings();

    openvpn::ServerProtocolConfig serverProtocolConfig;
    openvpn::ClientProtocolConfig clientProtocolConfig;
};

#endif // OPENVPNPROTOCOLCONFIG_H 
