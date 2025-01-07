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

#include "linuxfirewall.h"
#include "logger.h"
#include <QProcess>

#define BRAND_CODE "amn"

namespace {
Logger logger("LinuxFirewall");
}  // namespace

namespace
{
const QString kAnchorName{BRAND_CODE "vpn"};
const QString kPacketTag{"0x3211"};
const QString kCGroupId{"0x567"};
const QString enabledKeyTemplate = "enabled:%1:%2";
const QString disabledKeyTemplate = "disabled:%1:%2";
const QString kVpnGroupName = BRAND_CODE "vpn";
QHash<QString, LinuxFirewall::FilterCallbackFunc> anchorCallbacks;
}

QString LinuxFirewall::kRtableName = QStringLiteral("%1rt").arg(kAnchorName);
QString LinuxFirewall::kOutputChain = QStringLiteral("OUTPUT");
QString LinuxFirewall::kPostRoutingChain = QStringLiteral("POSTROUTING");
QString LinuxFirewall::kPreRoutingChain = QStringLiteral("PREROUTING");
QString LinuxFirewall::kRootChain = QStringLiteral("%1.anchors").arg(kAnchorName);
QString LinuxFirewall::kFilterTable = QStringLiteral("filter");
QString LinuxFirewall::kNatTable = QStringLiteral("nat");
QString LinuxFirewall::kRawTable = QStringLiteral("raw");
QString LinuxFirewall::kMangleTable = QStringLiteral("mangle");

static QString getCommand(LinuxFirewall::IPVersion ip)
{
    return ip == LinuxFirewall::IPv6 ? QStringLiteral("ip6tables") : QStringLiteral("iptables");
}

int LinuxFirewall::createChain(LinuxFirewall::IPVersion ip, const QString& chain, const QString& tableName)
{
    if (ip == Both)
    {
        int result4 = createChain(IPv4, chain, tableName);
        int result6 = createChain(IPv6, chain, tableName);
        return result4 ? result4 : result6;
    }
    const QString cmd = getCommand(ip);
    return execute(QStringLiteral("%1 -N %2 -t %3 || %1 -F %2 -t %3").arg(cmd, chain, tableName));
}

int LinuxFirewall::deleteChain(LinuxFirewall::IPVersion ip, const QString& chain, const QString& tableName)
{
    if (ip == Both)
    {
        int result4 = deleteChain(IPv4, chain, tableName);
        int result6 = deleteChain(IPv6, chain, tableName);
        return result4 ? result4 : result6;
    }
    const QString cmd = getCommand(ip);
    return execute(QStringLiteral("if %1 -L %2 -n -t %3 > /dev/null 2> /dev/null ; then %1 -F %2 -t %3 && %1 -X %2 -t %3; fi").arg(cmd, chain, tableName));
}

int LinuxFirewall::linkChain(LinuxFirewall::IPVersion ip, const QString& chain, const QString& parent, bool mustBeFirst, const QString& tableName)
{
    if (ip == Both)
    {
        int result4 = linkChain(IPv4, chain, parent, mustBeFirst, tableName);
        int result6 = linkChain(IPv6, chain, parent, mustBeFirst, tableName);
        return result4 ? result4 : result6;
    }
    const QString cmd = getCommand(ip);
    if (mustBeFirst)
    {
        // This monster shell script does the following:
        // 1. Check if a rule with the appropriate target exists at the top of the parent chain
        // 2. If not, insert a jump rule at the top of the parent chain
        // 3. Look for and delete a single rule with the designated target at an index > 1
        //    (we can't safely delete all rules at once since rule numbers change)
        // TODO: occasionally this script results in warnings in logs "Bad rule (does a matching rule exist in the chain?)" - this happens when
        // the e.g OUTPUT chain is empty but this script attempts to delete things from it anyway. It doesn't cause any problems, but we should still fix at some point..
        return execute(QStringLiteral("if ! %1 -L %2 -n --line-numbers -t %4 2> /dev/null | awk 'int($1) == 1 && $2 == \"%3\" { found=1 } END { if(found==1) { exit 0 } else { exit 1 } }' ; then %1 -I %2 -j %3 -t %4 && %1 -L %2 -n --line-numbers -t %4 2> /dev/null | awk 'int($1) > 1 && $2 == \"%3\" { print $1; exit }' | xargs %1 -t %4 -D %2 ; fi").arg(cmd, parent, chain, tableName));
    }
    else
        return execute(QStringLiteral("if ! %1 -C %2 -j %3 -t %4 2> /dev/null ; then %1 -A %2 -j %3 -t %4; fi").arg(cmd, parent, chain, tableName));
}

