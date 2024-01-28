// Copyright (c) 2023 Private Internet Access, Inc.
//
// This file is part of the Private Internet Access Desktop Client.
//
// The Private Internet Access Desktop Client is free software: you can
// redistribute it and/or modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// The Private Internet Access Desktop Client is distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the Private Internet Access Desktop Client.  If not, see
// <https://www.gnu.org/licenses/>.

// Copyright (c) 2024 AmneziaVPN
// This file has been modified for AmneziaVPN
//
// This file is based on the work of the Private Internet Access Desktop Client.
// The original code of the Private Internet Access Desktop Client is copyrighted (c) 2023 Private Internet Access, Inc. and licensed under GPL3.
//
// The modified version of this file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this file. If not, see <https://www.gnu.org/licenses/>.

#ifndef LINUXFIREWALL_H
#define LINUXFIREWALL_H


#include <QString>
#include <QStringList>

// Descriptor for a set of firewall rules to be appled.
//
struct FirewallParams
{
    QStringList dnsServers;
    QVector<QString> excludeApps; // Apps to exclude if VPN exemptions are enabled
    QStringList allowAddrs;
    QStringList blockAddrs;
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
    bool allowNets;
    bool blockNets;
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
    static QStringList getAllowRule(const QStringList& servers);
    static QStringList getBlockRule(const QStringList& servers);
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
    static void updateAllowNets(const QStringList& servers);
    static void updateBlockNets(const QStringList& servers);
};

#endif // LINUXFIREWALL_H
