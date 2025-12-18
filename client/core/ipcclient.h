#ifndef IPCCLIENT_H
#define IPCCLIENT_H

#include <QLocalSocket>
#include <QObject>

#include "rep_ipc_interface_replica.h"
#include "rep_ipc_process_tun2socks_replica.h"

#include "privileged_process.h"

class IpcClient : public QObject
{
    Q_OBJECT
public:
    explicit IpcClient(QObject *parent = nullptr);

    static IpcClient *Instance();

    static QSharedPointer<IpcInterfaceReplica> Interface();
    static QSharedPointer<IpcProcessTun2SocksReplica> InterfaceTun2Socks();
    static QSharedPointer<PrivilegedProcess> CreatePrivilegedProcess();

    template <typename Func>
    static auto withInterface(Func func)
    {
        QSharedPointer<IpcInterfaceReplica> iface = Instance()->m_ipcClient;
        using ReturnType = decltype(func(std::declval<QSharedPointer<IpcInterfaceReplica>>()));

        if (iface.isNull() || !iface->isReplicaValid()) {
            qWarning() << "IpcClient::withInterface(): Service is not running";

            if constexpr (std::is_void_v<ReturnType>)
                return;
            else
                return ReturnType{};
        }

        return func(iface);
    }

    template <typename OnSuccess, typename OnFailure>
    static auto withInterface(OnSuccess onSuccess, OnFailure onFailure)
    {
        QSharedPointer<IpcInterfaceReplica> iface = Instance()->m_ipcClient;

        if (iface.isNull() || !iface->isReplicaValid()) {
            return onFailure();
        }

        return onSuccess(iface);
    }

    bool isSocketConnected() const;
signals:

private:
    bool establishConnection();

    QLocalSocket m_localSocket;
    QSharedPointer<QRemoteObjectNode> m_ClientNode;
    QSharedPointer<IpcInterfaceReplica> m_ipcClient;
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

    bool m_isSocketConnected {false};
};

#endif // IPCCLIENT_H
