#include "containerUtils.h"

#include <QMetaEnum>
#include <QObject>
#include <QJsonDocument>

using namespace amnezia;

DockerContainer ContainerUtils::containerFromString(const QString &container)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        DockerContainer c = static_cast<DockerContainer>(i);
        if (container == containerToString(c))
            return c;
    }
    return DockerContainer::None;
}

QString ContainerUtils::containerToString(DockerContainer c)
{
    if (c == DockerContainer::None)
        return "none";
    if (c == DockerContainer::Cloak)
        return "amnezia-openvpn-cloak";
    if (c == DockerContainer::Awg)
        return "amnezia-awg";
    if (c == DockerContainer::Awg2)
        return "amnezia-awg2";
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QString containerKey = metaEnum.valueToKey(static_cast<int>(c));

    return "amnezia-" + containerKey.toLower();
}

QString ContainerUtils::containerTypeToString(DockerContainer c)
{
    if (c == DockerContainer::None)
        return "none";
    if (c == DockerContainer::Ipsec)
        return "ikev2";
    if (c == DockerContainer::Awg)
        return "awg";
    if (c == DockerContainer::Awg2)
        return "awg";
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QString containerKey = metaEnum.valueToKey(static_cast<int>(c));

    return containerKey.toLower();
}

QList<DockerContainer> ContainerUtils::allContainers()
{
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QList<DockerContainer> all;
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        all.append(static_cast<DockerContainer>(i));
    }

    return all;
}

QMap<DockerContainer, QString> ContainerUtils::containerHumanNames()
{
    return { { DockerContainer::None, "Not installed" },
             { DockerContainer::OpenVpn, "OpenVPN" },
             { DockerContainer::ShadowSocks, "OpenVPN over SS" },
             { DockerContainer::Cloak, "OpenVPN over Cloak" },
             { DockerContainer::WireGuard, "WireGuard" },
             { DockerContainer::Awg, "AmneziaWG" },
             { DockerContainer::Awg2, "AmneziaWG" },
             { DockerContainer::Xray, "XRay" },
             { DockerContainer::Ipsec, QObject::tr("IPsec") },
             { DockerContainer::SSXray, "Shadowsocks"},

             { DockerContainer::TorWebSite, QObject::tr("Website in Tor network") },
             { DockerContainer::Dns, QObject::tr("AmneziaDNS") },
             { DockerContainer::Sftp, QObject::tr("SFTP file sharing service") },
             { DockerContainer::Socks5Proxy, QObject::tr("SOCKS5 proxy server") },
             { DockerContainer::MtProxy, QObject::tr("MTProxy (Telegram)") },
             { DockerContainer::Telemt, QObject::tr("Telemt (Telegram)") },
    };
}

QMap<DockerContainer, QString> ContainerUtils::containerDescriptions()
{
    return {              { DockerContainer::OpenVpn,
               QObject::tr("OpenVPN is the most popular VPN protocol, with flexible configuration options. It uses its "
                           "own security protocol with SSL/TLS for key exchange.") },
             { DockerContainer::ShadowSocks,
               QObject::tr("This protocol is no longer supported.") },
             { DockerContainer::Cloak,
               QObject::tr("This protocol is no longer supported.") },
             { DockerContainer::WireGuard,
               QObject::tr("WireGuard - popular VPN protocol with high performance, high speed and low power "
                           "consumption.") },
             { DockerContainer::Awg,
               QObject::tr("AmneziaWG is a special protocol from Amnezia based on WireGuard. "
                           "It provides high connection speed and ensures stable operation even in the most challenging network conditions.") },
             { DockerContainer::Awg2,
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
               QObject::tr("") },
             { DockerContainer::MtProxy,
               QObject::tr("Telegram MTProto proxy server") },
             { DockerContainer::Telemt,
               QObject::tr("Telegram MTProto proxy (Telemt, Rust)") },
    };
}

QMap<DockerContainer, QString> ContainerUtils::containerDetailedDescriptions()
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
        { DockerContainer::Awg2,
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
        { DockerContainer::Socks5Proxy, QObject::tr("SOCKS5 proxy server") },
        { DockerContainer::MtProxy,
          QObject::tr("Telegram MTProto proxy server. "
                      "Allows Telegram clients to connect through your server "
                      "using the MTProto protocol. Supports FakeTLS mode for "
                      "bypassing DPI-based blocking.") },
        { DockerContainer::Telemt,
          QObject::tr("Telegram MTProto proxy powered by Telemt (Rust). "
                      "Supports secure and TLS fronting modes with optional traffic masking.") },
    };
}

ServiceType ContainerUtils::containerService(DockerContainer c)
{
    if (isUnsupportedContainer(c)) {
        return ServiceType::Vpn;
    }
    return ProtocolUtils::protocolService(defaultProtocol(c));
}

