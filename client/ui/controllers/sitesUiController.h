#ifndef SITESUICONTROLLER_H
#define SITESUICONTROLLER_H

#include <QObject>

#include "core/controllers/sitesController.h"
#include "ui/models/sitesModel.h"
#include "vpnConnection.h"

class SitesUiController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int routeMode READ getRouteMode WRITE setRouteMode NOTIFY routeModeChanged)
    Q_PROPERTY(bool isTunnelingEnabled READ isTunnelingEnabled NOTIFY isTunnelingEnabledChanged)

public:
    explicit SitesUiController(SitesController* sitesController,
                                VpnConnection* vpnConnection,
                                SitesModel* sitesModel, QObject *parent = nullptr);

public slots:
    void addSite(QString hostname);
    void removeSite(int index);
    void removeSites();
    void importSites(const QString &fileName, bool replaceExisting);
    void exportSites(const QString &fileName);
    void toggleSplitTunneling(bool enabled);
    void setRouteMode(int routeMode);

    int getRouteMode() const;
    bool isTunnelingEnabled() const;

    void updateModel();

signals:
    void routeModeChanged();
    void isTunnelingEnabledChanged();
    void errorOccurred(const QString &errorMessage);
    void finished(const QString &message);
    void saveFile(const QString &fileName, const QString &data);

private:
    SitesController* m_sitesController;
    VpnConnection* m_vpnConnection;
    SitesModel* m_sitesModel;
};

#endif // SITESUICONTROLLER_H