int LinuxFirewall::unlinkChain(LinuxFirewall::IPVersion ip, const QString& chain, const QString& parent, const QString& tableName)
{
    if (ip == Both)
    {
        int result4 = unlinkChain(IPv4, chain, parent, tableName);
        int result6 = unlinkChain(IPv6, chain, parent, tableName);
        return result4 ? result4 : result6;
    }
    const QString cmd = getCommand(ip);
    return execute(QStringLiteral("if %1 -C %2 -j %3 -t %4 2> /dev/null ; then %1 -D %2 -j %3 -t %4; fi").arg(cmd, parent, chain, tableName));
}

void LinuxFirewall::ensureRootAnchorPriority(LinuxFirewall::IPVersion ip)
{
    linkChain(ip, kRootChain, kOutputChain, true);
}

void LinuxFirewall::installAnchor(LinuxFirewall::IPVersion ip, const QString& anchor, const QStringList& rules, const QString& tableName,
                                     const FilterCallbackFunc& enableFunc, const FilterCallbackFunc& disableFunc)
{
    if (ip == Both)
    {
        installAnchor(IPv4, anchor, rules, tableName, enableFunc, disableFunc);
        installAnchor(IPv6, anchor, rules, tableName, enableFunc, disableFunc);
        return;
    }

    const QString cmd = getCommand(ip);
    const QString anchorChain = QStringLiteral("%1.a.%2").arg(kAnchorName, anchor);
    const QString actualChain = QStringLiteral("%1.%2").arg(kAnchorName, anchor);

    // Start by defining a placeholder chain, which stays locked into place
    // in the root chain without being removed or recreated, ensuring the
    // intended precedence order.
    createChain(ip, anchorChain, tableName);
    linkChain(ip, anchorChain, kRootChain, false, tableName);

    if(enableFunc)
    {
        const QString key = enabledKeyTemplate.arg(tableName, anchor);
        if(!anchorCallbacks.contains(key)) anchorCallbacks[key] = enableFunc;
    }
    if(disableFunc)
    {
        const QString key = disabledKeyTemplate.arg(tableName, anchor);
        if(!anchorCallbacks.contains(key)) anchorCallbacks[key] = disableFunc;
    }

    // Create the actual rule chain, which we'll insert or remove from the
    // placeholder anchor when needed.
    createChain(ip, actualChain, tableName);
    for (const QString& rule : rules)
        execute(QStringLiteral("%1 -A %2 %3 -t %4").arg(cmd, actualChain, rule, tableName));
}

void LinuxFirewall::uninstallAnchor(LinuxFirewall::IPVersion ip, const QString& anchor, const QString& tableName)
{
    if (ip == Both)
    {
        uninstallAnchor(IPv4, anchor, tableName);
        uninstallAnchor(IPv6, anchor, tableName);
        return;
    }

    const QString cmd = getCommand(ip);
    const QString anchorChain = QStringLiteral("%1.a.%2").arg(kAnchorName, anchor);
    const QString actualChain = QStringLiteral("%1.%2").arg(kAnchorName, anchor);

    unlinkChain(ip, anchorChain, kRootChain, tableName);
    deleteChain(ip, anchorChain, tableName);
    deleteChain(ip, actualChain, tableName);
}

