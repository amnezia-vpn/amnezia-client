#include "sites_model.h"

SitesModel::SitesModel(std::shared_ptr<Settings> settings, QObject *parent)
    : QAbstractListModel(parent), m_settings(settings)
{
    // Legacy site split tunneling is deprecated.
    m_isSplitTunnelingEnabled = false;
    m_currentRouteMode = Settings::VpnAllSites;
    m_sites.clear();
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
        m_sites.clear();
    }
    for (auto i = sites.constBegin(); i != sites.constEnd(); ++i) {
        m_sites.append(qMakePair(i.key(), i.value()));
    }

    endResetModel();
}

void SitesModel::removeSite(QModelIndex index)
{
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    m_sites.removeAt(index.row());
    endRemoveRows();
}

void SitesModel::removeSites()
{
    beginResetModel();

    m_sites.clear();

    endResetModel();
}

int SitesModel::getRouteMode()
{
    return m_currentRouteMode;
}

void SitesModel::setRouteMode(int routeMode)
{
    beginResetModel();
    m_currentRouteMode = static_cast<Settings::RouteMode>(routeMode);
    m_sites.clear();
    endResetModel();
    emit routeModeChanged();
}

bool SitesModel::isSplitTunnelingEnabled()
{
    return m_isSplitTunnelingEnabled;
}

void SitesModel::toggleSplitTunneling(bool enabled)
{
    m_isSplitTunnelingEnabled = false;
    Q_UNUSED(enabled)
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
}
