#include "containers_defs.h"

QDebug operator<<(QDebug debug, const amnezia::DockerContainer &c)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << containerToString(c);

    return debug;
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

QVector<amnezia::DockerContainer> amnezia::allContainers()
{
    return QVector<amnezia::DockerContainer> {
        DockerContainer::OpenVpn,
        DockerContainer::OpenVpnOverShadowSocks,
        DockerContainer::OpenVpnOverCloak,
        DockerContainer::WireGuard
    };
}

QMap<DockerContainer, QString> amnezia::containerHumanNames()
{
    return {
        {DockerContainer::OpenVpn, "OpenVPN"},
        {DockerContainer::OpenVpnOverShadowSocks, "OpenVpn over ShadowSocks"},
        {DockerContainer::OpenVpnOverCloak, "OpenVpn over Cloak"},
        {DockerContainer::WireGuard, "WireGuard"}
    };
}

QMap<DockerContainer, QString> amnezia::containerDescriptions()
{
    return {
        {DockerContainer::OpenVpn, QObject::tr("OpenVPN container")},
        {DockerContainer::OpenVpnOverShadowSocks, QObject::tr("Container with OpenVpn and ShadowSocks")},
        {DockerContainer::OpenVpnOverCloak, QObject::tr("Container with OpenVpn and ShadowSocks protocols "
                                                        "configured with traffic masking by Cloak plugin")},
        {DockerContainer::WireGuard, QObject::tr("WireGuard container")}
    };
}

bool amnezia::isContainerVpnType(DockerContainer c)
{
    switch (c) {
    case DockerContainer::None :                    return false;
    case DockerContainer::OpenVpn :                 return true;
    case DockerContainer::OpenVpnOverCloak :        return true;
    case DockerContainer::OpenVpnOverShadowSocks :  return true;
    case DockerContainer::WireGuard :               return true;
    default: return false;
    }
}
