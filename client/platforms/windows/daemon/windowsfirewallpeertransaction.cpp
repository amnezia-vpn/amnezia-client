#include "windowsfirewallpeertransaction.h"

#include <QScopeGuard>

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
    if (!rules.contains(peer) || !operations.begin()) {
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
        if (operations.writeFilters() && operations.commit()) {
            cleanup.dismiss();
            return Result::Committed;
        }
    }
    const auto result = rollback();
    cleanup.dismiss();
    return result;
}