QStringList LinuxFirewall::getDNSRules(const QStringList& servers)
{
    QStringList result;
    for (const QString& server : servers)
    {
        result << QStringLiteral("-o amn0+ -d %1 -p udp --dport 53 -j ACCEPT").arg(server);
        result << QStringLiteral("-o amn0+ -d %1 -p tcp --dport 53 -j ACCEPT").arg(server);
        result << QStringLiteral("-o tun0+ -d %1 -p udp --dport 53 -j ACCEPT").arg(server);
        result << QStringLiteral("-o tun0+ -d %1 -p tcp --dport 53 -j ACCEPT").arg(server);
        result << QStringLiteral("-o tun2+ -d %1 -p udp --dport 53 -j ACCEPT").arg(server);
        result << QStringLiteral("-o tun2+ -d %1 -p tcp --dport 53 -j ACCEPT").arg(server);
    }
    return result;
}

QStringList LinuxFirewall::getAllowRule(const QStringList& servers)
{
    QStringList result;
    for (const QString& server : servers)
    {
        result << QStringLiteral("-d %1 -j ACCEPT").arg(server);
    }
    return result;
}

QStringList LinuxFirewall::getBlockRule(const QStringList& servers)
{
    QStringList result;
    for (const QString& server : servers)
    {
        result << QStringLiteral("-d %1 -j REJECT").arg(server);
    }
    return result;
}


