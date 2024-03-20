#ifndef APPSPLITTUNNELINGMODEL_H
#define APPSPLITTUNNELINGMODEL_H

#include <QAbstractListModel>

#include "settings.h"

class AppSplitTunnelingModel: public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        AppPathRole = Qt::UserRole + 1,
        PackageAppNameRole,
        PackageAppIconRole
    };

    explicit AppSplitTunnelingModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_PROPERTY(int routeMode READ getRouteMode WRITE setRouteMode NOTIFY routeModeChanged)
    Q_PROPERTY(bool isTunnelingEnabled READ isSplitTunnelingEnabled NOTIFY splitTunnelingToggled)

public slots:
    bool addApp(const QString &path);
    void removeApp(QModelIndex index);

    int getRouteMode();
    void setRouteMode(int routeMode);

    bool isSplitTunnelingEnabled();
    void toggleSplitTunneling(bool enabled);

signals:
    void routeModeChanged();
    void splitTunnelingToggled();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    std::shared_ptr<Settings> m_settings;

    bool m_isSplitTunnelingEnabled;
    Settings::AppsRouteMode m_currentRouteMode;

    QVector<QString> m_apps;
};

#endif // APPSPLITTUNNELINGMODEL_H
