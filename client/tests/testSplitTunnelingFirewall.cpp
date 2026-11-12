#include <QFile>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>
#include <algorithm>

#include "daemon/interfaceconfig.h"
#include "daemon/splitTunnelingFirewall.h"

namespace
{
    using Result = SplitTunnelingUpdateResult;
    using Platform = SplitTunnelingFirewall::Platform;

    struct FirewallState
    {
        bool blockAll = false;
        QSet<QString> allowed;
        QSet<QString> blocked;

        bool protects(const QString &address) const
        {
            const QHostAddress destination(address.section(QLatin1Char('/'), 0, 0));
            const auto matches = [&](const QSet<QString> &ranges) {
                return std::any_of(ranges.cbegin(), ranges.cend(),
                                   [&](const auto &range) { return IPAddress(range).contains(destination); });
            };
            return matches(blocked) || (blockAll && !matches(allowed));
        }

        void commit(const SplitTunnelingFirewall::Command &command)
        {
            const QString input = QString::fromUtf8(command.input);
            if (command.program == QStringLiteral("/sbin/pfctl")) {
                const auto anchor = command.arguments.at(2);
                if (anchor.endsWith(QStringLiteral("100.blockAll"))) {
                    blockAll = input.contains(QStringLiteral("block out all"));
                } else {
                    auto &addresses = anchor.endsWith(QStringLiteral("110.allowNets")) ? allowed : blocked;
                    addresses.clear();
                    const QRegularExpression table(QStringLiteral(R"(\{ ([^}]*) \})"));
                    const auto match = table.match(input);
                    for (const auto &item : match.captured(1).split(QStringLiteral(", "), Qt::SkipEmptyParts)) {
                        addresses.insert(item);
                    }
                }
            } else {
                FirewallState replacement;
                replacement.blockAll = input.contains(QStringLiteral("-A amnvpn.a.100.blockAll -j"));
                const QRegularExpression rule(QStringLiteral(R"(-A amnvpn\.(110.allowNets|120.blockNets) -d (\S+) -j)"));
                auto matches = rule.globalMatch(input);
                while (matches.hasNext()) {
                    const auto match = matches.next();
                    (match.captured(1) == QStringLiteral("110.allowNets") ? replacement.allowed : replacement.blocked)
                            .insert(match.captured(2));
                }
                *this = replacement;
            }
        }
    };

    InterfaceConfig configuration(bool fullTunnel, const QStringList &sites)
    {
        InterfaceConfig config;
        config.m_killSwitchEnabled = true;
        if (fullTunnel) {
            config.m_allowedIPAddressRanges = { IPAddress("0.0.0.0/0"), IPAddress("::/0") };
            config.m_excludedAddresses = sites;
        } else {
            for (const auto &site : sites) {
                config.m_allowedIPAddressRanges.append(IPAddress(site));
            }
        }
        return config;
    }

    FirewallState stateFor(bool fullTunnel, const QStringList &sites)
    {
        FirewallState state;
        state.blockAll = fullTunnel;
        auto &addresses = fullTunnel ? state.allowed : state.blocked;
        for (const auto &site : sites) {
            addresses.insert(site);
        }
        return state;
    }
}

class TestSplitTunnelingFirewall : public QObject
{
    Q_OBJECT

private slots:
    void transitionsPreserveProtection_data()
    {
        QTest::addColumn<bool>("macos");
        QTest::addColumn<bool>("oldFull");
        QTest::addColumn<QStringList>("oldSites");
        QTest::addColumn<bool>("newFull");
        QTest::addColumn<QStringList>("newSites");
        const QStringList a { "192.0.2.1/32" };
        const QStringList ab { "192.0.2.1/32", "198.51.100.9/32" };
        for (bool macos : { false, true }) {
            const QByteArray prefix = macos ? "macos-" : "linux-";
            QTest::newRow(prefix + "first-inclusion") << macos << true << QStringList { } << false << a;
            QTest::newRow(prefix + "first-inclusion-keeps-endpoint")
                    << macos << true << QStringList { "203.0.113.25/32" } << false << a;
            QTest::newRow(prefix + "subnet-addition") << macos << false << QStringList { "192.0.2.0/24" } << false
                                                      << QStringList { "192.0.2.0/24", "198.51.100.0/24" };
            QTest::newRow(prefix + "inclusion-addition") << macos << false << a << false << ab;
            QTest::newRow(prefix + "exclusion-addition") << macos << true << a << true << ab;
            QTest::newRow(prefix + "inclusion-rollback") << macos << false << ab << false << a;
            QTest::newRow(prefix + "first-inclusion-rollback") << macos << false << a << true << QStringList { };
            QTest::newRow(prefix + "exclusion-rollback") << macos << true << ab << true << a;
        }
    }

