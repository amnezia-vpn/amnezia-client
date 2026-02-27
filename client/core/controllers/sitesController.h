#ifndef SITESCONTROLLER_H
#define SITESCONTROLLER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QPair>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonArray>
#include <QHostInfo>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/secureAppSettingsRepository.h"

class VpnConnection;

using namespace amnezia;

class SitesController : public QObject
{
    Q_OBJECT

public:
    explicit SitesController(SecureAppSettingsRepository* appSettingsRepository, VpnConnection* vpnConnection = nullptr, QObject* parent = nullptr);

    bool addSite(const QString &hostname);
    void addSites(const QMap<QString, QString> &sites, bool replaceExisting);
    bool removeSite(const QString &hostname);
    void removeSites();
    void setRouteMode(RouteMode routeMode);
    void toggleSplitTunneling(bool enabled);

    RouteMode getRouteMode() const;
    bool isSplitTunnelingEnabled() const;
    QVector<QPair<QString, QString>> getCurrentSites() const;

    bool importSitesFromJson(const QByteArray& jsonData, bool replaceExisting, QString &errorMessage);
    QByteArray exportSitesToJson() const;

private slots:
    void onHostResolved(const QHostInfo &hostInfo);

private:
    void fillSites();
    bool addSiteInternal(const QString &hostname, const QString &ip);
    QString normalizeHostname(const QString &hostname) const;
    bool validateHostname(const QString &hostname) const;
    void processSiteAfterResolve(const QString &hostname, const QString &ip);
    void processSite(const QString &hostname, const QString &ip);

    SecureAppSettingsRepository* m_appSettingsRepository;
    VpnConnection* m_vpnConnection;
    RouteMode m_currentRouteMode;
    QVector<QPair<QString, QString>> m_sites;
};

#endif // SITESCONTROLLER_H

