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

#include "macosfirewall.h"
#include "logger.h"
#include <QProcess>
#include <QCoreApplication>

#define BRAND_IDENTIFIER "amn"

namespace {
    Logger logger("MacOSFirewall");
}  // namespace

#include "macosfirewall.h"

#define ResourceDir qApp->applicationDirPath() + "/pf"
#define DaemonDataDir qApp->applicationDirPath() + "/pf"

#include <QProcess>

static QString kRootAnchor = QStringLiteral(BRAND_IDENTIFIER);
static QByteArray kPfWarning = "pfctl: Use of -f option, could result in flushing of rules\npresent in the main ruleset added by the system at startup.\nSee /etc/pf.conf for further details.\n";

int waitForExitCode(QProcess& process)
{
    if (!process.waitForFinished() || process.error() == QProcess::FailedToStart)
        return -2;
    else if (process.exitStatus() != QProcess::NormalExit)
        return -1;
    else
        return process.exitCode();
}

int MacOSFirewall::execute(const QString& command, bool ignoreErrors)
{
    QProcess p;

    p.start(QStringLiteral("/bin/bash"), { QStringLiteral("-c"), command }, QProcess::ReadOnly);
    p.closeWriteChannel();
    int exitCode = waitForExitCode(p);
    auto out = p.readAllStandardOutput().trimmed();

    auto err = p.readAllStandardError().replace(kPfWarning, "").trimmed();
    if ((exitCode != 0 || !err.isEmpty()) && !ignoreErrors)
        logger.info() << "(" << exitCode << ") $ " << command;
    else if (false)
        logger.info() << "(" << exitCode << ") $ " << command;
    if (!out.isEmpty()) logger.info() << out;
    if (!err.isEmpty()) logger.info() << err;
    return exitCode;
}

