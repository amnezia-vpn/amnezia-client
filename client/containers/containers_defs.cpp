#include "containers_defs.h"

#include "QJsonObject"
#include "QJsonDocument"

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

    case DockerContainer::Xray: return { Proto::Xray };

    case DockerContainer::SSXray: return { Proto::SSXray };

    case DockerContainer::Dns: return { Proto::Dns };

    case DockerContainer::Sftp: return { Proto::Sftp };

    case DockerContainer::Socks5Proxy: return { Proto::Socks5Proxy };

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
             { DockerContainer::ShadowSocks, "OpenVPN over SS" },
             { DockerContainer::Cloak, "OpenVPN over Cloak" },
             { DockerContainer::WireGuard, "WireGuard" },
             { DockerContainer::Awg, "AmneziaWG" },
             { DockerContainer::Xray, "XRay" },
             { DockerContainer::Ipsec, QObject::tr("IPsec") },
             { DockerContainer::SSXray, "Shadowsocks"},

             { DockerContainer::TorWebSite, QObject::tr("Website in Tor network") },
             { DockerContainer::Dns, QObject::tr("AmneziaDNS") },
             { DockerContainer::Sftp, QObject::tr("SFTP file sharing service") },
             { DockerContainer::Socks5Proxy, QObject::tr("SOCKS5 proxy server") } };
}

QMap<DockerContainer, QString> ContainerProps::containerDescriptions()
{
    return { { DockerContainer::OpenVpn,
               QObject::tr("OpenVPN is the most popular VPN protocol, with flexible configuration options. It uses its "
                           "own security protocol with SSL/TLS for key exchange.") },
             { DockerContainer::ShadowSocks,
               QObject::tr("Shadowsocks - masks VPN traffic, making it similar to normal web traffic, but it "
                           "may be recognized by analysis systems in some highly censored regions.") },
             { DockerContainer::Cloak,
               QObject::tr("OpenVPN over Cloak - OpenVPN with VPN masquerading as web traffic and protection against "
                           "active-probing detection. Ideal for bypassing blocking in regions with the highest levels "
                           "of censorship.") },
             { DockerContainer::WireGuard,
               QObject::tr("WireGuard - New popular VPN protocol with high performance, high speed and low power "
                           "consumption. Recommended for regions with low levels of censorship.") },
             { DockerContainer::Awg,
               QObject::tr("AmneziaWG - Special protocol from Amnezia, based on WireGuard. It's fast like WireGuard, "
                           "but very resistant to blockages. "
                           "Recommended for regions with high levels of censorship.") },
             { DockerContainer::Xray,
               QObject::tr("XRay with REALITY - Suitable for countries with the highest level of internet censorship. "
                           "Traffic masking as web traffic at the TLS level, and protection against detection by active probing methods.") },
             { DockerContainer::Ipsec,
               QObject::tr("IKEv2/IPsec -  Modern stable protocol, a bit faster than others, restores connection after "
                           "signal loss. It has native support on the latest versions of Android and iOS.") },

             { DockerContainer::TorWebSite, QObject::tr("Deploy a WordPress site on the Tor network in two clicks.") },
             { DockerContainer::Dns,
               QObject::tr("Replace the current DNS server with your own. This will increase your privacy level.") },
             { DockerContainer::Sftp,
               QObject::tr("Create a file vault on your server to securely store and transfer files.") },
             { DockerContainer::Socks5Proxy,
               QObject::tr("") } };
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
                      "* Configurable encryption protocol\n"
                      "* Detectable by some DPI systems\n"
                      "* Works over TCP network protocol.") },
        { DockerContainer::Cloak,
          QObject::tr("This is a combination of the OpenVPN protocol and the Cloak plugin designed specifically for "
                      "protecting against blocking.\n\n"
                      "OpenVPN provides a secure VPN connection by encrypting all internet traffic between the client "
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
                      "WireGuard provides stable VPN connection and high performance on all devices. It uses hard-coded encryption "
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
        { DockerContainer::Xray,
          QObject::tr("The REALITY protocol, a pioneering development by the creators of XRay, "
                      "is specifically designed to counteract the highest levels of internet censorship through its novel approach to evasion.\n"
                      "It uniquely identifies censors during the TLS handshake phase, seamlessly operating as a proxy for legitimate clients while diverting censors to genuine websites like google.com, "
                      "thus presenting an authentic TLS certificate and data. \n"
                      "This advanced capability differentiates REALITY from similar technologies by its ability to disguise web traffic as coming from random, "
                      "legitimate sites without the need for specific configurations. \n"
                      "Unlike older protocols such as VMess, VLESS, and the XTLS-Vision transport, "
                      "REALITY's innovative \"friend or foe\" recognition at the TLS handshake enhances security and circumvents detection by sophisticated DPI systems employing active probing techniques. "
                      "This makes REALITY a robust solution for maintaining internet freedom in environments with stringent censorship.")
        },
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
        { DockerContainer::Sftp,
          QObject::tr("After installation, Amnezia will create a\n\n file storage on your server. "
                      "You will be able to access it using\n FileZilla or other SFTP clients, "
                      "as well as mount the disk on your device to access\n it directly from your device.\n\n"
                      "For more detailed information, you can\n find it in the support section under \"Create SFTP file storage.\" ") },
        { DockerContainer::Socks5Proxy, QObject::tr("SOCKS5 proxy server") }
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
    case DockerContainer::Xray: return Proto::Xray;
    case DockerContainer::Ipsec: return Proto::Ikev2;
    case DockerContainer::SSXray: return Proto::SSXray;

    case DockerContainer::TorWebSite: return Proto::TorWebSite;
    case DockerContainer::Dns: return Proto::Dns;
    case DockerContainer::Sftp: return Proto::Sftp;
    case DockerContainer::Socks5Proxy: return Proto::Socks5Proxy;
    default: return Proto::Any;
    }
}

