#ifndef WINDOWSFIREWALLPEERTRANSACTION_H
#define WINDOWSFIREWALLPEERTRANSACTION_H

#include <QMultiMap>
#include <QString>
#include <functional>

class WindowsFirewallPeerTransaction
{
public:
    using PeerRules = QMultiMap<QString, quint64>;

    enum class Result {
        Committed,
        Failed,
        RollbackFailed
    };

    struct Operations
    {
        std::function<bool()> begin;
        std::function<bool()> writeFilters;
        std::function<bool()> commit;
        std::function<bool()> abort;
        std::function<bool(quint64)> removeFilter;
        std::function<void()> disableKillSwitch;
    };

    static bool enable(const Operations &operations);
    static Result replace(const QString &peer, PeerRules &rules, const Operations &operations);
};

#endif