void MacOSFirewall::installRootAnchors()
{
    logger.info() << "Installing PF root anchors";

    // Append our NAT anchors by reading back and re-applying NAT rules only
    auto insertNatAnchors = QStringLiteral(
        "( "
        R"(pfctl -sn | grep -v '%1/*'; )"   // Translation rules (includes both nat and rdr, despite the modifier being 'nat')
        R"(echo 'nat-anchor "%2/*"'; )"     // PIA's translation anchors
        R"(echo 'rdr-anchor "%3/*"'; )"
        R"(echo 'load anchor "%4" from "%5/%6.conf"'; )" // Load the PIA anchors from file
        ") | pfctl -N -f -").arg(kRootAnchor, kRootAnchor, kRootAnchor, kRootAnchor, ResourceDir, kRootAnchor);

    execute(insertNatAnchors);

    // Append our filter anchor by reading back and re-applying filter rules
    // only.  pfctl -sr also includes scrub rules, but these will be ignored
    // due to -R.
    auto insertFilterAnchor = QStringLiteral(
        "( "
        R"(pfctl -sr | grep -v '%1/*'; )"   // Filter rules (everything from pfctl -sr except 'scrub')
        R"(echo 'anchor "%2/*"'; )"         // PIA's filter anchors
        R"(echo 'load anchor "%3" from "%4/%5.conf"'; )" // Load the PIA anchors from file
        " ) | pfctl -R -f -").arg(kRootAnchor, kRootAnchor, kRootAnchor, ResourceDir, kRootAnchor);
    execute(insertFilterAnchor);
}

void MacOSFirewall::install()
{
    // remove hard-coded (legacy) pia anchor from /etc/pf.conf if it exists
    execute(QStringLiteral("if grep -Fq '%1' /etc/pf.conf ; then echo \"`cat /etc/pf.conf | grep -vF '%1'`\" > /etc/pf.conf ; fi").arg(kRootAnchor));

    // Clean up any existing rules if they exist.
    uninstall();

    timespec waitTime{0, 10'000'000};
    ::nanosleep(&waitTime, nullptr);

    logger.info() << "Installing PF root anchor";

    installRootAnchors();
    execute(QStringLiteral("pfctl -E 2>&1 | grep -F 'Token : ' | cut -c9- > '%1/pf.token'").arg(DaemonDataDir));
}


void MacOSFirewall::uninstall()
{
    logger.info() << "Uninstalling PF root anchor";

    execute(QStringLiteral("pfctl -q -a '%1' -F all").arg(kRootAnchor));
    execute(QStringLiteral("test -f '%1/pf.token' && pfctl -X `cat '%1/pf.token'` && rm '%1/pf.token'").arg(DaemonDataDir));
    execute(QStringLiteral("test -f /etc/pf.conf && pfctl -F all -f /etc/pf.conf"));
}

bool MacOSFirewall::isInstalled()
{
    return isPFEnabled() && isRootAnchorLoaded();
}

bool MacOSFirewall::isPFEnabled()
{
    return 0 == execute(QStringLiteral("test -s '%1/pf.token' && pfctl -s References | grep -qFf '%1/pf.token'").arg(DaemonDataDir), true);
}

void MacOSFirewall::ensureRootAnchorPriority()
{
    // We check whether our anchor appears last in the ruleset. If it does not, then remove it and re-add it last (this happens atomically).
    // Appearing last ensures priority.
    execute(QStringLiteral("if ! pfctl -sr | tail -1 | grep -qF '%1'; then echo -e \"$(pfctl -sr | grep -vF '%1')\\n\"'anchor \"%1\"' | pfctl -f - ; fi").arg(kRootAnchor));
}

bool MacOSFirewall::isRootAnchorLoaded()
{
    // Our Root anchor is loaded if:
    // 1. It is is included among the top-level anchors
    // 2. It is not empty (i.e it contains sub-anchors)
    return 0 == execute(QStringLiteral("pfctl -sr | grep -q '%1' && pfctl -q -a '%1' -s rules 2> /dev/null | grep -q .").arg(kRootAnchor), true);
}

void MacOSFirewall::enableAnchor(const QString& anchor)
{
    execute(QStringLiteral("if pfctl -q -a '%1/%2' -s rules 2> /dev/null | grep -q . ; then echo '%2: ON' ; else echo '%2: OFF -> ON' ; pfctl -q -a '%1/%2' -F all -f '%3/%1.%2.conf' ; fi").arg(kRootAnchor, anchor, ResourceDir));
}

void MacOSFirewall::disableAnchor(const QString& anchor)
{
    execute(QStringLiteral("if ! pfctl -q -a '%1/%2' -s rules 2> /dev/null | grep -q . ; then echo '%2: OFF' ; else echo '%2: ON -> OFF' ; pfctl -q -a '%1/%2' -F all ; fi").arg(kRootAnchor, anchor));
}

bool MacOSFirewall::isAnchorEnabled(const QString& anchor)
{
    return 0 == execute(QStringLiteral("pfctl -q -a '%1/%2' -s rules 2> /dev/null | grep -q .").arg(kRootAnchor, anchor), true);
}

void MacOSFirewall::setAnchorEnabled(const QString& anchor, bool enabled)
{
    if (enabled)
        enableAnchor(anchor);
    else
        disableAnchor(anchor);
}

void MacOSFirewall::setAnchorTable(const QString& anchor, bool enabled, const QString& table, const QStringList& items)
{
    if (enabled)
        execute(QStringLiteral("pfctl -q -a '%1/%2' -t '%3' -T replace %4").arg(kRootAnchor, anchor, table, items.join(' ')));
    else
        execute(QStringLiteral("pfctl -q -a '%1/%2' -t '%3' -T kill").arg(kRootAnchor, anchor, table), true);
}

void MacOSFirewall::setAnchorWithRules(const QString& anchor, bool enabled, const QStringList &ruleList)
{
    if (!enabled)
        return (void)execute(QStringLiteral("pfctl -q -a '%1/%2' -F rules").arg(kRootAnchor, anchor), true);
    else
        return (void)execute(QStringLiteral("echo -e \"%1\" | pfctl -q -a '%2/%3' -f -").arg(ruleList.join('\n'), kRootAnchor, anchor), true);
}
