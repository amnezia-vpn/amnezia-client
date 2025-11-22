#ifndef APPSPLITTUNNELINGUICONTROLLER_H
#define APPSPLITTUNNELINGUICONTROLLER_H

#include <QObject>
#include <QVector>

#include "core/controllers/appSplitTunnelingController.h"
#include "core/defs.h"
#include "ui/models/appSplitTunnelingModel.h"

class AppSplitTunnelingUiController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int routeMode READ getRouteMode WRITE setRouteMode NOTIFY routeModeChanged)
    Q_PROPERTY(bool isTunnelingEnabled READ isTunnelingEnabled NOTIFY isTunnelingEnabledChanged)

public:
    explicit AppSplitTunnelingUiController(const QSharedPointer<AppSplitTunnelingController> &appSplitTunnelingController,
                                          const QSharedPointer<AppSplitTunnelingModel> &appSplitTunnelingModel,
                                          QObject *parent = nullptr);

public slots:
    void addApp(const QString &appPath);
    void addApps(QVector<QPair<QString, QString>> apps);
    void removeApp(const int index);
    void toggleSplitTunneling(bool enabled);
    void setRouteMode(int routeMode);

    int getRouteMode() const;
    bool isTunnelingEnabled() const;

signals:
    void routeModeChanged();
    void isTunnelingEnabledChanged();
    void errorOccurred(const QString &errorMessage);
    void finished(const QString &message);

private:
    QSharedPointer<AppSplitTunnelingController> m_appSplitTunnelingController;
    QSharedPointer<AppSplitTunnelingModel> m_appSplitTunnelingModel;
};

#endif // APPSPLITTUNNELINGUICONTROLLER_H
