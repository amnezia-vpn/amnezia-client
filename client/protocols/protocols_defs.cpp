#include "protocols_defs.h"

QDebug operator<<(QDebug debug, const amnezia::Protocol &p)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << protoToString(p);

    return debug;
}

QDebug operator<<(QDebug debug, const amnezia::DockerContainer &c)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << containerToString(c);

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

amnezia::DockerContainer amnezia::containerFromString(const QString &container){
    if (container == config_key::amnezia_openvpn) return DockerContainer::OpenVpn;
    if (container == config_key::amnezia_openvpn_cloak) return DockerContainer::OpenVpnOverCloak;
    if (container == config_key::amnezia_shadowsocks) return DockerContainer::OpenVpnOverShadowSocks;
    if (container == config_key::amnezia_wireguard) return DockerContainer::WireGuard;
    return DockerContainer::None;
}

QString amnezia::containerToString(amnezia::DockerContainer container){
    switch (container) {
    case(DockerContainer::OpenVpn): return config_key::amnezia_openvpn;
    case(DockerContainer::OpenVpnOverCloak): return config_key::amnezia_openvpn_cloak;
    case(DockerContainer::OpenVpnOverShadowSocks): return config_key::amnezia_shadowsocks;
    case(DockerContainer::WireGuard): return config_key::amnezia_wireguard;
    default: return "none";
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

QVector<amnezia::Protocol> amnezia::protocolsForContainer(amnezia::DockerContainer container)
{
    switch (container) {
    case DockerContainer::OpenVpn:
        return { Protocol::OpenVpn };

    case DockerContainer::OpenVpnOverShadowSocks:
        return { Protocol::OpenVpn, Protocol::ShadowSocks };

    case DockerContainer::OpenVpnOverCloak:
        return { Protocol::OpenVpn, Protocol::ShadowSocks, Protocol::Cloak };

    default:
        return {};
    }
}
