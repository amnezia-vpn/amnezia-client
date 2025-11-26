#ifndef APPSPLITTUNNELINGCONTROLLER_H
#define APPSPLITTUNNELINGCONTROLLER_H

#include <QVector>

#include "core/defs.h"
#include "core/repositories/appSettingsRepository.h"

class AppSplitTunnelingController
{
public:
    explicit AppSplitTunnelingController(AppSettingsRepository* appSettingsRepository);

    bool addApp(const amnezia::InstalledAppInfo &appInfo);
    void removeApp(int index);
    void clearAppsList();
    void setRouteMode(AppsRouteMode routeMode);
    void toggleSplitTunneling(bool enabled);

    AppsRouteMode getRouteMode() const;
    bool isSplitTunnelingEnabled() const;
    QVector<amnezia::InstalledAppInfo> getApps() const;

private:
    AppSettingsRepository* m_appSettingsRepository;
    AppsRouteMode m_currentRouteMode;
    QVector<amnezia::InstalledAppInfo> m_apps;
};

#endif // APPSPLITTUNNELINGCONTROLLER_H