void LinuxFirewall::install()
{
    // Clean up any existing rules if they exist.
    uninstall();

    // Create a root filter chain to hold all our other anchors in order.
    createChain(Both, kRootChain, kFilterTable);

    // Create a root raw chain
    createChain(Both, kRootChain, kRawTable);

    // Create a root NAT chain
    createChain(Both, kRootChain, kNatTable);

    // Create a root Mangle chain
    createChain(Both, kRootChain, kMangleTable);

    // Install our filter rulesets in each corresponding anchor chain.
    installAnchor(Both, QStringLiteral("000.allowLoopback"), {
                                                                 QStringLiteral("-o lo+ -j ACCEPT"),
                                                             });

    installAnchor(IPv4, QStringLiteral("320.allowDNS"), {});

    installAnchor(Both, QStringLiteral("310.blockDNS"), {
                                                            QStringLiteral("-p udp --dport 53 -j REJECT"),
                                                            QStringLiteral("-p tcp --dport 53 -j REJECT"),
                                                        });
    installAnchor(IPv4, QStringLiteral("300.allowLAN"), {
                                                            QStringLiteral("-d 10.0.0.0/8 -j ACCEPT"),
                                                            QStringLiteral("-d 169.254.0.0/16 -j ACCEPT"),
                                                            QStringLiteral("-d 172.16.0.0/12 -j ACCEPT"),
                                                            QStringLiteral("-d 192.168.0.0/16 -j ACCEPT"),
                                                            QStringLiteral("-d 224.0.0.0/4 -j ACCEPT"),
                                                            QStringLiteral("-d 255.255.255.255/32 -j ACCEPT"),
                                                        });
    installAnchor(IPv6, QStringLiteral("300.allowLAN"), {
                                                            QStringLiteral("-d fc00::/7 -j ACCEPT"),
                                                            QStringLiteral("-d fe80::/10 -j ACCEPT"),
                                                            QStringLiteral("-d ff00::/8 -j ACCEPT"),
                                                        });


    installAnchor(IPv4, QStringLiteral("290.allowDHCP"), {
                                                             QStringLiteral("-p udp -d 255.255.255.255 --sport 68 --dport 67 -j ACCEPT"),
                                                         });
    installAnchor(IPv6, QStringLiteral("290.allowDHCP"), {
                                                             QStringLiteral("-p udp -d ff00::/8 --sport 546 --dport 547 -j ACCEPT"),
                                                         });
    installAnchor(IPv6, QStringLiteral("250.blockIPv6"), {
                                                             QStringLiteral("! -o lo+ -j REJECT"),
                                                         });

    installAnchor(Both, QStringLiteral("200.allowVPN"), {
                                                            QStringLiteral("-o amn0+ -j ACCEPT"),
                                                            QStringLiteral("-o tun0+ -j ACCEPT"),
                                                            QStringLiteral("-o tun2+ -j ACCEPT"),
                                                        });

    installAnchor(IPv4, QStringLiteral("120.blockNets"), {});

    installAnchor(IPv4, QStringLiteral("110.allowNets"), {});

    installAnchor(Both, QStringLiteral("100.blockAll"), {
                                                            QStringLiteral("-j REJECT"),
                                                        });
    // NAT rules
    installAnchor(Both, QStringLiteral("100.transIp"), {

                                                           // Only need the original interface, not the IP.
                                                           // The interface should remain much more stable/unchangeable than the IP
                                                           // (IP can change when changing networks, but interface only changes if adding/removing NICs)
                                                           // this is just a stub rule - the real rule is set at run-time
                                                           // and updates dynamically (via replaceAnchor) when our interface changes
                                                           // it'll take this form: "-o <interface name> -j MASQUERADE"
                                                           QStringLiteral("-j MASQUERADE")
                                                       }, kNatTable);

    // Mangle rules
    installAnchor(Both, QStringLiteral("100.tagPkts"), {
                                                           QStringLiteral("-m cgroup --cgroup %1 -j MARK --set-mark %2").arg(kCGroupId, kPacketTag)
                                                       }, kMangleTable, setupTrafficSplitting, teardownTrafficSplitting);

    // A rule to mitigate CVE-2019-14899 - drop packets addressed to the local
    // VPN IP but that are not actually received on the VPN interface.
    // See here: https://seclists.org/oss-sec/2019/q4/122
    installAnchor(Both, QStringLiteral("100.vpnTunOnly"), {
                                                              // To be replaced at runtime
                                                              QStringLiteral("-j ACCEPT")
                                                          }, kRawTable);


    // Insert our fitler root chain at the top of the OUTPUT chain.
    linkChain(Both, kRootChain, kOutputChain, true, kFilterTable);

    // Insert our NAT root chain at the top of the POSTROUTING chain.
    linkChain(Both, kRootChain, kPostRoutingChain, true, kNatTable);

    // Insert our Mangle root chain at the top of the OUTPUT chain.
    linkChain(Both, kRootChain, kOutputChain, true, kMangleTable);

    // Insert our Raw root chain at the top of the PREROUTING chain.
    linkChain(Both, kRootChain, kPreRoutingChain, true, kRawTable);

    setupTrafficSplitting();
}

void LinuxFirewall::uninstall()
{
    // Filter chain
    unlinkChain(Both, kRootChain, kOutputChain, kFilterTable);
    deleteChain(Both, kRootChain, kFilterTable);

    // Raw chain
    unlinkChain(Both, kRootChain, kPreRoutingChain, kRawTable);
    deleteChain(Both, kRootChain, kRawTable);

    // NAT chain
    unlinkChain(Both, kRootChain, kPostRoutingChain, kNatTable);
    deleteChain(Both, kRootChain, kNatTable);

    // Mangle chain
    unlinkChain(Both, kRootChain, kOutputChain, kMangleTable);
    deleteChain(Both, kRootChain, kMangleTable);

    // Remove filter anchors
    uninstallAnchor(Both, QStringLiteral("000.allowLoopback"));
    uninstallAnchor(Both, QStringLiteral("400.allowPIA"));
    uninstallAnchor(IPv4, QStringLiteral("320.allowDNS"));
    uninstallAnchor(Both, QStringLiteral("310.blockDNS"));
    uninstallAnchor(Both, QStringLiteral("300.allowLAN"));
    uninstallAnchor(Both, QStringLiteral("290.allowDHCP"));
    uninstallAnchor(IPv6, QStringLiteral("250.blockIPv6"));
    uninstallAnchor(Both, QStringLiteral("200.allowVPN"));
    uninstallAnchor(IPv4, QStringLiteral("120.blockNets"));
    uninstallAnchor(IPv4, QStringLiteral("110.allowNets"));
    uninstallAnchor(Both, QStringLiteral("100.blockAll"));

    // Remove Nat anchors
    uninstallAnchor(Both, QStringLiteral("100.transIp"), kNatTable);

    // Remove Mangle anchors
    uninstallAnchor(Both, QStringLiteral("100.tagPkts"), kMangleTable);

    // Remove Raw anchors
    uninstallAnchor(Both, QStringLiteral("100.vpnTunOnly"), kRawTable);

    teardownTrafficSplitting();

    logger.debug() << "LinuxFirewall::uninstall() complete";
}

