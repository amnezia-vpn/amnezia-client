#ifndef SITESCONTROLLER_H
#define SITESCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QVector>

#include "core/controllers/selfhosted/installController.h"
#include "core/repositories/secureServersRepository.h"
#include "core/utils/commonStructs.h"
#include "core/utils/routeModes.h"
#include "ui/controllers/serversUiController.h"
#include "ui/models/ipSplitTunnelingModel.h"

class SitesController : public QObject
{
    Q_OBJECT

public:
    explicit SitesController(SecureServersRepository *serversRepository,
                             ServersUiController *serversUiController,
                             InstallController *installController,
                             IpSplitTunnelingModel *managedExceptSitesModel,
                             QObject *parent = nullptr);

public slots:
    bool canEditManagedSites() const;
    bool isManagedSplitTunnelingForceEnabled() const;
    bool isDefaultManagedSplitTunnelingForceEnabled() const;
    void setManagedSplitTunnelingForceEnabled(bool enabled);

    void addManagedSite(int routeMode, const QString &hostname);
    void removeManagedSite(int routeMode, int index);
    void removeManagedSites(int routeMode);
    void importManagedSites(int routeMode, const QString &fileName, bool replaceExisting);
    void exportManagedSites(int routeMode, const QString &fileName);
    void reloadManagedSites();
    void reloadDefaultManagedSites();

signals:
    void errorOccurred(const QString &errorMessage);
    void finished(const QString &message);
    void managedSplitTunnelingForceChanged();
    void managedSplitTunnelingRulesPublished(int serverIndex);

private:
    int currentServerIndex() const;
    amnezia::RouteMode normalizeRouteMode(int routeMode) const;
    QVector<QPair<QString, QString>> currentManagedSites(amnezia::RouteMode routeMode) const;
    QVector<QPair<QString, QString>> managedSitesForServer(int serverIndex, amnezia::RouteMode routeMode) const;
    QString normalizeHostname(const QString &hostname) const;
    bool validateHostname(const QString &hostname) const;
    QJsonObject managedRoutingRulesPayload(int serverIndex) const;
    void publishManagedSplitTunnelingRules(int serverIndex);
    void startNextManagedSplitTunnelingPublish();

    SecureServersRepository *m_serversRepository;
    ServersUiController *m_serversUiController;
    InstallController *m_installController;
    IpSplitTunnelingModel *m_managedExceptSitesModel;
    struct ManagedSplitTunnelingPublishJob {
        int serverIndex = -1;
        amnezia::ServerCredentials credentials;
        QJsonObject rules;
        amnezia::DockerContainer container = amnezia::DockerContainer::None;
    };
    QVector<ManagedSplitTunnelingPublishJob> m_pendingManagedSplitTunnelingPublishJobs;
    bool m_isManagedSplitTunnelingPublishInProgress = false;
};

#endif // SITESCONTROLLER_H
