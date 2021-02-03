#ifndef IPCCLIENT_H
#define IPCCLIENT_H

#include <QLocalSocket>
#include <QObject>

#include "ipc.h"
#include "rep_ipcinterface_replica.h"

class IpcClient : public QObject
{
    Q_OBJECT
public:
   static IpcClient &Instance();
   static bool init() { return Instance().m_ipcClient->isReplicaValid(); }
   static QSharedPointer<IpcInterfaceReplica> ipcClient() { return Instance().m_ipcClient; }

   static QSharedPointer<IpcProcessInterfaceReplica> createPrivilegedProcess();

signals:

private:
    explicit IpcClient(QObject *parent = nullptr);

    QRemoteObjectNode m_ClientNode; // create remote object node
    QSharedPointer<IpcInterfaceReplica> m_ipcClient;
    QSharedPointer<QLocalSocket> m_localSocket;

    //QMap<int, QSharedPointer<QRemoteObjectNode>> m_processNodes;
};

#endif // IPCCLIENT_H
