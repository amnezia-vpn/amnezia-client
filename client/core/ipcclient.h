#ifndef IPCCLIENT_H
#define IPCCLIENT_H

#include <QObject>

#include "ipc.h"
#include "rep_ipcinterface_replica.h"

class IpcClient : public QObject
{
    Q_OBJECT
public:
   static IpcClient &Instance();

   static QSharedPointer<IpcProcessInterfaceReplica> createPrivilegedProcess();

   static QSharedPointer<IpcInterfaceReplica> ipcClient() { return Instance().m_ipcClient; }

signals:

private:
    explicit IpcClient(QObject *parent = nullptr);

    QRemoteObjectNode m_ClientNode; // create remote object node
    QSharedPointer<IpcInterfaceReplica> m_ipcClient;

    //QMap<int, QSharedPointer<QRemoteObjectNode>> m_processNodes;
};

#endif // IPCCLIENT_H
