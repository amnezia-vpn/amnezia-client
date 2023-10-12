#include "containers_defs.h"

QDebug operator<<(QDebug debug, const amnezia::DockerContainer &c)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << ContainerProps::containerToString(c);

    return debug;
}

amnezia::DockerContainer ContainerProps::containerFromString(const QString &container)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        DockerContainer c = static_cast<DockerContainer>(i);
        if (container == containerToString(c))
            return c;
    }
    return DockerContainer::None;
}

QString ContainerProps::containerToString(amnezia::DockerContainer c)
{
    if (c == DockerContainer::None)
        return "none";
    if (c == DockerContainer::Cloak)
        return "amnezia-openvpn-cloak";

    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QString containerKey = metaEnum.valueToKey(static_cast<int>(c));

    return "amnezia-" + containerKey.toLower();
}

QString ContainerProps::containerTypeToString(amnezia::DockerContainer c)
{
    if (c == DockerContainer::None)
        return "none";
    if (c == DockerContainer::Ipsec)
        return "ikev2";

    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QString containerKey = metaEnum.valueToKey(static_cast<int>(c));

    return containerKey.toLower();
}

QVector<amnezia::Proto> ContainerProps::protocolsForContainer(amnezia::DockerContainer container)
{
    switch (container) {
    case DockerContainer::None: return {};

    case DockerContainer::OpenVpn: return { Proto::OpenVpn };

    case DockerContainer::ShadowSocks: return { Proto::OpenVpn, Proto::ShadowSocks };

    case DockerContainer::Cloak: return { Proto::OpenVpn, Proto::ShadowSocks, Proto::Cloak };

    case DockerContainer::Ipsec: return { Proto::Ikev2 /*, Protocol::L2tp */ };

    case DockerContainer::Dns: return {};

    case DockerContainer::Sftp: return { Proto::Sftp };

    default: return { defaultProtocol(container) };
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
    return { { DockerContainer::None, "Not installed" },
             { DockerContainer::OpenVpn, "OpenVPN" },
             { DockerContainer::ShadowSocks, "ShadowSocks" },
             { DockerContainer::Cloak, "OpenVPN over Cloak" },
             { DockerContainer::WireGuard, "WireGuard" },
             { DockerContainer::Awg, "AmneziaWG" },
             { DockerContainer::Ipsec, QObject::tr("IPsec") },

             { DockerContainer::TorWebSite, QObject::tr("Website in Tor network") },
             { DockerContainer::Dns, QObject::tr("Amnezia DNS") },
             //{DockerContainer::FileShare, QObject::tr("SMB file sharing service")},
             { DockerContainer::Sftp, QObject::tr("Sftp file sharing service") } };
}

QMap<DockerContainer, QString> ContainerProps::containerDescriptions()
{
    return { { DockerContainer::OpenVpn,
               QObject::tr("OpenVPN is the most popular VPN protocol, with flexible configuration options. It uses its "
                           "own security protocol with SSL/TLS for key exchange.") },
             { DockerContainer::ShadowSocks,
               QObject::tr("ShadowSocks - masks VPN traffic, making it similar to normal web traffic, but is "
                           "recognised by analysis systems in some highly censored regions.") },
             { DockerContainer::Cloak,
               QObject::tr("OpenVPN over Cloak - OpenVPN with VPN masquerading as web traffic and protection against "
                           "active-probbing detection. Ideal for bypassing blocking in regions with the highest levels "
                           "of censorship.") },
             { DockerContainer::WireGuard,
               QObject::tr("WireGuard - New popular VPN protocol with high performance, high speed and low power "
                           "consumption. Recommended for regions with low levels of censorship.") },
            { DockerContainer::Awg,
               QObject::tr("AmneziaWG - Special protocol from Amnezia, based on WireGuard. It's fast like WireGuard, but very resistant to blockages. "
                         "Recommended for regions with high levels of censorship.") },
             { DockerContainer::Ipsec,
               QObject::tr("IKEv2 -  Modern stable protocol, a bit faster than others, restores connection after "
                           "signal loss. It has native support on the latest versions of Android and iOS.") },

             { DockerContainer::TorWebSite, QObject::tr("Deploy a WordPress site on the Tor network in two clicks.") },
             { DockerContainer::Dns,
               QObject::tr("Replace the current DNS server with your own. This will increase your privacy level.") },
             //{DockerContainer::FileShare, QObject::tr("SMB file sharing service - is Window file sharing protocol")},
             { DockerContainer::Sftp,
               QObject::tr("Creates a file vault on your server to securely store and transfer files.") } };
}

QMap<DockerContainer, QString> ContainerProps::containerDetailedDescriptions()
{
    return { { DockerContainer::OpenVpn, QObject::tr("OpenVPN container") },
             { DockerContainer::ShadowSocks, QObject::tr("Container with OpenVpn and ShadowSocks") },
             { DockerContainer::Cloak,
               QObject::tr("Container with OpenVpn and ShadowSocks protocols "
                           "configured with traffic masking by Cloak plugin") },
             { DockerContainer::WireGuard, QObject::tr("WireGuard container") },
             { DockerContainer::WireGuard, QObject::tr("AmneziaWG container") },
             { DockerContainer::Ipsec, QObject::tr("IPsec container") },

             { DockerContainer::TorWebSite, QObject::tr("Website in Tor network") },
             { DockerContainer::Dns, QObject::tr("DNS Service") },
             //{DockerContainer::FileShare, QObject::tr("SMB file sharing service - is Window file sharing protocol")},
             { DockerContainer::Sftp, QObject::tr("Sftp file sharing service - is secure FTP service") } };
}

amnezia::ServiceType ContainerProps::containerService(DockerContainer c)
{
    switch (c) {
    case DockerContainer::None: return ServiceType::None;
    case DockerContainer::OpenVpn: return ServiceType::Vpn;
    case DockerContainer::Cloak: return ServiceType::Vpn;
    case DockerContainer::ShadowSocks: return ServiceType::Vpn;
    case DockerContainer::WireGuard: return ServiceType::Vpn;
    case DockerContainer::Awg: return ServiceType::Vpn;
    case DockerContainer::Ipsec: return ServiceType::Vpn;
    case DockerContainer::TorWebSite: return ServiceType::Other;
    case DockerContainer::Dns: return ServiceType::Other;
    // case DockerContainer::FileShare :    return ServiceType::Other;
    case DockerContainer::Sftp: return ServiceType::Other;
    default: return ServiceType::Other;
    }
}

Proto ContainerProps::defaultProtocol(DockerContainer c)
{
    switch (c) {
    case DockerContainer::None: return Proto::Any;
    case DockerContainer::OpenVpn: return Proto::OpenVpn;
    case DockerContainer::Cloak: return Proto::Cloak;
    case DockerContainer::ShadowSocks: return Proto::ShadowSocks;
    case DockerContainer::WireGuard: return Proto::WireGuard;
    case DockerContainer::Awg: return Proto::Awg;
    case DockerContainer::Ipsec: return Proto::Ikev2;

    case DockerContainer::TorWebSite: return Proto::TorWebSite;
    case DockerContainer::Dns: return Proto::Dns;
    // case DockerContainer::FileShare :    return Protocol::FileShare;
    case DockerContainer::Sftp: return Proto::Sftp;
    default: return Proto::Any;
    }
}

bool ContainerProps::isSupportedByCurrentPlatform(DockerContainer c)
{
#ifdef Q_OS_WINDOWS
    return true;

#elif defined(Q_OS_IOS)
    switch (c) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::OpenVpn: return true;
    case DockerContainer::Awg: return true;
    case DockerContainer::Cloak:
        return true;
        //    case DockerContainer::ShadowSocks: return true;
    default: return false;
    }
#elif defined(Q_OS_MAC)
    switch (c) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::Ipsec: return false;
    default: return true;
    }

