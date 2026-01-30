#include "ipcclient.h"
#include "ipc.h"
#include <QRemoteObjectNode>
#include <QtNetwork/qlocalsocket.h>

IpcClient::IpcClient(QObject *parent) : QObject(parent)
{
    m_node.connectToNode(QUrl("local:" + amnezia::getIpcServiceUrl()));
    m_interface.reset(m_node.acquire<IpcInterfaceReplica>());
    m_tun2socks.reset(m_node.acquire<IpcProcessTun2SocksReplica>());
}

IpcClient& IpcClient::Instance()
{
    thread_local IpcClient ipcClient;
    return ipcClient;
}

QSharedPointer<IpcInterfaceReplica> IpcClient::Interface()
{
    QSharedPointer<IpcInterfaceReplica> rep = Instance().m_interface;
    if (rep.isNull()) {
        qCritical() << "IpcClient::Interface(): Failed to acquire replica";
        return nullptr;
    }
    if (!rep->waitForSource(1000)) {
        qCritical() << "IpcClient::Interface(): Failed to initialize replica";
        return nullptr;
    }
    if (!rep->isReplicaValid()) {
        qWarning() << "IpcClient::Interface(): Replica is invalid";
    }
    return rep;
}

QSharedPointer<IpcProcessTun2SocksReplica> IpcClient::InterfaceTun2Socks()
{
    QSharedPointer<IpcProcessTun2SocksReplica> rep = Instance().m_tun2socks;
    if (rep.isNull()) {
        qCritical() << "IpcClient::InterfaceTun2Socks: Replica is undefined";
        return nullptr;
    }
    if (!rep->waitForSource(1000)) {
        qCritical() << "IpcClient::InterfaceTun2Socks: Failed to initialize replica";
        return nullptr;
    }
    if (!rep->isReplicaValid()) {
        qWarning() << "IpcClient::InterfaceTun2Socks(): Replica is invalid";
    }
    return rep;
}

QSharedPointer<PrivilegedProcess> IpcClient::CreatePrivilegedProcess()
{
    QSharedPointer<IpcInterfaceReplica> rep = Interface();
    if (!rep) {
        qCritical() << "IpcClient::createPrivilegedProcess: Replica is invalid";
        return nullptr;
    }

    QRemoteObjectPendingReply<int> pidReply = rep->createPrivilegedProcess();
    if (!pidReply.waitForFinished(5000)){
        qCritical() << "IpcClient::createPrivilegedProcess: Failed to execute RO createPrivilegedProcess call";
        return nullptr;
    }

    int pid = pidReply.returnValue();
    QSharedPointer<ProcessDescriptor> pd(new ProcessDescriptor());

    pd->localSocket.reset(new QLocalSocket(pd->replicaNode.data()));

    connect(pd->localSocket.data(), &QLocalSocket::connected, pd->replicaNode.data(), [pd]() {
        pd->replicaNode->addClientSideConnection(pd->localSocket.data());

        IpcProcessInterfaceReplica *repl = pd->replicaNode->acquire<IpcProcessInterfaceReplica>();
        // TODO: rework the unsafe cast below
        PrivilegedProcess *priv = static_cast<PrivilegedProcess *>(repl);
        pd->ipcProcess.reset(priv);
        if (!pd->ipcProcess) {
            qWarning() << "Acquire PrivilegedProcess failed";
        } else {
            pd->ipcProcess->waitForSource(1000);
            if (!pd->ipcProcess->isReplicaValid()) {
                qWarning() << "PrivilegedProcess replica is not connected!";
            }

            QObject::connect(pd->ipcProcess.data(), &PrivilegedProcess::destroyed, pd->ipcProcess.data(),
                             [pd]() { pd->replicaNode->deleteLater(); });
        }
    });

    pd->localSocket->connectToServer(amnezia::getIpcProcessUrl(pid));
    if (!pd->localSocket->waitForConnected()) {
        qCritical() << "IpcClient::createPrivilegedProcess: Failed to connect to process' socket";
        return nullptr;
    }

    auto processReplica = QSharedPointer<PrivilegedProcess>(pd->ipcProcess);
    return processReplica;
}
