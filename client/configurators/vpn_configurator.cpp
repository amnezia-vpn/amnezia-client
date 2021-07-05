#include "vpn_configurator.h"
#include "openvpn_configurator.h"
#include "cloak_configurator.h"
#include "shadowsocks_configurator.h"
//#include "wireguard_configurator.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

#include "protocols/protocols_defs.h"


QString VpnConfigurator::genVpnProtocolConfig(const ServerCredentials &credentials,
    DockerContainer container, const QJsonObject &containerConfig, Protocol proto, ErrorCode *errorCode)
{
    switch (proto) {
    case Protocol::OpenVpn:
        return OpenVpnConfigurator::genOpenVpnConfig(credentials, container, containerConfig, errorCode);

    case Protocol::ShadowSocks:
        return ShadowSocksConfigurator::genShadowSocksConfig(credentials, container, containerConfig, errorCode);

    case Protocol::Cloak:
        return CloakConfigurator::genCloakConfig(credentials, container, containerConfig, errorCode);

//    case Protocol::WireGuard:
//        return WireGuardConfigurator::genWireGuardConfig(credentials, container, containerConfig, errorCode);

    default:
        return "";
    }
}
