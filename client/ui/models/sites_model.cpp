#include "sites_model.h"

SitesModel::SitesModel(std::shared_ptr<Settings> settings, QObject *parent)
    : QAbstractListModel(parent), m_settings(settings)
{
    auto routeMode = m_settings->routeMode();
    if (routeMode == Settings::RouteMode::VpnAllSites) {
        m_isSplitTunnelingEnabled = false;
        m_currentRouteMode = Settings::RouteMode::VpnOnlyForwardSites;
    } else {
        m_isSplitTunnelingEnabled = true;
        m_currentRouteMode = routeMode;
    }
    fillSites();
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
    if (!m_settings->addVpnSite(m_currentRouteMode, hostname, ip)) {
        return false;
    }
    for (int i = 0; i < m_sites.size(); i++) {
        if (m_sites[i].first == hostname && (m_sites[i].second.isEmpty() && !ip.isEmpty())) {
            m_sites[i].second = ip;
            QModelIndex index = createIndex(i, i);
            emit dataChanged(index, index);
            return true;
        } else if (m_sites[i].first == hostname && (m_sites[i].second == ip)) {
            return false;
        }
    }
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_sites.append(qMakePair(hostname, ip));
    endInsertRows();
    return true;
}

void SitesModel::addSites(const QMap<QString, QString> &sites, bool replaceExisting)
{
    beginResetModel();

    if (replaceExisting) {
        m_settings->removeAllVpnSites(m_currentRouteMode);
    }
    m_settings->addVpnSites(m_currentRouteMode, sites);
    fillSites();

    endResetModel();
}

void SitesModel::removeSite(QModelIndex index)
{
    auto hostname = m_sites.at(index.row()).first;
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    m_settings->removeVpnSite(m_currentRouteMode, hostname);
    m_sites.removeAt(index.row());
    endRemoveRows();
}

int SitesModel::getRouteMode()
{
    return m_currentRouteMode;
}

void SitesModel::setRouteMode(int routeMode)
{
    beginResetModel();
    m_settings->setRouteMode(static_cast<Settings::RouteMode>(routeMode));
    m_currentRouteMode = m_settings->routeMode();
    fillSites();
    endResetModel();
    emit routeModeChanged();
}

bool SitesModel::isSplitTunnelingEnabled()
{
    return m_isSplitTunnelingEnabled;
}

void SitesModel::toggleSplitTunneling(bool enabled)
{
    if (enabled) {
        setRouteMode(m_currentRouteMode);
    } else {
        m_settings->setRouteMode(Settings::RouteMode::VpnAllSites);
    }
    m_isSplitTunnelingEnabled = enabled;
    emit splitTunnelingToggled();
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

void SitesModel::fillSites()
{
    m_sites.clear();
    const QVariantMap &sites = m_settings->vpnSites(m_currentRouteMode);
    auto i = sites.constBegin();
    while (i != sites.constEnd()) {
        m_sites.append(qMakePair(i.key(), i.value().toString()));
        ++i;
    }
}
