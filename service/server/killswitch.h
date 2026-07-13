#ifndef KILLSWITCH_H
#define KILLSWITCH_H

#include <QJsonObject>
#include <QSharedPointer>

#include "secureQSettings.h"

class KillSwitch : public QObject
{
    Q_OBJECT
public:
    static KillSwitch *instance();
    bool init();
    bool refresh(bool enabled);
    bool disableKillSwitch();
    bool disableKillSwitchForTunnel(const QString& ifname, const QStringList& remainingRanges);
    bool disableAllTraffic();
    bool enablePeerTraffic(const QJsonObject &configStr);
    bool enableKillSwitch(const QJsonObject &configStr, int vpnAdapterIndex);
    bool resetAllowedRange(const QStringList &ranges);
    bool addAllowedRange(const QString &ifname, const QStringList &ranges);
    bool isStrictKillSwitchEnabled();

private:
    KillSwitch(QObject* parent) {};
    QStringList combinedAllowNets() const;
    QStringList m_allowedRanges;
    QStringList m_splitTunnelAllows;
    QSharedPointer<SecureQSettings> m_appSettigns;

};

#endif // KILLSWITCH_H
