#include "ipcClient.h"
#include "ipc.h"
#include <QRemoteObjectNode>
#include <QtNetwork/qlocalsocket.h>

IpcClient::IpcClient(QObject *parent) : QObject(parent)
{
    m_node.connectToNode(QUrl("local:" + amnezia::getIpcServiceUrl()));
    m_interface.reset(m_node.acquire<IpcInterfaceReplica>());
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
