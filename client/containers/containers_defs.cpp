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

QString ContainerProps::containerTypeToString(amnezia::DockerContainer c){
    if (c == DockerContainer::None) return "none";
    if (c == DockerContainer::Ipsec) return "ikev2";

    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QString containerKey = metaEnum.valueToKey(static_cast<int>(c));

    return containerKey.toLower();
}

QVector<amnezia::Proto> ContainerProps::protocolsForContainer(amnezia::DockerContainer container)
{
    switch (container) {
    case DockerContainer::None:
        return { };

    case DockerContainer::OpenVpn:
        return { Proto::OpenVpn };

    case DockerContainer::ShadowSocks:
        return { Proto::OpenVpn, Proto::ShadowSocks };

    case DockerContainer::Cloak:
        return { Proto::OpenVpn, Proto::ShadowSocks, Proto::Cloak };

    case DockerContainer::Ipsec:
        return { Proto::Ikev2 /*, Protocol::L2tp */};

    case DockerContainer::Dns:
        return { };

    case DockerContainer::Sftp:
        return { Proto::Sftp};

    case DockerContainer::Nextcloud:
        return { Proto::Nextcloud };

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
        {DockerContainer::None, "Not installed"},
        {DockerContainer::OpenVpn, "OpenVPN"},
        {DockerContainer::ShadowSocks, "OpenVpn over ShadowSocks"},
        {DockerContainer::Cloak, "OpenVpn over Cloak"},
        {DockerContainer::WireGuard, "WireGuard"},
        {DockerContainer::Ipsec, QObject::tr("IPsec")},

        {DockerContainer::TorWebSite, QObject::tr("Web site in Tor network")},
        {DockerContainer::Dns, QObject::tr("DNS Service")},
        //{DockerContainer::FileShare, QObject::tr("SMB file sharing service")},
        {DockerContainer::Sftp, QObject::tr("Sftp file sharing service")},
        {DockerContainer::Nextcloud, QObject::tr("Nextcloud")}
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

        {DockerContainer::TorWebSite, QObject::tr("Web site in Tor network")},
        {DockerContainer::Dns, QObject::tr("DNS Service")},
        //{DockerContainer::FileShare, QObject::tr("SMB file sharing service - is Window file sharing protocol")},
        {DockerContainer::Sftp, QObject::tr("Sftp file sharing service - is secure FTP service")},
        {DockerContainer::Nextcloud, QObject::tr("Nextcloud private cloud")},
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
    //case DockerContainer::FileShare :    return ServiceType::Other;
    case DockerContainer::Sftp :         return ServiceType::Other;
    case DockerContainer::Nextcloud :    return ServiceType::Other;
    default:                             return ServiceType::Other;
    }
}

Proto ContainerProps::defaultProtocol(DockerContainer c)
{
    switch (c) {
    case DockerContainer::None :         return Proto::Any;
    case DockerContainer::OpenVpn :      return Proto::OpenVpn;
    case DockerContainer::Cloak :        return Proto::Cloak;
    case DockerContainer::ShadowSocks :  return Proto::ShadowSocks;
    case DockerContainer::WireGuard :    return Proto::WireGuard;
    case DockerContainer::Ipsec :        return Proto::Ikev2;

    case DockerContainer::TorWebSite :   return Proto::TorWebSite;
    case DockerContainer::Dns :          return Proto::Dns;
    //case DockerContainer::FileShare :    return Protocol::FileShare;
    case DockerContainer::Sftp :         return Proto::Sftp;
    case DockerContainer::Nextcloud :    return Proto::Nextcloud;
    default:                             return Proto::Any;
    }
}

bool ContainerProps::isSupportedByCurrentPlatform(DockerContainer c)
{
#ifdef Q_OS_WINDOWS
    return true;

#elif defined (Q_OS_IOS)
    switch (c) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::OpenVpn: return true;
//    case DockerContainer::ShadowSocks: return true;
    default: return false;
    }
#elif defined (Q_OS_MAC)
    switch (c) {
    case DockerContainer::WireGuard: return false;
    case DockerContainer::Ipsec: return false;
    default: return true;
    }

#elif defined (Q_OS_ANDROID)
    switch (c) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::OpenVpn: return true;
    case DockerContainer::ShadowSocks: return true;
    default: return false;
    }

#elif defined (Q_OS_LINUX)
    return true;

#else
return false;
#endif
}

QStringList ContainerProps::fixedPortsForContainer(DockerContainer c)
{
    switch (c) {
    case DockerContainer::Ipsec :        return QStringList{"500", "4500"};
    default:                             return {};
    }
}
