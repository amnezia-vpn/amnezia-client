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
   static bool init();
   static QSharedPointer<IpcInterfaceReplica> Interface() { return Instance().m_ipcClient; }
   static QSharedPointer<IpcProcessInterfaceReplica> CreatePrivilegedProcess();

signals:

private:
    explicit IpcClient(QObject *parent = nullptr);

    QRemoteObjectNode m_ClientNode;
    QSharedPointer<IpcInterfaceReplica> m_ipcClient;
    QSharedPointer<QLocalSocket> m_localSocket;

    struct ProcessDescriptor {
        ProcessDescriptor () {
            replicaNode = QSharedPointer<QRemoteObjectNode>(new QRemoteObjectNode());
            ipcProcess = QSharedPointer<IpcProcessInterfaceReplica>();
            localSocket = QSharedPointer<QLocalSocket>();
        }
        QSharedPointer<IpcProcessInterfaceReplica> ipcProcess;
        QSharedPointer<QRemoteObjectNode> replicaNode;
        QSharedPointer<QLocalSocket> localSocket;
    };

    QMap<int, QSharedPointer<ProcessDescriptor>> m_processNodes;
};

#endif // IPCCLIENT_H
