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

    case DockerContainer::Cloak: return { Proto::OpenVpn, /*Proto::ShadowSocks,*/ Proto::Cloak };

    case DockerContainer::Ipsec: return { Proto::Ikev2 /*, Protocol::L2tp */ };

    case DockerContainer::Dns: return { Proto::Dns };

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
               QObject::tr("AmneziaWG - Special protocol from Amnezia, based on WireGuard. It's fast like WireGuard, "
                           "but very resistant to blockages. "
                           "Recommended for regions with high levels of censorship.") },
             { DockerContainer::Ipsec,
               QObject::tr("IKEv2 -  Modern stable protocol, a bit faster than others, restores connection after "
                           "signal loss. It has native support on the latest versions of Android and iOS.") },

             { DockerContainer::TorWebSite, QObject::tr("Deploy a WordPress site on the Tor network in two clicks.") },
             { DockerContainer::Dns,
               QObject::tr("Replace the current DNS server with your own. This will increase your privacy level.") },
             { DockerContainer::Sftp,
               QObject::tr("Creates a file vault on your server to securely store and transfer files.") } };
}

QMap<DockerContainer, QString> ContainerProps::containerDetailedDescriptions()
{
    return {
        { DockerContainer::OpenVpn,
          QObject::tr(
                      "OpenVPN stands as one of the most popular and time-tested VPN protocols available.\n"
                      "It employs its unique security protocol, "
                      "leveraging the strength of SSL/TLS for encryption and key exchange. "
                      "Furthermore, OpenVPN's support for a multitude of authentication methods makes it versatile and adaptable, "
                      "catering to a wide range of devices and operating systems. "
                      "Due to its open-source nature, OpenVPN benefits from extensive scrutiny by the global community, "
                      "which continually reinforces its security. "
                      "With a strong balance of performance, security, and compatibility, "
                      "OpenVPN remains a top choice for privacy-conscious individuals and businesses alike.\n\n"
                      "* Available in the AmneziaVPN across all platforms\n"
                      "* Normal power consumption on mobile devices\n"
                      "* Flexible customisation to suit user needs to work with different operating systems and devices\n"
                      "* Recognised by DPI analysis systems and therefore susceptible to blocking\n"
                      "* Can operate over both TCP and UDP network protocols.") },
        { DockerContainer::ShadowSocks,
          QObject::tr("Shadowsocks, inspired by the SOCKS5 protocol, safeguards the connection using the AEAD cipher. "
                      "Although Shadowsocks is designed to be discreet and challenging to identify, it isn't identical to a standard HTTPS connection."
                      "However, certain traffic analysis systems might still detect a Shadowsocks connection. "
                      "Due to limited support in Amnezia, it's recommended to use AmneziaWG protocol.\n\n"
                      "* Available in the AmneziaVPN only on desktop platforms\n"
                      "* Normal power consumption on mobile devices\n\n"
                      "* Configurable encryption protocol\n"
                      "* Detectable by some DPI systems\n"
                      "* Works over TCP network protocol.") },
        { DockerContainer::Cloak,
          QObject::tr("This is a combination of the OpenVPN protocol and the Cloak plugin designed specifically for "
                      "blocking protection.\n\n"
                      "OpenVPN provides a secure VPN connection by encrypting all Internet traffic between the client "
                      "and the server.\n\n"
                      "Cloak protects OpenVPN from detection and blocking. \n\n"
                      "Cloak can modify packet metadata so that it completely masks VPN traffic as normal web traffic, "
                      "and also protects the VPN from detection by Active Probing. This makes it very resistant to "
                      "being detected\n\n"
                      "Immediately after receiving the first data packet, Cloak authenticates the incoming connection. "
                      "If authentication fails, the plugin masks the server as a fake website and your VPN becomes "
                      "invisible to analysis systems.\n\n"
                      "If there is a extreme level of Internet censorship in your region, we advise you to use only "
                      "OpenVPN over Cloak from the first connection\n\n"
                      "* Available in the AmneziaVPN across all platforms\n"
                      "* High power consumption on mobile devices\n"
                      "* Flexible settings\n"
                      "* Not recognised by DPI analysis systems\n"
                      "* Works over TCP network protocol, 443 port.\n") },
        { DockerContainer::WireGuard,
          QObject::tr("A relatively new popular VPN protocol with a simplified architecture.\n"
                      "Provides stable VPN connection, high performance on all devices. Uses hard-coded encryption "
                      "settings. WireGuard compared to OpenVPN has lower latency and better data transfer throughput.\n"
                      "WireGuard is very susceptible to blocking due to its distinct packet signatures. "
                      "Unlike some other VPN protocols that employ obfuscation techniques, "
                      "the consistent signature patterns of WireGuard packets can be more easily identified and "
                      "thus blocked by advanced Deep Packet Inspection (DPI) systems and other network monitoring tools.\n\n"
                      "* Available in the AmneziaVPN across all platforms\n"
                      "* Low power consumption\n"
                      "* Minimum number of settings\n"
                      "* Easily recognised by DPI analysis systems, susceptible to blocking\n"
                      "* Works over UDP network protocol.") },
        { DockerContainer::Awg,
          QObject::tr("A modern iteration of the popular VPN protocol, "
                      "AmneziaWG builds upon the foundation set by WireGuard, "
                      "retaining its simplified architecture and high-performance capabilities across devices.\n"
                      "While WireGuard is known for its efficiency, "
                      "it had issues with being easily detected due to its distinct packet signatures. "
                      "AmneziaWG solves this problem by using better obfuscation methods, "
                      "making its traffic blend in with regular internet traffic.\n"
                      "This means that AmneziaWG keeps the fast performance of the original "
                      "while adding an extra layer of stealth, "
                      "making it a great choice for those wanting a fast and discreet VPN connection.\n\n"
                      "* Available in the AmneziaVPN across all platforms\n"
                      "* Low power consumption\n"
                      "* Minimum number of settings\n"
                      "* Not recognised by DPI analysis systems, resistant to blocking\n"
                      "* Works over UDP network protocol.") },
        { DockerContainer::Ipsec,
          QObject::tr("IKEv2, paired with the IPSec encryption layer, stands as a modern and stable VPN protocol.\n"
                      "One of its distinguishing features is its ability to swiftly switch between networks and devices, "
                      "making it particularly adaptive in dynamic network environments. \n"
                      "While it offers a blend of security, stability, and speed, "
                      "it's essential to note that IKEv2 can be easily detected and is susceptible to blocking.\n\n"
                      "* Available in the AmneziaVPN only on Windows\n"
                      "* Low power consumption, on mobile devices\n"
                      "* Minimal configuration\n"
                      "* Recognised by DPI analysis systems\n"
                      "* Works over UDP network protocol, ports 500 and 4500.") },

        { DockerContainer::TorWebSite, QObject::tr("Website in Tor network") },
        { DockerContainer::Dns, QObject::tr("DNS Service") },
        { DockerContainer::Sftp, QObject::tr("Sftp file sharing service - is secure FTP service") }
    };
}

amnezia::ServiceType ContainerProps::containerService(DockerContainer c)
{
    return ProtocolProps::protocolService(defaultProtocol(c));
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
    case DockerContainer::Awg: return true;
    case DockerContainer::Cloak: return true;
    default: return false;
    }
}

QString ContainerProps::easySetupHeader(DockerContainer container)
{
    switch (container) {
    case DockerContainer::WireGuard: return tr("Low");
    case DockerContainer::Awg: return tr("Medium or High");
    case DockerContainer::Cloak: return tr("Extreme");
    default: return "";
    }
}

QString ContainerProps::easySetupDescription(DockerContainer container)
{
    switch (container) {
    case DockerContainer::WireGuard: return tr("I just want to increase the level of my privacy.");
    case DockerContainer::Awg: return tr("I want to bypass censorship. This option recommended in most cases.");
    case DockerContainer::Cloak:
        return tr("Most VPN protocols are blocked. Recommended if other options are not working.");
    default: return "";
    }
}

int ContainerProps::easySetupOrder(DockerContainer container)
{
    switch (container) {
    case DockerContainer::WireGuard: return 3;
    case DockerContainer::Awg: return 2;
    case DockerContainer::Cloak: return 1;
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
