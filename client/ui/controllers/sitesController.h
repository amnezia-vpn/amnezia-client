#ifndef SITESCONTROLLER_H
#define SITESCONTROLLER_H

#include <QObject>

#include "settings.h"
#include "ui/models/sites_model.h"
#include "vpnconnection.h"

class SitesController : public QObject
{
    Q_OBJECT
public:
    explicit SitesController(const std::shared_ptr<Settings> &settings,
                             const QSharedPointer<VpnConnection> &vpnConnection,
                             const QSharedPointer<SitesModel> &sitesModel, QObject *parent = nullptr);

public slots:
    void addSite(QString hostname);
    void removeSite(int index);

    void importSites(bool replaceExisting);
    void exportSites();

signals:
    void errorOccurred(const QString &errorMessage);
    void finished(const QString &message);

private:
    std::shared_ptr<Settings> m_settings;

    QSharedPointer<VpnConnection> m_vpnConnection;
    QSharedPointer<SitesModel> m_sitesModel;
};

#endif // SITESCONTROLLER_H
