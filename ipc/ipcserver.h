#ifndef IPCSERVER_H
#define IPCSERVER_H

#include <QObject>

#include "ipc.h"
#include "xtunController.h"

#include "rep_ipc_interface_source.h"

class IpcServer : public IpcInterfaceSource
{
public:
    explicit IpcServer(QObject *parent = nullptr);

    virtual void setLogsEnabled(bool enabled) override;
    virtual void clearLogs() override;

    virtual int tunnelUp(const TunnelConfig &config) override;
    virtual bool tunnelDown() override;
    virtual bool setKillSwitchMode(int mode) override;
    virtual bool addKillSwitchAllowedRanges(const QStringList &ranges) override;

    virtual bool localProxyUp(const QString &config) override;
    virtual bool localProxyDown() override;
    virtual bool isLocalProxyRunning() override;

private:
    XtunController m_xtunController;
};

#endif // IPCSERVER_H
