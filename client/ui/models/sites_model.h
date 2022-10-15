#ifndef SITESMODEL_H
#define SITESMODEL_H

#include <QAbstractListModel>

#include "settings.h"

class SitesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum SiteRoles {
        UrlRole = Qt::UserRole + 1,
        IpRole
    };

    explicit SitesModel(std::shared_ptr<Settings> settings, Settings::RouteMode mode, QObject *parent = nullptr);
    void resetCache();

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant data(int row, int column);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    void genCache() const;

private:
    Settings::RouteMode m_mode;
    std::shared_ptr<Settings> m_settings;

    mutable QVector<QPair<QString, QString>> m_ipsCache;
    mutable bool m_cacheReady = false;
};

#endif // SITESMODEL_H
