#ifndef SITESCONTROLLER_H
#define SITESCONTROLLER_H

#include <QVector>
#include <QMap>
#include <QPair>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/secureAppSettingsRepository.h"

using namespace amnezia;

class SitesController
{
public:
    explicit SitesController(SecureAppSettingsRepository* appSettingsRepository);

    bool addSite(const QString &hostname, const QString &ip);
    void addSites(const QMap<QString, QString> &sites, bool replaceExisting);
    void removeSite(const QString &hostname);
    void removeSites();
    void setRouteMode(RouteMode routeMode);
    void toggleSplitTunneling(bool enabled);

    RouteMode getRouteMode() const;
    bool isSplitTunnelingEnabled() const;
    QVector<QPair<QString, QString>> getCurrentSites() const;

private:
    void fillSites();

    SecureAppSettingsRepository* m_appSettingsRepository;
    RouteMode m_currentRouteMode;
    QVector<QPair<QString, QString>> m_sites;
};

#endif // SITESCONTROLLER_H