Proto ContainerUtils::defaultProtocol(DockerContainer c)
{
    switch (c) {
    case DockerContainer::None: return Proto::Unknown;
    case DockerContainer::OpenVpn: return Proto::OpenVpn;
    case DockerContainer::Cloak:
    case DockerContainer::ShadowSocks: return Proto::Unknown;
    case DockerContainer::WireGuard: return Proto::WireGuard;
    case DockerContainer::Awg2: return Proto::Awg;
    case DockerContainer::Awg: return Proto::Awg;
    case DockerContainer::Xray: return Proto::Xray;
    case DockerContainer::Ipsec: return Proto::Ikev2;
    case DockerContainer::SSXray: return Proto::SSXray;

    case DockerContainer::TorWebSite: return Proto::TorWebSite;
    case DockerContainer::Dns: return Proto::Dns;
    case DockerContainer::Sftp: return Proto::Sftp;
    case DockerContainer::Socks5Proxy: return Proto::Socks5Proxy;
    case DockerContainer::MtProxy: return Proto::MtProxy;
    case DockerContainer::Telemt: return Proto::Telemt;
    default: return Proto::Unknown;
    }
}

QString ContainerUtils::containerTypeToProtocolString(DockerContainer c)
{
    if (c == DockerContainer::None)
        return "none";

    Proto p = defaultProtocol(c);
    return ProtocolUtils::protoToString(p);
}

bool ContainerUtils::isSupportedByCurrentPlatform(DockerContainer c)
{
#ifdef Q_OS_WINDOWS
    return true;

#elif defined(Q_OS_IOS)
    // Standard iOS build (without Network Extension limitations)
    switch (c) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::OpenVpn: return true;
    case DockerContainer::Awg2: return true;
    case DockerContainer::Awg: return true;
    case DockerContainer::Xray: return true;
    case DockerContainer::SSXray: return true;
    case DockerContainer::MtProxy: return true;
    case DockerContainer::Telemt: return true;
    default:
        return false;
    }

#elif defined(MACOS_NE)
    // macOS build using Network Extension – allow OpenVPN for parity with iOS.
    switch (c) {
    case DockerContainer::OpenVpn: return true;
    case DockerContainer::Cloak: return false;
    case DockerContainer::ShadowSocks: return false;
    case DockerContainer::WireGuard: return true;
    case DockerContainer::Awg2: return true;
    case DockerContainer::Awg: return true;
    case DockerContainer::Xray: return true;
    case DockerContainer::SSXray: return true;
    case DockerContainer::MtProxy: return true;
    case DockerContainer::Telemt: return true;
    default:
        return false;
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
    case DockerContainer::Awg2: return true;
    case DockerContainer::Awg: return true;
    case DockerContainer::Xray: return true;
    case DockerContainer::SSXray: return true;
    case DockerContainer::MtProxy: return true;
    case DockerContainer::Telemt: return true;
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

QStringList ContainerUtils::fixedPortsForContainer(DockerContainer c)
{
    switch (c) {
    case DockerContainer::Ipsec: return QStringList { "500", "4500" };
    default: return {};
    }
}

bool ContainerUtils::isEasySetupContainer(DockerContainer container)
{
    switch (container) {
    case DockerContainer::Awg2: return true;
    default: return false;
    }
}

QString ContainerUtils::easySetupHeader(DockerContainer container)
{
    switch (container) {
    case DockerContainer::Awg2: return QObject::tr("Automatic");
    default: return "";
    }
}

QString ContainerUtils::easySetupDescription(DockerContainer container)
{
    switch (container) {
    case DockerContainer::Awg2: return QObject::tr("AmneziaWG protocol will be installed. "
                                         "It provides high connection speed and ensures stable operation even in the most challenging network conditions.");
    default: return "";
    }
}

int ContainerUtils::easySetupOrder(DockerContainer container)
{
    switch (container) {
    case DockerContainer::Awg2: return 1;
    default: return 0;
    }
}

bool ContainerUtils::isShareable(DockerContainer container)
{
    if (isUnsupportedContainer(container)) {
        return false;
    }

    switch (container) {
    case DockerContainer::TorWebSite: return false;
    case DockerContainer::Dns: return false;
    case DockerContainer::Sftp: return false;
    case DockerContainer::Socks5Proxy: return false;
    case DockerContainer::MtProxy: return false;
    case DockerContainer::Telemt: return false;
    default: return true;
    }
}

bool ContainerUtils::isAwgContainer(DockerContainer container)
{
    return container == DockerContainer::Awg || container == DockerContainer::Awg2;
}

bool ContainerUtils::isUnsupportedContainer(DockerContainer container)
{
    return container == DockerContainer::Cloak || container == DockerContainer::ShadowSocks;
}

QJsonObject ContainerUtils::getProtocolConfigFromContainer(const Proto protocol, const QJsonObject &containerConfig)
{
    QString protocolConfigString = containerConfig.value(ProtocolUtils::protoToString(protocol))
    .toObject()
            .value(configKey::lastConfig)
            .toString();

    return QJsonDocument::fromJson(protocolConfigString.toUtf8()).object();
}

int ContainerUtils::installPageOrder(DockerContainer container)
{
    switch (container) {
    case DockerContainer::OpenVpn: return 4;
    case DockerContainer::WireGuard: return 2;
    case DockerContainer::Awg2: return 1;
    case DockerContainer::Xray: return 3;
    case DockerContainer::Ipsec: return 7;
    case DockerContainer::SSXray: return 8;
    case DockerContainer::MtProxy:
    case DockerContainer::Telemt:
        return 20;
    default: return 0;
    }
}