bool ContainerProps::isSupportedByCurrentPlatform(DockerContainer c)
{
#if defined(Q_OS_WINDOWS) || defined(Q_OS_LINUX)
    return true;

#elif defined(Q_OS_IOS)
    switch (c) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::OpenVpn: return true;
    case DockerContainer::Awg: return true;
    case DockerContainer::Xray: return true;
    case DockerContainer::Cloak: return true;
    case DockerContainer::SSXray: return true;
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
    case DockerContainer::ShadowSocks: return false;
    case DockerContainer::Awg: return true;
    case DockerContainer::Cloak: return true;
    case DockerContainer::Xray: return true;
    case DockerContainer::SSXray: return true;
    default: return false;
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
    // case DockerContainer::Cloak: return true;
    default: return false;
    }
}

QString ContainerProps::easySetupHeader(DockerContainer container)
{
    switch (container) {
    case DockerContainer::WireGuard: return tr("Low");
    case DockerContainer::Awg: return tr("High");
    // case DockerContainer::Cloak: return tr("Extreme");
    default: return "";
    }
}

QString ContainerProps::easySetupDescription(DockerContainer container)
{
    switch (container) {
    case DockerContainer::WireGuard: return tr("I just want to increase the level of my privacy.");
    case DockerContainer::Awg: return tr("I want to bypass censorship. This option recommended in most cases.");
    // case DockerContainer::Cloak:
    //     return tr("Most VPN protocols are blocked. Recommended if other options are not working.");
    default: return "";
    }
}

int ContainerProps::easySetupOrder(DockerContainer container)
{
    switch (container) {
    case DockerContainer::WireGuard: return 3;
    case DockerContainer::Awg: return 2;
    // case DockerContainer::Cloak: return 1;
    default: return 0;
    }
}

bool ContainerProps::isShareable(DockerContainer container)
{
    switch (container) {
    case DockerContainer::TorWebSite: return false;
    case DockerContainer::Dns: return false;
    case DockerContainer::Sftp: return false;
    case DockerContainer::Socks5Proxy: return false;
    default: return true;
    }
}

QJsonObject ContainerProps::getProtocolConfigFromContainer(const Proto protocol, const QJsonObject &containerConfig)
{
    QString protocolConfigString = containerConfig.value(ProtocolProps::protoToString(protocol))
                                           .toObject()
                                           .value(config_key::last_config)
                                           .toString();

    return QJsonDocument::fromJson(protocolConfigString.toUtf8()).object();
}

int ContainerProps::installPageOrder(DockerContainer container)
{
    switch (container) {
    case DockerContainer::OpenVpn: return 4;
    case DockerContainer::Cloak: return 5;
    case DockerContainer::ShadowSocks: return 6;
    case DockerContainer::WireGuard: return 2;
    case DockerContainer::Awg: return 1;
    case DockerContainer::Xray: return 3;
    case DockerContainer::Ipsec: return 7;
    case DockerContainer::SSXray: return 8;
    default: return 0;
    }
}
