#include "windowsfirewallpeertransaction.h"

#include <QScopeGuard>
#include <QSet>

bool WindowsFirewallPeerTransaction::enable(const Operations &operations)
{
    if (!operations.begin()) {
        operations.disableKillSwitch();
        return false;
    }
    auto cleanup = qScopeGuard([&]() {
        operations.abort();
        operations.disableKillSwitch();
    });
    if (!operations.writeFilters() || !operations.commit()) {
        return false;
    }
    cleanup.dismiss();
    return true;
}

WindowsFirewallPeerTransaction::Result WindowsFirewallPeerTransaction::replace(const QString &peer, PeerRules &rules,
                                                                               const Operations &operations)
{
    return changePeer(peer, rules, operations, true);
}

WindowsFirewallPeerTransaction::Result WindowsFirewallPeerTransaction::remove(const QString &peer, PeerRules &rules,
                                                                              const Operations &operations)
{
    return changePeer(peer, rules, operations, false);
}

WindowsFirewallPeerTransaction::Result WindowsFirewallPeerTransaction::changePeer(const QString &peer, PeerRules &rules,
                                                                                  const Operations &operations,
                                                                                  bool writeFilters)
{
    if (!rules.contains(peer)) {
        return writeFilters ? Result::Failed : Result::Committed;
    }
    if (!operations.begin()) {
        return Result::Failed;
    }
    const auto previous = rules;
    const auto rollback = [&]() {
        if (operations.abort()) {
            rules = previous;
            return Result::Failed;
        }
        for (auto it = previous.cbegin(); it != previous.cend(); ++it) {
            if (!rules.contains(it.key(), it.value())) {
                rules.insert(it.key(), it.value());
            }
        }
        return Result::RollbackFailed;
    };
    auto cleanup = qScopeGuard([&]() { rollback(); });
    bool removed = true;
    for (const auto filterId : previous.values(peer)) {
        if (!operations.removeFilter(filterId)) {
            removed = false;
            break;
        }
    }
    if (removed) {
        rules.remove(peer);
        if ((!writeFilters || operations.writeFilters()) && operations.commit()) {
            cleanup.dismiss();
            return Result::Committed;
        }
    }
    const auto result = rollback();
    cleanup.dismiss();
    return result;
}

bool WindowsFirewallPeerTransaction::clear(PeerRules &rules, QList<quint64> &activeRules, bool &synchronized,
                                           const Operations &operations)
{
    synchronized = false;
    if (!operations.resetTransaction()) {
        return false;
    }
    if (!operations.begin())
        return false;
    auto rollback = qScopeGuard([&] {
        if (!operations.abort())
            synchronized = false;
    });
    QSet<quint64> filters(activeRules.cbegin(), activeRules.cend());
    for (const auto filter : rules)
        filters.insert(filter);
    for (const auto filter : filters) {
        if (!operations.removeFilterIfPresent(filter))
            return false;
    }
    if (!operations.commit())
        return false;
    rollback.dismiss();
    rules.clear();
    activeRules.clear();
    synchronized = true;
    return true;
}
