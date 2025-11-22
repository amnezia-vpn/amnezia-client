#ifndef SITESCONTROLLER_H
#define SITESCONTROLLER_H

#include <QVector>
#include <QMap>
#include <QPair>

#include "settings.h"

class SitesController
{
public:
    explicit SitesController(std::shared_ptr<Settings> settings);

    bool addSite(const QString &hostname, const QString &ip);
    void addSites(const QMap<QString, QString> &sites, bool replaceExisting);
    void removeSite(const QString &hostname);
    void removeSites();
    void setRouteMode(int routeMode);
    void toggleSplitTunneling(bool enabled);

    int getRouteMode() const;
    bool isSplitTunnelingEnabled() const;
    QVector<QPair<QString, QString>> getCurrentSites() const;

private:
    void fillSites();

    std::shared_ptr<Settings> m_settings;
    Settings::RouteMode m_currentRouteMode;
    QVector<QPair<QString, QString>> m_sites;
};

#endif // SITESCONTROLLER_H

