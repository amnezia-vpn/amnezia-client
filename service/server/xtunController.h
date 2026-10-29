#ifndef XTUNCONTROLLER_H
#define XTUNCONTROLLER_H

#include <QObject>
#include <QStringList>

#include <xtun.h>

#include "core/utils/errorCodes.h"
#include "ipc.h"
#include "rep_ipc_interface_source.h"

class XtunController : public QObject
{
    Q_OBJECT

public:
    explicit XtunController(QObject *parent = nullptr);
    ~XtunController() override;

    amnezia::ErrorCode up(const TunnelConfig &config);
    void down();
    bool setKillSwitchMode(amnezia::KillSwitchMode mode);
    bool addKillSwitchAllowedRanges(const QStringList &ranges);

    bool startLocalProxy(const QString &config);
    bool stopLocalProxy();
    bool isLocalProxyRunning() const;

signals:
    void statusChanged(amnezia::TunnelStatus status);

private:
    static void onLog(const char *line, void *controller);
    static void onStatus(int status, void *controller);

    libxtun_tunnel m_tunnel {};
    libxtun_proxy m_proxy {};
};

#endif // XTUNCONTROLLER_H