#elif defined(Q_OS_ANDROID)
    switch (c) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::OpenVpn: return true;
    case DockerContainer::ShadowSocks: return true;
    case DockerContainer::Awg: return true;
    case DockerContainer::Cloak: return true;
    default: return false;
    }

#elif defined(Q_OS_LINUX)
    switch (c) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::Ipsec: return false;
    default: return true;
    }

#else
    return false;
#endif
}

QStringList ContainerProps::fixedPortsForContainer(DockerContainer c)
{
    switch (c) {
    case DockerContainer::Ipsec: return QStringList { "500", "4500" };
    default: return {};
    }
}

bool ContainerProps::isEasySetupContainer(DockerContainer container)
{
    switch (container) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::Cloak: return true;
    case DockerContainer::OpenVpn: return true;
    default: return false;
    }
}

QString ContainerProps::easySetupHeader(DockerContainer container)
{
    switch (container) {
    case DockerContainer::WireGuard: return tr("Low");
    case DockerContainer::Cloak: return tr("High");
    case DockerContainer::OpenVpn: return tr("Medium");
    default: return "";
    }
}

QString ContainerProps::easySetupDescription(DockerContainer container)
{
    switch (container) {
    case DockerContainer::WireGuard: return tr("I just want to increase the level of privacy");
    case DockerContainer::Cloak: return tr("Many foreign websites and VPN providers are blocked");
    case DockerContainer::OpenVpn: return tr("Some foreign sites are blocked, but VPN providers are not blocked");
    default: return "";
    }
}

int ContainerProps::easySetupOrder(DockerContainer container)
{
    switch (container) {
    case DockerContainer::WireGuard: return 1;
    case DockerContainer::Cloak: return 3;
    case DockerContainer::OpenVpn: return 2;
    default: return 0;
    }
}

bool ContainerProps::isShareable(DockerContainer container)
{
    switch (container) {
    case DockerContainer::TorWebSite: return false;
    case DockerContainer::Dns: return false;
    case DockerContainer::Sftp: return false;
    default: return true;
    }
}
