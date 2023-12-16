#ifndef LINUXFIREWALL_H
#define LINUXFIREWALL_H


#include <QString>
#include <QStringList>

// Descriptor for a set of firewall rules to be appled.
//
struct FirewallParams
{
    QStringList dnsServers;
    //    QSharedPointer<NetworkAdapter> adapter;
    QVector<QString> excludeApps; // Apps to exclude if VPN exemptions are enabled

    QStringList excludeAddrs;
    // The follow flags indicate which general rulesets are needed. Note that
    // this is after some sanity filtering, i.e. an allow rule may be listed
    // as not needed if there were no block rules preceding it. The rulesets
    // should be thought of as in last-match order.

    bool blockAll;      // Block all traffic by default
    bool allowVPN;      // Exempt traffic through VPN tunnel
    bool allowDHCP;     // Exempt DHCP traffic
    bool blockIPv6;     // Block all IPv6 traffic
    bool allowLAN;      // Exempt LAN traffic, including IPv6 LAN traffic
    bool blockDNS;      // Block all DNS traffic except specified DNS servers
    bool allowPIA;      // Exempt PIA executables
    bool allowLoopback; // Exempt loopback traffic
    bool allowHnsd;     // Exempt Handshake DNS traffic
    bool allowVpnExemptions; // Exempt specified traffic from the tunnel (route it over the physical uplink instead)
};

class LinuxFirewall
{
public:
    enum IPVersion { IPv4, IPv6, Both };
    // Table names
    static QString kFilterTable, kNatTable, kMangleTable, kRtableName, kRawTable;
public:
    using FilterCallbackFunc = std::function<void()>;
private:
    static int createChain(IPVersion ip, const QString& chain, const QString& tableName = kFilterTable);
    static int deleteChain(IPVersion ip, const QString& chain, const QString& tableName = kFilterTable);
    static int linkChain(IPVersion ip, const QString& chain, const QString& parent, bool mustBeFirst = false, const QString& tableName = kFilterTable);
    static int unlinkChain(IPVersion ip, const QString& chain, const QString& parent, const QString& tableName = kFilterTable);
    static void installAnchor(IPVersion ip, const QString& anchor, const QStringList& rules, const QString& tableName = kFilterTable, const FilterCallbackFunc& enableFunc = {}, const FilterCallbackFunc& disableFunc = {});
    static void uninstallAnchor(IPVersion ip, const QString& anchor, const QString& tableName = kFilterTable);
    static QStringList getDNSRules(const QStringList& servers);
    static QStringList getExcludeRule(const QStringList& servers);
    static void setupTrafficSplitting();
    static void teardownTrafficSplitting();
    static int execute(const QString& command, bool ignoreErrors = false);
private:
    // Chain names
    static QString kOutputChain, kRootChain, kPostRoutingChain, kPreRoutingChain;

public:
    static void install();
    static void uninstall();
    static bool isInstalled();
    static void ensureRootAnchorPriority(IPVersion ip = Both);
    static void enableAnchor(IPVersion ip, const QString& anchor, const QString& tableName = kFilterTable);
    static void disableAnchor(IPVersion ip, const QString& anchor, const QString& tableName = kFilterTable);
    static bool isAnchorEnabled(IPVersion ip, const QString& anchor, const QString& tableName = kFilterTable);
    static void setAnchorEnabled(IPVersion ip, const QString& anchor, bool enabled, const QString& tableName = kFilterTable);
    static void replaceAnchor(LinuxFirewall::IPVersion ip, const QString &anchor, const QString &newRule, const QString& tableName);
    static void updateDNSServers(const QStringList& servers);
    static void updateExcludeAddrs(const QStringList& servers);
};


#endif // LINUXFIREWALL_H
