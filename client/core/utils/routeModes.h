#ifndef ROUTEMODES_H
#define ROUTEMODES_H

#include <QMetaEnum>
#include <QObject>

namespace amnezia
{
    namespace route_mode_ns
    {
        Q_NAMESPACE
        enum RouteMode {
            VpnAllSites,
            VpnOnlyForwardSites,
            VpnAllExceptSites
        };
        Q_ENUM_NS(RouteMode)
    }

    using RouteMode = route_mode_ns::RouteMode;

    namespace apps_route_mode_ns
    {
        Q_NAMESPACE
        enum AppsRouteMode {
            VpnAllApps,
            VpnOnlyForwardApps,
            VpnAllExceptApps
        };
        Q_ENUM_NS(AppsRouteMode)
    }

    using AppsRouteMode = apps_route_mode_ns::AppsRouteMode;
}

#endif // ROUTEMODES_H


