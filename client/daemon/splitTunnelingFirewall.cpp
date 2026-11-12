#include "splitTunnelingFirewall.h"

#include <QHostAddress>
#include <QProcess>

#include "interfaceconfig.h"

namespace
{
    bool normalizeAddresses(const QStringList &addresses, QStringList &normalized)
    {
        for (const auto &address : addresses) {
            const auto subnet = QHostAddress::parseSubnet(address);
            if (subnet.first.protocol() != QAbstractSocket::IPv4Protocol || subnet.second < 0 || subnet.second > 32) {
                return false;
            }
            normalized.append(QStringLiteral("%1/%2").arg(subnet.first.toString()).arg(subnet.second));
        }
        normalized.removeDuplicates();
        return true;
    }

    SplitTunnelingFirewall::Command pfCommand(const QString &anchor, const QString &rules)
    {
        return { QStringLiteral("/sbin/pfctl"),
                 { QStringLiteral("-q"), QStringLiteral("-a"), QStringLiteral("amn/%1").arg(anchor),
                   QStringLiteral("-f"), QStringLiteral("-") },
                 rules.toUtf8() };
    }

    QList<SplitTunnelingFirewall::Command> macosCommands(bool blockAll, const QStringList &addresses)
    {
        const auto all = pfCommand(QStringLiteral("100.blockAll"),
                                   blockAll ? QStringLiteral("block out all flags any no state\n") : QString());
        const auto allow = pfCommand(
                QStringLiteral("110.allowNets"),
                blockAll && !addresses.isEmpty()
                        ? QStringLiteral("table <allownets> { %1 }\npass out to <allownets> flags any no state\n")
                                  .arg(addresses.join(QStringLiteral(", ")))
                        : QString());
        const auto block = pfCommand(
                QStringLiteral("120.blockNets"),
                !blockAll ? QStringLiteral("table <blocknets> { %1 }\nblock out to <blocknets> flags any no state\n")
                                    .arg(addresses.join(QStringLiteral(", ")))
                          : QString());
        return blockAll ? QList { allow, all, block } : QList { block, all, allow };
    }

    SplitTunnelingFirewall::Command linuxCommand(bool blockAll, const QStringList &addresses)
    {
        QString rules = QStringLiteral("*filter\n");
        for (const auto &chain :
             { "a.100.blockAll", "a.110.allowNets", "a.120.blockNets", "110.allowNets", "120.blockNets" }) {
            rules += QStringLiteral(":amnvpn.%1 - [0:0]\n").arg(QLatin1String(chain));
        }
        for (const auto &address : addresses) {
            rules += QStringLiteral("-A amnvpn.%1 -d %2 -j %3\n")
                             .arg(blockAll ? QStringLiteral("110.allowNets") : QStringLiteral("120.blockNets"), address,
                                  blockAll ? QStringLiteral("ACCEPT") : QStringLiteral("REJECT"));
        }
        if (blockAll) {
            rules += QStringLiteral("-A amnvpn.a.100.blockAll -j amnvpn.100.blockAll\n");
            if (!addresses.isEmpty()) {
                rules += QStringLiteral("-A amnvpn.a.110.allowNets -j amnvpn.110.allowNets\n");
            }
        } else {
            rules += QStringLiteral("-A amnvpn.a.120.blockNets -j amnvpn.120.blockNets\n");
        }
        rules += QStringLiteral("COMMIT\n");
        return { QStringLiteral("iptables-restore"),
                 { QStringLiteral("--noflush"), QStringLiteral("--wait"), QStringLiteral("5") },
                 rules.toUtf8() };
    }
}

SplitTunnelingUpdateResult SplitTunnelingFirewall::apply(Platform platform, const InterfaceConfig &config,
                                                         const Executor &execute)
{
    if (!config.m_killSwitchEnabled) {
        return SplitTunnelingUpdateResult::Applied;
    }

    const bool blockAll = config.m_allowedIPAddressRanges.contains(IPAddress("0.0.0.0/0"));
    QStringList addresses;
    if (blockAll) {
        if (!normalizeAddresses(config.m_excludedAddresses, addresses)) {
            return SplitTunnelingUpdateResult::NotApplied;
        }
    } else {
        QStringList ranges;
        for (const auto &range : config.m_allowedIPAddressRanges) {
            ranges.append(range.toString());
        }
        if (!normalizeAddresses(ranges, addresses)) {
            return SplitTunnelingUpdateResult::NotApplied;
        }
    }

    const auto commands = platform == Platform::MacOS ? macosCommands(blockAll, addresses)
                                                      : QList { linuxCommand(blockAll, addresses) };
    bool changed = false;
    for (const auto &command : commands) {
        const auto result = execute(command);
        if (result != SplitTunnelingUpdateResult::Applied) {
            return changed ? SplitTunnelingUpdateResult::StateUnknown : result;
        }
        changed = true;
    }
    return SplitTunnelingUpdateResult::Applied;
}

SplitTunnelingUpdateResult SplitTunnelingFirewall::runCommand(const Command &command)
{
    QProcess process;
    process.start(command.program, command.arguments);
    if (!process.waitForStarted(1000)) {
        return process.error() == QProcess::FailedToStart ? SplitTunnelingUpdateResult::NotApplied
                                                          : SplitTunnelingUpdateResult::StateUnknown;
    }
    if (process.write(command.input) != command.input.size()) {
        process.kill();
        process.waitForFinished();
        return SplitTunnelingUpdateResult::StateUnknown;
    }
    process.closeWriteChannel();
    if (!process.waitForFinished(7000)) {
        process.kill();
        process.waitForFinished();
        return SplitTunnelingUpdateResult::StateUnknown;
    }
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0
            ? SplitTunnelingUpdateResult::Applied
            : SplitTunnelingUpdateResult::StateUnknown;
}
