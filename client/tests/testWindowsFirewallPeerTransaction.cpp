#include <QSet>
#include <QTest>
#include <stdexcept>

#include "platforms/windows/daemon/windowsfirewallpeertransaction.h"

class TestFirewallOperations
{
public:
    using Transaction = WindowsFirewallPeerTransaction;
    Transaction::PeerRules rules { { "peer", 10 }, { "peer", 11 }, { "other-peer", 20 } };
    QSet<quint64> activeFilters { 10, 11, 20 };
    QSet<quint64> pendingFilters;
    QString failure;
    bool failAbort = false;
    int beginCalls = 0;
    int removalCalls = 0;
    int abortCalls = 0;
    int disableCalls = 0;
    int resetCalls = 0;

    Transaction::Operations operations()
    {
        Transaction::Operations operations;
        operations.begin = [this]() {
            ++beginCalls;
            if (failure == "begin") {
                return false;
            }
            pendingFilters = activeFilters;
            return true;
        };
        operations.removeFilter = [this](quint64 filterId) {
            ++removalCalls;
            if (failure == "delete" && removalCalls == 2) {
                return false;
            }
            pendingFilters.remove(filterId);
            return true;
        };
        operations.writeFilters = [this]() {
            pendingFilters.insert(30);
            rules.insert("peer", 30);
            if (failure == "write") {
                return false;
            }
            pendingFilters.insert(31);
            rules.insert("peer", 31);
            return true;
        };
        operations.commit = [this]() {
            if (failure == "commit") {
                return false;
            }
            activeFilters = pendingFilters;
            return true;
        };
        operations.abort = [this]() {
            ++abortCalls;
            if (failAbort) {
                return false;
            }
            pendingFilters.clear();
            return true;
        };
        operations.resetTransaction = [this]() {
            ++resetCalls;
            return failure != "reset";
        };
        operations.removeFilterIfPresent = operations.removeFilter;
        operations.disableKillSwitch = [this]() {
            ++disableCalls;
            activeFilters.clear();
            rules.clear();
        };
        return operations;
    }
};

class TestWindowsFirewallPeerTransaction : public QObject
{
    Q_OBJECT
    using Transaction = WindowsFirewallPeerTransaction;

private slots:
    void initialEnableKeepsTheExistingSuccessBehavior()
    {
        TestFirewallOperations firewall;
        QVERIFY(Transaction::enable(firewall.operations()));
        QCOMPARE(firewall.activeFilters, (QSet<quint64> { 10, 11, 20, 30, 31 }));
        QCOMPARE(firewall.removalCalls, 0);
        QCOMPARE(firewall.abortCalls, 0);
        QCOMPARE(firewall.disableCalls, 0);
    }

    void initialEnableKeepsTheExistingFailureBehavior_data()
    {
        QTest::addColumn<QString>("failure");
        QTest::addColumn<bool>("failAbort");
        QTest::newRow("begin") << QString("begin") << false;
        QTest::newRow("write") << QString("write") << false;
        QTest::newRow("commit") << QString("commit") << false;
        QTest::newRow("abort") << QString("write") << true;
    }

    void initialEnableKeepsTheExistingFailureBehavior()
    {
        QFETCH(QString, failure);
        QFETCH(bool, failAbort);
        TestFirewallOperations firewall;
        firewall.failure = failure;
        firewall.failAbort = failAbort;
        QVERIFY(!Transaction::enable(firewall.operations()));
        QCOMPARE(firewall.disableCalls, 1);
        QCOMPARE(firewall.abortCalls, failure == "begin" ? 0 : 1);
        QVERIFY(firewall.activeFilters.isEmpty());
        QVERIFY(firewall.rules.isEmpty());
    }

    void liveUpdateReplacesOnlyTheActivePeer()
    {
        TestFirewallOperations firewall;
        QCOMPARE(Transaction::replace("peer", firewall.rules, firewall.operations()), Transaction::Result::Committed);
        QCOMPARE(firewall.activeFilters, (QSet<quint64> { 20, 30, 31 }));
        QVERIFY(!firewall.rules.contains("peer", 10));
        QVERIFY(!firewall.rules.contains("peer", 11));
        QVERIFY(firewall.rules.contains("peer", 30));
        QVERIFY(firewall.rules.contains("peer", 31));
        QVERIFY(firewall.rules.contains("other-peer", 20));
        QCOMPARE(firewall.disableCalls, 0);
        QCOMPARE(firewall.abortCalls, 0);
    }

    void failedLiveUpdateKeepsCommittedFilters_data()
    {
        QTest::addColumn<QString>("failure");
        QTest::newRow("begin") << QString("begin");
        QTest::newRow("partial-delete") << QString("delete");
        QTest::newRow("partial-write") << QString("write");
        QTest::newRow("commit") << QString("commit");
    }

    void failedLiveUpdateKeepsCommittedFilters()
    {
        QFETCH(QString, failure);
        TestFirewallOperations firewall;
        const auto initialRules = firewall.rules;
        const auto initialFilters = firewall.activeFilters;
        firewall.failure = failure;
        QCOMPARE(Transaction::replace("peer", firewall.rules, firewall.operations()), Transaction::Result::Failed);
        QCOMPARE(firewall.rules, initialRules);
        QCOMPARE(firewall.activeFilters, initialFilters);
        QCOMPARE(firewall.disableCalls, 0);
        QCOMPARE(firewall.abortCalls, failure == "begin" ? 0 : 1);
    }

