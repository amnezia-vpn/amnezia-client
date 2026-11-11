#ifndef TUNNELSETTINGS_H
#define TUNNELSETTINGS_H

#include <QJsonObject>
#include <QStringList>

#include "core/utils/containerEnum.h"
#include "core/utils/routeModes.h"

class SecureAppSettingsRepository;

namespace amnezia
{
    enum class XtunProto {
        None,
        Awg,
        Xray
    };

    enum class TunnelStatus {
        Disconnected,
        Connected,
        NoNetwork
    };

    enum class KillSwitchMode {
        Disabled,
        Soft,
        Hard
    };

    struct TunnelSettings
    {
        XtunProto proto = XtunProto::None;
        QString config;
        QStringList dnsServers;
        KillSwitchMode killSwitchMode = KillSwitchMode::Disabled;
        QStringList allowedDnsServers;
        RouteMode sitesRouteMode = RouteMode::VpnAllSites;
        QStringList sites;
        AppsRouteMode appsRouteMode = AppsRouteMode::VpnAllApps;
        QStringList apps;
        int mtu = 0;

        QJsonObject toJson() const;
    };

    TunnelSettings tunnelSettings(const SecureAppSettingsRepository &appSettingsRepository, DockerContainer container,
                                  const QJsonObject &vpnConfiguration);
    KillSwitchMode killSwitchMode(const SecureAppSettingsRepository &appSettingsRepository, bool killSwitchEnabled);
}

#endif // TUNNELSETTINGS_H
