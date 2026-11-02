#ifndef ROUTEMUTATION_H
#define ROUTEMUTATION_H

#include <functional>

#include "core/splitTunneling/splitTunnelingUpdateResult.h"
#include "ipaddress.h"

enum class RouteMutationResult {
    Applied,
    Unchanged,
    NotApplied,
    StateUnknown
};

inline bool routeMutationSucceeded(RouteMutationResult result)
{
    return result == RouteMutationResult::Applied || result == RouteMutationResult::Unchanged;
}

struct RouteCreationResult
{
    SplitTunnelingUpdateResult result = SplitTunnelingUpdateResult::NotApplied;
    QList<IPAddress> created;
};

inline RouteCreationResult createTunnelRoutes(const IPAddress &prefix,
                                              const std::function<RouteMutationResult(const IPAddress &)> &create)
{
    QList<IPAddress> prefixes { prefix };
    if (prefix.prefixLength() == 0) {
        if (prefix.type() == QAbstractSocket::IPv4Protocol) {
            prefixes = { IPAddress("0.0.0.0/1"), IPAddress("128.0.0.0/1") };
        } else if (prefix.type() == QAbstractSocket::IPv6Protocol) {
            prefixes = { IPAddress("::/1"), IPAddress("8000::/1") };
        } else {
            return { };
        }
    }
    RouteCreationResult changes;
    for (const auto &route : prefixes) {
        const auto result = create(route);
        if (result != RouteMutationResult::Applied) {
            changes.result = result == RouteMutationResult::StateUnknown ? SplitTunnelingUpdateResult::StateUnknown
                                                                         : SplitTunnelingUpdateResult::NotApplied;
            return changes;
        }
        changes.created.append(route);
    }
    changes.result = SplitTunnelingUpdateResult::Applied;
    return changes;
}

#endif
