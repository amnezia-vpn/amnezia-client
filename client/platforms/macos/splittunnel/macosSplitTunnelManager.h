#ifndef MACOSSPLITTUNNELMANAGER_H
#define MACOSSPLITTUNNELMANAGER_H

#include <QObject>
#include <QVector>

#include "core/utils/commonStructs.h"
#include "core/utils/routeModes.h"

class MacOSSplitTunnelManager : public QObject
{
    Q_OBJECT

public:
    static MacOSSplitTunnelManager *instance();

    void activateExtension();
    void reconcile(bool vpnConnected, bool splitTunnelEnabled, amnezia::AppsRouteMode mode,
                   const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer);
    void stopProxy();

signals:
    void needsUserApproval();
    void errorOccurred(const QString &message);

private:
    explicit MacOSSplitTunnelManager(QObject *parent = nullptr);

    void startProxy(const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer);
    QByteArray optionsJson(const QVector<amnezia::InstalledAppInfo> &apps, const QString &vpnServer) const;

    bool m_activationRequested = false;
};

#endif
