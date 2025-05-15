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
               QObject::tr("Shadowsocks masks VPN traffic, making it resemble normal web traffic, but it may still be detected by certain analysis systems.") },
             { DockerContainer::Cloak,
               QObject::tr("OpenVPN over Cloak - OpenVPN with VPN masquerading as web traffic and protection against "
                           "active-probing detection. It is very resistant to detection, but offers low speed.") },
             { DockerContainer::WireGuard,
               QObject::tr("WireGuard - popular VPN protocol with high performance, high speed and low power "
                           "consumption.") },
             { DockerContainer::Awg,
               QObject::tr("AmneziaWG is a special protocol from Amnezia based on WireGuard. "
                           "It provides high connection speed and ensures stable operation even in the most challenging network conditions.") },
             { DockerContainer::Xray,
               QObject::tr("XRay with REALITY masks VPN traffic as web traffic and protects against active probing. "
                           "It is highly resistant to detection and offers high speed.") },
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
          QObject::tr("OpenVPN is one of the most popular and reliable VPN protocols. "
                      "It uses SSL/TLS encryption, supports a wide variety of devices and operating systems, "
                      "and is continuously improved by the community due to its open-source nature. "
                      "It provides a good balance between speed and security but is easily recognized by DPI systems, "
                      "making it susceptible to blocking.\n"
                      "\nFeatures:\n"
                      "* Available on all AmneziaVPN platforms\n"
                      "* Normal battery consumption on mobile devices\n"
                      "* Flexible customization for various devices and OS\n"
                      "* Operates over both TCP and UDP protocols") },
        { DockerContainer::ShadowSocks,
          QObject::tr("Shadowsocks is based on the SOCKS5 protocol and encrypts connections using AEAD cipher. "
                      "Although designed to be discreet, it doesn't mimic a standard HTTPS connection and can be detected by some DPI systems. "
                      "Due to limited support in Amnezia, we recommend using the AmneziaWG protocol.\n"
                      "\nFeatures:\n"
                      "* Available in AmneziaVPN only on desktop platforms\n"
                      "* Customizable encryption protocol\n"
                      "* Detectable by some DPI systems\n"
                      "* Operates over TCP protocol\n") },
        { DockerContainer::Cloak,
          QObject::tr("This combination includes the OpenVPN protocol and the Cloak plugin, specifically designed to protect against blocking.\n"
                      "\nOpenVPN securely encrypts all internet traffic between your device and the server.\n"
                      "\nThe Cloak plugin further protects the connection from DPI detection. "
                      "It modifies traffic metadata to disguise VPN traffic as regular web traffic and prevents detection through active probing. "
                      "If an incoming connection fails authentication, Cloak serves a fake website, making your VPN invisible to traffic analysis systems.\n"
                      "\nIn regions with heavy internet censorship, we strongly recommend using OpenVPN with Cloak from your first connection.\n"
                      "\nFeatures:\n"
                      "* Available on all AmneziaVPN platforms\n"
                      "* High power consumption on mobile devices\n"
                      "* Flexible configuration options\n"
                      "* Undetectable by DPI systems\n"
                      "* Operates over TCP protocol on port 443") },
        { DockerContainer::WireGuard,
          QObject::tr("WireGuard is a modern, streamlined VPN protocol offering stable connectivity and excellent performance across all devices. "
                      "It uses fixed encryption settings, delivering lower latency and higher data transfer speeds compared to OpenVPN. "
                      "However, WireGuard is easily identifiable by DPI systems due to its distinctive packet signatures, making it susceptible to blocking.\n"
                      "\nFeatures:\n"
                      "* Available on all AmneziaVPN platforms\n"
                      "* Low power consumption on mobile devices\n"
                      "* Minimal configuration required\n"
                      "* Easily detected by DPI systems (susceptible to blocking)\n"
                      "* Operates over UDP protocol") },
        { DockerContainer::Awg,
          QObject::tr("AmneziaWG is a modern VPN protocol based on WireGuard, "
                      "combining simplified architecture with high performance across all devices. "
                      "It addresses WireGuard's main vulnerability (easy detection by DPI systems) through advanced obfuscation techniques, "
                      "making VPN traffic indistinguishable from regular internet traffic.\n"
                      "\nAmneziaWG is an excellent choice for those seeking a fast, stealthy VPN connection.\n"
                      "\nFeatures:\n"
                      "* Available on all AmneziaVPN platforms\n"
                      "* Low battery consumption on mobile devices\n"
                      "* Minimal settings required\n"
                      "* Undetectable by traffic analysis systems (DPI)\n"
                      "* Operates over UDP protocol") },
        { DockerContainer::Xray,
          QObject::tr("REALITY is an innovative protocol developed by the creators of XRay, designed specifically to combat high levels of internet censorship. "
                      "REALITY identifies censorship systems during the TLS handshake, "
                      "redirecting suspicious traffic seamlessly to legitimate websites like google.com while providing genuine TLS certificates. "
                      "This allows VPN traffic to blend indistinguishably with regular web traffic without special configuration."
                      "\nUnlike older protocols such as VMess, VLESS, and XTLS-Vision, REALITY incorporates an advanced built-in \"friend-or-foe\" detection mechanism, "
                      "effectively protecting against DPI and other traffic analysis methods.\n"
                      "\nFeatures:\n"
                      "* Resistant to active probing and DPI detection\n"
                      "* No special configuration required to disguise traffic\n"
                      "* Highly effective in heavily censored regions\n"
                      "* Minimal battery consumption on devices\n"
                      "* Operates over TCP protocol") },
        { DockerContainer::Ipsec,
          QObject::tr("IKEv2, combined with IPSec encryption, is a modern and reliable VPN protocol. "
                      "It reconnects quickly when switching networks or devices, making it ideal for dynamic network environments. "
                      "While it provides good security and speed, it's easily recognized by DPI systems and susceptible to blocking.\n"
                      "\nFeatures:\n"
                      "* Available in AmneziaVPN only on Windows\n"
                      "* Low battery consumption on mobile devices\n"
                      "* Minimal configuration required\n"
                      "* Detectable by DPI analysis systems(easily blocked)\n"
                      "* Operates over UDP protocol(ports 500 and 4500)") },

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
#ifdef Q_OS_WINDOWS
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

#elif defined(Q_OS_LINUX)
    switch (c) {
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
    case DockerContainer::Awg: return true;
    default: return false;
    }
}

QString ContainerProps::easySetupHeader(DockerContainer container)
{
    switch (container) {
    case DockerContainer::Awg: return tr("Automatic");
    default: return "";
    }
}

QString ContainerProps::easySetupDescription(DockerContainer container)
{
    switch (container) {
    case DockerContainer::Awg: return tr("AmneziaWG protocol will be installed. "
                                         "It provides high connection speed and ensures stable operation even in the most challenging network conditions.");
    default: return "";
    }
}

int ContainerProps::easySetupOrder(DockerContainer container)
{
    switch (container) {
    case DockerContainer::Awg: return 1;
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
