#ifndef IPCSERVER_H
#define IPCSERVER_H

#include <QObject>

#include "ipc.h"
#include "ipcserverprocess.h"

#include "rep_ipcinterface_source.h"

class IpcServer : public IpcInterfaceSource
{
public:
    explicit IpcServer(QObject *parent = nullptr);
    virtual int createPrivilegedProcess() override;

private:
    int m_localpid = 0;

    struct ProcessDescriptor {
        ProcessDescriptor (QObject *parent = nullptr) {
            serverNode = QSharedPointer<QRemoteObjectHost>(new QRemoteObjectHost(parent));
            ipcProcess = QSharedPointer<IpcServerProcess>(new IpcServerProcess(parent));
        }
        QSharedPointer<IpcServerProcess> ipcProcess;
        QSharedPointer<QRemoteObjectHost> serverNode;
    };

    QMap<int, ProcessDescriptor> m_processes;
};

#endif // IPCSERVER_H
