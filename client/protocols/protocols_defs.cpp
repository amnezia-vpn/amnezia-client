#include "protocols_defs.h"

QDebug operator<<(QDebug debug, const amnezia::Protocol &p)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << protoToString(p);

    return debug;
}

amnezia::Protocol amnezia::protoFromString(QString proto){
    if (proto == config_key::openvpn) return Protocol::OpenVpn;
    if (proto == config_key::cloak) return Protocol::Cloak;
    if (proto == config_key::shadowsocks) return Protocol::ShadowSocks;
    if (proto == config_key::wireguard) return Protocol::WireGuard;
    return Protocol::Any;
}

QString amnezia::protoToString(amnezia::Protocol proto){
    switch (proto) {
    case(Protocol::OpenVpn): return config_key::openvpn;
    case(Protocol::Cloak): return config_key::cloak;
    case(Protocol::ShadowSocks): return config_key::shadowsocks;
    case(Protocol::WireGuard): return config_key::wireguard;
    default: return "";
    }
}

QVector<amnezia::Protocol> amnezia::allProtocols()
{
    return QVector<amnezia::Protocol> {
        Protocol::OpenVpn,
        Protocol::ShadowSocks,
        Protocol::Cloak,
        Protocol::WireGuard
    };
}


QMap<amnezia::Protocol, QString> amnezia::protocolHumanNames()
{
    return {
        {Protocol::OpenVpn, "OpenVPN"},
        {Protocol::ShadowSocks, "ShadowSocks"},
        {Protocol::Cloak, "Cloak"},
        {Protocol::WireGuard, "WireGuard"}
    };
}

QMap<amnezia::Protocol, QString> amnezia::protocolDescriptions()
{
    return {};
}

bool amnezia::isProtocolVpnType(Protocol p)
{
    switch (p) {
    case Protocol::Any :          return false;
    case Protocol::OpenVpn :      return true;
    case Protocol::Cloak :        return true;
    case Protocol::ShadowSocks :  return true;
    case Protocol::WireGuard :    return true;
    default:                      return false;
    }
}
