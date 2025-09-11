#include "killswitch.h"


#include <QApplication>
#include <QHostAddress>

#include "../client/protocols/protocols_defs.h"
#include "qjsonarray.h"
#include "version.h"

#ifdef Q_OS_WIN
    #include "../client/platforms/windows/daemon/windowsfirewall.h"
    #include "../client/platforms/windows/daemon/windowsdaemon.h"
#endif

#ifdef Q_OS_LINUX
    #include "../client/platforms/linux/daemon/linuxfirewall.h"
#endif

#ifdef Q_OS_MACOS
    #include "../client/platforms/macos/daemon/macosfirewall.h"
#endif

KillSwitch* s_instance = nullptr;

KillSwitch* KillSwitch::instance()
{
    if (s_instance == nullptr) {
        s_instance = new KillSwitch(qApp);
    }
    return s_instance;
}

bool KillSwitch::init()
{
#ifdef Q_OS_LINUX
    if (!LinuxFirewall::isInstalled()) {
        LinuxFirewall::install();
    }
    m_appSettigns = QSharedPointer<SecureQSettings>(new SecureQSettings(ORGANIZATION_NAME, APPLICATION_NAME, nullptr));
#endif
#ifdef Q_OS_MACOS
    if (!MacOSFirewall::isInstalled()) {
        MacOSFirewall::install();
    }
    m_appSettigns = QSharedPointer<SecureQSettings>(new SecureQSettings(ORGANIZATION_NAME, APPLICATION_NAME, nullptr));
#endif
    if (isStrictKillSwitchEnabled()) {
        return disableAllTraffic();
    }

    return true;
}

bool KillSwitch::refresh(bool enabled)
{
#ifdef Q_OS_WIN
    QSettings RegHLM("HKEY_LOCAL_MACHINE\\Software\\" + QString(ORGANIZATION_NAME)
                             + "\\" + QString(APPLICATION_NAME), QSettings::NativeFormat);
    RegHLM.setValue("strictKillSwitchEnabled", enabled);
#endif

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    m_appSettigns->setValue("Conf/strictKillSwitchEnabled", enabled);
#endif

    if (isStrictKillSwitchEnabled()) {
        return disableAllTraffic();
    }  else {
        return disableKillSwitch();
    }

}

bool KillSwitch::isStrictKillSwitchEnabled()
{
#ifdef Q_OS_WIN
    QSettings RegHLM("HKEY_LOCAL_MACHINE\\Software\\" + QString(ORGANIZATION_NAME)
                             + "\\" + QString(APPLICATION_NAME), QSettings::NativeFormat);
    return RegHLM.value("strictKillSwitchEnabled", false).toBool();
#endif
    m_appSettigns->sync();
    return m_appSettigns->value("Conf/strictKillSwitchEnabled", false).toBool();
}

bool KillSwitch::disableKillSwitch() {
#ifdef Q_OS_LINUX
    if (isStrictKillSwitchEnabled()) {
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("000.allowLoopback"), true);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("100.blockAll"), true);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("110.allowNets"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("120.blockNets"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("200.allowVPN"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv6, QStringLiteral("250.blockIPv6"), true);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("290.allowDHCP"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("300.allowLAN"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("310.blockDNS"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("320.allowDNS"), false);
        LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("400.allowPIA"), false);
    } else {
        LinuxFirewall::uninstall();
    }
#endif

#ifdef Q_OS_MACOS
    if (isStrictKillSwitchEnabled()) {
        MacOSFirewall::setAnchorEnabled(QStringLiteral("000.allowLoopback"), true);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("100.blockAll"), true);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("110.allowNets"), false);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("120.blockNets"), false);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("200.allowVPN"), false);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("250.blockIPv6"), true);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("290.allowDHCP"), false);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("300.allowLAN"), false);
        MacOSFirewall::setAnchorEnabled(QStringLiteral("310.blockDNS"), false);
    } else {
        MacOSFirewall::uninstall();
    }
#endif

#ifdef Q_OS_WIN
    if (isStrictKillSwitchEnabled()) {
        return disableAllTraffic();
    }
    return WindowsFirewall::create(this)->allowAllTraffic();
#endif

    m_allowedRanges.clear();
    return true;
}

