#ifndef SITESMODEL_H
#define SITESMODEL_H

#include <QAbstractTableModel>

#include "settings.h"

class SitesModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit SitesModel(Settings::RouteMode mode, QObject *parent = nullptr);
    void resetCache();

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    void genCache() const;

private:
    Settings::RouteMode m_mode;
    Settings m_settings;

    mutable QVector<QPair<QString, QString>> m_ipsCache;
    mutable bool m_cacheReady = false;
};

#endif // SITESMODEL_H
