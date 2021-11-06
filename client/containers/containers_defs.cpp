#include "containers_defs.h"

QDebug operator<<(QDebug debug, const amnezia::DockerContainer &c)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << ContainerProps::containerToString(c);

    return debug;
}

amnezia::DockerContainer ContainerProps::containerFromString(const QString &container){
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        DockerContainer c = static_cast<DockerContainer>(i);
        if (container == containerToString(c)) return c;
    }
    return DockerContainer::None;
}

QString ContainerProps::containerToString(amnezia::DockerContainer c){
    if (c == DockerContainer::None) return "none";
    if (c == DockerContainer::Cloak) return "amnezia-openvpn-cloak";

    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QString containerKey = metaEnum.valueToKey(static_cast<int>(c));

    return "amnezia-" + containerKey.toLower();
}

QVector<amnezia::Protocol> ContainerProps::protocolsForContainer(amnezia::DockerContainer container)
{
    switch (container) {
    case DockerContainer::None:
        return { };

    case DockerContainer::OpenVpn:
        return { Protocol::OpenVpn };

    case DockerContainer::ShadowSocks:
        return { Protocol::OpenVpn, Protocol::ShadowSocks };

    case DockerContainer::Cloak:
        return { Protocol::OpenVpn, Protocol::ShadowSocks, Protocol::Cloak };

    case DockerContainer::Ipsec:
        return { Protocol::Ikev2 /*, Protocol::L2tp */};

    case DockerContainer::Dns:
        return { };

    default:
        return { defaultProtocol(container) };
    }
}

QList<DockerContainer> ContainerProps::allContainers()
{
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QList<DockerContainer> all;
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        all.append(static_cast<DockerContainer>(i));
    }

    return all;
}

QMap<DockerContainer, QString> ContainerProps::containerHumanNames()
{
    return {
        {DockerContainer::None, "Unknown (Old version)"},
        {DockerContainer::OpenVpn, "OpenVPN"},
        {DockerContainer::ShadowSocks, "OpenVpn over ShadowSocks"},
        {DockerContainer::Cloak, "OpenVpn over Cloak"},
        {DockerContainer::WireGuard, "WireGuard"},
        {DockerContainer::Ipsec, QObject::tr("IPsec container")},

        {DockerContainer::TorWebSite, QObject::tr("Web site in TOR network")},
        {DockerContainer::Dns, QObject::tr("DNS Service")},
        {DockerContainer::FileShare, QObject::tr("SMB file sharing service")},
        {DockerContainer::Sftp, QObject::tr("Sftp file sharing service")}
    };
}

QMap<DockerContainer, QString> ContainerProps::containerDescriptions()
{
    return {
        {DockerContainer::OpenVpn, QObject::tr("OpenVPN container")},
        {DockerContainer::ShadowSocks, QObject::tr("Container with OpenVpn and ShadowSocks")},
        {DockerContainer::Cloak, QObject::tr("Container with OpenVpn and ShadowSocks protocols "
                                                        "configured with traffic masking by Cloak plugin")},
        {DockerContainer::WireGuard, QObject::tr("WireGuard container")},
        {DockerContainer::Ipsec, QObject::tr("IPsec container")},

        {DockerContainer::TorWebSite, QObject::tr("Web site in TOR network")},
        {DockerContainer::Dns, QObject::tr("DNS Service")},
        {DockerContainer::FileShare, QObject::tr("SMB file sharing service - is Window file sharing protocol")},
        {DockerContainer::Sftp, QObject::tr("Sftp file sharing service - is secure FTP service")}
    };
}

amnezia::ServiceType ContainerProps::containerService(DockerContainer c)
{
    switch (c) {
    case DockerContainer::None :         return ServiceType::None;
    case DockerContainer::OpenVpn :      return ServiceType::Vpn;
    case DockerContainer::Cloak :        return ServiceType::Vpn;
    case DockerContainer::ShadowSocks :  return ServiceType::Vpn;
    case DockerContainer::WireGuard :    return ServiceType::Vpn;
    case DockerContainer::Ipsec :        return ServiceType::Vpn;
    case DockerContainer::TorWebSite :   return ServiceType::Other;
    case DockerContainer::Dns :          return ServiceType::Other;
    case DockerContainer::FileShare :    return ServiceType::Other;
    case DockerContainer::Sftp :         return ServiceType::Other;
    default:                             return ServiceType::Other;
    }
}

Protocol ContainerProps::defaultProtocol(DockerContainer c)
{
    switch (c) {
    case DockerContainer::None :         return Protocol::Any;
    case DockerContainer::OpenVpn :      return Protocol::OpenVpn;
    case DockerContainer::Cloak :        return Protocol::Cloak;
    case DockerContainer::ShadowSocks :  return Protocol::ShadowSocks;
    case DockerContainer::WireGuard :    return Protocol::WireGuard;
    case DockerContainer::Ipsec :        return Protocol::Ikev2;

    case DockerContainer::TorWebSite :   return Protocol::TorWebSite;
    case DockerContainer::Dns :          return Protocol::Dns;
    case DockerContainer::FileShare :    return Protocol::FileShare;
    case DockerContainer::Sftp :         return Protocol::Sftp;
    default:                             return Protocol::Any;
    }
}