bool KillSwitch::disableAllTraffic() {
#ifdef Q_OS_WIN
    WindowsFirewall::create(this)->enableInterface(-1);
#endif
#ifdef Q_OS_LINUX
    if (!LinuxFirewall::isInstalled()) {
        LinuxFirewall::install();
    }
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("100.blockAll"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("000.allowLoopback"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv6, QStringLiteral("250.blockIPv6"), true);
#endif
#ifdef Q_OS_MACOS
    // double-check + ensure our firewall is installed and enabled. This is necessary as
    // other software may disable pfctl before re-enabling with their own rules (e.g other VPNs)
    if (!MacOSFirewall::isInstalled())
        MacOSFirewall::install();
    MacOSFirewall::ensureRootAnchorPriority();
    MacOSFirewall::setAnchorEnabled(QStringLiteral("100.blockAll"), true);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("000.allowLoopback"), true);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("250.blockIPv6"), true);
#endif
    m_allowedRanges.clear();
    return true;
}

bool KillSwitch::resetAllowedRange(const QStringList &ranges) {

    m_allowedRanges = ranges;

#ifdef Q_OS_LINUX
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("110.allowNets"), true);
    LinuxFirewall::updateAllowNets(m_allowedRanges);
#endif

#ifdef Q_OS_MACOS
    MacOSFirewall::setAnchorEnabled(QStringLiteral("110.allowNets"), true);
    MacOSFirewall::setAnchorTable(QStringLiteral("110.allowNets"), true, QStringLiteral("allownets"), m_allowedRanges);
#endif

#ifdef Q_OS_WIN
    if (isStrictKillSwitchEnabled()) {
        WindowsFirewall::create(this)->enableInterface(-1);
    }
    WindowsFirewall::create(this)->allowTrafficRange(m_allowedRanges);
#endif

    return true;
}

bool KillSwitch::addAllowedRange(const QStringList &ranges) {
    for (const QString &range : ranges) {
        if (!range.isEmpty() && !m_allowedRanges.contains(range)) {
            m_allowedRanges.append(range);
        }
    }

    return resetAllowedRange(m_allowedRanges);
}

bool KillSwitch::enablePeerTraffic(const QJsonObject &configStr) {
#ifdef Q_OS_WIN
    InterfaceConfig config;

    config.m_primaryDnsServer = configStr.value(amnezia::config_key::dns1).toString();

    // We don't use secondary DNS if primary DNS is AmneziaDNS
    if (!config.m_primaryDnsServer.contains(amnezia::protocols::dns::amneziaDnsIp)) {
        config.m_secondaryDnsServer = configStr.value(amnezia::config_key::dns2).toString();
    }

    config.m_serverPublicKey = "openvpn";
    config.m_serverIpv4Gateway = configStr.value("vpnGateway").toString();
    config.m_serverIpv4AddrIn = configStr.value("vpnServer").toString();
    int vpnAdapterIndex = configStr.value("vpnAdapterIndex").toInt();
    int inetAdapterIndex = configStr.value("inetAdapterIndex").toInt();

    int splitTunnelType = configStr.value("splitTunnelType").toInt();
    QJsonArray splitTunnelSites = configStr.value("splitTunnelSites").toArray();

    // Use APP split tunnel
    if (splitTunnelType == 0 || splitTunnelType == 2) {
        config.m_allowedIPAddressRanges.append(IPAddress(QHostAddress("0.0.0.0"), 0));
        config.m_allowedIPAddressRanges.append(IPAddress(QHostAddress("::"), 0));
    }

    if (splitTunnelType == 1) {
        for (auto v : splitTunnelSites) {
            QString ipRange = v.toString();
            if (ipRange.split('/').size() > 1) {
                config.m_allowedIPAddressRanges.append(
                        IPAddress(QHostAddress(ipRange.split('/')[0]), atoi(ipRange.split('/')[1].toLocal8Bit())));
            } else {
                config.m_allowedIPAddressRanges.append(IPAddress(QHostAddress(ipRange), 32));
            }
        }
    }

    config.m_excludedAddresses.append(configStr.value("vpnServer").toString());
    if (splitTunnelType == 2) {
        for (auto v : splitTunnelSites) {
            QString ipRange = v.toString();
            config.m_excludedAddresses.append(ipRange);
        }
    }

    for (const QJsonValue &i : configStr.value(amnezia::config_key::splitTunnelApps).toArray()) {
        if (!i.isString()) {
            break;
        }
        config.m_vpnDisabledApps.append(i.toString());
    }

    for (auto dns : configStr.value(amnezia::config_key::allowedDnsServers).toArray()) {
        if (!dns.isString()) {
            break;
        }
        config.m_allowedDnsServers.append(dns.toString());
    }

    // killSwitch toggle
    if (QVariant(configStr.value(amnezia::config_key::killSwitchOption).toString()).toBool()) {
        WindowsFirewall::create(this)->enablePeerTraffic(config);
    }

    WindowsDaemon::instance()->prepareActivation(config, inetAdapterIndex);
    WindowsDaemon::instance()->activateSplitTunnel(config, vpnAdapterIndex);
#endif
    return true;
}