bool LinuxFirewall::isInstalled()
{
    return execute(QStringLiteral("iptables -C %1 -j %2 2> /dev/null").arg(kOutputChain, kRootChain)) == 0;
}

void LinuxFirewall::enableAnchor(LinuxFirewall::IPVersion ip, const QString &anchor, const QString& tableName)
{
    if (ip == Both)
    {
        enableAnchor(IPv4, anchor, tableName);
        enableAnchor(IPv6, anchor, tableName);
        return;
    }
    const QString cmd = getCommand(ip);
    const QString ipStr = ip == IPv6 ? QStringLiteral("(IPv6)") : QStringLiteral("(IPv4)");

    execute(QStringLiteral("if %1 -C %5.a.%2 -j %5.%2 -t %4 2> /dev/null ; then echo '%2%3: ON' ; else echo '%2%3: OFF -> ON' ; %1 -A %5.a.%2 -j %5.%2 -t %4; fi").arg(cmd, anchor, ipStr, tableName, kAnchorName));
}

void LinuxFirewall::replaceAnchor(LinuxFirewall::IPVersion ip, const QString &anchor, const QString &newRule, const QString& tableName)
{
    if (ip == Both)
    {
        replaceAnchor(IPv4, anchor, newRule, tableName);
        replaceAnchor(IPv6, anchor, newRule, tableName);
        return;
    }
    const QString cmd = getCommand(ip);
    const QString ipStr = ip == IPv6 ? QStringLiteral("(IPv6)") : QStringLiteral("(IPv4)");

    execute(QStringLiteral("%1 -R %7.%2 1 %3 -t %4 ; echo 'Replaced rule %7.%2 %5 with %6'").arg(cmd, anchor, newRule, tableName, ipStr, newRule, kAnchorName));
}

void LinuxFirewall::disableAnchor(LinuxFirewall::IPVersion ip, const QString &anchor, const QString& tableName)
{
    if (ip == Both)
    {
        disableAnchor(IPv4, anchor, tableName);
        disableAnchor(IPv6, anchor, tableName);
        return;
    }
    const QString cmd = getCommand(ip);
    const QString ipStr = ip == IPv6 ? QStringLiteral("(IPv6)") : QStringLiteral("(IPv4)");
    execute(QStringLiteral("if ! %1 -C %5.a.%2 -j %5.%2 -t %4 2> /dev/null ; then echo '%2%3: OFF' ; else echo '%2%3: ON -> OFF' ; %1 -F %5.a.%2 -t %4; fi").arg(cmd, anchor, ipStr, tableName, kAnchorName));
}

bool LinuxFirewall::isAnchorEnabled(LinuxFirewall::IPVersion ip, const QString &anchor, const QString& tableName)
{
    const QString cmd = getCommand(ip);
    return execute(QStringLiteral("%1 -C %4.a.%2 -j %4.%2 -t %3 2> /dev/null").arg(cmd, anchor, tableName, kAnchorName)) == 0;
}

void LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPVersion ip, const QString &anchor, bool enabled, const QString &tableName)
{
    if (enabled)
    {
        enableAnchor(ip, anchor, tableName);
        const QString key = enabledKeyTemplate.arg(tableName, anchor);
        if(anchorCallbacks.contains(key)) anchorCallbacks[key]();
    }
    else
    {
        disableAnchor(ip, anchor, tableName);
        const QString key = disabledKeyTemplate.arg(tableName, anchor);
        if(anchorCallbacks.contains(key)) anchorCallbacks[key]();
    }
}

void LinuxFirewall::updateDNSServers(const QStringList& servers)
{
    static QStringList existingServers {};

    existingServers = servers;
    execute(QStringLiteral("iptables -F %1.320.allowDNS").arg(kAnchorName));
    for (const QString& rule : getDNSRules(servers))
        execute(QStringLiteral("iptables -A %1.320.allowDNS %2").arg(kAnchorName, rule));
}

void LinuxFirewall::updateAllowNets(const QStringList& servers)
{
    static QStringList existingServers {};

    existingServers = servers;
    execute(QStringLiteral("iptables -F %1.110.allowNets").arg(kAnchorName));
    for (const QString& rule : getAllowRule(servers))
        execute(QStringLiteral("iptables -A %1.110.allowNets %2").arg(kAnchorName, rule));
}

void LinuxFirewall::updateBlockNets(const QStringList& servers)
{
    static QStringList existingServers {};

    existingServers = servers;
    execute(QStringLiteral("iptables -F %1.120.blockNets").arg(kAnchorName));
    for (const QString& rule : getBlockRule(servers))
        execute(QStringLiteral("iptables -A %1.120.blockNets %2").arg(kAnchorName, rule));
}

int waitForExitCode(QProcess& process)
{
    if (!process.waitForFinished() || process.error() == QProcess::FailedToStart)
        return -2;
    else if (process.exitStatus() != QProcess::NormalExit)
        return -1;
    else
        return process.exitCode();
}

int LinuxFirewall::execute(const QString &command, bool ignoreErrors)
{
    QProcess p;
    p.start(QStringLiteral("/bin/bash"), {QStringLiteral("-c"), command}, QProcess::ReadOnly);
    p.closeWriteChannel();

    int exitCode = waitForExitCode(p);
    auto out = p.readAllStandardOutput().trimmed();
    auto err = p.readAllStandardError().trimmed();
    if ((exitCode != 0 || !err.isEmpty()) && !ignoreErrors)
        logger.warning()  << "(" << exitCode << ") $ " << command;
    else if (false)
        logger.debug() << "(" << exitCode << ") $ " << command;
    if (!out.isEmpty())
        logger.info() << out;
    if (!err.isEmpty())
        logger.warning() << err;
    return exitCode;
}

void LinuxFirewall::setupTrafficSplitting()
{
    auto cGroupDir = "/sys/fs/cgroup/net_cls/" BRAND_CODE "vpnexclusions/";
    logger.info() << "Should be setting up cgroup in" << cGroupDir << "for traffic splitting";
    execute(QStringLiteral("if [ ! -d %1 ] ; then mkdir %1 ; sleep 0.1 ; echo %2 > %1/net_cls.classid ; fi").arg(cGroupDir).arg(kCGroupId));
    // Set a rule with priority 100 (lower priority than local but higher than main/default, 0 is highest priority)
    execute(QStringLiteral("if ! ip rule list | grep -q %1 ; then ip rule add from all fwmark %1 lookup %2 pri 100 ; fi").arg(kPacketTag, kRtableName));
}

void LinuxFirewall::teardownTrafficSplitting()
{
    logger.info() << "Tearing down cgroup and routing rules";
    execute(QStringLiteral("if ip rule list | grep -q %1; then ip rule del from all fwmark %1 lookup %2 2> /dev/null ; fi").arg(kPacketTag, kRtableName));
    execute(QStringLiteral("ip route flush table %1").arg(kRtableName));
    execute(QStringLiteral("ip route flush cache"));
}
