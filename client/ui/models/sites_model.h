#ifndef SITESMODEL_H
#define SITESMODEL_H

#include <QAbstractListModel>

#include "settings.h"

class SitesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        UrlRole = Qt::UserRole + 1,
        IpRole
    };

    explicit SitesModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_PROPERTY(int routeMode READ getRouteMode WRITE setRouteMode NOTIFY routeModeChanged)
    Q_PROPERTY(bool isTunnelingEnabled READ isSplitTunnelingEnabled NOTIFY splitTunnelingToggled)

public slots:
    bool addSite(const QString &hostname, const QString &ip);
    void addSites(const QMap<QString, QString> &sites, bool replaceExisting);
    void removeSite(QModelIndex index);

    int getRouteMode();
    void setRouteMode(int routeMode);

    bool isSplitTunnelingEnabled();
    void toggleSplitTunneling(bool enabled);

    QVector<QPair<QString, QString>> getCurrentSites();

signals:
    void routeModeChanged();
    void splitTunnelingToggled();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    void fillSites();

    std::shared_ptr<Settings> m_settings;

    bool m_isSplitTunnelingEnabled;
    Settings::RouteMode m_currentRouteMode;

    QVector<QPair<QString, QString>> m_sites;
};

#endif // SITESMODEL_H