bool KillSwitch::enableKillSwitch(const QJsonObject &configStr, int vpnAdapterIndex) {
#ifdef Q_OS_WIN
    if (configStr.value("splitTunnelType").toInt() != 0) {
        WindowsFirewall::create(this)->allowAllTraffic();
    }
    return WindowsFirewall::create(this)->enableInterface(vpnAdapterIndex);
#endif

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    int splitTunnelType = configStr.value("splitTunnelType").toInt();
    QJsonArray splitTunnelSites = configStr.value("splitTunnelSites").toArray();
    bool blockAll = 0;
    bool allowNets = 0;
    bool blockNets = 0;
    QStringList allownets;
    QStringList blocknets;

    if (splitTunnelType == 0) {
        blockAll = true;
        allowNets = true;
        allownets.append(configStr.value("vpnServer").toString());
    } else if (splitTunnelType == 1) {
        blockNets = true;
        for (auto v : splitTunnelSites) {
            blocknets.append(v.toString());
        }
    } else if (splitTunnelType == 2) {
        blockAll = true;
        allowNets = true;
        allownets.append(configStr.value("vpnServer").toString());
        for (auto v : splitTunnelSites) {
            allownets.append(v.toString());
        }
    }
#endif

#ifdef Q_OS_LINUX
    if (!LinuxFirewall::isInstalled()) {
        LinuxFirewall::install();
    }

    // double-check + ensure our firewall is installed and enabled
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("000.allowLoopback"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("100.blockAll"), blockAll);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("110.allowNets"), allowNets);
    LinuxFirewall::updateAllowNets(allownets);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("120.blockNets"), blockAll);
    LinuxFirewall::updateBlockNets(blocknets);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("200.allowVPN"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv6, QStringLiteral("250.blockIPv6"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("290.allowDHCP"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("300.allowLAN"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("310.blockDNS"), true);
    QStringList dnsServers;

    dnsServers.append(configStr.value(amnezia::config_key::dns1).toString());

    // We don't use secondary DNS if primary DNS is AmneziaDNS
    if (!configStr.value(amnezia::config_key::dns1).toString().contains(amnezia::protocols::dns::amneziaDnsIp)) {
        dnsServers.append(configStr.value(amnezia::config_key::dns2).toString());
    }

    dnsServers.append("127.0.0.1");
    dnsServers.append("127.0.0.53");
    
    for (auto dns : configStr.value(amnezia::config_key::allowedDnsServers).toArray()) {
        if (!dns.isString()) {
            break;
        }
        dnsServers.append(dns.toString());
    }
    
    LinuxFirewall::updateDNSServers(dnsServers);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("320.allowDNS"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("400.allowPIA"), true);
#endif

#ifdef Q_OS_MACOS
    // double-check + ensure our firewall is installed and enabled. This is necessary as
    // other software may disable pfctl before re-enabling with their own rules (e.g other VPNs)
    if (!MacOSFirewall::isInstalled())
        MacOSFirewall::install();

    MacOSFirewall::ensureRootAnchorPriority();
    MacOSFirewall::setAnchorEnabled(QStringLiteral("000.allowLoopback"), true);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("100.blockAll"), blockAll);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("110.allowNets"), allowNets);
    MacOSFirewall::setAnchorTable(QStringLiteral("110.allowNets"), allowNets, QStringLiteral("allownets"), allownets);

    MacOSFirewall::setAnchorEnabled(QStringLiteral("120.blockNets"), blockNets);
    MacOSFirewall::setAnchorTable(QStringLiteral("120.blockNets"), blockNets, QStringLiteral("blocknets"), blocknets);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("200.allowVPN"), true);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("250.blockIPv6"), true);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("290.allowDHCP"), true);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("300.allowLAN"), true);

    QStringList dnsServers;
    dnsServers.append(configStr.value(amnezia::config_key::dns1).toString());

    // We don't use secondary DNS if primary DNS is AmneziaDNS
    if (!configStr.value(amnezia::config_key::dns1).toString().contains(amnezia::protocols::dns::amneziaDnsIp)) {
        dnsServers.append(configStr.value(amnezia::config_key::dns2).toString());
    }
    
    for (auto dns : configStr.value(amnezia::config_key::allowedDnsServers).toArray()) {
        if (!dns.isString()) {
            break;
        }
        dnsServers.append(dns.toString());
    }
    
    MacOSFirewall::setAnchorEnabled(QStringLiteral("310.blockDNS"), true);
    MacOSFirewall::setAnchorTable(QStringLiteral("310.blockDNS"), true, QStringLiteral("dnsaddr"), dnsServers);
#endif
    return true;
}
