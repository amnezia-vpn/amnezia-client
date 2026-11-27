#ifndef XTUNCONTROLLER_H
#define XTUNCONTROLLER_H

#include <QObject>
#include <QStringList>

#include <memory>

#include <xtun.h>

#include "core/utils/errorCodes.h"
#include "core/utils/tunnelSettings.h"
#include "rep_ipc_interface_source.h"

class XtunController : public QObject
{
    Q_OBJECT

public:
    explicit XtunController(QObject *parent = nullptr);
    ~XtunController() override;

    amnezia::ErrorCode up(const TunnelConfig &config);
    amnezia::ErrorCode switchProxy(const TunnelConfig &config);
    void down();
    bool setKillSwitchMode(amnezia::KillSwitchMode mode);
    bool addKillSwitchAllowedRanges(const QStringList &ranges);

    bool startLocalProxy(const QString &config);
    bool stopLocalProxy();
    bool isLocalProxyRunning() const;

signals:
    void statusChanged(amnezia::TunnelStatus status);

private:
    struct Proxy
    {
        ~Proxy();

        libxtun_proxy handle {};
        XtunController *controller = nullptr;
        quint64 generation = 0;
    };

    amnezia::ErrorCode createProxy(const TunnelConfig &config, std::unique_ptr<Proxy> &proxy);
    amnezia::ErrorCode attachProxy(std::unique_ptr<Proxy> proxy);

    static void onLog(const char *line, void *controller);
    static void onStatus(int status, void *proxy);

    libxtun_tunnel m_tunnel {};
    std::unique_ptr<Proxy> m_proxy;
    quint64 m_proxyGeneration = 0;
};

#endif // XTUNCONTROLLER_H