    void transitionsPreserveProtection()
    {
        QFETCH(bool, macos);
        QFETCH(bool, oldFull);
        QFETCH(QStringList, oldSites);
        QFETCH(bool, newFull);
        QFETCH(QStringList, newSites);
        const auto before = stateFor(oldFull, oldSites);
        const auto after = stateFor(newFull, newSites);
        const auto config = configuration(newFull, newSites);
        const auto platform = macos ? Platform::MacOS : Platform::Linux;
        const int commandCount = macos ? 3 : 1;
        const QStringList destinations { "192.0.2.1/32", "198.51.100.9/32", "203.0.113.25/32" };

        for (int failedCommand = -1; failedCommand < commandCount; ++failedCommand) {
            for (bool commitBeforeFailure : { false, true }) {
                auto state = before;
                int calls = 0;
                QList<FirewallState> trace { state };
                const auto result = SplitTunnelingFirewall::apply(platform, config, [&](const auto &command) {
                    const bool fail = calls++ == failedCommand;
                    if (!fail || commitBeforeFailure) {
                        state.commit(command);
                        trace.append(state);
                    }
                    return fail ? Result::StateUnknown : Result::Applied;
                });
                QCOMPARE(result, failedCommand < 0 ? Result::Applied : Result::StateUnknown);
                QCOMPARE(calls, failedCommand < 0 ? commandCount : failedCommand + 1);
                for (const auto &step : trace) {
                    for (const auto &address : destinations) {
                        if (before.protects(address) && after.protects(address)) {
                            QVERIFY2(step.protects(address), qPrintable(address));
                        }
                        if (!before.protects(address) && !after.protects(address)) {
                            QVERIFY2(!step.protects(address), qPrintable(address));
                        }
                    }
                }
                if (result == Result::Applied) {
                    for (const auto &address : destinations) {
                        QCOMPARE(state.protects(address), after.protects(address));
                    }
                }
            }
        }
    }

    void disabledKillSwitchDoesNotTouchFirewall()
    {
        auto config = configuration(false, { "192.0.2.1/32" });
        config.m_killSwitchEnabled = false;
        for (const auto platform : { Platform::MacOS, Platform::Linux }) {
            int calls = 0;
            QCOMPARE(SplitTunnelingFirewall::apply(platform, config,
                                                   [&](const auto &) {
                                                       ++calls;
                                                       return Result::StateUnknown;
                                                   }),
                     Result::Applied);
            QCOMPARE(calls, 0);
        }
    }

    void failedLaunchAfterPartialUpdateIsUnknown()
    {
        const auto config = configuration(false, { "192.0.2.1/32" });
        for (int failedCommand = 0; failedCommand < 3; ++failedCommand) {
            int calls = 0;
            QCOMPARE(SplitTunnelingFirewall::apply(Platform::MacOS, config,
                                                   [&](const auto &) {
                                                       return calls++ == failedCommand ? Result::NotApplied
                                                                                       : Result::Applied;
                                                   }),
                     failedCommand == 0 ? Result::NotApplied : Result::StateUnknown);
            QCOMPARE(calls, failedCommand + 1);
        }
    }

    void invalidExclusionsNeverReachTheCommand()
    {
        for (const auto &site : { "example.com", "192.0.2.1\nCOMMIT\n*filter", "0.0.0.0/33", "::1" }) {
            const auto config = configuration(true, { QLatin1String(site) });
            for (const auto platform : { Platform::MacOS, Platform::Linux }) {
                int calls = 0;
                QCOMPARE(SplitTunnelingFirewall::apply(platform, config,
                                                       [&](const auto &) {
                                                           ++calls;
                                                           return Result::Applied;
                                                       }),
                         Result::NotApplied);
                QCOMPARE(calls, 0);
            }
        }
    }

    void linuxOnlyReplacesSplitTunnelingChains()
    {
        const auto config = configuration(false, { "192.0.2.1/32", "198.51.100.9/32" });
        int calls = 0;
        SplitTunnelingFirewall::Command captured;
        QCOMPARE(SplitTunnelingFirewall::apply(Platform::Linux, config,
                                               [&](const auto &command) {
                                                   ++calls;
                                                   captured = command;
                                                   return Result::Applied;
                                               }),
                 Result::Applied);
        QCOMPARE(captured.program, QStringLiteral("iptables-restore"));
        QVERIFY(captured.arguments.contains(QStringLiteral("--noflush")));
        QVERIFY(captured.arguments.contains(QStringLiteral("--wait")));
        QVERIFY(captured.input.startsWith("*filter\n"));
        QVERIFY(captured.input.endsWith("COMMIT\n"));
        QVERIFY(!captured.input.contains("OUTPUT"));
        QVERIFY(!captured.input.contains("DNS"));
        QVERIFY(!captured.input.contains("allowVPN"));
        QVERIFY(!captured.input.contains("allowLAN"));
        QCOMPARE(calls, 1);
    }

    void processFailuresNeverReportSuccess()
    {
#ifdef Q_OS_UNIX
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto previousPath = qgetenv("PATH");
        const bool hadPath = qEnvironmentVariableIsSet("PATH");
        const auto restorePath = qScopeGuard([&] {
            if (hadPath) {
                qputenv("PATH", previousPath);
            } else {
                qunsetenv("PATH");
            }
        });
        qputenv("PATH", directory.path().toUtf8());
        const auto config = configuration(false, { "192.0.2.1/32" });
        QCOMPARE(SplitTunnelingFirewall::apply(Platform::Linux, config), Result::NotApplied);

        const QList<QPair<QByteArray, Result>> cases {
            { "while IFS= read -r line; do [ \"$line\" = COMMIT ] && exit 0; done\nexit 1\n", Result::Applied },
            { "exit 2\n", Result::StateUnknown },
            { "kill -KILL $$\n", Result::StateUnknown },
            { "exec /bin/sleep 20\n",
              Result::StateUnknown }
        };
        for (const auto &[script, expected] : cases) {
            QFile executable(directory.filePath(QStringLiteral("iptables-restore")));
            QVERIFY(executable.open(QIODevice::WriteOnly | QIODevice::Truncate));
            QVERIFY(executable.write("#!/bin/sh\n" + script) > 0);
            executable.close();
            QVERIFY(executable.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
            QCOMPARE(SplitTunnelingFirewall::apply(Platform::Linux, config), expected);
        }
#else
        QSKIP("The firewall command executor is used on Unix platforms");
#endif
    }
};

QTEST_GUILESS_MAIN(TestSplitTunnelingFirewall)
#include "testSplitTunnelingFirewall.moc"
