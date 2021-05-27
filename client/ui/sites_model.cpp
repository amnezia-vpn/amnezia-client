#include "sites_model.h"

SitesModel::SitesModel(Settings::RouteMode mode, QObject *parent)
    : m_mode(mode),
      QAbstractTableModel(parent)
{
}

void SitesModel::resetCache()
{
    beginResetModel();
    m_ipsCache.clear();
    m_cacheReady = false;
    endResetModel();
}

QVariant SitesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // FIXME: Implement me!
    return QVariant();
}

int SitesModel::rowCount(const QModelIndex &parent) const
{
    if (!m_cacheReady) genCache();
    return m_ipsCache.size();
}

int SitesModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant SitesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (!m_cacheReady) genCache();

    if (role == Qt::DisplayRole){
        if (m_ipsCache.isEmpty()) return QVariant();

        if (index.column() == 0) {
            return m_ipsCache.at(index.row()).first;
        }
        if (index.column() == 1) {
            return m_ipsCache.at(index.row()).second;
        }
    }

//    if (role == Qt::TextAlignmentRole && index.column() == 1) {
//        return Qt::AlignRight;
//    }

    return QVariant();
}

void SitesModel::genCache() const
{
    qDebug() << "SitesModel::genCache";
    m_ipsCache.clear();

    const QVariantMap &sites = m_settings.vpnSites(m_mode);
    auto i = sites.constBegin();
    while (i != sites.constEnd()) {
        m_ipsCache.append(qMakePair(i.key(), i.value().toString()));
        ++i;
    }

    m_cacheReady= true;
}
