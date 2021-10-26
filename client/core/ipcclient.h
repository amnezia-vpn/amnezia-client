#ifndef IPCCLIENT_H
#define IPCCLIENT_H

#include <QLocalSocket>
#include <QObject>

#include "ipc.h"
#include "rep_ipc_interface_replica.h"

#ifndef Q_OS_IOS
#include "rep_ipc_process_interface_replica.h"
#else
class IpcProcessInterfaceReplica {

};
#endif

class IpcClient : public QObject
{
    Q_OBJECT
public:
   explicit IpcClient(QObject *parent = nullptr);

   static IpcClient *Instance();
   static bool init(IpcClient *instance);
   static QSharedPointer<IpcInterfaceReplica> Interface();
   static QSharedPointer<IpcProcessInterfaceReplica> CreatePrivilegedProcess();

   bool isSocketConnected() const;

signals:

private:
    ~IpcClient() override;

    QRemoteObjectNode m_ClientNode;
    QSharedPointer<IpcInterfaceReplica> m_ipcClient;
    QPointer<QLocalSocket> m_localSocket;

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
    bool m_isSocketConnected {false};

    static IpcClient *m_instance;
};

#endif // IPCCLIENT_H
