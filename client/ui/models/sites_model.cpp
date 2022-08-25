#include "sites_model.h"

SitesModel::SitesModel(std::shared_ptr<Settings> settings, Settings::RouteMode mode, QObject *parent)
    : QAbstractListModel(parent),
      m_settings(settings),
      m_mode(mode)
{
}

void SitesModel::resetCache()
{
    beginResetModel();
    m_ipsCache.clear();
    m_cacheReady = false;
    endResetModel();
}

int SitesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (!m_cacheReady) genCache();
    return m_ipsCache.size();
}


QVariant SitesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (!m_cacheReady) genCache();

    if (role == SitesModel::UrlRole || role == SitesModel::IpRole) {
        if (m_ipsCache.isEmpty()) return QVariant();

        if (role == SitesModel::UrlRole) {
            return m_ipsCache.at(index.row()).first;
        }
        if (role == SitesModel::IpRole) {
            return m_ipsCache.at(index.row()).second;
        }
    }

    //    if (role == Qt::TextAlignmentRole && index.column() == 1) {
    //        return Qt::AlignRight;
    //    }

    return QVariant();
}

QVariant SitesModel::data(int row, int column)
{
    if (row < 0 || row >= rowCount() || column < 0 || column >= 2) {
        return QVariant();
    }
    if (!m_cacheReady) genCache();

    if (column == 0) {
        return m_ipsCache.at(row).first;
    }
    if (column == 1) {
        return m_ipsCache.at(row).second;
    }
    return QVariant();
}

void SitesModel::genCache() const
{
    qDebug() << "SitesModel::genCache";
    m_ipsCache.clear();

    const QVariantMap &sites = m_settings->vpnSites(m_mode);
    auto i = sites.constBegin();
    while (i != sites.constEnd()) {
        m_ipsCache.append(qMakePair(i.key(), i.value().toString()));
        ++i;
    }

    m_cacheReady= true;
}

QHash<int, QByteArray> SitesModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[UrlRole] = "url_path";
    roles[IpRole] = "ip";
    return roles;
}
