#ifndef IPCCLIENT_H
#define IPCCLIENT_H

#include <QLocalSocket>
#include <QObject>

#include "ipc.h"
#include "rep_ipc_interface_replica.h"
#include "rep_ipc_process_tun2socks_replica.h"

#include "privileged_process.h"

class IpcClient : public QObject
{
    Q_OBJECT
public:
   explicit IpcClient(QObject *parent = nullptr);

   static IpcClient *Instance();
   static bool init(IpcClient *instance);
   static QSharedPointer<IpcInterfaceReplica> Interface();
   static QSharedPointer<IpcProcessTun2SocksReplica> InterfaceTun2Socks();
   static QSharedPointer<PrivilegedProcess> CreatePrivilegedProcess();

   bool isSocketConnected() const;

signals:

private:
    ~IpcClient() override;

    QRemoteObjectNode m_ClientNode;
    QRemoteObjectNode m_Tun2SocksNode;
    QSharedPointer<IpcInterfaceReplica> m_ipcClient;
    QPointer<QLocalSocket> m_localSocket;
    QPointer<QLocalSocket> m_tun2socksSocket;
    QSharedPointer<IpcProcessTun2SocksReplica> m_Tun2SocksClient;

    struct ProcessDescriptor {
        ProcessDescriptor () {
            replicaNode = QSharedPointer<QRemoteObjectNode>(new QRemoteObjectNode());
            ipcProcess = QSharedPointer<PrivilegedProcess>();
            localSocket = QSharedPointer<QLocalSocket>();
        }
        QSharedPointer<PrivilegedProcess> ipcProcess;
        QSharedPointer<QRemoteObjectNode> replicaNode;
        QSharedPointer<QLocalSocket> localSocket;
    };

    QMap<int, QSharedPointer<ProcessDescriptor>> m_processNodes;
    bool m_isSocketConnected {false};

    static IpcClient *m_instance;
};

#endif // IPCCLIENT_H
