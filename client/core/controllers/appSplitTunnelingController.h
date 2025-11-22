#ifndef APPSPLITTUNNELINGCONTROLLER_H
#define APPSPLITTUNNELINGCONTROLLER_H

#include <QVector>

#include "core/defs.h"
#include "settings.h"

class AppSplitTunnelingController
{
public:
    explicit AppSplitTunnelingController(std::shared_ptr<Settings> settings);

    bool addApp(const amnezia::InstalledAppInfo &appInfo);
    void removeApp(int index);
    void clearAppsList();
    void setRouteMode(int routeMode);
    void toggleSplitTunneling(bool enabled);

    int getRouteMode() const;
    bool isSplitTunnelingEnabled() const;
    QVector<amnezia::InstalledAppInfo> getApps() const;

private:
    std::shared_ptr<Settings> m_settings;
    Settings::AppsRouteMode m_currentRouteMode;
    QVector<amnezia::InstalledAppInfo> m_apps;
};

#endif // APPSPLITTUNNELINGCONTROLLER_H

