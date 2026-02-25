#ifndef COMMONSTRUCTS_H
#define COMMONSTRUCTS_H

#include <QString>
#include "core/utils/routeModes.h"

namespace amnezia
{
    struct ServerCredentials
    {
        QString hostName;
        QString userName;
        QString secretData;
        int port = 22;

        bool isValid() const
        {
            return !hostName.isEmpty() && !userName.isEmpty() && !secretData.isEmpty() && port > 0;
        }
    };

    struct InstalledAppInfo {
        QString appName;
        QString packageName;
        QString appPath;

        bool operator==(const InstalledAppInfo& other) const {
            if (!packageName.isEmpty()) {
                return packageName == other.packageName;
            } else {
                return appPath == other.appPath;
            }
        }
    };

    struct DnsSettings
    {
        QString primaryDns;
        QString secondaryDns;
    };

    struct SplitTunnelingSettings
    {
        bool isSitesSplitTunnelingEnabled;
        RouteMode routeMode;
    };

    struct ConnectionSettings
    {
        DnsSettings dns;
        bool isApiConfig;
        SplitTunnelingSettings splitTunneling;
    };

    struct ExportSettings
    {
        DnsSettings dns;
    };
}

#endif // COMMONSTRUCTS_H


