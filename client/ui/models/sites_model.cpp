#include "sites_model.h"
#include "core/controllers/splitTunnelingController.h"

SitesModel::SitesModel(QSharedPointer<SplitTunnelingController> splitTunnelingController,
                       QObject *parent)
    : QAbstractListModel(parent), 
      m_splitTunnelingController(splitTunnelingController)
{
    // Connect to controller signals
    connect(m_splitTunnelingController.data(), &SplitTunnelingController::siteAdded,
            this, &SitesModel::onSiteAdded);
    connect(m_splitTunnelingController.data(), &SplitTunnelingController::siteRemoved,
            this, &SitesModel::onSiteRemoved);
    connect(m_splitTunnelingController.data(), &SplitTunnelingController::sitesRouteModelChanged,
            this, &SitesModel::onSitesRouteModelChanged);
    connect(m_splitTunnelingController.data(), &SplitTunnelingController::sitesSplitTunnelingToggled,
            this, &SitesModel::onSitesSplitTunnelingToggled);

    // Initialize data
    refreshData();
}

int SitesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_sites.size();
}

QVariant SitesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(rowCount()))
        return QVariant();

    switch (role) {
    case UrlRole: {
        return m_sites.at(index.row()).first;
        break;
    }
    case IpRole: {
        return m_sites.at(index.row()).second;
        break;
    }
    default: {
        return true;
    }
    }

    return QVariant();
}

bool SitesModel::addSite(const QString &hostname, const QString &ip)
{
    return m_splitTunnelingController->addSite(hostname, ip);
}

void SitesModel::addSites(const QMap<QString, QString> &sites, bool replaceExisting)
{
    m_splitTunnelingController->addSites(sites, replaceExisting);
}

void SitesModel::removeSite(QModelIndex index)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_sites.size()) {
        return;
    }
    
    auto hostname = m_sites.at(index.row()).first;
    m_splitTunnelingController->removeSite(hostname);
}

int SitesModel::getRouteMode()
{
    return static_cast<int>(m_splitTunnelingController->getSitesRouteMode());
}

void SitesModel::setRouteMode(int routeMode)
{
    m_splitTunnelingController->setSitesRouteMode(static_cast<Settings::RouteMode>(routeMode));
}

bool SitesModel::isSplitTunnelingEnabled()
{
    return m_splitTunnelingController->isSitesSplitTunnelingEnabled();
}

void SitesModel::toggleSplitTunneling(bool enabled)
{
    m_splitTunnelingController->setSitesSplitTunnelingEnabled(enabled);
}

QVector<QPair<QString, QString> > SitesModel::getCurrentSites()
{
    return m_sites;
}

QHash<int, QByteArray> SitesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[UrlRole] = "url";
    roles[IpRole] = "ip";
    return roles;
}

void SitesModel::onSiteAdded(const QString &hostname, const QString &ip)
{
    Q_UNUSED(hostname)
    Q_UNUSED(ip)
    beginResetModel();
    refreshData();
    endResetModel();
}

void SitesModel::onSiteRemoved(const QString &hostname)
{
    Q_UNUSED(hostname)
    beginResetModel();
    refreshData();
    endResetModel();
}

void SitesModel::onSitesRouteModelChanged()
{
    beginResetModel();
    refreshData();
    endResetModel();
    emit routeModeChanged();
}

void SitesModel::onSitesSplitTunnelingToggled()
{
    emit splitTunnelingToggled();
}

void SitesModel::refreshData()
{
    m_sites = m_splitTunnelingController->getSites(m_splitTunnelingController->getSitesRouteMode());
}
