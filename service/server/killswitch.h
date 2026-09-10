#ifndef KILLSWITCH_H
#define KILLSWITCH_H

#include <QJsonObject>
#include <QSharedPointer>

#include "secureQSettings.h"

#ifdef Q_OS_WIN
class WindowsRouteMonitor;
#endif

class KillSwitch : public QObject
{
    Q_OBJECT
public:
    static KillSwitch *instance();
    bool init();
    bool refresh(bool enabled);
    bool disableKillSwitch();
    bool disableAllTraffic();
    bool enablePeerTraffic(const QJsonObject &configStr);
    bool enableKillSwitch(const QJsonObject &configStr, int vpnAdapterIndex);
    bool resetAllowedRange(const QStringList &ranges);
    bool addAllowedRange(const QStringList &ranges);
    bool isStrictKillSwitchEnabled();

private:
    KillSwitch(QObject* parent) {};
    QStringList m_allowedRanges;
    QSharedPointer<SecureQSettings> m_appSettigns;

#ifdef Q_OS_WIN
    void setupTunnelRouteMonitor(int vpnAdapterIndex, const QString &serverAddress);
    void teardownTunnelRouteMonitor();

    WindowsRouteMonitor *m_tunnelRouteMonitor = nullptr;
#endif
};

#endif // KILLSWITCH_H