    void failedRollbackIsReportedAsAnUnknownState_data()
    {
        QTest::addColumn<QString>("failure");
        QTest::newRow("delete-and-abort") << QString("delete");
        QTest::newRow("write-and-abort") << QString("write");
        QTest::newRow("commit-and-abort") << QString("commit");
    }

    void failedRollbackIsReportedAsAnUnknownState()
    {
        QFETCH(QString, failure);
        TestFirewallOperations firewall;
        firewall.failure = failure;
        firewall.failAbort = true;
        QCOMPARE(Transaction::replace("peer", firewall.rules, firewall.operations()),
                 Transaction::Result::RollbackFailed);
        QCOMPARE(firewall.disableCalls, 0);
        QCOMPARE(firewall.abortCalls, 1);
        QVERIFY(firewall.rules.contains("peer", 10));
        QVERIFY(firewall.rules.contains("peer", 11));
        QVERIFY(firewall.rules.contains("other-peer", 20));
        if (failure != "delete") {
            QVERIFY(firewall.rules.contains("peer", 30));
        }
    }

    void initialEnableReleasesTransactionOnException()
    {
        TestFirewallOperations firewall;
        auto operations = firewall.operations();
        operations.writeFilters = []() -> bool { throw std::runtime_error("filter failure"); };
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, Transaction::enable(operations));
        QCOMPARE(firewall.abortCalls, 1);
        QCOMPARE(firewall.disableCalls, 1);
    }

    void liveUpdateReleasesTransactionOnException()
    {
        TestFirewallOperations firewall;
        const auto initialRules = firewall.rules;
        auto operations = firewall.operations();
        operations.writeFilters = []() -> bool { throw std::runtime_error("filter failure"); };
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, Transaction::replace("peer", firewall.rules, operations));
        QCOMPARE(firewall.abortCalls, 1);
        QCOMPARE(firewall.disableCalls, 0);
        QCOMPARE(firewall.rules, initialRules);
    }

    void missingPeerCannotBeTreatedAsALiveUpdate()
    {
        TestFirewallOperations firewall;
        const auto initialRules = firewall.rules;
        QCOMPARE(Transaction::replace("missing-peer", firewall.rules, firewall.operations()),
                 Transaction::Result::Failed);
        QCOMPARE(firewall.rules, initialRules);
        QCOMPARE(firewall.beginCalls, 0);
        QCOMPARE(firewall.disableCalls, 0);
    }
    void verifiedCleanupRestoresHotUpdatesAfterFailedAbort()
    {
        TestFirewallOperations firewall;
        firewall.failure = "write";
        firewall.failAbort = true;
        QCOMPARE(Transaction::replace("peer", firewall.rules, firewall.operations()),
                 Transaction::Result::RollbackFailed);
        bool synchronized = false;
        QList<quint64> activeRules { 90 };
        firewall.activeFilters.insert(90);
        firewall.failure.clear();
        firewall.failAbort = false;
        QVERIFY(Transaction::clear(firewall.rules, activeRules, synchronized, firewall.operations()));
        QVERIFY(synchronized);
        QVERIFY(firewall.rules.isEmpty());
        QVERIFY(firewall.activeFilters.isEmpty());
        QVERIFY(activeRules.isEmpty());
        QCOMPARE(firewall.disableCalls, 0);
        QVERIFY(Transaction::enable(firewall.operations()));
        QCOMPARE(Transaction::replace("peer", firewall.rules, firewall.operations()), Transaction::Result::Committed);
    }

    void failedCleanupNeverUnlocksHotUpdates_data()
    {
        QTest::addColumn<QString>("failure");
        for (const auto &failure : { "reset", "begin", "delete", "commit" }) {
            QTest::newRow(failure) << QString::fromLatin1(failure);
        }
    }

    void failedCleanupNeverUnlocksHotUpdates()
    {
        QFETCH(QString, failure);
        TestFirewallOperations firewall;
        bool synchronized = false;
        QList<quint64> activeRules { 90 };
        firewall.activeFilters.insert(90);
        const auto before = firewall.rules;
        const auto beforeActive = activeRules;
        firewall.failure = failure;
        QVERIFY(!Transaction::clear(firewall.rules, activeRules, synchronized, firewall.operations()));
        QVERIFY(!synchronized);
        QCOMPARE(firewall.rules, before);
        QCOMPARE(activeRules, beforeActive);
        QCOMPARE(firewall.disableCalls, 0);
        firewall.failure.clear();
        QVERIFY(Transaction::clear(firewall.rules, activeRules, synchronized, firewall.operations()));
        QVERIFY(synchronized);
    }

    void failedPeerRemovalRetainsFilterLedger()
    {
        TestFirewallOperations firewall;
        const auto before = firewall.rules;
        firewall.failure = "commit";
        QCOMPARE(Transaction::remove("peer", firewall.rules, firewall.operations()), Transaction::Result::Failed);
        QCOMPARE(firewall.rules, before);
        QCOMPARE(firewall.disableCalls, 0);
        firewall.failure.clear();
        QCOMPARE(Transaction::remove("peer", firewall.rules, firewall.operations()), Transaction::Result::Committed);
        QVERIFY(!firewall.rules.contains("peer"));
        QVERIFY(firewall.rules.contains("other-peer"));
    }
};

QTEST_GUILESS_MAIN(TestWindowsFirewallPeerTransaction)
#include "testWindowsFirewallPeerTransaction.moc"
